#include "capture/WindowsGraphicsCapture.h"
#include "core/Logger.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <dwmapi.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

#if defined(__has_include)
#  if __has_include(<winrt/Windows.Graphics.Capture.h>) \
   && __has_include(<windows.graphics.capture.interop.h>)
#    define RAILSHOT_HAS_WGC 1
#  endif
#endif

#ifdef RAILSHOT_HAS_WGC
#include <unknwn.h>
#include <inspectable.h>
#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Metadata.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>
#include <windows.graphics.capture.interop.h>
#include <windows.graphics.directx.direct3d11.interop.h>
#endif
#endif

#include <QElapsedTimer>
#include <chrono>

namespace railshot {

namespace {

#ifdef _WIN32
struct MonitorPick {
    int targetIndex = 0;
    int current = 0;
    HMONITOR monitor = nullptr;
    RECT rect{};
};

BOOL CALLBACK pickMonitorProc(HMONITOR monitor, HDC, LPRECT, LPARAM lParam)
{
    auto* pick = reinterpret_cast<MonitorPick*>(lParam);
    if (!pick)
        return FALSE;
    if (pick->current == pick->targetIndex) {
        pick->monitor = monitor;
        MONITORINFO mi{};
        mi.cbSize = sizeof(mi);
        if (GetMonitorInfoW(monitor, &mi))
            pick->rect = mi.rcMonitor;
        return FALSE;
    }
    ++pick->current;
    return TRUE;
}

bool resolveMonitor(int index, HMONITOR* outMonitor, RECT* outRect, QString* error)
{
    MonitorPick pick;
    pick.targetIndex = index;
    EnumDisplayMonitors(nullptr, nullptr, pickMonitorProc, reinterpret_cast<LPARAM>(&pick));
    if (!pick.monitor) {
        if (error) *error = QStringLiteral("Monitor index %1 not found").arg(index);
        return false;
    }
    if (outMonitor) *outMonitor = pick.monitor;
    if (outRect) *outRect = pick.rect;
    return true;
}

bool windowPixelSize(HWND hwnd, int* width, int* height)
{
    if (!hwnd || !IsWindow(hwnd))
        return false;
    RECT rc{};
    if (FAILED(DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &rc, sizeof(rc)))) {
        if (!GetClientRect(hwnd, &rc))
            return false;
        POINT tl{rc.left, rc.top};
        POINT br{rc.right, rc.bottom};
        ClientToScreen(hwnd, &tl);
        ClientToScreen(hwnd, &br);
        rc.left = tl.x;
        rc.top = tl.y;
        rc.right = br.x;
        rc.bottom = br.y;
    }
    const int w = rc.right - rc.left;
    const int h = rc.bottom - rc.top;
    if (w < 1 || h < 1)
        return false;
    if (width) *width = w;
    if (height) *height = h;
    return true;
}
#endif

} // namespace

WindowsGraphicsCapture::WindowsGraphicsCapture(QString id, QString name, TargetKind kind,
                                               int monitorIndex, quintptr hwnd)
    : m_id(std::move(id))
    , m_name(std::move(name))
    , m_kind(kind)
    , m_monitorIndex(monitorIndex)
    , m_hwnd(hwnd)
{
    if (kind == TargetKind::Window && hwnd != 0)
        m_hwnd = hwnd;
}

WindowsGraphicsCapture::WindowsGraphicsCapture(QString id, QString name, quintptr hwnd)
    : m_id(std::move(id))
    , m_name(std::move(name))
    , m_kind(TargetKind::Window)
    , m_hwnd(hwnd)
{
}

WindowsGraphicsCapture::~WindowsGraphicsCapture() { stop(); }

bool WindowsGraphicsCapture::isSupported()
{
#ifdef _WIN32
#ifdef RAILSHOT_HAS_WGC
    try {
        return winrt::Windows::Graphics::Capture::GraphicsCaptureSession::IsSupported();
    } catch (...) {
        return false;
    }
#else
    return true; // BitBlt window path still available
#endif
#else
    return false;
#endif
}

void WindowsGraphicsCapture::signalStartResult(bool ok, const QString& error)
{
    std::lock_guard lock(m_startMutex);
    m_startOk = ok;
    m_startError = error;
    m_startSignaled = true;
    m_startCv.notify_one();
}

bool WindowsGraphicsCapture::resolveTargetSize(int* width, int* height, QString* error) const
{
#ifdef _WIN32
    if (m_kind == TargetKind::Window) {
        HWND hwnd = reinterpret_cast<HWND>(m_hwnd);
        if (!windowPixelSize(hwnd, width, height)) {
            if (error) *error = QStringLiteral("Invalid capture window HWND");
            return false;
        }
        return true;
    }
    RECT rc{};
    if (!resolveMonitor(m_monitorIndex, nullptr, &rc, error))
        return false;
    if (width) *width = rc.right - rc.left;
    if (height) *height = rc.bottom - rc.top;
    return true;
#else
    Q_UNUSED(width);
    Q_UNUSED(height);
    Q_UNUSED(error);
    return false;
#endif
}

