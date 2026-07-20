#include "recording/RecordingSession.h"
#include "encoding/EncoderFactory.h"
#include "core/Logger.h"
#include <QDateTime>
#include <QDir>
#include <QMutexLocker>

namespace railshot {

RecordingSession::RecordingSession(QObject* parent)
    : QObject(parent)
{
}

bool RecordingSession::start(const QString& directory, const OutputProfile& profile, QString* error)
{
    QMutexLocker lock(&m_mutex);
    if (m_active) {
        if (error) *error = QStringLiteral("Already recording");
        return false;
    }
    m_profile = profile;
    QDir().mkpath(directory);
    const QString path = directory + QStringLiteral("/RailShotTV-%1.mkv")
                             .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss")));

    QString encErr;
    m_videoEnc = EncoderFactory::createVideo(profile, nullptr);
    if (!m_videoEnc) {
        // Factory opens the encoder; if preference failed, try software explicitly.
        OutputProfile soft = profile;
        soft.encoderPreference = QStringLiteral("software");
        m_videoEnc = EncoderFactory::createVideo(soft, nullptr);
    }
    if (!m_videoEnc) {
        if (error) *error = QStringLiteral("No working video encoder");
        return false;
    }

    m_audioEnc = EncoderFactory::createAudio(profile);
    if (!m_audioEnc) {
        m_videoEnc.reset();
        if (error) *error = QStringLiteral("No working audio encoder");
        return false;
    }

    m_recorder = std::make_unique<MkvRecorder>();
    connect(m_recorder.get(), &MkvRecorder::writeError, this, &RecordingSession::errorOccurred);
    if (!m_recorder->open(path,
                          profile,
                          m_videoEnc->extradata(),
                          m_audioEnc->extradata(),
                          m_audioEnc->sampleRate(),
                          m_audioEnc->channels(),
                          error)) {
        m_audioEnc.reset();
        m_videoEnc.reset();
        m_recorder.reset();
        return false;
    }

    m_active = true;
    m_timer.start();
    Logger::info(QStringLiteral("Recording session started: %1").arg(path));
    emit started(path);
    return true;
}

void RecordingSession::stop()
{
    QMutexLocker lock(&m_mutex);
    if (!m_active) return;
    const QString path = m_recorder ? m_recorder->path() : QString();

    QVector<EncodedPacket> flushed;
    if (m_videoEnc && m_videoEnc->flush(flushed) && m_recorder) {
        for (const auto& pkt : flushed)
            m_recorder->writeVideo(pkt);
    }
    flushed.clear();
    if (m_audioEnc && m_audioEnc->flush(flushed) && m_recorder) {
        for (const auto& pkt : flushed)
            m_recorder->writeAudio(pkt);
    }

    if (m_recorder) m_recorder->close();
    if (m_videoEnc) m_videoEnc->close();
    if (m_audioEnc) m_audioEnc->close();
    m_videoEnc.reset();
    m_audioEnc.reset();
    m_recorder.reset();
    m_active = false;
    Logger::info(QStringLiteral("Recording session stopped: %1").arg(path));
    emit stopped(path);
}

void RecordingSession::submitVideo(ID3D11Texture2D* texture, qint64 ptsUs)
{
    QMutexLocker lock(&m_mutex);
    if (!m_active || !m_videoEnc || !m_recorder) return;
    EncodedPacket pkt;
    if (m_videoEnc->encodeTexture(texture, ptsUs, pkt) && !pkt.data.isEmpty())
        m_recorder->writeVideo(pkt);
}

void RecordingSession::submitAudio(const AudioBuffer& buffer)
{
    QMutexLocker lock(&m_mutex);
    if (!m_active || !m_audioEnc || !m_recorder) return;
    QVector<EncodedPacket> packets;
    if (!m_audioEnc->encode(buffer, packets))
        return;
    for (const auto& pkt : packets) {
        if (!pkt.data.isEmpty())
            m_recorder->writeAudio(pkt);
    }
}

qint64 RecordingSession::uptimeSec() const
{
    return m_active ? m_timer.elapsed() / 1000 : 0;
}

QString RecordingSession::filePath() const
{
    return m_recorder ? m_recorder->path() : QString();
}

} // namespace railshot
