#include "encoding/SoftwareEncoder.h"
#include "core/Logger.h"

#include <QImage>
#include <QtGlobal>
#include <cstring>

#if RAILSHOT_HAS_FFMPEG
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}
#endif

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <d3d11.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;
#endif

namespace railshot {

SoftwareEncoder::SoftwareEncoder() = default;

SoftwareEncoder::~SoftwareEncoder()
{
    close();
}

bool SoftwareEncoder::open(const OutputProfile& profile, QString* error)
{
    std::lock_guard lock(m_mutex);
    // Release previous context without re-locking.
#if RAILSHOT_HAS_FFMPEG
    if (m_sws) { sws_freeContext(m_sws); m_sws = nullptr; }
    if (m_packet) { av_packet_free(&m_packet); m_packet = nullptr; }
    if (m_frame) { av_frame_free(&m_frame); m_frame = nullptr; }
    if (m_ctx) { avcodec_free_context(&m_ctx); m_ctx = nullptr; }
#endif
    m_open = false;
    m_profile = profile;
    if (m_profile.width <= 0 || m_profile.height <= 0 || m_profile.fps <= 0.0) {
        if (error) *error = QStringLiteral("Invalid video profile");
        return false;
    }

#if !RAILSHOT_HAS_FFMPEG
    if (error) *error = QStringLiteral("FFmpeg not linked");
    return false;
#else
    const AVCodec* codec = avcodec_find_encoder_by_name("libopenh264");
    if (!codec)
        codec = avcodec_find_encoder_by_name("h264_mf");
    if (!codec)
        codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
        if (error) *error = QStringLiteral("No H.264 encoder available in FFmpeg build");
        return false;
    }

    m_ctx = avcodec_alloc_context3(codec);
    if (!m_ctx) {
        if (error) *error = QStringLiteral("avcodec_alloc_context3 failed");
        return false;
    }

    m_ctx->width = m_profile.width;
    m_ctx->height = m_profile.height;
    m_ctx->time_base = AVRational{1, 1000000};
    m_ctx->framerate = av_d2q(m_profile.fps, 100000);
    m_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    m_ctx->gop_size = qMax(1, static_cast<int>(m_profile.fps * m_profile.keyframeIntervalSec));
    m_ctx->max_b_frames = 0;
    m_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    const QString rc = m_profile.rateControl.toUpper();
    if (rc == QLatin1String("CQP") || rc == QLatin1String("CRF")) {
        // Approximate quality from bitrate slider (higher kbps → lower CRF).
        const int crf = qBound(18, 32, 32 - (m_profile.videoBitrateKbps / 500));
        m_ctx->bit_rate = 0;
        if (m_ctx->priv_data) {
            av_opt_set(m_ctx->priv_data, "crf", QByteArray::number(crf).constData(), 0);
            av_opt_set(m_ctx->priv_data, "rc", "crf", 0);
        }
    } else {
        m_ctx->bit_rate = static_cast<int64_t>(m_profile.videoBitrateKbps) * 1000;
        if (rc == QLatin1String("CBR") && m_ctx->priv_data) {
            av_opt_set(m_ctx->priv_data, "rc", "cbr", 0);
            m_ctx->rc_max_rate = m_ctx->bit_rate;
            m_ctx->rc_min_rate = m_ctx->bit_rate;
            m_ctx->rc_buffer_size = static_cast<int>(m_ctx->bit_rate);
        } else if (m_ctx->priv_data) {
            av_opt_set(m_ctx->priv_data, "rc", "vbr", 0);
            m_ctx->rc_max_rate = m_ctx->bit_rate * 3 / 2;
        }
    }

    if (codec->id == AV_CODEC_ID_H264) {
        av_opt_set(m_ctx->priv_data, "profile", "baseline", 0);
    }

    if (avcodec_open2(m_ctx, codec, nullptr) < 0) {
        if (error) *error = QStringLiteral("avcodec_open2 failed for %1").arg(QString::fromUtf8(codec->name));
        if (m_ctx) { avcodec_free_context(&m_ctx); m_ctx = nullptr; }
        return false;
    }

    m_frame = av_frame_alloc();
    m_packet = av_packet_alloc();
    if (!m_frame || !m_packet) {
        if (error) *error = QStringLiteral("Failed to allocate AVFrame/AVPacket");
        if (m_packet) { av_packet_free(&m_packet); m_packet = nullptr; }
        if (m_frame) { av_frame_free(&m_frame); m_frame = nullptr; }
        if (m_ctx) { avcodec_free_context(&m_ctx); m_ctx = nullptr; }
        return false;
    }
    m_frame->format = m_ctx->pix_fmt;
    m_frame->width = m_ctx->width;
    m_frame->height = m_ctx->height;
    if (av_frame_get_buffer(m_frame, 32) < 0) {
        if (error) *error = QStringLiteral("av_frame_get_buffer failed");
        if (m_packet) { av_packet_free(&m_packet); m_packet = nullptr; }
        if (m_frame) { av_frame_free(&m_frame); m_frame = nullptr; }
        if (m_ctx) { avcodec_free_context(&m_ctx); m_ctx = nullptr; }
        return false;
    }

    m_sws = sws_getContext(m_profile.width, m_profile.height, AV_PIX_FMT_BGRA,
                           m_profile.width, m_profile.height, AV_PIX_FMT_YUV420P,
                           SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!m_sws) {
        if (error) *error = QStringLiteral("sws_getContext failed");
        if (m_packet) { av_packet_free(&m_packet); m_packet = nullptr; }
        if (m_frame) { av_frame_free(&m_frame); m_frame = nullptr; }
        if (m_ctx) { avcodec_free_context(&m_ctx); m_ctx = nullptr; }
        return false;
    }

