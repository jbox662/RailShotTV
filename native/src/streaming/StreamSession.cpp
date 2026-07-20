#include "streaming/StreamSession.h"
#include "encoding/EncoderFactory.h"
#include "core/Logger.h"
#include <QVector>
#include <QMutexLocker>

namespace railshot {

StreamSession::StreamSession(QObject* parent)
    : QObject(parent)
{
}

bool StreamSession::start(const StreamTarget& target, const OutputProfile& profile, QString* error)
{
    QMutexLocker lock(&m_mutex);
    if (m_active) {
        if (error) *error = QStringLiteral("Already streaming");
        return false;
    }
    m_target = target;
    m_profile = profile;

    QString encName;
    OutputProfile soft = profile;
    if (soft.encoderPreference.isEmpty() || soft.encoderPreference == QLatin1String("auto"))
        soft.encoderPreference = QStringLiteral("software");

    m_videoEnc = EncoderFactory::createVideo(soft, &encName);
    if (!m_videoEnc) {
        if (error) *error = QStringLiteral("No video encoder available");
        return false;
    }
    m_audioEnc = EncoderFactory::createAudio(soft);
    if (!m_audioEnc) {
        m_videoEnc.reset();
        if (error) *error = QStringLiteral("No audio encoder available");
        return false;
    }

    m_rtmp = std::make_unique<RtmpOutput>();
    connect(m_rtmp.get(), &RtmpOutput::stateChanged, this, &StreamSession::stateChanged);
    connect(m_rtmp.get(), &RtmpOutput::networkError, this, &StreamSession::errorOccurred);

    QString key;
    if (!target.streamKeySecretId.isEmpty()) {
        auto loaded = SecretStore::load(target.streamKeySecretId);
        if (loaded) key = *loaded;
    }
    if (key.isEmpty())
        Logger::warn(QStringLiteral("Stream key missing for target %1").arg(target.name));

    if (!m_rtmp->connectTo(target.rtmpUrl, key, profile,
                           m_videoEnc->extradata(), m_audioEnc->extradata(),
                           m_audioEnc->sampleRate(), m_audioEnc->channels(), error)) {
        m_audioEnc.reset();
        m_videoEnc.reset();
        m_rtmp.reset();
        return false;
    }

    m_active = true;
    m_droppedFrames = 0;
    m_encodedFrames = 0;
    m_bytesAtStart = m_rtmp->bytesSent();
    m_timer.start();
    Logger::info(QStringLiteral("Stream session started via %1").arg(encName));
    emit started();
    return true;
}

void StreamSession::stop()
{
    QMutexLocker lock(&m_mutex);
    if (!m_active) return;

    QVector<EncodedPacket> flushed;
    if (m_videoEnc && m_videoEnc->flush(flushed) && m_rtmp) {
        for (const auto& pkt : flushed)
            m_rtmp->pushVideo(pkt);
    }
    flushed.clear();
    if (m_audioEnc && m_audioEnc->flush(flushed) && m_rtmp) {
        for (const auto& pkt : flushed)
            m_rtmp->pushAudio(pkt);
    }

    if (m_rtmp) m_rtmp->disconnectFrom();
    if (m_videoEnc) m_videoEnc->close();
    if (m_audioEnc) m_audioEnc->close();
    m_videoEnc.reset();
    m_audioEnc.reset();
    m_rtmp.reset();
    m_active = false;
    emit stopped();
    Logger::info(QStringLiteral("Stream session stopped"));
}

void StreamSession::submitVideo(ID3D11Texture2D* texture, qint64 ptsUs)
{
    QMutexLocker lock(&m_mutex);
    if (!m_active || !m_videoEnc || !m_rtmp) return;
    EncodedPacket pkt;
    if (!m_videoEnc->encodeTexture(texture, ptsUs, pkt) || pkt.data.isEmpty()) {
        // Encoder may be buffering — not always a drop.
        return;
    }
    m_rtmp->pushVideo(pkt);
    m_encodedFrames++;
}

void StreamSession::submitAudio(const AudioBuffer& buffer)
{
    QMutexLocker lock(&m_mutex);
    if (!m_active || !m_audioEnc || !m_rtmp) return;
    QVector<EncodedPacket> packets;
    if (!m_audioEnc->encode(buffer, packets))
        return;
    for (const auto& pkt : packets)
        m_rtmp->pushAudio(pkt);
}

ConnectionState StreamSession::connectionState() const
{
    return m_rtmp ? m_rtmp->state() : ConnectionState::Disconnected;
}

qint64 StreamSession::uptimeSec() const
{
    return m_active ? m_timer.elapsed() / 1000 : 0;
}

qint64 StreamSession::bitrateKbps() const
{
    if (!m_active || !m_rtmp || m_timer.elapsed() < 1000) return 0;
    const qint64 bytes = m_rtmp->bytesSent() - m_bytesAtStart;
    return (bytes * 8) / m_timer.elapsed();
}

} // namespace railshot
