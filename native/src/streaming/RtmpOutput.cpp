#include "streaming/RtmpOutput.h"
#include "core/Logger.h"
#include <QUrl>
#include <cstring>

#if RAILSHOT_HAS_FFMPEG
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
}
#endif

namespace railshot {

RtmpOutput::RtmpOutput(QObject* parent)
    : QObject(parent)
{
}

RtmpOutput::~RtmpOutput()
{
    disconnectFrom();
}

void RtmpOutput::setState(ConnectionState s)
{
    if (m_state == s) return;
    m_state = s;
    emit stateChanged(s);
}

QString RtmpOutput::buildOutputUrl() const
{
    // Local FLV smoke path: file:///... or bare *.flv path
    if (m_url.endsWith(QLatin1String(".flv"), Qt::CaseInsensitive)
        || m_url.startsWith(QLatin1String("file:"), Qt::CaseInsensitive)) {
        QUrl u(m_url);
        if (u.isValid() && u.isLocalFile())
            return u.toLocalFile();
        if (m_url.startsWith(QLatin1String("file:///"), Qt::CaseInsensitive))
            return m_url.mid(8);
        return m_url;
    }

    QString base = m_url.trimmed();
    if (m_key.isEmpty())
        return base;
    if (base.endsWith(QLatin1Char('/')))
        return base + m_key;
    return base + QLatin1Char('/') + m_key;
}

bool RtmpOutput::openMux(QString* error)
{
#if !RAILSHOT_HAS_FFMPEG
    if (error) *error = QStringLiteral("FFmpeg not linked");
    return false;
#else
    closeMux(false);
    m_outputUrl = buildOutputUrl();
    const QByteArray urlUtf8 = m_outputUrl.toUtf8();

    const bool localFlv = m_outputUrl.endsWith(QLatin1String(".flv"), Qt::CaseInsensitive);
    if (avformat_alloc_output_context2(&m_fmt, nullptr, "flv", urlUtf8.constData()) < 0 || !m_fmt) {
        if (error) *error = QStringLiteral("Failed to allocate FLV/RTMP context");
        return false;
    }

    m_videoStream = avformat_new_stream(m_fmt, nullptr);
    m_audioStream = avformat_new_stream(m_fmt, nullptr);
    if (!m_videoStream || !m_audioStream) {
        if (error) *error = QStringLiteral("Failed to create RTMP streams");
        closeMux(false);
        return false;
    }

    m_videoStream->id = 0;
    m_videoStream->time_base = AVRational{1, 1000000};
    m_videoStream->avg_frame_rate = av_d2q(m_profile.fps, 100000);
    m_videoStream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    m_videoStream->codecpar->codec_id = AV_CODEC_ID_H264;
    m_videoStream->codecpar->width = m_profile.width;
    m_videoStream->codecpar->height = m_profile.height;
    m_videoStream->codecpar->format = AV_PIX_FMT_YUV420P;
    m_videoStream->codecpar->bit_rate = static_cast<int64_t>(m_profile.videoBitrateKbps) * 1000;
    if (!m_videoExtra.isEmpty()) {
        m_videoStream->codecpar->extradata = static_cast<uint8_t*>(
            av_malloc(static_cast<size_t>(m_videoExtra.size()) + AV_INPUT_BUFFER_PADDING_SIZE));
        if (!m_videoStream->codecpar->extradata) {
            if (error) *error = QStringLiteral("Video extradata alloc failed");
            closeMux(false);
            return false;
        }
        memcpy(m_videoStream->codecpar->extradata, m_videoExtra.constData(),
               static_cast<size_t>(m_videoExtra.size()));
        memset(m_videoStream->codecpar->extradata + m_videoExtra.size(), 0, AV_INPUT_BUFFER_PADDING_SIZE);
        m_videoStream->codecpar->extradata_size = m_videoExtra.size();
    }

    m_audioStream->id = 1;
    m_audioStream->time_base = AVRational{1, m_audioSampleRate};
    m_audioStream->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
    m_audioStream->codecpar->codec_id = AV_CODEC_ID_AAC;
    m_audioStream->codecpar->sample_rate = m_audioSampleRate;
    m_audioStream->codecpar->bit_rate = static_cast<int64_t>(m_profile.audioBitrateKbps) * 1000;
    m_audioStream->codecpar->format = AV_SAMPLE_FMT_FLTP;
#if LIBAVUTIL_VERSION_MAJOR >= 57
    av_channel_layout_default(&m_audioStream->codecpar->ch_layout, m_audioChannels);
#else
    m_audioStream->codecpar->channels = m_audioChannels;
    m_audioStream->codecpar->channel_layout = AV_CH_LAYOUT_STEREO;
#endif
    if (!m_audioExtra.isEmpty()) {
        m_audioStream->codecpar->extradata = static_cast<uint8_t*>(
            av_malloc(static_cast<size_t>(m_audioExtra.size()) + AV_INPUT_BUFFER_PADDING_SIZE));
        if (!m_audioStream->codecpar->extradata) {
            if (error) *error = QStringLiteral("Audio extradata alloc failed");
            closeMux(false);
            return false;
        }
        memcpy(m_audioStream->codecpar->extradata, m_audioExtra.constData(),
               static_cast<size_t>(m_audioExtra.size()));
        memset(m_audioStream->codecpar->extradata + m_audioExtra.size(), 0, AV_INPUT_BUFFER_PADDING_SIZE);
        m_audioStream->codecpar->extradata_size = m_audioExtra.size();
    }

    if (!(m_fmt->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&m_fmt->pb, urlUtf8.constData(), AVIO_FLAG_WRITE) < 0) {
            if (error) {
                *error = localFlv ? QStringLiteral("Failed to open FLV file: %1").arg(m_outputUrl)
                                  : QStringLiteral("Failed to open RTMP URL (check server/key)");
            }
            closeMux(false);
            return false;
        }
    }

