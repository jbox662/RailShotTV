#include "capture/AudioSource.h"
#include "audio/WasapiCapture.h"
#include "audio/AudioGraph.h"
#include "core/Logger.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <d3d11.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;
#endif

namespace railshot {

AudioSource::AudioSource(QString id, QString name, AudioDeviceKind kind, QString deviceId, AudioGraph* graph)
    : m_id(std::move(id))
    , m_name(std::move(name))
    , m_kind(kind)
    , m_deviceId(std::move(deviceId))
    , m_graph(graph)
{
}

AudioSource::~AudioSource()
{
    stop();
}

bool AudioSource::ensureTexture(QString* error)
{
#ifdef _WIN32
    if (m_texture || !m_device) return m_texture != nullptr;
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = 320;
    desc.Height = 180;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    ComPtr<ID3D11Texture2D> tex;
    if (FAILED(m_device->CreateTexture2D(&desc, nullptr, &tex))) {
        if (error) *error = QStringLiteral("Failed to create audio placeholder texture");
        return false;
    }
    // Fill dark navy
    ComPtr<ID3D11DeviceContext> ctx;
    m_device->GetImmediateContext(&ctx);
    ComPtr<ID3D11RenderTargetView> rtv;
    if (SUCCEEDED(m_device->CreateRenderTargetView(tex.Get(), nullptr, &rtv))) {
        const float clear[4] = {0.06f, 0.08f, 0.12f, 1.0f};
        ctx->ClearRenderTargetView(rtv.Get(), clear);
    }
    m_texture = tex.Detach();
    return true;
#else
    Q_UNUSED(error);
    return false;
#endif
}

bool AudioSource::start(ID3D11Device* device, QString* error)
{
    if (m_running.load()) return true;
    m_device = device;
    if (!ensureTexture(error))
        return false;

    if (m_graph)
        m_graph->ensureChannel(m_id, m_name);

    m_capture = std::make_unique<WasapiCapture>(m_kind, m_deviceId);
    const QString id = m_id;
    AudioGraph* graph = m_graph;
    m_capture->setCallback([id, graph](const AudioBuffer& buf) {
        if (graph)
            graph->inject(id, buf);
    });
    if (!m_capture->start(error)) {
        m_capture.reset();
        return false;
    }
    m_running = true;
    Logger::info(QStringLiteral("Audio source started: %1").arg(m_name));
    return true;
}

void AudioSource::stop()
{
    if (!m_running.exchange(false)) return;
    if (m_capture) {
        m_capture->stop();
        m_capture.reset();
    }
    if (m_graph)
        m_graph->removeChannel(m_id);
#ifdef _WIN32
    std::lock_guard lock(m_mutex);
    if (m_texture) {
        m_texture->Release();
        m_texture = nullptr;
    }
#endif
}

bool AudioSource::acquireLatest(VideoFrame& out)
{
    std::lock_guard lock(m_mutex);
    if (!m_texture) return false;
    out.texture = m_texture;
    out.ptsUs = ++m_ptsUs;
    out.width = 320;
    out.height = 180;
    out.sourceId = m_id;
    return true;
}

} // namespace railshot
