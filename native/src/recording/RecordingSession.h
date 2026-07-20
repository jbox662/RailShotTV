#pragma once

#include "recording/MkvRecorder.h"
#include "encoding/IEncoder.h"
#include "audio/AudioTypes.h"
#include "core/Types.h"
#include <QObject>
#include <QMutex>
#include <memory>
#include <QElapsedTimer>

namespace railshot {

class RecordingSession : public QObject {
    Q_OBJECT
public:
    explicit RecordingSession(QObject* parent = nullptr);

    bool start(const QString& directory, const OutputProfile& profile, QString* error = nullptr);
    void stop();
    bool isActive() const { return m_active; }

    void submitVideo(ID3D11Texture2D* texture, qint64 ptsUs);
    void submitAudio(const AudioBuffer& buffer);

    qint64 uptimeSec() const;
    QString filePath() const;

signals:
    void started(const QString& path);
    void stopped(const QString& path);
    void errorOccurred(const QString& message);

private:
    mutable QMutex m_mutex;
    bool m_active = false;
    OutputProfile m_profile;
    std::unique_ptr<IVideoEncoder> m_videoEnc;
    std::unique_ptr<IAudioEncoder> m_audioEnc;
    std::unique_ptr<MkvRecorder> m_recorder;
    QElapsedTimer m_timer;
};

} // namespace railshot
