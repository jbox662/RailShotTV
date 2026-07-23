#include "capture/SlideshowSource.h"
#include "core/Logger.h"
#include <QRandomGenerator>
#include <algorithm>
#include <cstring>

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

SlideshowSource::SlideshowSource(QString id, QString name, QStringList paths, int intervalMs, bool loop,
                                 QString transition, int transitionMs, bool randomize, int swipeDir)
    : m_id(std::move(id))
    , m_name(std::move(name))
    , m_paths(std::move(paths))
    , m_intervalMs((std::max)(250, intervalMs))
    , m_loop(loop)
    , m_transition(std::move(transition))
    , m_transitionMs((std::max)(0, transitionMs))
    , m_randomize(randomize)
    , m_swipeDir(std::clamp(swipeDir, 0, 3))
{
}

SlideshowSource::~SlideshowSource() { stop(); }

bool SlideshowSource::loadImage(int index, QImage* out, QString* error)
{
    if (!out || m_paths.isEmpty() || index < 0 || index >= m_paths.size()) {
        if (error) *error = QStringLiteral("No slideshow images");
        return false;
    }
    QImage img(m_paths.at(index));
    if (img.isNull()) {
        if (error) *error = QStringLiteral("Failed to load image: %1").arg(m_paths.at(index));
        return false;
    }
    *out = img.convertToFormat(QImage::Format_ARGB32);
    return true;
}

bool SlideshowSource::uploadImage(const QImage& img)
{
#ifdef _WIN32
    if (!m_device || img.isNull()) return false;
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
    if (FAILED(m_device->CreateTexture2D(&desc, &init, &tex)))
        return false;
    if (m_texture) {
        m_texture->Release();
        m_texture = nullptr;
    }
    m_texture = tex.Detach();
    return true;
#else
    Q_UNUSED(img);
    return false;
#endif
}

int SlideshowSource::pickNextIndex() const
{
    if (m_paths.size() <= 1)
        return m_index;
    if (m_randomize) {
        for (int tries = 0; tries < 16; ++tries) {
            const int n = QRandomGenerator::global()->bounded(m_paths.size());
            if (n != m_index)
                return n;
        }
        return (m_index + 1) % m_paths.size();
    }
    int next = m_index + 1;
    if (next >= m_paths.size())
        return m_loop ? 0 : m_index;
    return next;
}

int SlideshowSource::pickPrevIndex() const
{
    if (m_paths.size() <= 1)
        return m_index;
    if (m_randomize)
        return pickNextIndex();
    int prev = m_index - 1;
    if (prev < 0)
        return m_loop ? m_paths.size() - 1 : m_index;
    return prev;
}

