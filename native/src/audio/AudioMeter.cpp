#include "audio/AudioMeter.h"
#include <algorithm>

namespace railshot {

void AudioMeter::process(const float* interleaved, int frames, int channels)
{
    if (!interleaved || frames <= 0 || channels < 1) return;
    float peakL = 0, peakR = 0;
    double sumL = 0, sumR = 0;
    for (int i = 0; i < frames; ++i) {
        const float l = interleaved[i * channels];
        const float r = channels > 1 ? interleaved[i * channels + 1] : l;
        peakL = std::max(peakL, std::abs(l));
        peakR = std::max(peakR, std::abs(r));
        sumL += double(l) * l;
        sumR += double(r) * r;
    }
    m_peakL = std::max(m_peakL, peakL);
    m_peakR = std::max(m_peakR, peakR);
    m_rmsL = float(std::sqrt(sumL / frames));
    m_rmsR = float(std::sqrt(sumR / frames));
}

void AudioMeter::decay(float factor)
{
    m_peakL *= factor;
    m_peakR *= factor;
}

} // namespace railshot
