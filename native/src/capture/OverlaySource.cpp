#include "capture/OverlaySource.h"
#include "capture/OverlayRenderer.h"
#include "core/Logger.h"
#include <QImage>
#include <cstring>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <d3d11.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;
#endif

namespace railshot {

OverlaySource::OverlaySource(QString id, QString name, SourceType type, QJsonObject settings)
    : m_id(std::move(id))
    , m_name(std::move(name))
    , m_type(type)
    , m_settings(std::move(settings))
{
    if (m_type == SourceType::Scoreboard) {
        m_width = kScoreboardTexWidth;
        m_height = kScoreboardTexHeight;
    }
}

OverlaySource::~OverlaySource()
{
    stop();
}

void OverlaySource::setCanvasSize(int width, int height)
{
    std::lock_guard lock(m_mutex);
    // Scoreboard is a compact bug texture — never stretch it to full canvas
    // (that caused circles/text to squash when the source was placed as a strip).
    if (m_type == SourceType::Scoreboard) {
        m_width = kScoreboardTexWidth;
        m_height = kScoreboardTexHeight;
    } else {
        if (width > 0) m_width = width;
        if (height > 0) m_height = height;
    }
    if (m_running.load())
        rebuildTexture(nullptr);
}

void OverlaySource::applySettings(const QJsonObject& settings)
{
    std::lock_guard lock(m_mutex);
    m_settings = settings;
    if (m_running.load())
        rebuildTexture(nullptr);
}

bool OverlaySource::rebuildTexture(QString* error)
{
#ifdef _WIN32
    if (!m_device) {
        if (error) *error = QStringLiteral("No D3D device");
        return false;
    }

    OverlayRenderer renderer;
    QImage img;
    if (m_type == SourceType::Scoreboard) {
        img = renderer.renderScoreboard(m_settings, m_width, m_height);
    } else if (m_type == SourceType::Alert) {
        img = renderer.renderAlert(
            m_settings.value(QStringLiteral("title")).toString(m_name),
            m_settings.value(QStringLiteral("body")).toString(),
            m_width, m_height);
    } else {
        img = renderer.renderLowerThird(
            m_settings.value(QStringLiteral("title")).toString(m_name),
            m_settings.value(QStringLiteral("subtitle")).toString(),
            m_width, m_height);
    }

    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = static_cast<UINT>(m_width);
    desc.Height = static_cast<UINT>(m_height);
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA init{};
    init.pSysMem = img.constBits();
    init.SysMemPitch = static_cast<UINT>(img.bytesPerLine());

    ComPtr<ID3D11Texture2D> tex;
    if (FAILED(m_device->CreateTexture2D(&desc, &init, &tex))) {
        if (error) *error = QStringLiteral("Overlay texture create failed");
        return false;
    }
    if (m_texture) m_texture->Release();
    m_texture = tex.Detach();
    return true;
#else
    Q_UNUSED(error);
    return false;
#endif
}

bool OverlaySource::start(ID3D11Device* device, QString* error)
{
    m_device = device;
    std::lock_guard lock(m_mutex);
    if (!rebuildTexture(error))
        return false;
    m_running = true;
    return true;
}

void OverlaySource::stop()
{
    std::lock_guard lock(m_mutex);
    m_running = false;
#ifdef _WIN32
    if (m_texture) {
        m_texture->Release();
        m_texture = nullptr;
    }
#endif
}

bool OverlaySource::acquireLatest(VideoFrame& out)
{
    std::lock_guard lock(m_mutex);
    if (!m_texture) return false;
    out.texture = m_texture;
    out.width = m_width;
    out.height = m_height;
    out.sourceId = m_id;
    out.opaque = false;
    return true;
}

} // namespace railshot