void SlideshowSource::requestStep(int delta)
{
    std::lock_guard lock(m_mutex);
    if (delta == 0) return;
    m_pendingStep = delta > 0 ? 1 : -1;
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
    for (int i = 0; i < m_paths.size(); ++i) {
        QImage img;
        if (!loadImage(i, &img, error))
            continue;
        if (!uploadImage(img)) {
            if (error) *error = QStringLiteral("CreateTexture2D failed for slideshow");
            return false;
        }
        m_toImg = img;
        m_fromImg = img;
        m_index = i;
        m_running = true;
        m_fading = false;
        m_pendingStep = 0;
        m_slideTimer.restart();
        Logger::info(QStringLiteral("Slideshow started: %1 images, %2 ms, %3")
                         .arg(m_paths.size())
                         .arg(m_intervalMs)
                         .arg(m_transition));
        return true;
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
    m_fading = false;
    m_pendingStep = 0;
#ifdef _WIN32
    if (m_texture) {
        m_texture->Release();
        m_texture = nullptr;
    }
#endif
    m_fromImg = {};
    m_toImg = {};
}

void SlideshowSource::beginAdvance()
{
    beginAdvanceTo(pickNextIndex());
}

void SlideshowSource::beginAdvanceTo(int tryIdx)
{
    if (!m_running || m_paths.size() <= 1 || m_fading)
        return;
    if (tryIdx == m_index && !m_loop)
        return;

    QImage nextImg;
    bool loaded = false;
    for (int tries = 0; tries < m_paths.size(); ++tries) {
        if (loadImage(tryIdx, &nextImg, nullptr)) {
            loaded = true;
            break;
        }
        tryIdx = (tryIdx + 1) % m_paths.size();
        if (!m_loop && tryIdx == 0)
            return;
    }
    if (!loaded)
        return;

    const bool animated = (m_transition.compare(QLatin1String("fade"), Qt::CaseInsensitive) == 0
                           || m_transition.compare(QLatin1String("swipe"), Qt::CaseInsensitive) == 0)
                          && m_transitionMs > 0;
    if (!animated) {
        if (!uploadImage(nextImg))
            return;
        m_fromImg = nextImg;
        m_toImg = nextImg;
        m_index = tryIdx;
        m_slideTimer.restart();
        return;
    }

    m_fromImg = m_toImg.isNull() ? nextImg : m_toImg;
    m_toImg = nextImg;
    m_index = tryIdx;
    m_fading = true;
    m_fadeTimer.restart();
    tickTransition();
}

void SlideshowSource::tickTransition()
{
    if (!m_fading)
        return;
    const float t = m_transitionMs > 0
                        ? float(m_fadeTimer.elapsed()) / float(m_transitionMs)
                        : 1.f;
    const float p = (std::min)(1.f, (std::max)(0.f, t));

    QImage a = m_fromImg;
    QImage b = m_toImg;
    if (a.size() != b.size())
        a = a.scaled(b.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    if (a.format() != QImage::Format_ARGB32)
        a = a.convertToFormat(QImage::Format_ARGB32);
    if (b.format() != QImage::Format_ARGB32)
        b = b.convertToFormat(QImage::Format_ARGB32);

    QImage out(b.size(), QImage::Format_ARGB32);
    const int w = out.width();
    const int h = out.height();
    const bool swipe = m_transition.compare(QLatin1String("swipe"), Qt::CaseInsensitive) == 0;

    if (swipe) {
        for (int y = 0; y < h; ++y) {
            const QRgb* ra = reinterpret_cast<const QRgb*>(a.constScanLine(y));
            const QRgb* rb = reinterpret_cast<const QRgb*>(b.constScanLine(y));
            QRgb* ro = reinterpret_cast<QRgb*>(out.scanLine(y));
            for (int x = 0; x < w; ++x) {
                float edge = 0.f;
                if (m_swipeDir == 0) edge = float(x) / float((std::max)(1, w - 1));
                else if (m_swipeDir == 1) edge = 1.f - float(x) / float((std::max)(1, w - 1));
                else if (m_swipeDir == 2) edge = float(y) / float((std::max)(1, h - 1));
                else edge = 1.f - float(y) / float((std::max)(1, h - 1));
                ro[x] = (edge < p) ? rb[x] : ra[x];
            }
        }
    } else {
        const int bytes = out.sizeInBytes();
        const uchar* pa = a.constBits();
        const uchar* pb = b.constBits();
        uchar* po = out.bits();
        const float inv = 1.f - p;
        for (int i = 0; i < bytes; ++i)
            po[i] = static_cast<uchar>(pa[i] * inv + pb[i] * p + 0.5f);
    }
    uploadImage(out);

    if (p >= 1.f - 1e-4f) {
        m_fading = false;
        m_fromImg = m_toImg;
        uploadImage(m_toImg);
        m_slideTimer.restart();
    }
}

bool SlideshowSource::acquireLatest(VideoFrame& out)
{
    std::lock_guard lock(m_mutex);
    if (!m_running)
        return false;
    if (m_pendingStep != 0 && !m_fading) {
        const int step = m_pendingStep;
        m_pendingStep = 0;
        beginAdvanceTo(step > 0 ? pickNextIndex() : pickPrevIndex());
    }
    if (m_fading)
        tickTransition();
    else if (m_slideTimer.elapsed() >= m_intervalMs)
        beginAdvance();
    if (!m_texture) return false;
    out.texture = m_texture;
    out.width = m_width;
    out.height = m_height;
    out.sourceId = m_id;
    out.opaque = false;
    return true;
}

} // namespace railshot
