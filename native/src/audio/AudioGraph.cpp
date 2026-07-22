#include "audio/AudioGraph.h"
#include "audio/WasapiCapture.h"
#include "audio/AudioMonitor.h"
#include "core/Logger.h"
#include <cmath>
#include <algorithm>

namespace railshot {

namespace {
constexpr int kMaxSyncMs = 2000;
constexpr int kTrack1Bit = 0x01;

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
    return initialize({}, {}, error);
}

bool AudioGraph::initialize(const QString& desktopDeviceId, const QString& micDeviceId, QString* error)
{
    shutdown();
    m_desktopDeviceId = desktopDeviceId;
    m_micDeviceId = micDeviceId;
    m_clock.start();
    m_desktop = std::make_unique<WasapiCapture>(AudioDeviceKind::Loopback, m_desktopDeviceId);
    m_mic = std::make_unique<WasapiCapture>(AudioDeviceKind::Capture, m_micDeviceId);
    m_monitor = std::make_unique<AudioMonitor>();

    m_desktop->setCallback([this](const AudioBuffer& b) { onCapture(QStringLiteral("desktop"), b); });
    m_mic->setCallback([this](const AudioBuffer& b) { onCapture(QStringLiteral("mic"), b); });

    if (!m_desktop->start(error)) return false;
    if (!m_mic->start(error)) return false;
    m_monitor->start(error);
    Logger::info(QStringLiteral("Audio graph initialized (desktop=%1 mic=%2)")
                     .arg(m_desktopDeviceId.isEmpty() ? QStringLiteral("default") : m_desktopDeviceId,
                          m_micDeviceId.isEmpty() ? QStringLiteral("default") : m_micDeviceId));
    return true;
}

bool AudioGraph::reconfigureDevices(const QString& desktopDeviceId, const QString& micDeviceId, QString* error)
{
    if (desktopDeviceId == m_desktopDeviceId && micDeviceId == m_micDeviceId
        && m_desktop && m_mic && m_desktop->isRunning() && m_mic->isRunning()) {
        return true;
    }
    return initialize(desktopDeviceId, micDeviceId, error);
}

void AudioGraph::shutdown()
{
    if (m_desktop) m_desktop->stop();
    if (m_mic) m_mic->stop();
    if (m_monitor) m_monitor->stop();
    m_desktop.reset();
    m_mic.reset();
    m_monitor.reset();
    m_clock.stop();
}

void AudioGraph::setChannelState(const QString& id, const AudioChannelState& state)
{
    std::lock_guard lock(m_mutex);
    AudioChannelState s = state;
    s.id = id;
    s.syncOffsetMs = std::clamp(s.syncOffsetMs, -kMaxSyncMs, kMaxSyncMs);
    s.volume = std::clamp(s.volume, 0.f, 20.f); // up to ~+26 dB like OBS
    s.pan = std::clamp(s.pan, -1.f, 1.f);
    m_channels[id] = s;
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
    // Stable order: desktop, mic, then others by id
    auto appendIf = [&](const QString& id) {
        auto it = m_channels.constFind(id);
        if (it == m_channels.cend()) return;
        AudioChannelState s = it.value();
        auto mit = m_meters.constFind(id);
        if (mit != m_meters.cend()) {
            s.peakL = mit->peakL();
            s.peakR = mit->peakR();
            s.rmsL = mit->rmsL();
            s.rmsR = mit->rmsR();
        }
        out.append(s);
    };
    appendIf(QStringLiteral("desktop"));
    appendIf(QStringLiteral("mic"));
    QStringList rest;
    for (auto it = m_channels.begin(); it != m_channels.end(); ++it) {
        if (it.key() == QLatin1String("desktop") || it.key() == QLatin1String("mic"))
            continue;
        rest.append(it.key());
    }
    rest.sort();
    for (const auto& id : rest)
        appendIf(id);

    // Master strip (synthetic) for the mixer UI
    AudioChannelState master;
    master.id = QStringLiteral("master");
    master.name = QStringLiteral("Master");
    master.volume = m_masterVolume;
    master.muted = m_masterMuted;
    master.peakL = m_masterMeter.peakL();
    master.peakR = m_masterMeter.peakR();
    master.rmsL = m_masterMeter.rmsL();
    master.rmsR = m_masterMeter.rmsR();
    out.prepend(master);
    return out;
}

void AudioGraph::ensureChannel(const QString& id, const QString& name)
{
    (void)ensureChannelEx(id, name);
}

bool AudioGraph::ensureChannelEx(const QString& id, const QString& name)
{
    bool created = false;
    bool renamed = false;
    {
        std::lock_guard lock(m_mutex);
        auto it = m_channels.find(id);
        if (it == m_channels.end()) {
            AudioChannelState ch;
            ch.id = id;
            ch.name = name;
            m_channels.insert(id, ch);
            created = true;
        } else if (!name.isEmpty() && it->name != name) {
            it->name = name;
            renamed = true;
        }
    }
    if (created || renamed)
        emit channelsChanged();
    return created;
}

void AudioGraph::syncDynamicChannels(const QHash<QString, QString>& keepIdToName)
{
    bool changed = false;
    {
        std::lock_guard lock(m_mutex);
        QStringList drop;
        for (auto it = m_channels.constBegin(); it != m_channels.constEnd(); ++it) {
            const QString& id = it.key();
            if (id == QLatin1String("desktop") || id == QLatin1String("mic") || id == QLatin1String("master"))
                continue;
            if (!keepIdToName.contains(id))
                drop.append(id);
        }
        for (const QString& id : drop) {
            m_channels.remove(id);
            m_pending.remove(id);
            m_meters.remove(id);
            m_delayLines.remove(id);
            changed = true;
        }
        for (auto it = keepIdToName.constBegin(); it != keepIdToName.constEnd(); ++it) {
            auto chIt = m_channels.find(it.key());
            if (chIt == m_channels.end()) {
                AudioChannelState ch;
                ch.id = it.key();
                ch.name = it.value();
                m_channels.insert(it.key(), ch);
                changed = true;
            } else if (!it.value().isEmpty() && chIt->name != it.value()) {
                chIt->name = it.value();
                changed = true;
            }
        }
    }
    if (changed)
        emit channelsChanged();
}

void AudioGraph::removeChannel(const QString& id)
{
    if (id == QLatin1String("desktop") || id == QLatin1String("mic") || id == QLatin1String("master"))
        return;
    {
        std::lock_guard lock(m_mutex);
        m_channels.remove(id);
        m_pending.remove(id);
        m_meters.remove(id);
        m_delayLines.remove(id);
        m_echoLines.remove(id);
        m_nsEnv.remove(id);
        m_nsHpZL.remove(id);
        m_nsHpZR.remove(id);
        m_gateEnv.remove(id);
        m_gateHold.remove(id);
        m_compEnv.remove(id);
        m_expEnv.remove(id);
        m_eqL.remove(id);
        m_eqR.remove(id);
        m_limEnv.remove(id);
    }
    emit channelsChanged();
}

void AudioGraph::inject(const QString& channelId, const AudioBuffer& buffer)
{
    onCapture(channelId, buffer);
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

QJsonArray AudioGraph::channelsToJson() const
{
    std::lock_guard lock(m_mutex);
    QJsonArray arr;
    for (auto it = m_channels.begin(); it != m_channels.end(); ++it)
        arr.append(it.value().toJson());
    return arr;
}

void AudioGraph::applyChannelsFromJson(const QJsonArray& arr)
{
    {
        std::lock_guard lock(m_mutex);
        for (const auto& v : arr) {
            const auto s = AudioChannelState::fromJson(v.toObject());
            if (s.id.isEmpty() || s.id == QLatin1String("master"))
                continue;
            auto it = m_channels.find(s.id);
            if (it == m_channels.end()) {
                m_channels.insert(s.id, s);
            } else {
                // Keep live peaks; restore user settings
                AudioChannelState cur = it.value();
                cur.volume = s.volume;
                cur.pan = s.pan;
                cur.gainDb = s.gainDb;
                cur.muted = s.muted;
                cur.solo = s.solo;
                cur.forceMono = s.forceMono;
                cur.locked = s.locked;
                cur.syncOffsetMs = std::clamp(s.syncOffsetMs, -kMaxSyncMs, kMaxSyncMs);
                cur.monitoring = s.monitoring;
                cur.trackMask = s.trackMask;
                cur.nsEnabled = s.nsEnabled;
                cur.nsStrength = s.nsStrength;
                cur.nsFloorDb = s.nsFloorDb;
                cur.nsHpHz = s.nsHpHz;
                cur.gateEnabled = s.gateEnabled;
                cur.gateOpenDb = s.gateOpenDb;
                cur.gateAttackMs = s.gateAttackMs;
                cur.gateHoldMs = s.gateHoldMs;
                cur.gateReleaseMs = s.gateReleaseMs;
                cur.compEnabled = s.compEnabled;
                cur.compThresholdDb = s.compThresholdDb;
                cur.compRatio = s.compRatio;
                cur.compAttackMs = s.compAttackMs;
                cur.compReleaseMs = s.compReleaseMs;
                cur.compMakeupDb = s.compMakeupDb;
                cur.expEnabled = s.expEnabled;
                cur.expThresholdDb = s.expThresholdDb;
                cur.expRatio = s.expRatio;
                cur.expAttackMs = s.expAttackMs;
                cur.expReleaseMs = s.expReleaseMs;
                cur.echoEnabled = s.echoEnabled;
                cur.echoDelayMs = s.echoDelayMs;
                cur.echoDecay = s.echoDecay;
                cur.echoWet = s.echoWet;
                cur.eqLowDb = s.eqLowDb;
                cur.eqMidDb = s.eqMidDb;
                cur.eqHighDb = s.eqHighDb;
                cur.limiterEnabled = s.limiterEnabled;
                cur.limiterThresholdDb = s.limiterThresholdDb;
                cur.limiterReleaseMs = s.limiterReleaseMs;
                if (!s.name.isEmpty())
                    cur.name = s.name;
                *it = cur;
            }
        }
    }
    emit channelsChanged();
}

AudioBuffer AudioGraph::applySyncDelay(const QString& channelId, const AudioBuffer& in, int syncOffsetMs)
{
    if (syncOffsetMs == 0 || in.frameCount() <= 0)
        return in;

    // Positive offset = delay this source (push past samples). Negative = advance (drop samples).
    auto& line = m_delayLines[channelId];
    const int ch = std::max(1, in.channels);
    const int delayFrames = std::abs(syncOffsetMs) * kAudioSampleRate / 1000;
    const int delaySamples = delayFrames * 2; // always store as stereo interleaved

    // Push incoming as stereo
    for (int i = 0; i < in.frameCount(); ++i) {
        const float l = in.samples[size_t(i) * ch];
        const float r = ch > 1 ? in.samples[size_t(i) * ch + 1] : l;
        line.push_back(l);
        line.push_back(r);
    }

    const int maxKeep = delaySamples + in.frameCount() * 2 + 64;
    while (int(line.size()) > maxKeep)
        line.pop_front();

    AudioBuffer out;
    out.channels = 2;
    out.sampleRate = kAudioSampleRate;
    out.ptsUs = in.ptsUs;
    out.sourceId = in.sourceId;
    out.samples.resize(size_t(in.frameCount()) * 2);

    if (syncOffsetMs > 0) {
        // Need delaySamples of history before emitting current block
        while (int(line.size()) < delaySamples + in.frameCount() * 2) {
            line.push_front(0.f);
            line.push_front(0.f);
        }
        const int start = int(line.size()) - delaySamples - in.frameCount() * 2;
        for (int i = 0; i < in.frameCount() * 2; ++i)
            out.samples[size_t(i)] = line[size_t(start + i)];
    } else {
        // Negative: skip delayFrames from the front of the newest block
        const int available = int(line.size()) / 2;
        const int skip = std::min(delayFrames, std::max(0, available - in.frameCount()));
        const int startSample = skip * 2;
        for (int i = 0; i < in.frameCount() * 2; ++i) {
            const int idx = startSample + i;
            out.samples[size_t(i)] = (idx < int(line.size())) ? line[size_t(idx)] : 0.f;
        }
        // Drop consumed early samples so the buffer doesn't grow forever
        const int drop = std::min(int(line.size()), in.frameCount() * 2);
        for (int i = 0; i < drop; ++i)
            line.pop_front();
    }
    return out;
}

void AudioGraph::applyChannelDsp(const QString& channelId, const AudioChannelState& ch, AudioBuffer& buf)
{
    if (buf.samples.empty() || buf.frameCount() <= 0)
        return;
    const int n = buf.frameCount();
    const int bch = std::max(1, buf.channels);
    const float sr = float(buf.sampleRate > 0 ? buf.sampleRate : kAudioSampleRate);

    // Lightweight noise suppress: optional HPF + soft downward expander (no RNNoise).
    if (ch.nsEnabled) {
        float& env = m_nsEnv[channelId];
        float& zL = m_nsHpZL[channelId];
        float& zR = m_nsHpZR[channelId];
        const float strength = std::clamp(ch.nsStrength, 0.f, 100.f) / 100.f;
        const float floorLin = std::pow(10.f, ch.nsFloorDb / 20.f);
        const float att = std::exp(-1.f / std::max(1.f, 0.005f * sr)); // ~5 ms
        const float rel = std::exp(-1.f / std::max(1.f, 0.080f * sr)); // ~80 ms
        const float hpHz = std::clamp(ch.nsHpHz, 0.f, 200.f);
        const float hpRc = (hpHz > 0.5f) ? std::exp(-2.f * 3.14159265f * hpHz / sr) : 0.f;

        for (int i = 0; i < n; ++i) {
            float l = buf.samples[size_t(i) * bch];
            float r = bch > 1 ? buf.samples[size_t(i) * bch + 1] : l;
            if (hpRc > 0.f) {
                const float nl = l - zL;
                const float nr = r - zR;
                zL = l + hpRc * (zL - l);
                zR = r + hpRc * (zR - r);
                l = nl;
                r = nr;
            }
            const float level = std::max(std::abs(l), std::abs(r));
            const float coef = (level > env) ? att : rel;
            env = level + coef * (env - level);
            float gain = 1.f;
            if (env < floorLin && floorLin > 1e-8f) {
                const float ratio = env / floorLin;
                // Soft expand: pull quieter material down by strength
                gain = std::pow(std::max(ratio, 1e-4f), strength * 2.5f);
            }
            buf.samples[size_t(i) * bch] = l * gain;
            if (bch > 1)
                buf.samples[size_t(i) * bch + 1] = r * gain;
        }
    }

    if (ch.gateEnabled) {
        float& env = m_gateEnv[channelId];
        float& hold = m_gateHold[channelId];
        const float openLin = std::pow(10.f, ch.gateOpenDb / 20.f);
        const float att = std::exp(-1.f / std::max(1.f, ch.gateAttackMs * 0.001f * sr));
        const float rel = std::exp(-1.f / std::max(1.f, ch.gateReleaseMs * 0.001f * sr));
        const float holdSamples = std::max(1.f, ch.gateHoldMs * 0.001f * sr);
        for (int i = 0; i < n; ++i) {
            float l = buf.samples[size_t(i) * bch];
            float r = bch > 1 ? buf.samples[size_t(i) * bch + 1] : l;
            const float level = std::max(std::abs(l), std::abs(r));
            if (level >= openLin)
                hold = holdSamples;
            const bool open = hold > 0.f;
            if (open) hold -= 1.f;
            const float target = open ? 1.f : 0.f;
            const float coef = (target > env) ? att : rel;
            env = target + coef * (env - target);
            buf.samples[size_t(i) * bch] = l * env;
            if (bch > 1)
                buf.samples[size_t(i) * bch + 1] = r * env;
        }
    }

    if (ch.compEnabled) {
        float& env = m_compEnv[channelId];
        const float thrLin = std::pow(10.f, ch.compThresholdDb / 20.f);
        const float ratio = std::max(1.f, ch.compRatio);
        const float att = std::exp(-1.f / std::max(1.f, ch.compAttackMs * 0.001f * sr));
        const float rel = std::exp(-1.f / std::max(1.f, ch.compReleaseMs * 0.001f * sr));
        const float makeup = std::pow(10.f, ch.compMakeupDb / 20.f);
        for (int i = 0; i < n; ++i) {
            float l = buf.samples[size_t(i) * bch];
            float r = bch > 1 ? buf.samples[size_t(i) * bch + 1] : l;
            const float level = std::max(std::abs(l), std::abs(r));
            const float coef = (level > env) ? att : rel;
            env = level + coef * (env - level);
            float gain = 1.f;
            if (env > thrLin && thrLin > 1e-6f) {
                const float over = env / thrLin;
                const float compressed = std::pow(over, 1.f / ratio);
                gain = (compressed / over);
            }
            gain *= makeup;
            buf.samples[size_t(i) * bch] = l * gain;
            if (bch > 1)
                buf.samples[size_t(i) * bch + 1] = r * gain;
        }
    }

    if (ch.expEnabled) {
        float& env = m_expEnv[channelId];
        const float thrLin = std::pow(10.f, ch.expThresholdDb / 20.f);
        const float ratio = std::max(1.f, ch.expRatio);
        const float att = std::exp(-1.f / std::max(1.f, ch.expAttackMs * 0.001f * sr));
        const float rel = std::exp(-1.f / std::max(1.f, ch.expReleaseMs * 0.001f * sr));
        for (int i = 0; i < n; ++i) {
            float l = buf.samples[size_t(i) * bch];
            float r = bch > 1 ? buf.samples[size_t(i) * bch + 1] : l;
            const float level = std::max(std::abs(l), std::abs(r));
            const float coef = (level > env) ? att : rel;
            env = level + coef * (env - level);
            float gain = 1.f;
            if (env < thrLin && thrLin > 1e-6f) {
                const float under = std::max(env / thrLin, 1e-4f);
                gain = std::pow(under, ratio - 1.f);
            }
            buf.samples[size_t(i) * bch] = l * gain;
            if (bch > 1)
                buf.samples[size_t(i) * bch + 1] = r * gain;
        }
    }

    // OBS basic_eq_filter: 3-band shelving via cascaded one-poles (800 / 5000 Hz).
    const bool eqActive = std::abs(ch.eqLowDb) > 0.01f || std::abs(ch.eqMidDb) > 0.01f
                          || std::abs(ch.eqHighDb) > 0.01f;
    if (eqActive) {
        constexpr float kEqEps = 1.f / 4294967295.f;
        constexpr float kLowFreq = 800.f;
        constexpr float kHighFreq = 5000.f;
        const float lf = 2.f * std::sin(3.14159265f * kLowFreq / sr);
        const float hf = 2.f * std::sin(3.14159265f * kHighFreq / sr);
        const float lowG = std::pow(10.f, ch.eqLowDb / 20.f);
        const float midG = std::pow(10.f, ch.eqMidDb / 20.f);
        const float highG = std::pow(10.f, ch.eqHighDb / 20.f);

        auto processBand = [&](EqBandState& c, float sample) -> float {
            c.lfDelay[0] += lf * (sample - c.lfDelay[0]) + kEqEps;
            c.lfDelay[1] += lf * (c.lfDelay[0] - c.lfDelay[1]);
            c.lfDelay[2] += lf * (c.lfDelay[1] - c.lfDelay[2]);
            c.lfDelay[3] += lf * (c.lfDelay[2] - c.lfDelay[3]);
            const float lBand = c.lfDelay[3];

            c.hfDelay[0] += hf * (sample - c.hfDelay[0]) + kEqEps;
            c.hfDelay[1] += hf * (c.hfDelay[0] - c.hfDelay[1]);
            c.hfDelay[2] += hf * (c.hfDelay[1] - c.hfDelay[2]);
            c.hfDelay[3] += hf * (c.hfDelay[2] - c.hfDelay[3]);

            const float hBand = c.sampleDelay[2] - c.hfDelay[3];
            const float mBand = c.sampleDelay[2] - (hBand + lBand);

            c.sampleDelay[2] = c.sampleDelay[1];
            c.sampleDelay[1] = c.sampleDelay[0];
            c.sampleDelay[0] = sample;

            return lBand * lowG + mBand * midG + hBand * highG;
        };

        EqBandState& left = m_eqL[channelId];
        EqBandState& right = m_eqR[channelId];
        for (int i = 0; i < n; ++i) {
            float l = buf.samples[size_t(i) * bch];
            float r = bch > 1 ? buf.samples[size_t(i) * bch + 1] : l;
            l = processBand(left, l);
            r = processBand(right, r);
            buf.samples[size_t(i) * bch] = l;
            if (bch > 1)
                buf.samples[size_t(i) * bch + 1] = r;
        }
    }

    if (ch.limiterEnabled) {
        float& env = m_limEnv[channelId];
        const float thrDb = ch.limiterThresholdDb;
        // OBS limiter: ~1 ms attack, user release; slope = 1 (brick-wall above threshold).
        const float att = std::exp(-1.f / std::max(1.f, 0.001f * sr));
        const float rel = std::exp(-1.f / std::max(1.f, ch.limiterReleaseMs * 0.001f * sr));
        for (int i = 0; i < n; ++i) {
            float l = buf.samples[size_t(i) * bch];
            float r = bch > 1 ? buf.samples[size_t(i) * bch + 1] : l;
            const float level = std::max(std::abs(l), std::abs(r));
            const float coef = (level > env) ? att : rel;
            env = level + coef * (env - level);
            float envDb = -96.f;
            if (env > 1e-6f)
                envDb = 20.f * std::log10(env);
            float gainDb = thrDb - envDb; // slope=1
            if (gainDb > 0.f)
                gainDb = 0.f;
            const float gain = std::pow(10.f, gainDb / 20.f);
            buf.samples[size_t(i) * bch] = l * gain;
            if (bch > 1)
                buf.samples[size_t(i) * bch + 1] = r * gain;
        }
    }

    if (ch.echoEnabled) {
        auto& line = m_echoLines[channelId];
        const int delayMs = std::clamp(int(std::lround(ch.echoDelayMs)), 1, 1000);
        const int delayFrames = std::max(1, int(std::lround(float(delayMs) * 0.001f * sr)));
        const int delaySamples = delayFrames * 2;
        const float decay = std::clamp(ch.echoDecay, 0.f, 0.95f);
        const float wet = std::clamp(ch.echoWet, 0.f, 1.f);
        const float dry = 1.f - wet;
        for (int i = 0; i < n; ++i) {
            float l = buf.samples[size_t(i) * bch];
            float r = bch > 1 ? buf.samples[size_t(i) * bch + 1] : l;
            float dL = 0.f, dR = 0.f;
            if (int(line.size()) >= delaySamples) {
                const int idx = int(line.size()) - delaySamples;
                dL = line[size_t(idx)];
                dR = line[size_t(idx + 1)];
            }
            line.push_back(l + dL * decay);
            line.push_back(r + dR * decay);
            const int maxKeep = delaySamples + 64;
            while (int(line.size()) > maxKeep) {
                line.pop_front();
                line.pop_front();
            }
            buf.samples[size_t(i) * bch] = l * dry + dL * wet;
            if (bch > 1)
                buf.samples[size_t(i) * bch + 1] = r * dry + dR * wet;
            else
                Q_UNUSED(dR);
        }
    }
}

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

    AudioBuffer monitorBuf;
    monitorBuf.channels = kAudioChannels;
    monitorBuf.sampleRate = kAudioSampleRate;

    std::function<void(const AudioBuffer&)> cb;
    {
        std::lock_guard lock(m_mutex);
        int frames = kAudioFramesPerBuffer;
        for (const auto& b : m_pending)
            frames = std::max(frames, b.frameCount());
        mixed.samples.assign(static_cast<size_t>(frames) * kAudioChannels, 0.f);
        monitorBuf.samples.assign(static_cast<size_t>(frames) * kAudioChannels, 0.f);
        monitorBuf.ptsUs = mixed.ptsUs;

        bool anySolo = false;
        for (const auto& ch : m_channels)
            if (ch.solo) { anySolo = true; break; }

        for (auto it = m_pending.begin(); it != m_pending.end(); ++it) {
            const auto ch = m_channels.value(it.key());
            if (ch.muted) continue;
            if (anySolo && !ch.solo) continue;

            AudioBuffer buf = applySyncDelay(it.key(), it.value(), ch.syncOffsetMs);
            applyChannelDsp(it.key(), ch, buf);

            const float linear = ch.volume * std::pow(10.f, ch.gainDb / 20.f);
            const float panL = std::cos((ch.pan + 1.f) * 0.25f * 3.14159265f);
            const float panR = std::sin((ch.pan + 1.f) * 0.25f * 3.14159265f);
            const int n = std::min(frames, buf.frameCount());
            const int bch = std::max(1, buf.channels);

            const bool toOutput = (ch.monitoring != AudioMonitoringType::MonitorOnly)
                                  && (ch.trackMask & kTrack1Bit);
            const bool toMonitor = (ch.monitoring != AudioMonitoringType::None);

            for (int i = 0; i < n; ++i) {
                float l = buf.samples[size_t(i) * bch];
                float r = bch > 1 ? buf.samples[size_t(i) * bch + 1] : l;
                if (ch.forceMono) {
                    const float m = 0.5f * (l + r);
                    l = m;
                    r = m;
                }
                l *= linear * panL;
                r *= linear * panR;
                if (toOutput) {
                    mixed.samples[size_t(i) * 2] += l;
                    mixed.samples[size_t(i) * 2 + 1] += r;
                }
                if (toMonitor) {
                    monitorBuf.samples[size_t(i) * 2] += l;
                    monitorBuf.samples[size_t(i) * 2 + 1] += r;
                }
            }
        }

        const float master = m_masterMuted ? 0.f : m_masterVolume;
        for (float& s : mixed.samples)
            s = std::clamp(s * master, -1.f, 1.f);
        for (float& s : monitorBuf.samples)
            s = std::clamp(s * master, -1.f, 1.f);

        m_masterMeter.process(mixed.samples.data(), frames, 2);
        m_masterMeter.decay();
        cb = m_outputCb;
        m_pending.clear();
    }

    if (m_monitor && m_monitorEnabled)
        m_monitor->push(monitorBuf);
    if (cb)
        cb(mixed);
    emit metersUpdated();
}

} // namespace railshot
