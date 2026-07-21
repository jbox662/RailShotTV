#pragma once

#include "encoding/IEncoder.h"
#include "core/Types.h"
#include <QObject>
#include <QByteArray>
#include <deque>
#include <mutex>

namespace railshot {

/// Circular replay buffer (last N seconds of encoded video/audio) → playable MKV.
class ReplayBuffer : public QObject {
    Q_OBJECT
public:
    explicit ReplayBuffer(QObject* parent = nullptr);

    void setCapacitySeconds(int seconds);
    int capacitySeconds() const { return m_capacitySec; }
    qint64 bufferedDurationUs() const;
    /// OBS Start/Stop Replay Buffer — when false, push* is ignored.
    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }
    void setCodecConfig(const OutputProfile& profile,
                        const QByteArray& videoExtradata,
                        const QByteArray& audioExtradata,
                        int audioSampleRate,
                        int audioChannels);
    void pushVideo(const EncodedPacket& pkt);
    void pushAudio(const EncodedPacket& pkt);
    bool saveReplay(const QString& path, QString* error = nullptr);
    void clear();
    int videoPacketCount() const;
    int audioPacketCount() const;
    bool hasConfig() const;

signals:
    void enabledChanged(bool enabled);

private:
    void trimLocked(qint64 newestPtsUs);

    int m_capacitySec = 30;
    bool m_enabled = false;
    mutable std::mutex m_mutex;
    std::deque<EncodedPacket> m_video;
    std::deque<EncodedPacket> m_audio;
    OutputProfile m_profile;
    QByteArray m_videoExtra;
    QByteArray m_audioExtra;
    int m_audioSampleRate = 48000;
    int m_audioChannels = 2;
    bool m_hasConfig = false;
};

} // namespace railshot
