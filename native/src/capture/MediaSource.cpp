#include "capture/MediaSource.h"
#include "core/Logger.h"
#include <QFileInfo>
#include <QImage>
#include <QThread>
#include <QUrl>
#include <cstring>
#include <vector>
#include <algorithm>

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
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}
#endif

namespace railshot {

namespace {

#if defined(RAILSHOT_HAS_FFMPEG) && RAILSHOT_HAS_FFMPEG
void applyFfmpegOptionString(AVDictionary** opts, const QString& options)
{
    const QStringList parts = options.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    for (const QString& part : parts) {
        const int eq = part.indexOf(QLatin1Char('='));
        if (eq <= 0) continue;
        const QByteArray key = part.left(eq).trimmed().toUtf8();
        const QByteArray val = part.mid(eq + 1).trimmed().toUtf8();
        if (!key.isEmpty())
            av_dict_set(opts, key.constData(), val.constData(), 0);
    }
}
#endif

} // namespace

bool MediaSource::looksLikeNetworkUrl(const QString& input)
{
    const QString t = input.trimmed();
    if (t.isEmpty()) return false;
    const QUrl url = QUrl::fromUserInput(t);
    if (!url.isValid()) return false;
    const QString scheme = url.scheme().toLower();
    return scheme == QLatin1String("rtsp")
        || scheme == QLatin1String("rtsps")
        || scheme == QLatin1String("rtmp")
        || scheme == QLatin1String("rtmps")
        || scheme == QLatin1String("http")
        || scheme == QLatin1String("https")
        || scheme == QLatin1String("udp")
        || scheme == QLatin1String("tcp")
        || scheme == QLatin1String("srt")
        || scheme == QLatin1String("mms")
        || scheme == QLatin1String("mmsh");
}

MediaSource::MediaSource(QString id, QString name, QString input, bool loop,
                         bool isLocalFile, QString ffmpegOptions)
    : m_id(std::move(id))
    , m_name(std::move(name))
    , m_filePath(std::move(input))
    , m_loop(loop)
    , m_isLocalFile(isLocalFile)
    , m_ffmpegOptions(std::move(ffmpegOptions))
{
}

MediaSource::~MediaSource() { stop(); }

void MediaSource::setPath(const QString& path)
{
    m_filePath = path;
}

void MediaSource::setLoop(bool loop)
{
    m_loop = loop;
}

void MediaSource::setLocalFile(bool local)
{
    m_isLocalFile = local;
}

void MediaSource::setFfmpegOptions(const QString& opts)
{
    m_ffmpegOptions = opts;
}

bool MediaSource::useNetworkPath() const
{
    if (!m_isLocalFile)
        return true;
    return looksLikeNetworkUrl(m_filePath);
}

void MediaSource::setAudioCallback(std::function<void(const AudioBuffer&)> cb)
{
    std::lock_guard lock(m_audioCbMutex);
    m_audioCb = std::move(cb);
}

void MediaSource::emitAudio(const float* interleaved, int frames, int channels, int sampleRate)
{
    if (!interleaved || frames <= 0 || channels <= 0) return;
    AudioBuffer buf;
    buf.channels = channels;
    buf.sampleRate = sampleRate > 0 ? sampleRate : kAudioSampleRate;
    buf.samples.assign(interleaved, interleaved + size_t(frames) * size_t(channels));
    std::function<void(const AudioBuffer&)> cb;
    {
        std::lock_guard lock(m_audioCbMutex);
        cb = m_audioCb;
    }
    if (cb) cb(buf);
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
    QThread::msleep(80);
    if (!m_running.load()) {
        if (error) *error = QStringLiteral("FFmpeg failed to open media: %1").arg(m_filePath);
        return false;
    }
    return true;
#else
    if (error) *error = QStringLiteral("FFmpeg not available for network/media playback");
    return false;
#endif
}

void MediaSource::decodeLoop()
{
#if defined(RAILSHOT_HAS_FFMPEG) && RAILSHOT_HAS_FFMPEG
    const bool network = useNetworkPath();
    if (network)
        avformat_network_init();

    const QByteArray path = m_filePath.trimmed().toUtf8();
    // Live network: reconnect until stopped. Local: honor loop.
    const bool reconnect = network || m_loop;

    do {
        AVFormatContext* fmt = nullptr;
        AVDictionary* openOpts = nullptr;
        if (network) {
            // Sensible defaults for IP cameras / HLS (overridable via ffmpeg options).
            av_dict_set(&openOpts, "rtsp_transport", "tcp", 0);
            av_dict_set(&openOpts, "stimeout", "5000000", 0);
            av_dict_set(&openOpts, "rw_timeout", "5000000", 0);
            av_dict_set(&openOpts, "fflags", "nobuffer", 0);
            av_dict_set(&openOpts, "flags", "low_delay", 0);
            av_dict_set(&openOpts, "max_delay", "500000", 0);
            av_dict_set(&openOpts, "reconnect", "1", 0);
            av_dict_set(&openOpts, "reconnect_streamed", "1", 0);
            av_dict_set(&openOpts, "reconnect_delay_max", "5", 0);
        }
        applyFfmpegOptionString(&openOpts, m_ffmpegOptions);

        if (avformat_open_input(&fmt, path.constData(), nullptr, &openOpts) < 0) {
            av_dict_free(&openOpts);
            if (network && !m_stop.load()) {
                Logger::warn(QStringLiteral("Media network open failed, retrying: %1").arg(m_filePath));
                QThread::msleep(1000);
                continue;
            }
            m_running = false;
            return;
        }
        av_dict_free(&openOpts);

        if (avformat_find_stream_info(fmt, nullptr) < 0) {
            avformat_close_input(&fmt);
            if (network && !m_stop.load()) {
                QThread::msleep(1000);
                continue;
            }
            m_running = false;
            return;
        }
        int vIndex = av_find_best_stream(fmt, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        int aIndex = av_find_best_stream(fmt, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
        if (vIndex < 0) {
            avformat_close_input(&fmt);
            m_running = false;
            return;
        }

        AVStream* vst = fmt->streams[vIndex];
        const AVCodec* vcodec = avcodec_find_decoder(vst->codecpar->codec_id);
        AVCodecContext* vdec = avcodec_alloc_context3(vcodec);
        avcodec_parameters_to_context(vdec, vst->codecpar);
        if (network)
            vdec->flags |= AV_CODEC_FLAG_LOW_DELAY;
        if (avcodec_open2(vdec, vcodec, nullptr) < 0) {
            avcodec_free_context(&vdec);
            avformat_close_input(&fmt);
            m_running = false;
            return;
        }

        AVCodecContext* adec = nullptr;
        SwrContext* swr = nullptr;
        if (aIndex >= 0) {
            AVStream* ast = fmt->streams[aIndex];
            const AVCodec* acodec = avcodec_find_decoder(ast->codecpar->codec_id);
            if (acodec) {
                adec = avcodec_alloc_context3(acodec);
                avcodec_parameters_to_context(adec, ast->codecpar);
                if (avcodec_open2(adec, acodec, nullptr) < 0) {
                    avcodec_free_context(&adec);
                    adec = nullptr;
                }
            }
        }

        SwsContext* sws = nullptr;
        AVFrame* frame = av_frame_alloc();
        AVFrame* bgra = av_frame_alloc();
        AVFrame* aframe = av_frame_alloc();
        AVPacket* pkt = av_packet_alloc();
        uint8_t* buf = nullptr;
        int bufSize = 0;
        std::vector<float> interleaved;

        auto cleanup = [&] {
            if (buf) av_free(buf);
            av_frame_free(&bgra);
            av_frame_free(&frame);
            av_frame_free(&aframe);
            av_packet_free(&pkt);
            if (sws) sws_freeContext(sws);
            if (swr) swr_free(&swr);
            avcodec_free_context(&vdec);
            if (adec) avcodec_free_context(&adec);
            avformat_close_input(&fmt);
        };

        bool reopen = false;
        while (!m_stop.load()) {
            const int r = av_read_frame(fmt, pkt);
            if (r < 0) {
                if (network) {
                    // Live stream dropped — reopen.
                    reopen = true;
                    break;
                }
                if (m_loop) {
                    av_seek_frame(fmt, vIndex, 0, AVSEEK_FLAG_BACKWARD);
                    avcodec_flush_buffers(vdec);
                    if (adec) avcodec_flush_buffers(adec);
                    continue;
                }
                break;
            }

            if (pkt->stream_index == vIndex) {
                if (avcodec_send_packet(vdec, pkt) == 0) {
                    while (avcodec_receive_frame(vdec, frame) == 0) {
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
                        // Pace local files; for network, decode as fast as packets arrive (small yield).
                        if (!network) {
                            const double fps = av_q2d(vst->avg_frame_rate) > 1.0 ? av_q2d(vst->avg_frame_rate) : 30.0;
                            QThread::msleep(static_cast<unsigned long>(1000.0 / fps));
                        } else {
                            QThread::msleep(1);
                        }
                    }
                }
            } else if (adec && pkt->stream_index == aIndex) {
                if (avcodec_send_packet(adec, pkt) == 0) {
                    while (avcodec_receive_frame(adec, aframe) == 0) {
                        if (!swr) {
#if LIBAVUTIL_VERSION_MAJOR >= 57
                            AVChannelLayout outLayout{};
                            av_channel_layout_default(&outLayout, 2);
                            if (swr_alloc_set_opts2(&swr,
                                                    &outLayout, AV_SAMPLE_FMT_FLT, kAudioSampleRate,
                                                    &aframe->ch_layout, static_cast<AVSampleFormat>(aframe->format),
                                                    aframe->sample_rate, 0, nullptr) < 0
                                || swr_init(swr) < 0) {
                                swr_free(&swr);
                                continue;
                            }
#else
                            const int64_t inLayout = aframe->channel_layout
                                ? static_cast<int64_t>(aframe->channel_layout)
                                : av_get_default_channel_layout(aframe->channels);
                            swr = swr_alloc_set_opts(nullptr,
                                                     AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_FLT, kAudioSampleRate,
                                                     inLayout, static_cast<AVSampleFormat>(aframe->format),
                                                     aframe->sample_rate, 0, nullptr);
                            if (!swr || swr_init(swr) < 0) {
                                swr_free(&swr);
                                continue;
                            }
#endif
                        }
                        const int outSamples = swr_get_out_samples(swr, aframe->nb_samples);
                        interleaved.resize(size_t((std::max)(outSamples, 1)) * 2);
                        uint8_t* outPlanes[1] = { reinterpret_cast<uint8_t*>(interleaved.data()) };
                        const int converted = swr_convert(swr, outPlanes, outSamples,
                                                          const_cast<const uint8_t**>(aframe->extended_data),
                                                          aframe->nb_samples);
                        if (converted > 0)
                            emitAudio(interleaved.data(), converted, 2, kAudioSampleRate);
                    }
                }
            }
            av_packet_unref(pkt);
        }
        cleanup();
        if (reopen && !m_stop.load()) {
            QThread::msleep(500);
            continue;
        }
        if (!reconnect || m_stop.load())
            break;
        if (!network && !m_loop)
            break;
    } while (!m_stop.load() && reconnect);

    m_running = false;
#endif
}

bool MediaSource::start(ID3D11Device* device, QString* error)
{
    m_device = device;
    const QString input = m_filePath.trimmed();
    if (input.isEmpty()) {
        if (error) *error = QStringLiteral("Media path / URL is empty");
        return false;
    }

    const bool network = useNetworkPath();
    if (!network && !QFileInfo::exists(input)) {
        if (error) *error = QStringLiteral("Media path missing: %1").arg(input);
        return false;
    }

    if (network) {
#if defined(RAILSHOT_HAS_FFMPEG) && RAILSHOT_HAS_FFMPEG
        return startFfmpeg(error);
#else
        if (error) *error = QStringLiteral("Network media requires FFmpeg");
        return false;
#endif
    }

    const QString ext = QFileInfo(input).suffix().toLower();
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
