#include "recording/IsoRecordManager.h"
#include "encoding/EncoderFactory.h"
#include "core/Logger.h"
#include <QDateTime>
#include <QDir>

namespace railshot {

IsoRecordManager::IsoRecordManager(QObject* parent)
    : QObject(parent)
{
}

IsoRecordManager::~IsoRecordManager()
{
    stopAll();
}

bool IsoRecordManager::start(const QString& sourceId, const QString& directory,
                             const OutputProfile& profile, QString* error)
{
    if (sourceId.isEmpty()) {
        if (error) *error = QStringLiteral("Missing source id");
        return false;
    }
    if (m_arms.contains(sourceId))
        return true;

    QDir().mkpath(directory);
    auto arm = std::make_shared<Arm>();
    arm->sourceId = sourceId;
    arm->profile = profile;
    QString encName;
    arm->video = EncoderFactory::createVideo(profile, &encName);
    arm->audio = EncoderFactory::createAudio(profile);
    if (!arm->video || !arm->audio) {
        if (error) *error = QStringLiteral("ISO encoder unavailable");
        return false;
    }
    if (!arm->video->open(profile, error) || !arm->audio->open(profile, error))
        return false;

    const QString path = directory
        + QStringLiteral("/RailShotTV-ISO-%1-%2.mkv")
              .arg(sourceId.left(8),
                   QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss")));

    arm->recorder = std::make_unique<MkvRecorder>();
    if (!arm->recorder->open(path, profile, arm->video->extradata(), arm->audio->extradata(),
                             arm->audio->sampleRate(), arm->audio->channels(), error)) {
        return false;
    }
    m_arms.insert(sourceId, arm);
    Logger::info(QStringLiteral("ISO recording armed %1 -> %2 (%3)").arg(sourceId, path, encName));
    return true;
}

void IsoRecordManager::stop(const QString& sourceId)
{
    auto it = m_arms.find(sourceId);
    if (it == m_arms.end()) return;
    if (it.value()->recorder)
        it.value()->recorder->close();
    m_arms.erase(it);
}

void IsoRecordManager::stopAll()
{
    const auto ids = m_arms.keys();
    for (const auto& id : ids)
        stop(id);
}

bool IsoRecordManager::isArmed(const QString& sourceId) const
{
    return m_arms.contains(sourceId);
}

void IsoRecordManager::submitVideo(const QString& sourceId, ID3D11Texture2D* texture, qint64 ptsUs)
{
    auto it = m_arms.find(sourceId);
    if (it == m_arms.end() || !texture) return;
    auto& arm = *it.value();
    EncodedPacket pkt;
    if (arm.video->encodeTexture(texture, ptsUs, pkt) && arm.recorder)
        arm.recorder->writeVideo(pkt);
}

void IsoRecordManager::submitAudio(const AudioBuffer& buffer)
{
    if (m_arms.isEmpty()) return;
    for (auto it = m_arms.begin(); it != m_arms.end(); ++it) {
        auto& arm = *it.value();
        QVector<EncodedPacket> pkts;
        if (arm.audio->encode(buffer, pkts) && arm.recorder) {
            for (const auto& p : pkts)
                arm.recorder->writeAudio(p);
        }
    }
}

} // namespace railshot
