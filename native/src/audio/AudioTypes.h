#pragma once

#include <QString>
#include <cstdint>
#include <vector>

namespace railshot {

inline constexpr int kAudioSampleRate = 48000;
inline constexpr int kAudioChannels = 2;
inline constexpr int kAudioFramesPerBuffer = 480; // 10ms @ 48k

struct AudioBuffer {
    std::vector<float> samples; // interleaved float32 planar-interleaved stereo
    int channels = kAudioChannels;
    int sampleRate = kAudioSampleRate;
    qint64 ptsUs = 0;
    QString sourceId;

    int frameCount() const
    {
        return channels > 0 ? static_cast<int>(samples.size()) / channels : 0;
    }
};

enum class AudioDeviceKind {
    Capture,   // microphone
    Loopback   // desktop audio
};

struct AudioDeviceInfo {
    QString id;
    QString name;
    AudioDeviceKind kind = AudioDeviceKind::Capture;
};

} // namespace railshot