bool WindowsGraphicsCapture::ensureTexture(int width, int height, QString* error)
{
#ifdef _WIN32
    if (!m_device) {
        if (error) *error = QStringLiteral("No D3D11 device");
        return false;
    }
    if (width < 1 || height < 1) {
        if (error) *error = QStringLiteral("Invalid capture size");
        return false;
    }
    if (m_texture && m_width == width && m_height == height)
        return true;

    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = static_cast<UINT>(width);
    desc.Height = static_cast<UINT>(height);
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

    ComPtr<ID3D11Texture2D> tex;
    const HRESULT hr = m_device->CreateTexture2D(&desc, nullptr, &tex);
    if (FAILED(hr)) {
        if (error)
            *error = QStringLiteral("CreateTexture2D failed: 0x%1").arg(quint32(hr), 8, 16, QLatin1Char('0'));
        return false;
    }

    std::lock_guard lock(m_frameMutex);
    if (m_texture)
        m_texture->Release();
    m_texture = tex.Detach();
    m_width = width;
    m_height = height;
    return true;
#else
    Q_UNUSED(width);
    Q_UNUSED(height);
    Q_UNUSED(error);
    return false;
#endif
}

bool WindowsGraphicsCapture::start(ID3D11Device* device, QString* error)
{
    if (m_running.load())
        return true;
    m_device = device;
#ifdef _WIN32
    if (!m_device) {
        if (error) *error = QStringLiteral("No D3D11 device");
        return false;
    }
    if (m_context) {
        m_context->Release();
        m_context = nullptr;
    }
    m_device->GetImmediateContext(&m_context);

    int w = 0;
    int h = 0;
    if (!resolveTargetSize(&w, &h, error))
        return false;
    if (!ensureTexture(w, h, error))
        return false;

    bool useWgc = false;
#ifdef RAILSHOT_HAS_WGC
    useWgc = isSupported();
#endif

    if (!useWgc && m_kind == TargetKind::Monitor) {
        if (error) {
            *error = QStringLiteral(
                "Windows.Graphics.Capture unavailable; use DesktopDuplicationCapture for monitors");
        }
        return false;
    }

    m_useWgc.store(useWgc);
    {
        std::lock_guard lock(m_startMutex);
        m_startSignaled = false;
        m_startOk = false;
        m_startError.clear();
    }

    m_running = true;
    m_thread = std::thread([this] { captureLoop(); });

    {
        std::unique_lock lock(m_startMutex);
        m_startCv.wait(lock, [this] { return m_startSignaled; });
        if (!m_startOk) {
            const QString err = m_startError;
            lock.unlock();
            m_running = false;
            if (m_thread.joinable())
                m_thread.join();
            if (error) *error = err.isEmpty() ? QStringLiteral("WGC init failed") : err;
            return false;
        }
    }

    Logger::info(QStringLiteral("WGC capture started: %1 (%2, %3x%4, %5)")
                     .arg(m_name)
                     .arg(m_kind == TargetKind::Window ? QStringLiteral("window")
                                                       : QStringLiteral("monitor"))
                     .arg(m_width)
                     .arg(m_height)
                     .arg(useWgc ? QStringLiteral("WGC") : QStringLiteral("BitBlt")));
    return true;
#else
    Q_UNUSED(error);
    return false;
#endif
}

void WindowsGraphicsCapture::stop()
{
    if (!m_running.exchange(false))
        return;
    if (m_thread.joinable())
        m_thread.join();
#ifdef _WIN32
    if (m_context) {
        m_context->Release();
        m_context = nullptr;
    }
    std::lock_guard lock(m_frameMutex);
    if (m_texture) {
        m_texture->Release();
        m_texture = nullptr;
    }
#endif
}

bool WindowsGraphicsCapture::acquireLatest(VideoFrame& out)
{
    std::lock_guard lock(m_frameMutex);
    if (!m_texture)
        return false;
    out.texture = m_texture;
    out.ptsUs = m_ptsUs;
    out.width = m_width;
    out.height = m_height;
    out.sourceId = m_id;
    return true;
}

