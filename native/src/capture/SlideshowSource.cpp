#include "capture/SlideshowSource.h"
#include "core/Logger.h"
#include <QImage>
#include <algorithm>

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

SlideshowSource::SlideshowSource(QString id, QString name, QStringList paths, int intervalMs, bool loop)
    : m_id(std::move(id))
    , m_name(std::move(name))
    , m_paths(std::move(paths))
    , m_intervalMs((std::max)(250, intervalMs))
    , m_loop(loop)
{
}

SlideshowSource::~SlideshowSource() { stop(); }

bool SlideshowSource::loadIndex(int index, QString* error)
{
#ifdef _WIN32
    if (!m_device || m_paths.isEmpty() || index < 0 || index >= m_paths.size()) {
        if (error) *error = QStringLiteral("No slideshow images");
        return false;
    }
    QImage img(m_paths.at(index));
    if (img.isNull()) {
        if (error) *error = QStringLiteral("Failed to load image: %1").arg(m_paths.at(index));
        return false;
    }
    img = img.convertToFormat(QImage::Format_ARGB32);
    m_width = img.width();
    m_height = img.height();

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
        if (error) *error = QStringLiteral("CreateTexture2D failed for slideshow");
        return false;
    }
    if (m_texture) {
        m_texture->Release();
        m_texture = nullptr;
    }
    m_texture = tex.Detach();
    m_index = index;
    return true;
#else
    Q_UNUSED(index);
    Q_UNUSED(error);
    return false;
#endif
}

bool SlideshowSource::start(ID3D11Device* device, QString* error)
{
    m_device = device;
    if (m_paths.isEmpty()) {
        if (error) *error = QStringLiteral("Slideshow has no images");
        return false;
    }
#ifdef _WIN32
    if (!m_device) {
        if (error) *error = QStringLiteral("No D3D device");
        return false;
    }
    std::lock_guard lock(m_mutex);
    // Try first valid image
    for (int i = 0; i < m_paths.size(); ++i) {
        if (loadIndex(i, error)) {
            m_running = true;
            m_timer.restart();
            Logger::info(QStringLiteral("Slideshow started: %1 images, %2 ms")
                             .arg(m_paths.size())
                             .arg(m_intervalMs));
            return true;
        }
    }
    return false;
#else
    Q_UNUSED(error);
    return false;
#endif
}

void SlideshowSource::stop()
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

void SlideshowSource::advanceIfNeeded()
{
    if (!m_running || m_paths.size() <= 1)
        return;
    if (m_timer.elapsed() < m_intervalMs)
        return;
    m_timer.restart();
    int next = m_index + 1;
    if (next >= m_paths.size()) {
        if (!m_loop)
            return;
        next = 0;
    }
    // Skip unloadable frames
    for (int tries = 0; tries < m_paths.size(); ++tries) {
        if (loadIndex(next, nullptr))
            return;
        next = (next + 1) % m_paths.size();
        if (!m_loop && next == 0)
            return;
    }
}

bool SlideshowSource::acquireLatest(VideoFrame& out)
{
    std::lock_guard lock(m_mutex);
    advanceIfNeeded();
    if (!m_texture) return false;
    out.texture = m_texture;
    out.width = m_width;
    out.height = m_height;
    out.sourceId = m_id;
    out.opaque = false;
    return true;
}

} // namespace railshot
