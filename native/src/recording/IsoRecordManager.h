#pragma once

#include "core/Types.h"
#include "encoding/IEncoder.h"
#include "recording/MkvRecorder.h"
#include "audio/AudioTypes.h"
#include <QHash>
#include <QObject>
#include <QString>
#include <memory>

struct ID3D11Texture2D;

namespace railshot {

/// Per-source ISO (isolated) file recording arms.
class IsoRecordManager : public QObject {
    Q_OBJECT
public:
    explicit IsoRecordManager(QObject* parent = nullptr);
    ~IsoRecordManager() override;

    bool start(const QString& sourceId, const QString& directory, const OutputProfile& profile,
               QString* error = nullptr);
    void stop(const QString& sourceId);
    void stopAll();
    bool isArmed(const QString& sourceId) const;

    void submitVideo(const QString& sourceId, ID3D11Texture2D* texture, qint64 ptsUs);
    void submitAudio(const AudioBuffer& buffer);

signals:
    void errorOccurred(const QString& message);

private:
    struct Arm {
        QString sourceId;
        std::unique_ptr<IVideoEncoder> video;
        std::unique_ptr<IAudioEncoder> audio;
        std::unique_ptr<MkvRecorder> recorder;
        OutputProfile profile;
    };
    QHash<QString, std::shared_ptr<Arm>> m_arms;
};

} // namespace railshot