    AVDictionary* opts = nullptr;
    av_dict_set(&opts, "flvflags", "no_duration_filesize", 0);
    if (!localFlv)
        av_dict_set(&opts, "rtmp_live", "live", 0);

    if (avformat_write_header(m_fmt, &opts) < 0) {
        av_dict_free(&opts);
        if (error) *error = QStringLiteral("RTMP/FLV write_header failed");
        closeMux(false);
        return false;
    }
    av_dict_free(&opts);
    return true;
#endif
}

void RtmpOutput::closeMux(bool writeTrailer)
{
#if RAILSHOT_HAS_FFMPEG
    if (!m_fmt) return;
    if (writeTrailer)
        av_write_trailer(m_fmt);
    if (m_fmt->pb && !(m_fmt->oformat->flags & AVFMT_NOFILE))
        avio_closep(&m_fmt->pb);
    avformat_free_context(m_fmt);
    m_fmt = nullptr;
    m_videoStream = nullptr;
    m_audioStream = nullptr;
#else
    (void)writeTrailer;
#endif
}

bool RtmpOutput::connectTo(const QString& url,
                           const QString& streamKey,
                           const OutputProfile& profile,
                           const QByteArray& videoExtradata,
                           const QByteArray& audioExtradata,
                           int audioSampleRate,
                           int audioChannels,
                           QString* error)
{
    disconnectFrom();

    m_url = url;
    m_key = streamKey;
    m_profile = profile;
    m_videoExtra = videoExtradata;
    m_audioExtra = audioExtradata;
    m_audioSampleRate = audioSampleRate > 0 ? audioSampleRate : 48000;
    m_audioChannels = audioChannels > 0 ? audioChannels : 2;
    m_reconnectAttempt = 0;
    m_bytesSent = 0;
    m_dropped = 0;

    if (m_url.isEmpty()) {
        if (error) *error = QStringLiteral("RTMP URL required");
        setState(ConnectionState::Failed);
        return false;
    }

    setState(ConnectionState::Connecting);

    {
        QMutexLocker lock(&m_muxMutex);
        if (!openMux(error)) {
            setState(ConnectionState::Failed);
            return false;
        }
    }

    m_connected = true;
    m_running = true;
    setState(ConnectionState::Connected);

    m_thread = QThread::create([this] { writerLoop(); });
    m_thread->start();

    Logger::info(QStringLiteral("RTMP/FLV output connected: %1").arg(m_outputUrl));
    return true;
}

