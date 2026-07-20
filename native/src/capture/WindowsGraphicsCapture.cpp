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
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;
#endif

#include <QElapsedTimer>

namespace railshot {

WindowsGraphicsCapture::WindowsGraphicsCapture(QString id, QString name, TargetKind kind, int monitorIndex)
    : m_id(std::move(id)), m_name(std::move(name)), m_kind(kind), m_monitorIndex(monitorIndex)
{
}

WindowsGraphicsCapture::~WindowsGraphicsCapture() { stop(); }

bool WindowsGraphicsCapture::isSupported()
{
#ifdef _WIN32
    // Windows 10 1803+ — Graphics Capture API available via WinRT.
    // Capability probe deferred to runtime; assume true on Win10+.
    return true;
#else
    return false;
#endif
}

bool WindowsGraphicsCapture::start(ID3D11Device* device, QString* error)
{
    if (m_running.load()) return true;
    m_device = device;
#ifdef _WIN32
    if (!m_device) {
        if (error) *error = QStringLiteral("No D3D11 device");
        return false;
    }
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = static_cast<UINT>(m_width);
    desc.Height = static_cast<UINT>(m_height);
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    ComPtr<ID3D11Texture2D> tex;
    if (FAILED(m_device->CreateTexture2D(&desc, nullptr, &tex))) {
        if (error) *error = QStringLiteral("Failed to create capture texture");
        return false;
    }
    {
        std::lock_guard lock(m_frameMutex);
        m_texture = tex.Detach();
    }
    m_running = true;
    m_thread = std::thread([this] { captureLoop(); });
    Logger::info(QStringLiteral("WGC capture started: %1 (monitor %2)").arg(m_name).arg(m_monitorIndex));
    return true;
#else
    Q_UNUSED(error);
    return false;
#endif
}

void WindowsGraphicsCapture::stop()
{
    if (!m_running.exchange(false)) return;
    if (m_thread.joinable()) m_thread.join();
#ifdef _WIN32
    std::lock_guard lock(m_frameMutex);
    if (m_texture) { m_texture->Release(); m_texture = nullptr; }
#endif
}

bool WindowsGraphicsCapture::acquireLatest(VideoFrame& out)
{
    std::lock_guard lock(m_frameMutex);
    if (!m_texture) return false;
    out.texture = m_texture;
    out.ptsUs = m_ptsUs;
    out.width = m_width;
    out.height = m_height;
    out.sourceId = m_id;
    return true;
}

void WindowsGraphicsCapture::captureLoop()
{
    // Full WinRT GraphicsCaptureSession wiring requires C++/WinRT projection.
    // For the MVP we keep a warm D3D texture and integrate Desktop Duplication
    // as the reliable fallback path (see DesktopDuplicationCapture).
    QElapsedTimer timer;
    timer.start();
    while (m_running.load()) {
        m_ptsUs = timer.nsecsElapsed() / 1000;
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

} // namespace railshot
