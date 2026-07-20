#include "recording/MkvRecorder.h"
#include "core/Logger.h"
#include <QDir>
#include <QFileInfo>

#if RAILSHOT_HAS_FFMPEG
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
}
#endif

namespace railshot {

namespace {

#if RAILSHOT_HAS_FFMPEG
void releaseFmt(AVFormatContext*& fmt, AVStream*& video, AVStream*& audio, bool writeTrailer)
{
    if (fmt) {
        if (writeTrailer)
            av_write_trailer(fmt);
        if (fmt->pb && !(fmt->oformat->flags & AVFMT_NOFILE))
            avio_closep(&fmt->pb);
        avformat_free_context(fmt);
        fmt = nullptr;
        video = nullptr;
        audio = nullptr;
    }
}
#endif

} // namespace

MkvRecorder::MkvRecorder(QObject* parent)
    : QObject(parent)
{
}

MkvRecorder::~MkvRecorder()
{
    close();
}

bool MkvRecorder::open(const QString& path,
                       const OutputProfile& profile,
                       const QByteArray& videoExtradata,
                       const QByteArray& audioExtradata,
                       int audioSampleRate,
                       int audioChannels,
                       QString* error)
{
    QMutexLocker lock(&m_mutex);
#if RAILSHOT_HAS_FFMPEG
    releaseFmt(m_fmt, m_videoStream, m_audioStream, m_open && m_headerWritten);
#endif
    m_open = false;
    m_headerWritten = false;

#if !RAILSHOT_HAS_FFMPEG
    (void)videoExtradata;
    (void)audioExtradata;
    (void)audioSampleRate;
    (void)audioChannels;
    if (error) *error = QStringLiteral("FFmpeg not linked — cannot write MKV");
    return false;
#else
    m_path = path;
    m_profile = profile;
    QDir().mkpath(QFileInfo(path).absolutePath());

    const QByteArray pathUtf8 = path.toUtf8();
    if (avformat_alloc_output_context2(&m_fmt, nullptr, "matroska", pathUtf8.constData()) < 0 || !m_fmt) {
        if (error) *error = QStringLiteral("Failed to allocate Matroska context");
        return false;
    }

    m_videoStream = avformat_new_stream(m_fmt, nullptr);
    m_audioStream = avformat_new_stream(m_fmt, nullptr);
    if (!m_videoStream || !m_audioStream) {
        if (error) *error = QStringLiteral("Failed to create streams");
        releaseFmt(m_fmt, m_videoStream, m_audioStream, false);
        return false;
    }

    m_videoStream->id = 0;
    m_videoStream->time_base = AVRational{1, 1000000};
    m_videoStream->avg_frame_rate = av_d2q(profile.fps, 100000);
    m_videoStream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    m_videoStream->codecpar->codec_id = AV_CODEC_ID_H264;
    m_videoStream->codecpar->width = profile.width;
    m_videoStream->codecpar->height = profile.height;
    m_videoStream->codecpar->format = AV_PIX_FMT_YUV420P;
    m_videoStream->codecpar->bit_rate = static_cast<int64_t>(profile.videoBitrateKbps) * 1000;
    if (!videoExtradata.isEmpty()) {
        m_videoStream->codecpar->extradata = static_cast<uint8_t*>(
            av_malloc(static_cast<size_t>(videoExtradata.size()) + AV_INPUT_BUFFER_PADDING_SIZE));
        if (!m_videoStream->codecpar->extradata) {
            if (error) *error = QStringLiteral("Failed to allocate video extradata");
            releaseFmt(m_fmt, m_videoStream, m_audioStream, false);
            return false;
        }
        memcpy(m_videoStream->codecpar->extradata, videoExtradata.constData(),
               static_cast<size_t>(videoExtradata.size()));
        memset(m_videoStream->codecpar->extradata + videoExtradata.size(), 0, AV_INPUT_BUFFER_PADDING_SIZE);
        m_videoStream->codecpar->extradata_size = videoExtradata.size();
    }

    m_audioStream->id = 1;
    m_audioStream->time_base = AVRational{1, audioSampleRate > 0 ? audioSampleRate : kAudioSampleRate};
    m_audioStream->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
    m_audioStream->codecpar->codec_id = AV_CODEC_ID_AAC;
    m_audioStream->codecpar->sample_rate = audioSampleRate > 0 ? audioSampleRate : kAudioSampleRate;
    m_audioStream->codecpar->bit_rate = static_cast<int64_t>(profile.audioBitrateKbps) * 1000;
    m_audioStream->codecpar->format = AV_SAMPLE_FMT_FLTP;
#if LIBAVUTIL_VERSION_MAJOR >= 57
    av_channel_layout_default(&m_audioStream->codecpar->ch_layout, audioChannels > 0 ? audioChannels : kAudioChannels);
#else
    m_audioStream->codecpar->channels = audioChannels > 0 ? audioChannels : kAudioChannels;
    m_audioStream->codecpar->channel_layout = AV_CH_LAYOUT_STEREO;
#endif
    if (!audioExtradata.isEmpty()) {
        m_audioStream->codecpar->extradata = static_cast<uint8_t*>(
            av_malloc(static_cast<size_t>(audioExtradata.size()) + AV_INPUT_BUFFER_PADDING_SIZE));
        if (!m_audioStream->codecpar->extradata) {
            if (error) *error = QStringLiteral("Failed to allocate audio extradata");
            releaseFmt(m_fmt, m_videoStream, m_audioStream, false);
            return false;
        }
        memcpy(m_audioStream->codecpar->extradata, audioExtradata.constData(),
               static_cast<size_t>(audioExtradata.size()));
        memset(m_audioStream->codecpar->extradata + audioExtradata.size(), 0, AV_INPUT_BUFFER_PADDING_SIZE);
        m_audioStream->codecpar->extradata_size = audioExtradata.size();
    }

    if (!(m_fmt->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&m_fmt->pb, pathUtf8.constData(), AVIO_FLAG_WRITE) < 0) {
            if (error) *error = QStringLiteral("Failed to open output file: %1").arg(path);
            releaseFmt(m_fmt, m_videoStream, m_audioStream, false);
            return false;
        }
    }