void RtmpOutput::disconnectFrom()
{
    m_running = false;
    m_queueNotEmpty.wakeAll();
    if (m_thread) {
        m_thread->wait(5000);
        delete m_thread;
        m_thread = nullptr;
    }
    {
        QMutexLocker lock(&m_queueMutex);
        m_queue.clear();
    }
    {
        QMutexLocker lock(&m_muxMutex);
        closeMux(m_connected.load());
    }
    m_connected = false;
    setState(ConnectionState::Disconnected);
}

void RtmpOutput::pushVideo(const EncodedPacket& pkt)
{
    QMutexLocker lock(&m_queueMutex);
    if (m_queue.size() >= kMaxQueue) {
        // Drop oldest non-keyframes first to relieve backpressure.
        for (int i = 0; i < m_queue.size(); ++i) {
            if (!m_queue[i].keyframe) {
                m_queue.removeAt(i);
                m_dropped++;
                break;
            }
        }
        if (m_queue.size() >= kMaxQueue) {
            m_dropped++;
            return;
        }
    }
    m_queue.enqueue(pkt);
    m_queueNotEmpty.wakeOne();
}

void RtmpOutput::pushAudio(const EncodedPacket& pkt)
{
    pushVideo(pkt);
}

bool RtmpOutput::writePacketUnlocked(const EncodedPacket& pkt)
{
#if !RAILSHOT_HAS_FFMPEG
    (void)pkt;
    return false;
#else
    if (!m_fmt || pkt.data.isEmpty()) return false;
    AVStream* stream = pkt.video ? m_videoStream : m_audioStream;
    if (!stream) return false;

    AVPacket* avpkt = av_packet_alloc();
    if (!avpkt) return false;
    if (av_new_packet(avpkt, pkt.data.size()) < 0) {
        av_packet_free(&avpkt);
        return false;
    }
    memcpy(avpkt->data, pkt.data.constData(), static_cast<size_t>(pkt.data.size()));
    avpkt->stream_index = stream->index;
    avpkt->pts = av_rescale_q(pkt.ptsUs, AVRational{1, 1000000}, stream->time_base);
    avpkt->dts = av_rescale_q(pkt.dtsUs, AVRational{1, 1000000}, stream->time_base);
    if (pkt.keyframe)
        avpkt->flags |= AV_PKT_FLAG_KEY;

    const int ret = av_interleaved_write_frame(m_fmt, avpkt);
    av_packet_free(&avpkt);
    if (ret < 0)
        return false;

    m_bytesSent += pkt.data.size();
    if (m_fmt->pb)
        avio_flush(m_fmt->pb);
    return true;
#endif
}

bool RtmpOutput::reconnect()
{
#if !RAILSHOT_HAS_FFMPEG
    return false;
#else
    QMutexLocker lock(&m_muxMutex);
    closeMux(false);
    QString err;
    if (!openMux(&err)) {
        Logger::warn(QStringLiteral("RTMP reconnect failed: %1").arg(err));
        return false;
    }
    return true;
#endif
}

void RtmpOutput::writerLoop()
{
    while (m_running.load()) {
        EncodedPacket pkt;
        bool has = false;
        {
            QMutexLocker lock(&m_queueMutex);
            if (m_queue.isEmpty())
                m_queueNotEmpty.wait(&m_queueMutex, 50);
            if (!m_queue.isEmpty()) {
                pkt = m_queue.dequeue();
                has = true;
            }
        }
        if (!has)
            continue;

        bool ok = false;
        {
            QMutexLocker lock(&m_muxMutex);
            ok = writePacketUnlocked(pkt);
        }
        if (ok)
            continue;

        setState(ConnectionState::Reconnecting);
        emit reconnecting(++m_reconnectAttempt);
        const int backoffMs = qMin(30000, 500 * (1 << qMin(m_reconnectAttempt, 5)));
        QThread::msleep(backoffMs);
        if (!m_running.load())
            break;

        if (reconnect()) {
            setState(ConnectionState::Connected);
            // Re-queue failed packet if still a keyframe or audio
            if (pkt.keyframe || !pkt.video)
                pushVideo(pkt);
            emit networkError(QStringLiteral("RTMP recovered after reconnect"));
        } else {
            setState(ConnectionState::Failed);
            emit networkError(QStringLiteral("RTMP write failed and reconnect unsuccessful"));
            m_connected = false;
            break;
        }
    }
}

} // namespace railshot
