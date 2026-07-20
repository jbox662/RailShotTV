#pragma once

#include "audio/AudioTypes.h"
#include "audio/AudioMeter.h"
#include "audio/AudioClock.h"
#include "core/Types.h"
#include <QObject>
#include <QHash>
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
    void removeChannel(const QString& id);
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

    /// Mixed output callback for encoders (float stereo @ 48k).
    void setOutputCallback(std::function<void(const AudioBuffer&)> cb);

    AudioMeter& masterMeter() { return m_masterMeter; }

signals:
    void metersUpdated();
    void audioError(const QString& message);

private:
    void onCapture(const QString& channelId, const AudioBuffer& buffer);
    void mixAndEmit();

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
    AudioMeter m_masterMeter;
    float m_masterVolume = 1.0f;
    bool m_masterMuted = false;
    bool m_monitorEnabled = true;
    std::function<void(const AudioBuffer&)> m_outputCb;
};

} // namespace railshot
