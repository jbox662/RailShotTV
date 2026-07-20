#include "capture/ImageSource.h"
#include "core/Logger.h"
#include <QImage>

#ifdef _WIN32
#include <d3d11.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;
#endif

namespace railshot {

ImageSource::ImageSource(QString id, QString name, QString filePath)
    : m_id(std::move(id)), m_name(std::move(name)), m_filePath(std::move(filePath))
{
}

ImageSource::~ImageSource() { stop(); }

bool ImageSource::start(ID3D11Device* device, QString* error)
{
    m_device = device;
    QImage img(m_filePath);
    if (img.isNull()) {
        if (error) *error = QStringLiteral("Failed to load image: %1").arg(m_filePath);
        return false;
    }
    img = img.convertToFormat(QImage::Format_ARGB32);
    m_width = img.width();
    m_height = img.height();
#ifdef _WIN32
    if (!m_device) {
        if (error) *error = QStringLiteral("No D3D device");
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
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA init{};
    init.pSysMem = img.bits();
    init.SysMemPitch = static_cast<UINT>(img.bytesPerLine());

    ComPtr<ID3D11Texture2D> tex;
    if (FAILED(m_device->CreateTexture2D(&desc, &init, &tex))) {
        if (error) *error = QStringLiteral("CreateTexture2D failed for image");
        return false;
    }
    std::lock_guard lock(m_mutex);
    m_texture = tex.Detach();
    m_running = true;
    Logger::info(QStringLiteral("Image source loaded: %1 (%2x%3)").arg(m_filePath).arg(m_width).arg(m_height));
    return true;
#else
    Q_UNUSED(error);
    return false;
#endif
}

void ImageSource::stop()
{
    std::lock_guard lock(m_mutex);
    m_running = false;
#ifdef _WIN32
    if (m_texture) { m_texture->Release(); m_texture = nullptr; }
#endif
}

bool ImageSource::acquireLatest(VideoFrame& out)
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
