#include "capture/TextSource.h"
#include "core/Logger.h"
#include <QImage>
#include <QPainter>
#include <QFont>

#ifdef _WIN32
#include <d3d11.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;
#endif

namespace railshot {

TextSource::TextSource(QString id, QString name, QString text)
    : m_id(std::move(id)), m_name(std::move(name)), m_text(std::move(text))
{
}

TextSource::~TextSource() { stop(); }

bool TextSource::rebuildTexture(QString* error)
{
#ifdef _WIN32
    if (!m_device) {
        if (error) *error = QStringLiteral("No D3D device");
        return false;
    }
    QImage img(m_width, m_height, QImage::Format_ARGB32);
    img.fill(Qt::transparent);
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    QFont font(QStringLiteral("Segoe UI"), m_fontSize, QFont::Bold);
    p.setFont(font);
    p.setPen(QColor(QStringLiteral("#F8F8FF")));
    p.drawText(img.rect().adjusted(8, 8, -8, -8), Qt::AlignLeft | Qt::AlignVCenter, m_text);
    p.end();

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
        if (error) *error = QStringLiteral("Text texture create failed");
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

bool TextSource::start(ID3D11Device* device, QString* error)
{
    m_device = device;
    std::lock_guard lock(m_mutex);
    if (!rebuildTexture(error)) return false;
    m_running = true;
    return true;
}

void TextSource::stop()
{
    std::lock_guard lock(m_mutex);
    m_running = false;
#ifdef _WIN32
    if (m_texture) { m_texture->Release(); m_texture = nullptr; }
#endif
}

bool TextSource::acquireLatest(VideoFrame& out)
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

void TextSource::setText(const QString& text)
{
    std::lock_guard lock(m_mutex);
    m_text = text;
    rebuildTexture(nullptr);
}

void TextSource::setFontSize(int px)
{
    std::lock_guard lock(m_mutex);
    m_fontSize = px;
    rebuildTexture(nullptr);
}

} // namespace railshot
