#pragma once

#include "encoding/IEncoder.h"
#include <mutex>

struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwsContext;
struct ID3D11Texture2D;

namespace railshot {

/// CPU H.264 encoder via FFmpeg (libopenh264 / h264_mf). LGPL-safe, no libx264.
class SoftwareEncoder : public IVideoEncoder {
public:
    SoftwareEncoder();
    ~SoftwareEncoder() override;
    QString name() const override { return m_name; }
    bool open(const OutputProfile& profile, QString* error = nullptr) override;
    void close() override;
    bool encodeTexture(ID3D11Texture2D* texture, qint64 ptsUs, EncodedPacket& out) override;
    bool encodeImage(const QImage& image, qint64 ptsUs, EncodedPacket& out) override;
    bool flush(QVector<EncodedPacket>& out) override;
    QByteArray extradata() const override;
    int width() const override { return m_profile.width; }
    int height() const override { return m_profile.height; }
    double fps() const override { return m_profile.fps; }

private:
    bool encodeBgra(const uint8_t* bgra, int stride, qint64 ptsUs, EncodedPacket& out);
    bool receivePacket(EncodedPacket& out);
    bool readbackTexture(ID3D11Texture2D* texture, QByteArray& bgra, int& stride);

    OutputProfile m_profile;
    QString m_name = QStringLiteral("Software H.264");
    bool m_open = false;
    qint64 m_frameIndex = 0;
    mutable std::mutex m_mutex;

#if RAILSHOT_HAS_FFMPEG
    AVCodecContext* m_ctx = nullptr;
    AVFrame* m_frame = nullptr;
    AVPacket* m_packet = nullptr;
    SwsContext* m_sws = nullptr;
#endif
};

} // namespace railshot
