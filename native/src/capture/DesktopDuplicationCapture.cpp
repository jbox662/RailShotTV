#include "capture/DesktopDuplicationCapture.h"
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

DesktopDuplicationCapture::DesktopDuplicationCapture(QString id, QString name, int outputIndex)
    : m_id(std::move(id)), m_name(std::move(name)), m_outputIndex(outputIndex)
{
}

DesktopDuplicationCapture::~DesktopDuplicationCapture() { stop(); }

bool DesktopDuplicationCapture::recreateDuplication(QString* error)
{
#ifdef _WIN32
    if (m_duplication) { m_duplication->Release(); m_duplication = nullptr; }
    if (!m_device) {
        if (error) *error = QStringLiteral("No device");
        return false;
    }

    ComPtr<IDXGIDevice> dxgiDevice;
    if (FAILED(m_device->QueryInterface(IID_PPV_ARGS(&dxgiDevice)))) {
        if (error) *error = QStringLiteral("QueryInterface IDXGIDevice failed");
        return false;
    }
    ComPtr<IDXGIAdapter> adapter;
    dxgiDevice->GetAdapter(&adapter);
    ComPtr<IDXGIOutput> output;
    if (FAILED(adapter->EnumOutputs(static_cast<UINT>(m_outputIndex), &output))) {
        if (error) *error = QStringLiteral("EnumOutputs failed for index %1").arg(m_outputIndex);
        return false;
    }
    ComPtr<IDXGIOutput1> output1;
    if (FAILED(output.As(&output1))) {
        if (error) *error = QStringLiteral("IDXGIOutput1 unavailable");
        return false;
    }

    DXGI_OUTPUT_DESC od{};
    output->GetDesc(&od);
    m_width = od.DesktopCoordinates.right - od.DesktopCoordinates.left;
    m_height = od.DesktopCoordinates.bottom - od.DesktopCoordinates.top;

    HRESULT hr = output1->DuplicateOutput(m_device, &m_duplication);
    if (FAILED(hr)) {
        if (error) *error = QStringLiteral("DuplicateOutput failed: 0x%1").arg(quint32(hr), 8, 16, QLatin1Char('0'));
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
        if (error) *error = QStringLiteral("CreateTexture2D failed");
        return false;
    }
    std::lock_guard lock(m_frameMutex);
    if (m_texture) m_texture->Release();
    m_texture = tex.Detach();
    return true;
#else
    Q_UNUSED(error);
    return false;
#endif
}

bool DesktopDuplicationCapture::start(ID3D11Device* device, QString* error)
{
    if (m_running.load()) return true;
    m_device = device;
#ifdef _WIN32
    if (m_device)
        m_device->GetImmediateContext(&m_context);
    if (!recreateDuplication(error))
        return false;
    m_running = true;
    m_thread = std::thread([this] { captureLoop(); });
    Logger::info(QStringLiteral("Desktop Duplication started: %1 (%2x%3)")
                     .arg(m_name).arg(m_width).arg(m_height));
    return true;
#else
    Q_UNUSED(error);
    return false;
#endif
}

void DesktopDuplicationCapture::stop()
{
    if (!m_running.exchange(false)) return;
    if (m_thread.joinable()) m_thread.join();
#ifdef _WIN32
    if (m_duplication) { m_duplication->Release(); m_duplication = nullptr; }
    if (m_context) { m_context->Release(); m_context = nullptr; }
    std::lock_guard lock(m_frameMutex);
    if (m_texture) { m_texture->Release(); m_texture = nullptr; }
#endif
}

bool DesktopDuplicationCapture::acquireLatest(VideoFrame& out)
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

void DesktopDuplicationCapture::captureLoop()
{
#ifdef _WIN32
    QElapsedTimer timer;
    timer.start();
    while (m_running.load()) {
        if (!m_duplication) {
            QString err;
            if (!recreateDuplication(&err)) {
                Logger::warn(QStringLiteral("Desktop duplication recreate: %1").arg(err));
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                continue;
            }
        }

        DXGI_OUTDUPL_FRAME_INFO frameInfo{};
        ComPtr<IDXGIResource> resource;
        HRESULT hr = m_duplication->AcquireNextFrame(16, &frameInfo, &resource);
        if (hr == DXGI_ERROR_WAIT_TIMEOUT)
            continue;
        if (hr == DXGI_ERROR_ACCESS_LOST) {
            m_duplication->Release();
            m_duplication = nullptr;
            continue;
        }
        if (FAILED(hr)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            continue;
        }

        ComPtr<ID3D11Texture2D> srcTex;
        if (SUCCEEDED(resource.As(&srcTex)) && m_context && m_texture) {
            std::lock_guard lock(m_frameMutex);
            m_context->CopyResource(m_texture, srcTex.Get());
            m_ptsUs = timer.nsecsElapsed() / 1000;
        }
        m_duplication->ReleaseFrame();
    }
#else
    while (m_running.load())
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
#endif
}

} // namespace railshot
