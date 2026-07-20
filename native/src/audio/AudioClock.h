#pragma once

#include <cstdint>
#include <atomic>

#include <QtGlobal>

namespace railshot {

/// Master audio clock — all A/V timestamps synchronize to this.
class AudioClock {
public:
    void start();
    void stop();
    bool running() const { return m_running.load(); }

    /// Current media time in microseconds since start.
    qint64 nowUs() const;

    /// Convert video PTS to audio-clock domain and report drift.
    double driftMs(qint64 videoPtsUs) const;

private:
    std::atomic<bool> m_running{false};
    std::atomic<qint64> m_startNs{0};
};

} // namespace railshot