#ifdef _WIN32
#ifdef RAILSHOT_HAS_WGC
namespace {

winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice wrapD3dDevice(ID3D11Device* device)
{
    ComPtr<IDXGIDevice> dxgiDevice;
    winrt::check_hresult(device->QueryInterface(IID_PPV_ARGS(&dxgiDevice)));
    winrt::com_ptr<::IInspectable> inspectable;
    winrt::check_hresult(CreateDirect3D11DeviceFromDXGIDevice(dxgiDevice.Get(), inspectable.put()));
    return inspectable.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>();
}

winrt::Windows::Graphics::Capture::GraphicsCaptureItem createItemForMonitor(HMONITOR monitor)
{
    auto factory = winrt::get_activation_factory<winrt::Windows::Graphics::Capture::GraphicsCaptureItem>();
    auto interop = factory.as<IGraphicsCaptureItemInterop>();
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem item{nullptr};
    winrt::check_hresult(interop->CreateForMonitor(
        monitor,
        winrt::guid_of<winrt::Windows::Graphics::Capture::IGraphicsCaptureItem>(),
        winrt::put_abi(item)));
    return item;
}

winrt::Windows::Graphics::Capture::GraphicsCaptureItem createItemForWindow(HWND hwnd)
{
    auto factory = winrt::get_activation_factory<winrt::Windows::Graphics::Capture::GraphicsCaptureItem>();
    auto interop = factory.as<IGraphicsCaptureItemInterop>();
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem item{nullptr};
    winrt::check_hresult(interop->CreateForWindow(
        hwnd,
        winrt::guid_of<winrt::Windows::Graphics::Capture::IGraphicsCaptureItem>(),
        winrt::put_abi(item)));
    return item;
}

ComPtr<ID3D11Texture2D> textureFromSurface(
    const winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DSurface& surface)
{
    auto access = surface.as<::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();
    ComPtr<ID3D11Texture2D> tex;
    winrt::check_hresult(access->GetInterface(IID_PPV_ARGS(&tex)));
    return tex;
}

void maybeDisableBorder(winrt::Windows::Graphics::Capture::GraphicsCaptureSession const& session)
{
    using winrt::Windows::Foundation::Metadata::ApiInformation;
    if (ApiInformation::IsPropertyPresent(
            L"Windows.Graphics.Capture.GraphicsCaptureSession", L"IsBorderRequired")) {
        session.IsBorderRequired(false);
    }
}

} // namespace

