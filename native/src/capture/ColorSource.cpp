#include "capture/ColorSource.h"
#include "core/Logger.h"
#include <QImage>

#ifdef _WIN32
#include <d3d11.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;
#endif

namespace railshot {

ColorSource::ColorSource(QString id, QString name, QColor color, int width, int height)
    : m_id(std::move(id))
    , m_name(std::move(name))
    , m_color(color.isValid() ? color : QColor(QStringLiteral("#1A2035")))
    , m_width(width > 0 ? width : 1920)
    , m_height(height > 0 ? height : 1080)
{
}

ColorSource::~ColorSource() { stop(); }

bool ColorSource::rebuildTexture(QString* error)
{
#ifdef _WIN32
    if (!m_device) {
        if (error) *error = QStringLiteral("No D3D device");
        return false;
    }
    QImage img(m_width, m_height, QImage::Format_ARGB32);
    img.fill(m_color.rgba());

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
    init.pSysMem = img.bits();
    init.SysMemPitch = static_cast<UINT>(img.bytesPerLine());

    ComPtr<ID3D11Texture2D> tex;
    if (FAILED(m_device->CreateTexture2D(&desc, &init, &tex))) {
        if (error) *error = QStringLiteral("CreateTexture2D failed for colour source");
        return false;
    }
    if (m_texture) {
        m_texture->Release();
        m_texture = nullptr;
    }
    m_texture = tex.Detach();
    return true;
#else
    Q_UNUSED(error);
    return false;
#endif
}

bool ColorSource::start(ID3D11Device* device, QString* error)
{
    m_device = device;
    if (!rebuildTexture(error))
        return false;
    m_running = true;
    Logger::info(QStringLiteral("Colour source ready: %1").arg(m_color.name()));
    return true;
}

void ColorSource::stop()
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

void ColorSource::setColor(const QColor& color)
{
    if (!color.isValid()) return;
    std::lock_guard lock(m_mutex);
    m_color = color;
    if (m_running)
        rebuildTexture(nullptr);
}

bool ColorSource::acquireLatest(VideoFrame& out)
{
    std::lock_guard lock(m_mutex);
    if (!m_texture) return false;
    out.texture = m_texture;
    out.width = m_width;
    out.height = m_height;
    out.sourceId = m_id;
    out.opaque = true;
    return true;
}

} // namespace railshot
