#pragma once

#include "streaming/RtmpOutput.h"
#include "encoding/IEncoder.h"
#include "audio/AudioTypes.h"
#include "core/SecretStore.h"
#include <QObject>
#include <QMutex>
#include <memory>
#include <QElapsedTimer>

namespace railshot {

class StreamSession : public QObject {
    Q_OBJECT
public:
    explicit StreamSession(QObject* parent = nullptr);

    bool start(const StreamTarget& target, const OutputProfile& profile, QString* error = nullptr);
    void stop();
    bool isActive() const { return m_active; }

    void submitVideo(ID3D11Texture2D* texture, qint64 ptsUs);
    void submitAudio(const AudioBuffer& buffer);

    ConnectionState connectionState() const;
    qint64 uptimeSec() const;
    qint64 bitrateKbps() const;
    qint64 droppedFrames() const { return m_droppedFrames; }

signals:
    void started();
    void stopped();
    void stateChanged(ConnectionState state);
    void errorOccurred(const QString& message);

private:
    mutable QMutex m_mutex;
    bool m_active = false;
    StreamTarget m_target;
    OutputProfile m_profile;
    std::unique_ptr<IVideoEncoder> m_videoEnc;
    std::unique_ptr<IAudioEncoder> m_audioEnc;
    std::unique_ptr<RtmpOutput> m_rtmp;
    QElapsedTimer m_timer;
    qint64 m_droppedFrames = 0;
    qint64 m_encodedFrames = 0;
    qint64 m_bytesAtStart = 0;
};

} // namespace railshot