bool WindowsGraphicsCapture::captureLoopWgc(QString* initError)
{
    using winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool;
    using winrt::Windows::Graphics::Capture::GraphicsCaptureItem;
    using winrt::Windows::Graphics::DirectX::DirectXPixelFormat;
    using winrt::Windows::Graphics::SizeInt32;

    try {
        winrt::init_apartment(winrt::apartment_type::multi_threaded);
    } catch (winrt::hresult_error const& e) {
        if (initError) {
            *initError = QStringLiteral("WGC RoInitialize/WinRT apartment failed: 0x%1")
                             .arg(quint32(e.code()), 8, 16, QLatin1Char('0'));
        }
        return false;
    }

    Direct3D11CaptureFramePool framePool{nullptr};
    winrt::Windows::Graphics::Capture::GraphicsCaptureSession session{nullptr};
    winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice d3dDevice{nullptr};
    SizeInt32 poolSize{};

    try {
        GraphicsCaptureItem item{nullptr};
        if (m_kind == TargetKind::Window) {
            HWND hwnd = reinterpret_cast<HWND>(m_hwnd);
            if (!hwnd || !IsWindow(hwnd)) {
                if (initError) *initError = QStringLiteral("Invalid capture window HWND");
                return false;
            }
            item = createItemForWindow(hwnd);
        } else {
            HMONITOR monitor = nullptr;
            QString err;
            if (!resolveMonitor(m_monitorIndex, &monitor, nullptr, &err)) {
                if (initError) *initError = err;
                return false;
            }
            item = createItemForMonitor(monitor);
        }

        d3dDevice = wrapD3dDevice(m_device);
        poolSize = item.Size();
        if (poolSize.Width < 1 || poolSize.Height < 1)
            poolSize = {static_cast<int32_t>(m_width), static_cast<int32_t>(m_height)};

        framePool = Direct3D11CaptureFramePool::CreateFreeThreaded(
            d3dDevice,
            DirectXPixelFormat::B8G8R8A8UIntNormalized,
            2,
            poolSize);
        session = framePool.CreateCaptureSession(item);
        session.IsCursorCaptureEnabled(true);
        maybeDisableBorder(session);
        session.StartCapture();
    } catch (winrt::hresult_error const& e) {
        if (initError) {
            *initError = QStringLiteral("WGC init failed: 0x%1 (%2)")
                             .arg(quint32(e.code()), 8, 16, QLatin1Char('0'))
                             .arg(QString::fromWCharArray(e.message().c_str()));
        }
        return false;
    } catch (...) {
        if (initError) *initError = QStringLiteral("WGC init failed with unknown exception");
        return false;
    }

    signalStartResult(true, {});

    QElapsedTimer timer;
    timer.start();

    while (m_running.load()) {
        if (m_kind == TargetKind::Window) {
            HWND hwnd = reinterpret_cast<HWND>(m_hwnd);
            if (!hwnd || !IsWindow(hwnd))
                break;
        }

        try {
            auto frame = framePool.TryGetNextFrame();
            if (!frame) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            const auto contentSize = frame.ContentSize();
            if (contentSize.Width > 0 && contentSize.Height > 0
                && (contentSize.Width != poolSize.Width || contentSize.Height != poolSize.Height)) {
                poolSize = contentSize;
                framePool.Recreate(
                    d3dDevice,
                    DirectXPixelFormat::B8G8R8A8UIntNormalized,
                    2,
                    poolSize);
            }

            auto srcTex = textureFromSurface(frame.Surface());
            D3D11_TEXTURE2D_DESC srcDesc{};
            srcTex->GetDesc(&srcDesc);
            const int fw = static_cast<int>(srcDesc.Width);
            const int fh = static_cast<int>(srcDesc.Height);
            if (!ensureTexture(fw, fh, nullptr) || !m_context)
                continue;

            {
                std::lock_guard lock(m_frameMutex);
                m_context->CopyResource(m_texture, srcTex.Get());
                m_ptsUs = timer.nsecsElapsed() / 1000;
            }
        } catch (winrt::hresult_error const& e) {
            Logger::warn(QStringLiteral("WGC frame error: 0x%1")
                             .arg(quint32(e.code()), 8, 16, QLatin1Char('0')));
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }

    try {
        if (session)
            session.Close();
        if (framePool)
            framePool.Close();
    } catch (...) {
    }
    return true;
}
#else
bool WindowsGraphicsCapture::captureLoopWgc(QString* initError)
{
    if (initError) *initError = QStringLiteral("WGC headers not available at build time");
    return false;
}
#endif

void WindowsGraphicsCapture::captureLoopBitBlt()
{
    HWND hwnd = reinterpret_cast<HWND>(m_hwnd);
    QElapsedTimer timer;
    timer.start();

    while (m_running.load()) {
        if (!hwnd || !IsWindow(hwnd)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        int w = 0;
        int h = 0;
        if (!windowPixelSize(hwnd, &w, &h) || !ensureTexture(w, h, nullptr) || !m_context) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            continue;
        }

        HDC hdcWindow = GetWindowDC(hwnd);
        if (!hdcWindow) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            continue;
        }

        HDC hdcMem = CreateCompatibleDC(hdcWindow);
        BITMAPINFO bmi{};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = w;
        bmi.bmiHeader.biHeight = -h;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        void* bits = nullptr;
        HBITMAP hbm = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
        if (!hbm || !bits) {
            if (hbm) DeleteObject(hbm);
            DeleteDC(hdcMem);
            ReleaseDC(hwnd, hdcWindow);
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            continue;
        }

        HGDIOBJ old = SelectObject(hdcMem, hbm);
        if (!PrintWindow(hwnd, hdcMem, 2 /* PW_RENDERFULLCONTENT */))
            BitBlt(hdcMem, 0, 0, w, h, hdcWindow, 0, 0, SRCCOPY | CAPTUREBLT);

        D3D11_TEXTURE2D_DESC desc{};
        {
            std::lock_guard lock(m_frameMutex);
            if (m_texture)
                m_texture->GetDesc(&desc);
        }

        if (m_texture && bits && desc.Width == static_cast<UINT>(w) && desc.Height == static_cast<UINT>(h)) {
            std::lock_guard lock(m_frameMutex);
            m_context->UpdateSubresource(m_texture, 0, nullptr, bits, static_cast<UINT>(w * 4), 0);
            m_ptsUs = timer.nsecsElapsed() / 1000;
        }

        SelectObject(hdcMem, old);
        DeleteObject(hbm);
        DeleteDC(hdcMem);
        ReleaseDC(hwnd, hdcWindow);

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}
#endif // _WIN32

void WindowsGraphicsCapture::captureLoop()
{
#ifdef _WIN32
    if (m_useWgc.load()) {
        QString initError;
        if (captureLoopWgc(&initError))
            return;

        if (m_kind == TargetKind::Window && m_running.load()) {
            Logger::warn(QStringLiteral("WGC failed (%1); falling back to BitBlt for %2")
                             .arg(initError, m_name));
            signalStartResult(true, {});
            captureLoopBitBlt();
            return;
        }

        signalStartResult(false, initError.isEmpty() ? QStringLiteral("WGC init failed") : initError);
        return;
    }

    if (m_kind == TargetKind::Window) {
        signalStartResult(true, {});
        captureLoopBitBlt();
        return;
    }

    signalStartResult(false, QStringLiteral("Windows.Graphics.Capture unavailable for monitor capture"));
#else
    signalStartResult(false, QStringLiteral("WGC is Windows-only"));
    while (m_running.load())
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
#endif
}

} // namespace railshot
