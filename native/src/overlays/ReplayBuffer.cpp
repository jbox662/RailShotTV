#include "overlays/ReplayBuffer.h"
#include "recording/MkvRecorder.h"
#include "core/Logger.h"

namespace railshot {

ReplayBuffer::ReplayBuffer(QObject* parent)
    : QObject(parent)
{
}

void ReplayBuffer::setCapacitySeconds(int seconds)
{
    m_capacitySec = qMax(5, seconds);
}

void ReplayBuffer::setEnabled(bool enabled)
{
    if (m_enabled == enabled) return;
    m_enabled = enabled;
    if (!enabled)
        clear();
    emit enabledChanged(m_enabled);
}

qint64 ReplayBuffer::bufferedDurationUs() const
{
    std::lock_guard lock(m_mutex);
    if (m_video.size() < 2) return 0;
    return m_video.back().ptsUs - m_video.front().ptsUs;
}

void ReplayBuffer::setCodecConfig(const OutputProfile& profile,
                                  const QByteArray& videoExtradata,
                                  const QByteArray& audioExtradata,
                                  int audioSampleRate,
                                  int audioChannels)
{
    std::lock_guard lock(m_mutex);
    m_profile = profile;
    m_videoExtra = videoExtradata;
    m_audioExtra = audioExtradata;
    m_audioSampleRate = audioSampleRate > 0 ? audioSampleRate : 48000;
    m_audioChannels = audioChannels > 0 ? audioChannels : 2;
    m_hasConfig = true;
}

void ReplayBuffer::trimLocked(qint64 newestPtsUs)
{
    const qint64 cutoff = newestPtsUs - qint64(m_capacitySec) * 1000000;
    while (!m_video.empty() && m_video.front().ptsUs < cutoff)
        m_video.pop_front();
    while (!m_audio.empty() && m_audio.front().ptsUs < cutoff)
        m_audio.pop_front();
}

void ReplayBuffer::pushVideo(const EncodedPacket& pkt)
{
    if (!m_enabled) return;
    std::lock_guard lock(m_mutex);
    m_video.push_back(pkt);
    trimLocked(pkt.ptsUs);
}

void ReplayBuffer::pushAudio(const EncodedPacket& pkt)
{
    if (!m_enabled) return;
    std::lock_guard lock(m_mutex);
    m_audio.push_back(pkt);
    trimLocked(pkt.ptsUs);
}

bool ReplayBuffer::saveReplay(const QString& path, QString* error)
{
    std::deque<EncodedPacket> video;
    std::deque<EncodedPacket> audio;
    OutputProfile profile;
    QByteArray videoExtra;
    QByteArray audioExtra;
    int audioRate = 48000;
    int audioCh = 2;
    {
        std::lock_guard lock(m_mutex);
        if (!m_hasConfig) {
            if (error) *error = QStringLiteral("Replay buffer has no codec config (start record/stream first)");
            return false;
        }
        if (m_video.empty()) {
            if (error) *error = QStringLiteral("Replay buffer is empty");
            return false;
        }
        video = m_video;
        audio = m_audio;
        profile = m_profile;
        videoExtra = m_videoExtra;
        audioExtra = m_audioExtra;
        audioRate = m_audioSampleRate;
        audioCh = m_audioChannels;
    }

    // Start at first keyframe for decodable H.264.
    while (!video.empty() && !video.front().keyframe)
        video.pop_front();
    if (video.empty()) {
        if (error) *error = QStringLiteral("No keyframe in replay window");
        return false;
    }

    const qint64 basePts = video.front().ptsUs;
    // Drop audio before the keyframe.
    while (!audio.empty() && audio.front().ptsUs < basePts)
        audio.pop_front();

    MkvRecorder recorder;
    if (!recorder.open(path, profile, videoExtra, audioExtra, audioRate, audioCh, error))
        return false;

    // Interleave by PTS.
    auto vIt = video.begin();
    auto aIt = audio.begin();
    while (vIt != video.end() || aIt != audio.end()) {
        const bool takeVideo = (aIt == audio.end())
            || (vIt != video.end() && vIt->ptsUs <= aIt->ptsUs);
        if (takeVideo) {
            EncodedPacket pkt = *vIt++;
            pkt.ptsUs -= basePts;
            pkt.dtsUs = pkt.ptsUs;
            if (!recorder.writeVideo(pkt)) {
                if (error) *error = QStringLiteral("Failed writing replay video");
                recorder.close();
                return false;
            }
        } else {
            EncodedPacket pkt = *aIt++;
            pkt.ptsUs -= basePts;
            pkt.dtsUs = pkt.ptsUs;
            if (!recorder.writeAudio(pkt)) {
                if (error) *error = QStringLiteral("Failed writing replay audio");
                recorder.close();
                return false;
            }
        }
    }
    recorder.close();
    Logger::info(QStringLiteral("Replay MKV saved: %1 (%2v/%3a)")
                     .arg(path)
                     .arg(video.size())
                     .arg(audio.size()));
    return true;
}

void ReplayBuffer::clear()
{
    std::lock_guard lock(m_mutex);
    m_video.clear();
    m_audio.clear();
}

int ReplayBuffer::videoPacketCount() const
{
    std::lock_guard lock(m_mutex);
    return static_cast<int>(m_video.size());
}

int ReplayBuffer::audioPacketCount() const
{
    std::lock_guard lock(m_mutex);
    return static_cast<int>(m_audio.size());
}

bool ReplayBuffer::hasConfig() const
{
    std::lock_guard lock(m_mutex);
    return m_hasConfig;
}

} // namespace railshot