    AVDictionary* opts = nullptr;
    av_dict_set(&opts, "flush_packets", "1", 0);
    if (avformat_write_header(m_fmt, &opts) < 0) {
        av_dict_free(&opts);
        if (error) *error = QStringLiteral("avformat_write_header failed");
        releaseFmt(m_fmt, m_videoStream, m_audioStream, false);
        return false;
    }
    av_dict_free(&opts);

    m_headerWritten = true;
    m_open = true;
    m_writeCounter = 0;
    m_bytes = 0;
    if (m_fmt->pb)
        m_bytes = avio_tell(m_fmt->pb);

    Logger::info(QStringLiteral("MKV recording opened: %1").arg(path));
    return true;
#endif
}

void MkvRecorder::close()
{
    QMutexLocker lock(&m_mutex);
#if RAILSHOT_HAS_FFMPEG
    const bool wrote = m_open && m_headerWritten;
    releaseFmt(m_fmt, m_videoStream, m_audioStream, wrote);
#endif
    if (m_open)
        Logger::info(QStringLiteral("MKV recording closed: %1 (%2 bytes)").arg(m_path).arg(m_bytes.load()));
    m_open = false;
    m_headerWritten = false;
}

void MkvRecorder::flushIo()
{
#if RAILSHOT_HAS_FFMPEG
    if (m_fmt && m_fmt->pb)
        avio_flush(m_fmt->pb);
#endif
}

bool MkvRecorder::writePacket(const EncodedPacket& pkt, bool video)
{
    QMutexLocker lock(&m_mutex);
#if !RAILSHOT_HAS_FFMPEG
    (void)pkt;
    (void)video;
    return false;
#else
    if (!m_open || !m_fmt || pkt.data.isEmpty()) return false;
    AVStream* stream = video ? m_videoStream : m_audioStream;
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
    if (ret < 0) {
        emit writeError(QStringLiteral("av_interleaved_write_frame failed (%1)").arg(ret));
        return false;
    }

    if (m_fmt->pb)
        m_bytes = avio_tell(m_fmt->pb);

    if ((++m_writeCounter % 30) == 0)
        flushIo();
    return true;
#endif
}

bool MkvRecorder::writeVideo(const EncodedPacket& pkt) { return writePacket(pkt, true); }
bool MkvRecorder::writeAudio(const EncodedPacket& pkt) { return writePacket(pkt, false); }

} // namespace railshot
