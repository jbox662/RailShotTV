#include "audio/AudioClock.h"
#include <chrono>

namespace railshot {

void AudioClock::start()
{
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    m_startNs = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
    m_running = true;
}

void AudioClock::stop()
{
    m_running = false;
}

qint64 AudioClock::nowUs() const
{
    if (!m_running.load()) return 0;
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    const qint64 ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
    return (ns - m_startNs.load()) / 1000;
}

double AudioClock::driftMs(qint64 videoPtsUs) const
{
    return (double(videoPtsUs) - double(nowUs())) / 1000.0;
}

} // namespace railshot
