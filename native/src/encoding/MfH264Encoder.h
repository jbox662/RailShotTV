#pragma once

#include "encoding/IEncoder.h"
#include <mutex>
#include <vector>
#include <QByteArray>

struct IMFTransform;
struct IMFMediaEventGenerator;
struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11Texture2D;

namespace railshot {

/// Native Media Foundation H.264 MFT encoder (NV12 system-memory input).
class MfH264Encoder : public IVideoEncoder {
public:
    MfH264Encoder();
    ~MfH264Encoder() override;

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
    bool configureTransform(QString* error);
    bool encodeNv12(const uint8_t* nv12, size_t bytes, qint64 ptsUs, EncodedPacket& out);
    bool processOutput(EncodedPacket& out);
    bool readbackBgra(ID3D11Texture2D* texture, std::vector<uint8_t>& bgra, int& stride);
    void bgraToNv12(const uint8_t* bgra, int stride, std::vector<uint8_t>& nv12) const;
    bool ensureExtradataFromAnnexB(const QByteArray& annexB);
    static QByteArray annexBToAvcc(const QByteArray& annexB);
    static bool isKeyframeAnnexB(const QByteArray& annexB);

    OutputProfile m_profile;
    QString m_name = QStringLiteral("MediaFoundation H.264");
    bool m_open = false;
    QByteArray m_extradata;
    mutable std::mutex m_mutex;
    std::vector<uint8_t> m_nv12Scratch;
    std::vector<uint8_t> m_bgraScratch;

#ifdef _WIN32
    IMFTransform* m_transform = nullptr;
    unsigned long m_inputId = 0;
    unsigned long m_outputId = 0;
#endif
};

} // namespace railshot
