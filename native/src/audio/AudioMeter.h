#pragma once

#include "core/Types.h"
#include <QObject>
#include <deque>
#include <mutex>
#include <cmath>

namespace railshot {

class AudioMeter {
public:
    void process(const float* interleaved, int frames, int channels);
    float peakL() const { return m_peakL; }
    float peakR() const { return m_peakR; }
    float rmsL() const { return m_rmsL; }
    float rmsR() const { return m_rmsR; }
    void decay(float factor = 0.92f);

private:
    float m_peakL = 0, m_peakR = 0;
    float m_rmsL = 0, m_rmsR = 0;
};

} // namespace railshot
