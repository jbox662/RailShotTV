#include "capture/MediaSource.h"
#include "core/Logger.h"
#include <QFileInfo>
#include <QImage>
#include <QThread>
#include <cstring>

#ifdef _WIN32
#include <d3d11.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;
#endif

#if defined(RAILSHOT_HAS_FFMPEG) && RAILSHOT_HAS_FFMPEG
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}
#endif

namespace railshot {

MediaSource::MediaSource(QString id, QString name, QString filePath, bool loop)
    : m_id(std::move(id))
    , m_name(std::move(name))
    , m_filePath(std::move(filePath))
    , m_loop(loop)
{
}

MediaSource::~MediaSource() { stop(); }

void MediaSource::setPath(const QString& path)
{
    m_filePath = path;
}

bool MediaSource::uploadFrame(const uint8_t* bgra, int stride, int w, int h)
{
#ifdef _WIN32
    if (!m_device || !bgra || w <= 0 || h <= 0) return false;
    std::lock_guard lock(m_mutex);
    if (!m_texture || m_width != w || m_height != h) {
        if (m_texture) {
            m_texture->Release();
            m_texture = nullptr;
        }
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = static_cast<UINT>(w);
        desc.Height = static_cast<UINT>(h);
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        ComPtr<ID3D11Texture2D> tex;
        if (FAILED(m_device->CreateTexture2D(&desc, nullptr, &tex)))
            return false;
        m_texture = tex.Detach();
        m_width = w;
        m_height = h;
        if (!m_context)
            m_device->GetImmediateContext(&m_context);
    }
    if (!m_context) return false;
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(m_context->Map(m_texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        return false;
    for (int y = 0; y < h; ++y) {
        std::memcpy(static_cast<uint8_t*>(mapped.pData) + y * mapped.RowPitch,
                    bgra + y * stride, static_cast<size_t>(w) * 4);
    }
    m_context->Unmap(m_texture, 0);
    return true;
#else
    Q_UNUSED(bgra); Q_UNUSED(stride); Q_UNUSED(w); Q_UNUSED(h);
    return false;
#endif
}

bool MediaSource::startStill(QString* error)
{
    QImage img(m_filePath);
    if (img.isNull()) {
        if (error) *error = QStringLiteral("Failed to load media: %1").arg(m_filePath);
        return false;
    }
    img = img.convertToFormat(QImage::Format_ARGB32);
    if (!uploadFrame(img.constBits(), img.bytesPerLine(), img.width(), img.height())) {
        if (error) *error = QStringLiteral("Failed to upload media texture");
        return false;
    }
    m_running = true;
    return true;
}

bool MediaSource::startFfmpeg(QString* error)
{
#if defined(RAILSHOT_HAS_FFMPEG) && RAILSHOT_HAS_FFMPEG
    m_stop = false;
    m_running = true;
    m_thread = std::thread([this] { decodeLoop(); });
    // Give decoder a moment; if still fails, caller sees empty frames
    QThread::msleep(50);
    if (!m_running.load()) {
        if (error) *error = QStringLiteral("FFmpeg failed to open media");
        return false;
    }
    return true;
#else
    Q_UNUSED(error);
    return false;
#endif
}

void MediaSource::decodeLoop()
{
#if defined(RAILSHOT_HAS_FFMPEG) && RAILSHOT_HAS_FFMPEG
    const QByteArray path = m_filePath.toUtf8();
    do {
        AVFormatContext* fmt = nullptr;
        if (avformat_open_input(&fmt, path.constData(), nullptr, nullptr) < 0) {
            m_running = false;
            return;
        }
        if (avformat_find_stream_info(fmt, nullptr) < 0) {
            avformat_close_input(&fmt);
            m_running = false;
            return;
        }
        int vIndex = av_find_best_stream(fmt, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        if (vIndex < 0) {
            avformat_close_input(&fmt);
            m_running = false;
            return;
        }
        AVStream* st = fmt->streams[vIndex];
        const AVCodec* codec = avcodec_find_decoder(st->codecpar->codec_id);
        AVCodecContext* dec = avcodec_alloc_context3(codec);
        avcodec_parameters_to_context(dec, st->codecpar);
        if (avcodec_open2(dec, codec, nullptr) < 0) {
            avcodec_free_context(&dec);
            avformat_close_input(&fmt);
            m_running = false;
            return;
        }

        SwsContext* sws = nullptr;
        AVFrame* frame = av_frame_alloc();
        AVFrame* bgra = av_frame_alloc();
        AVPacket* pkt = av_packet_alloc();
        uint8_t* buf = nullptr;
        int bufSize = 0;

        auto cleanup = [&] {
            if (buf) av_free(buf);
            av_frame_free(&bgra);
            av_frame_free(&frame);
            av_packet_free(&pkt);
            if (sws) sws_freeContext(sws);
            avcodec_free_context(&dec);
            avformat_close_input(&fmt);
        };

        while (!m_stop.load()) {
            const int r = av_read_frame(fmt, pkt);
            if (r < 0) {
                if (m_loop) {
                    av_seek_frame(fmt, vIndex, 0, AVSEEK_FLAG_BACKWARD);
                    avcodec_flush_buffers(dec);
                    continue;
                }
                break;
            }
            if (pkt->stream_index != vIndex) {
                av_packet_unref(pkt);
                continue;
            }
            if (avcodec_send_packet(dec, pkt) == 0) {
                while (avcodec_receive_frame(dec, frame) == 0) {
                    if (!sws) {
                        sws = sws_getContext(frame->width, frame->height, static_cast<AVPixelFormat>(frame->format),
                                             frame->width, frame->height, AV_PIX_FMT_BGRA,
                                             SWS_BILINEAR, nullptr, nullptr, nullptr);
                        bufSize = av_image_get_buffer_size(AV_PIX_FMT_BGRA, frame->width, frame->height, 1);
                        buf = static_cast<uint8_t*>(av_malloc(bufSize));
                        av_image_fill_arrays(bgra->data, bgra->linesize, buf, AV_PIX_FMT_BGRA,
                                             frame->width, frame->height, 1);
                    }
                    sws_scale(sws, frame->data, frame->linesize, 0, frame->height, bgra->data, bgra->linesize);
                    uploadFrame(bgra->data[0], bgra->linesize[0], frame->width, frame->height);
                    // Pace roughly by stream timebase
                    const double fps = av_q2d(st->avg_frame_rate) > 1.0 ? av_q2d(st->avg_frame_rate) : 30.0;
                    QThread::msleep(static_cast<unsigned long>(1000.0 / fps));
                }
            }
            av_packet_unref(pkt);
        }
        cleanup();
    } while (m_loop && !m_stop.load());
    m_running = false;
#endif
}

bool MediaSource::start(ID3D11Device* device, QString* error)
{
    m_device = device;
    if (m_filePath.isEmpty() || !QFileInfo::exists(m_filePath)) {
        if (error) *error = QStringLiteral("Media path missing: %1").arg(m_filePath);
        return false;
    }
    const QString ext = QFileInfo(m_filePath).suffix().toLower();
    const bool still = (ext == QLatin1String("png") || ext == QLatin1String("jpg")
                        || ext == QLatin1String("jpeg") || ext == QLatin1String("bmp")
                        || ext == QLatin1String("webp") || ext == QLatin1String("gif"));
#if defined(RAILSHOT_HAS_FFMPEG) && RAILSHOT_HAS_FFMPEG
    if (!still) {
        if (startFfmpeg(error))
            return true;
        Logger::warn(QStringLiteral("FFmpeg media open failed, trying still decode"));
    }
#else
    Q_UNUSED(still);
#endif
    return startStill(error);
}

void MediaSource::stop()
{
    m_stop = true;
    if (m_thread.joinable())
        m_thread.join();
    m_running = false;
    std::lock_guard lock(m_mutex);
#ifdef _WIN32
    if (m_texture) {
        m_texture->Release();
        m_texture = nullptr;
    }
    if (m_context) {
        m_context->Release();
        m_context = nullptr;
    }
#endif
}

bool MediaSource::acquireLatest(VideoFrame& out)
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
