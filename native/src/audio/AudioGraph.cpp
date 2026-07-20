#include "audio/AudioGraph.h"
#include "audio/WasapiCapture.h"
#include "audio/AudioMonitor.h"
#include "core/Logger.h"
#include <cmath>
#include <algorithm>

namespace railshot {

AudioGraph::AudioGraph(QObject* parent)
    : QObject(parent)
{
    AudioChannelState desktop;
    desktop.id = QStringLiteral("desktop");
    desktop.name = QStringLiteral("Desktop Audio");
    m_channels.insert(desktop.id, desktop);

    AudioChannelState mic;
    mic.id = QStringLiteral("mic");
    mic.name = QStringLiteral("Mic/Aux");
    m_channels.insert(mic.id, mic);
}

AudioGraph::~AudioGraph()
{
    shutdown();
}

bool AudioGraph::initialize(QString* error)
{
    m_clock.start();
    m_desktop = std::make_unique<WasapiCapture>(AudioDeviceKind::Loopback);
    m_mic = std::make_unique<WasapiCapture>(AudioDeviceKind::Capture);
    m_monitor = std::make_unique<AudioMonitor>();

    m_desktop->setCallback([this](const AudioBuffer& b) { onCapture(QStringLiteral("desktop"), b); });
    m_mic->setCallback([this](const AudioBuffer& b) { onCapture(QStringLiteral("mic"), b); });

    if (!m_desktop->start(error)) return false;
    if (!m_mic->start(error)) return false;
    m_monitor->start(error);
    Logger::info(QStringLiteral("Audio graph initialized"));
    return true;
}

void AudioGraph::shutdown()
{
    if (m_desktop) m_desktop->stop();
    if (m_mic) m_mic->stop();
    if (m_monitor) m_monitor->stop();
    m_clock.stop();
}

void AudioGraph::setChannelState(const QString& id, const AudioChannelState& state)
{
    std::lock_guard lock(m_mutex);
    m_channels[id] = state;
}

AudioChannelState AudioGraph::channelState(const QString& id) const
{
    std::lock_guard lock(m_mutex);
    return m_channels.value(id);
}

QVector<AudioChannelState> AudioGraph::channels() const
{
    std::lock_guard lock(m_mutex);
    QVector<AudioChannelState> out;
    for (auto it = m_channels.begin(); it != m_channels.end(); ++it) {
        AudioChannelState s = it.value();
        auto mit = m_meters.constFind(it.key());
        if (mit != m_meters.cend()) {
            s.peakL = mit->peakL();
            s.peakR = mit->peakR();
            s.rmsL = mit->rmsL();
            s.rmsR = mit->rmsR();
        }
        out.append(s);
    }
    return out;
}

void AudioGraph::setMasterVolume(float v)
{
    m_masterVolume = std::clamp(v, 0.f, 1.f);
}

void AudioGraph::setMasterMuted(bool m)
{
    m_masterMuted = m;
}

void AudioGraph::setOutputCallback(std::function<void(const AudioBuffer&)> cb)
{
    std::lock_guard lock(m_mutex);
    m_outputCb = std::move(cb);
}

void AudioGraph::setMonitorEnabled(bool enabled)
{
    m_monitorEnabled = enabled;
}

namespace {
AudioBuffer resampleTo48k(const AudioBuffer& in)
{
    if (in.sampleRate == kAudioSampleRate || in.sampleRate <= 0 || in.frameCount() <= 0)
        return in;
    AudioBuffer out = in;
    out.sampleRate = kAudioSampleRate;
    const double ratio = double(kAudioSampleRate) / double(in.sampleRate);
    const int outFrames = int(std::ceil(in.frameCount() * ratio));
    out.samples.assign(size_t(outFrames) * size_t(in.channels), 0.f);
    for (int i = 0; i < outFrames; ++i) {
        const double srcPos = i / ratio;
        const int i0 = int(srcPos);
        const int i1 = std::min(i0 + 1, in.frameCount() - 1);
        const float t = float(srcPos - i0);
        for (int c = 0; c < in.channels; ++c) {
            const float a = in.samples[size_t(i0) * in.channels + c];
            const float b = in.samples[size_t(i1) * in.channels + c];
            out.samples[size_t(i) * in.channels + c] = a + (b - a) * t;
        }
    }
    return out;
}
} // namespace

void AudioGraph::onCapture(const QString& channelId, const AudioBuffer& buffer)
{
    const AudioBuffer normalized = resampleTo48k(buffer);
    {
        std::lock_guard lock(m_mutex);
        m_pending[channelId] = normalized;
        m_meters[channelId].process(normalized.samples.data(), normalized.frameCount(), normalized.channels);
        m_meters[channelId].decay();
    }
    mixAndEmit();
}

void AudioGraph::mixAndEmit()
{
    AudioBuffer mixed;
    mixed.channels = kAudioChannels;
    mixed.sampleRate = kAudioSampleRate;
    mixed.ptsUs = m_clock.nowUs();

    std::function<void(const AudioBuffer&)> cb;
    {
        std::lock_guard lock(m_mutex);
        // Determine frame count from first pending buffer
        int frames = kAudioFramesPerBuffer;
        for (const auto& b : m_pending)
            frames = std::max(frames, b.frameCount());
        mixed.samples.assign(static_cast<size_t>(frames) * kAudioChannels, 0.f);

        bool anySolo = false;
        for (const auto& ch : m_channels)
            if (ch.solo) { anySolo = true; break; }

        for (auto it = m_pending.begin(); it != m_pending.end(); ++it) {
            const auto ch = m_channels.value(it.key());
            if (ch.muted) continue;
            if (anySolo && !ch.solo) continue;
            const float linear = ch.volume * std::pow(10.f, ch.gainDb / 20.f);
            const float panL = std::cos((ch.pan + 1.f) * 0.25f * 3.14159265f);
            const float panR = std::sin((ch.pan + 1.f) * 0.25f * 3.14159265f);
            const auto& buf = it.value();
            const int n = std::min(frames, buf.frameCount());
            for (int i = 0; i < n; ++i) {
                const float l = buf.samples[i * buf.channels] * linear * panL;
                const float r = (buf.channels > 1 ? buf.samples[i * buf.channels + 1] : buf.samples[i * buf.channels])
                                * linear * panR;
                mixed.samples[i * 2] += l;
                mixed.samples[i * 2 + 1] += r;
            }
        }

        const float master = m_masterMuted ? 0.f : m_masterVolume;
        for (float& s : mixed.samples)
            s = std::clamp(s * master, -1.f, 1.f);

        m_masterMeter.process(mixed.samples.data(), frames, 2);
        m_masterMeter.decay();
        cb = m_outputCb;
        m_pending.clear();
    }

    if (m_monitor && m_monitorEnabled)
        m_monitor->push(mixed);
    if (cb)
        cb(mixed);
    emit metersUpdated();
}

} // namespace railshot