    m_name = QStringLiteral("Software H.264 (%1)").arg(QString::fromUtf8(codec->name));
    m_open = true;
    m_frameIndex = 0;
    Logger::info(QStringLiteral("%1 open %2x%3 @ %4 kbps")
                     .arg(m_name)
                     .arg(m_profile.width)
                     .arg(m_profile.height)
                     .arg(m_profile.videoBitrateKbps));
    return true;
#endif
}

void SoftwareEncoder::close()
{
    std::lock_guard lock(m_mutex);
#if RAILSHOT_HAS_FFMPEG
    if (m_sws) {
        sws_freeContext(m_sws);
        m_sws = nullptr;
    }
    if (m_packet) {
        av_packet_free(&m_packet);
        m_packet = nullptr;
    }
    if (m_frame) {
        av_frame_free(&m_frame);
        m_frame = nullptr;
    }
    if (m_ctx) {
        avcodec_free_context(&m_ctx);
        m_ctx = nullptr;
    }
#endif
    m_open = false;
}

QByteArray SoftwareEncoder::extradata() const
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

bool SoftwareEncoder::readbackTexture(ID3D11Texture2D* texture, QByteArray& bgra, int& stride)
{
#ifdef _WIN32
    if (!texture) return false;
    ComPtr<ID3D11Device> device;
    texture->GetDevice(&device);
    if (!device) return false;
    ComPtr<ID3D11DeviceContext> ctx;
    device->GetImmediateContext(&ctx);
    if (!ctx) return false;

    D3D11_TEXTURE2D_DESC desc{};
    texture->GetDesc(&desc);
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.MiscFlags = 0;

    ComPtr<ID3D11Texture2D> staging;
    if (FAILED(device->CreateTexture2D(&desc, nullptr, &staging)))
        return false;
    ctx->CopyResource(staging.Get(), texture);

    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(ctx->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped)))
        return false;

    const int w = static_cast<int>(desc.Width);
    const int h = static_cast<int>(desc.Height);
    stride = w * 4;
    bgra.resize(stride * h);
    for (int y = 0; y < h; ++y) {
        memcpy(bgra.data() + y * stride,
               static_cast<const char*>(mapped.pData) + y * mapped.RowPitch,
               static_cast<size_t>(stride));
    }
    ctx->Unmap(staging.Get(), 0);
    return true;
#else
    Q_UNUSED(texture);
    Q_UNUSED(bgra);
    Q_UNUSED(stride);
    return false;
#endif
}

bool SoftwareEncoder::receivePacket(EncodedPacket& out)
{
#if RAILSHOT_HAS_FFMPEG
    const int ret = avcodec_receive_packet(m_ctx, m_packet);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        return false;
    if (ret < 0)
        return false;

    out.data = QByteArray(reinterpret_cast<const char*>(m_packet->data), m_packet->size);
    out.ptsUs = m_packet->pts != AV_NOPTS_VALUE ? m_packet->pts : 0;
    out.dtsUs = m_packet->dts != AV_NOPTS_VALUE ? m_packet->dts : out.ptsUs;
    out.keyframe = (m_packet->flags & AV_PKT_FLAG_KEY) != 0;
    out.video = true;
    av_packet_unref(m_packet);
    return true;
#else
    Q_UNUSED(out);
    return false;
#endif
}

bool SoftwareEncoder::encodeBgra(const uint8_t* bgra, int stride, qint64 ptsUs, EncodedPacket& out)
{
#if RAILSHOT_HAS_FFMPEG
    if (!m_open || !bgra) return false;
    if (av_frame_make_writable(m_frame) < 0)
        return false;

    const uint8_t* srcSlice[1] = {bgra};
    int srcStride[1] = {stride};
    sws_scale(m_sws, srcSlice, srcStride, 0, m_profile.height, m_frame->data, m_frame->linesize);
    m_frame->pts = ptsUs;

    if (avcodec_send_frame(m_ctx, m_frame) < 0)
        return false;
    ++m_frameIndex;
    return receivePacket(out);
#else
    Q_UNUSED(bgra);
    Q_UNUSED(stride);
    Q_UNUSED(ptsUs);
    Q_UNUSED(out);
    return false;
#endif
}

bool SoftwareEncoder::encodeTexture(ID3D11Texture2D* texture, qint64 ptsUs, EncodedPacket& out)
{
    std::lock_guard lock(m_mutex);
    if (!m_open) return false;
    QByteArray bgra;
    int stride = 0;
    if (!readbackTexture(texture, bgra, stride))
        return false;
    return encodeBgra(reinterpret_cast<const uint8_t*>(bgra.constData()), stride, ptsUs, out);
}

bool SoftwareEncoder::encodeImage(const QImage& image, qint64 ptsUs, EncodedPacket& out)
{
    std::lock_guard lock(m_mutex);
    if (!m_open || image.isNull()) return false;
    QImage converted = image;
    if (converted.format() != QImage::Format_ARGB32 && converted.format() != QImage::Format_RGB32)
        converted = converted.convertToFormat(QImage::Format_ARGB32);
    if (converted.width() != m_profile.width || converted.height() != m_profile.height)
        converted = converted.scaled(m_profile.width, m_profile.height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    return encodeBgra(converted.constBits(), converted.bytesPerLine(), ptsUs, out);
}

bool SoftwareEncoder::flush(QVector<EncodedPacket>& out)
{
    std::lock_guard lock(m_mutex);
    out.clear();
#if RAILSHOT_HAS_FFMPEG
    if (!m_open || !m_ctx) return true;
    avcodec_send_frame(m_ctx, nullptr);
    EncodedPacket pkt;
    while (receivePacket(pkt))
        out.push_back(pkt);
#endif
    return true;
}

} // namespace railshot
