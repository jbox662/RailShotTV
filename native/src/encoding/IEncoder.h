#pragma once

#include "core/Types.h"
#include "audio/AudioTypes.h"
#include <QByteArray>
#include <QImage>
#include <QVector>
#include <memory>

struct ID3D11Texture2D;

namespace railshot {

struct EncodedPacket {
    QByteArray data;
    qint64 ptsUs = 0;
    qint64 dtsUs = 0;
    bool keyframe = false;
    bool video = true;
};

class IVideoEncoder {
public:
    virtual ~IVideoEncoder() = default;
    virtual QString name() const = 0;
    virtual bool open(const OutputProfile& profile, QString* error = nullptr) = 0;
    virtual void close() = 0;
    /// Returns true when a packet is produced. False can mean "need more input" (not always an error).
    virtual bool encodeTexture(ID3D11Texture2D* texture, qint64 ptsUs, EncodedPacket& out) = 0;
    virtual bool encodeImage(const QImage& image, qint64 ptsUs, EncodedPacket& out) = 0;
    virtual bool flush(QVector<EncodedPacket>& out)
    {
        (void)out;
        return true;
    }
    virtual QByteArray extradata() const { return {}; }
    virtual int width() const { return 0; }
    virtual int height() const { return 0; }
    virtual double fps() const { return 0.0; }
};

class IAudioEncoder {
public:
    virtual ~IAudioEncoder() = default;
    virtual bool open(const OutputProfile& profile, QString* error = nullptr) = 0;
    virtual void close() = 0;
    /// Encodes available AAC frames into out. Returns false on hard failure.
    /// out may be empty when still buffering toward frame_size.
    virtual bool encode(const AudioBuffer& buffer, QVector<EncodedPacket>& out) = 0;
    virtual bool flush(QVector<EncodedPacket>& out)
    {
        (void)out;
        return true;
    }
    virtual QByteArray extradata() const { return {}; }
    virtual int sampleRate() const { return kAudioSampleRate; }
    virtual int channels() const { return kAudioChannels; }
};

} // namespace railshot

Q_DECLARE_METATYPE(railshot::EncodedPacket)
