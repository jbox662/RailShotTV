#pragma once

#include "encoding/IEncoder.h"
#include <mutex>
#include <vector>

struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwrContext;
struct AVAudioFifo;

namespace railshot {

class AacEncoder : public IAudioEncoder {
public:
    AacEncoder();
    ~AacEncoder() override;
    bool open(const OutputProfile& profile, QString* error = nullptr) override;
    void close() override;
    bool encode(const AudioBuffer& buffer, QVector<EncodedPacket>& out) override;
    bool flush(QVector<EncodedPacket>& out) override;
    QByteArray extradata() const override;
    int sampleRate() const override;
    int channels() const override;

private:
    bool encodeFifoFrame(EncodedPacket& out);
    bool receivePacket(EncodedPacket& out);

    OutputProfile m_profile;
    bool m_open = false;
    qint64 m_nextPtsSamples = 0;
    mutable std::mutex m_mutex;

#if RAILSHOT_HAS_FFMPEG
    AVCodecContext* m_ctx = nullptr;
    AVFrame* m_frame = nullptr;
    AVPacket* m_packet = nullptr;
    SwrContext* m_swr = nullptr;
    AVAudioFifo* m_fifo = nullptr;
#endif
};

} // namespace railshot
