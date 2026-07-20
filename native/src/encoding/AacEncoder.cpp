#include "encoding/AacEncoder.h"
#include "core/Logger.h"

#if RAILSHOT_HAS_FFMPEG
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/audio_fifo.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
}
#endif

namespace railshot {

namespace {

#if RAILSHOT_HAS_FFMPEG
void releaseAac(AVCodecContext*& ctx, AVFrame*& frame, AVPacket*& packet, SwrContext*& swr, AVAudioFifo*& fifo)
{
    if (fifo) { av_audio_fifo_free(fifo); fifo = nullptr; }
    if (swr) { swr_free(&swr); swr = nullptr; }
    if (packet) { av_packet_free(&packet); packet = nullptr; }
    if (frame) { av_frame_free(&frame); frame = nullptr; }
    if (ctx) { avcodec_free_context(&ctx); ctx = nullptr; }
}
#endif

} // namespace

AacEncoder::AacEncoder() = default;

AacEncoder::~AacEncoder()
{
    close();
}

bool AacEncoder::open(const OutputProfile& profile, QString* error)
{
    std::lock_guard lock(m_mutex);
#if RAILSHOT_HAS_FFMPEG
    releaseAac(m_ctx, m_frame, m_packet, m_swr, m_fifo);
#endif
    m_open = false;
    m_profile = profile;

#if !RAILSHOT_HAS_FFMPEG
    if (error) *error = QStringLiteral("FFmpeg not linked");
    return false;
#else
    const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (!codec) {
        if (error) *error = QStringLiteral("AAC encoder not available");
        return false;
    }

    m_ctx = avcodec_alloc_context3(codec);
    if (!m_ctx) {
        if (error) *error = QStringLiteral("avcodec_alloc_context3 failed");
        return false;
    }

    m_ctx->sample_rate = kAudioSampleRate;
    m_ctx->bit_rate = static_cast<int64_t>(qMax(64, m_profile.audioBitrateKbps)) * 1000;
    m_ctx->time_base = AVRational{1, m_ctx->sample_rate};
    m_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

#if LIBAVUTIL_VERSION_MAJOR >= 57
    av_channel_layout_default(&m_ctx->ch_layout, kAudioChannels);
#else
    m_ctx->channel_layout = AV_CH_LAYOUT_STEREO;
    m_ctx->channels = kAudioChannels;
#endif

    m_ctx->sample_fmt = AV_SAMPLE_FMT_FLTP;

    if (avcodec_open2(m_ctx, codec, nullptr) < 0) {
        if (error) *error = QStringLiteral("avcodec_open2 failed for AAC");
        releaseAac(m_ctx, m_frame, m_packet, m_swr, m_fifo);
        return false;
    }

    m_frame = av_frame_alloc();
    m_packet = av_packet_alloc();
    if (!m_frame || !m_packet) {
        if (error) *error = QStringLiteral("Failed to allocate AAC frame/packet");
        releaseAac(m_ctx, m_frame, m_packet, m_swr, m_fifo);
        return false;
    }

    m_frame->format = m_ctx->sample_fmt;
    m_frame->nb_samples = m_ctx->frame_size > 0 ? m_ctx->frame_size : 1024;
    m_frame->sample_rate = m_ctx->sample_rate;
#if LIBAVUTIL_VERSION_MAJOR >= 57
    av_channel_layout_copy(&m_frame->ch_layout, &m_ctx->ch_layout);
#else
    m_frame->channel_layout = m_ctx->channel_layout;
    m_frame->channels = m_ctx->channels;
#endif
    if (av_frame_get_buffer(m_frame, 0) < 0) {
        if (error) *error = QStringLiteral("av_frame_get_buffer failed for AAC");
        releaseAac(m_ctx, m_frame, m_packet, m_swr, m_fifo);
        return false;
    }

#if LIBAVUTIL_VERSION_MAJOR >= 57
    if (swr_alloc_set_opts2(&m_swr,
                            &m_ctx->ch_layout, m_ctx->sample_fmt, m_ctx->sample_rate,
                            &m_ctx->ch_layout, AV_SAMPLE_FMT_FLT, kAudioSampleRate,
                            0, nullptr) < 0) {
        if (error) *error = QStringLiteral("swr_alloc_set_opts2 failed");
        releaseAac(m_ctx, m_frame, m_packet, m_swr, m_fifo);
        return false;
    }
#else
    m_swr = swr_alloc_set_opts(nullptr,
                               m_ctx->channel_layout, m_ctx->sample_fmt, m_ctx->sample_rate,
                               AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_FLT, kAudioSampleRate,
                               0, nullptr);
#endif
    if (!m_swr || swr_init(m_swr) < 0) {
        if (error) *error = QStringLiteral("swr_init failed");
        releaseAac(m_ctx, m_frame, m_packet, m_swr, m_fifo);
        return false;
    }

    m_fifo = av_audio_fifo_alloc(m_ctx->sample_fmt, kAudioChannels, m_frame->nb_samples * 4);
    if (!m_fifo) {
        if (error) *error = QStringLiteral("av_audio_fifo_alloc failed");
        releaseAac(m_ctx, m_frame, m_packet, m_swr, m_fifo);
        return false;
    }

    m_nextPtsSamples = 0;
    m_open = true;
    Logger::info(QStringLiteral("AAC encoder open @ %1 kbps, frame_size=%2")
                     .arg(m_profile.audioBitrateKbps)
                     .arg(m_frame->nb_samples));
    return true;
#endif
}

void AacEncoder::close()
{
    std::lock_guard lock(m_mutex);
#if RAILSHOT_HAS_FFMPEG
    releaseAac(m_ctx, m_frame, m_packet, m_swr, m_fifo);
#endif
    m_open = false;
}

QByteArray AacEncoder::extradata() const
{
    std::lock_guard lock(m_mutex);
#if RAILSHOT_HAS_FFMPEG
    if (!m_ctx || !m_ctx->extradata || m_ctx->extradata_size <= 0)
        return {};
    return QByteArray(reinterpret_cast<const char*>(m_ctx->extradata), m_ctx->extradata_size);
#else
    return {};
#endif
}

int AacEncoder::sampleRate() const
{
#if RAILSHOT_HAS_FFMPEG
    return m_ctx ? m_ctx->sample_rate : kAudioSampleRate;
#else
    return kAudioSampleRate;
#endif
}

int AacEncoder::channels() const
{
    return kAudioChannels;
}

bool AacEncoder::receivePacket(EncodedPacket& out)
{
#if RAILSHOT_HAS_FFMPEG
    const int ret = avcodec_receive_packet(m_ctx, m_packet);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        return false;
    if (ret < 0)
        return false;

    out.data = QByteArray(reinterpret_cast<const char*>(m_packet->data), m_packet->size);
    const qint64 ptsSamples = m_packet->pts != AV_NOPTS_VALUE ? m_packet->pts : 0;
    out.ptsUs = av_rescale_q(ptsSamples, m_ctx->time_base, AVRational{1, 1000000});
    out.dtsUs = out.ptsUs;
    out.keyframe = true;
    out.video = false;
    av_packet_unref(m_packet);
    return true;
#else
    (void)out;
    return false;
#endif
}

bool AacEncoder::encodeFifoFrame(EncodedPacket& out)
{
#if RAILSHOT_HAS_FFMPEG
    const int frameSize = m_frame->nb_samples;
    if (av_audio_fifo_size(m_fifo) < frameSize)
        return false;
    if (av_frame_make_writable(m_frame) < 0)
        return false;
    if (av_audio_fifo_read(m_fifo, reinterpret_cast<void**>(m_frame->data), frameSize) < frameSize)
        return false;
    m_frame->pts = m_nextPtsSamples;
    m_nextPtsSamples += frameSize;
    if (avcodec_send_frame(m_ctx, m_frame) < 0)
        return false;
    return receivePacket(out);
#else
    (void)out;
    return false;
#endif
}

bool AacEncoder::encode(const AudioBuffer& buffer, QVector<EncodedPacket>& out)
{
    std::lock_guard lock(m_mutex);
    out.clear();
    if (!m_open || buffer.samples.empty()) return false;

#if !RAILSHOT_HAS_FFMPEG
    return false;
#else
    const int inFrames = buffer.frameCount();
    if (inFrames <= 0) return false;

    uint8_t* convertedData[8] = {};
    int convertedLinesize = 0;
    if (av_samples_alloc(convertedData, &convertedLinesize, kAudioChannels, inFrames, m_ctx->sample_fmt, 0) < 0)
        return false;

    const uint8_t* inData[1] = {reinterpret_cast<const uint8_t*>(buffer.samples.data())};
    const int converted = swr_convert(m_swr, convertedData, inFrames, inData, inFrames);
    if (converted < 0) {
        av_freep(&convertedData[0]);
        return false;
    }

    if (av_audio_fifo_realloc(m_fifo, av_audio_fifo_size(m_fifo) + converted) < 0) {
        av_freep(&convertedData[0]);
        return false;
    }
    av_audio_fifo_write(m_fifo, reinterpret_cast<void**>(convertedData), converted);
    av_freep(&convertedData[0]);

    EncodedPacket pkt;
    while (encodeFifoFrame(pkt))
        out.push_back(pkt);
    return true;
#endif
}

bool AacEncoder::flush(QVector<EncodedPacket>& out)
{
    std::lock_guard lock(m_mutex);
    out.clear();
#if RAILSHOT_HAS_FFMPEG
    if (!m_open || !m_ctx) return true;

    EncodedPacket pkt;
    while (encodeFifoFrame(pkt))
        out.push_back(pkt);

    const int frameSize = m_frame->nb_samples;
    const int remaining = av_audio_fifo_size(m_fifo);
    if (remaining > 0) {
        const int pad = frameSize - remaining;
        uint8_t* silencePlanes[8] = {};
        int linesize = 0;
        if (av_samples_alloc(silencePlanes, &linesize, kAudioChannels, pad, m_ctx->sample_fmt, 0) >= 0) {
            av_samples_set_silence(silencePlanes, 0, pad, kAudioChannels, m_ctx->sample_fmt);
            av_audio_fifo_write(m_fifo, reinterpret_cast<void**>(silencePlanes), pad);
            av_freep(&silencePlanes[0]);
        }
        if (encodeFifoFrame(pkt))
            out.push_back(pkt);
    }

    avcodec_send_frame(m_ctx, nullptr);
    while (receivePacket(pkt))
        out.push_back(pkt);
#endif
    return true;
}

} // namespace railshot
