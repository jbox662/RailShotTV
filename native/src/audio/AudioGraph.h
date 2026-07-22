#pragma once

#include "audio/AudioTypes.h"
#include "audio/AudioMeter.h"
#include "audio/AudioClock.h"
#include "core/Types.h"
#include <QObject>
#include <QHash>
#include <QJsonArray>
#include <deque>
#include <memory>
#include <mutex>
#include <functional>

namespace railshot {

class WasapiCapture;
class AudioMonitor;

class AudioGraph : public QObject {
    Q_OBJECT
public:
    explicit AudioGraph(QObject* parent = nullptr);
    ~AudioGraph() override;

    bool initialize(QString* error = nullptr);
    bool initialize(const QString& desktopDeviceId, const QString& micDeviceId, QString* error = nullptr);
    bool reconfigureDevices(const QString& desktopDeviceId, const QString& micDeviceId, QString* error = nullptr);
    void shutdown();

    AudioClock& clock() { return m_clock; }

    void setChannelState(const QString& id, const AudioChannelState& state);
    AudioChannelState channelState(const QString& id) const;
    QVector<AudioChannelState> channels() const;

    void ensureChannel(const QString& id, const QString& name);
    /// Returns true if the channel was newly created.
    bool ensureChannelEx(const QString& id, const QString& name);
    void removeChannel(const QString& id);
    /// OBS-style: keep only these dynamic source strips (+ desktop/mic). Emits once.
    void syncDynamicChannels(const QHash<QString, QString>& keepIdToName);
    /// Inject a decoded media/NDI (or other) buffer into the mix.
    void inject(const QString& channelId, const AudioBuffer& buffer);

    void setMasterVolume(float v);
    void setMasterMuted(bool m);
    float masterVolume() const { return m_masterVolume; }
    bool masterMuted() const { return m_masterMuted; }

    void setMonitorEnabled(bool enabled);
    bool monitorEnabled() const { return m_monitorEnabled; }

    QString desktopDeviceId() const { return m_desktopDeviceId; }
    QString micDeviceId() const { return m_micDeviceId; }

    /// Persist / restore mixer strip settings (volume, pan, mono, sync, tracks…).
    QJsonArray channelsToJson() const;
    void applyChannelsFromJson(const QJsonArray& arr);

    /// Mixed output callback for encoders (float stereo @ 48k).
    void setOutputCallback(std::function<void(const AudioBuffer&)> cb);

    AudioMeter& masterMeter() { return m_masterMeter; }

signals:
    void metersUpdated();
    void audioError(const QString& message);
    void channelsChanged();

private:
    void onCapture(const QString& channelId, const AudioBuffer& buffer);
    void mixAndEmit();
    AudioBuffer applySyncDelay(const QString& channelId, const AudioBuffer& in, int syncOffsetMs);
    /// Noise Suppress → Gate → Comp → 3-band EQ → Limiter (OBS-style Adv Audio DSP chain).
    void applyChannelDsp(const QString& channelId, const AudioChannelState& ch, AudioBuffer& buf);

    struct EqBandState {
        float lfDelay[4] = {};
        float hfDelay[4] = {};
        float sampleDelay[3] = {};
    };

    AudioClock m_clock;
    std::unique_ptr<WasapiCapture> m_desktop;
    std::unique_ptr<WasapiCapture> m_mic;
    std::unique_ptr<AudioMonitor> m_monitor;
    QString m_desktopDeviceId;
    QString m_micDeviceId;

    mutable std::mutex m_mutex;
    QHash<QString, AudioChannelState> m_channels;
    QHash<QString, AudioBuffer> m_pending;
    QHash<QString, AudioMeter> m_meters;
    QHash<QString, std::deque<float>> m_delayLines; // interleaved stereo delay ring
    QHash<QString, std::deque<float>> m_echoLines;  // wet echo FX ring (stereo)
    QHash<QString, float> m_nsEnv;    // noise suppress envelope (linear)
    QHash<QString, float> m_nsHpZL;   // HPF z^-1 left
    QHash<QString, float> m_nsHpZR;   // HPF z^-1 right
    QHash<QString, float> m_gateEnv;  // 0..1 open amount
    QHash<QString, float> m_gateHold; // samples remaining
    QHash<QString, float> m_compEnv;  // envelope follower linear
    QHash<QString, float> m_expEnv;   // expander envelope
    QHash<QString, EqBandState> m_eqL; // per-channel left EQ state
    QHash<QString, EqBandState> m_eqR; // per-channel right EQ state
    QHash<QString, float> m_limEnv;   // limiter envelope (linear)
    AudioMeter m_masterMeter;
    float m_masterVolume = 1.0f;
    bool m_masterMuted = false;
    bool m_monitorEnabled = true;
    std::function<void(const AudioBuffer&)> m_outputCb;
};

} // namespace railshot
