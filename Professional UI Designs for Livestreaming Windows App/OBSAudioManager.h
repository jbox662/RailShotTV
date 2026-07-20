#pragma once
/**
 * OBSAudioManager — WASAPI audio device enumeration and VU meter feed
 *
 * Responsibilities:
 *  - Enumerate WASAPI input (microphone) and output (desktop audio) devices
 *  - Create and manage audio capture sources for the mixer
 *  - Provide per-channel volume levels for the VU meter widgets
 *  - Handle mute/solo/volume changes from the UI
 *
 * Audio channel layout (mirrors OBS defaults):
 *   Channel 0 — Desktop Audio (wasapi_output_capture)
 *   Channel 1 — Microphone / Aux (wasapi_input_capture)
 *   Channels 2–5 — Additional audio sources (configurable)
 *
 * VU meter data:
 *   libobs fires obs_volmeter_t callbacks at ~30fps with peak/magnitude/input_peak.
 *   OBSAudioManager marshals these to the Qt main thread and emits levelUpdated().
 */

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>

struct obs_source;
struct obs_volmeter;

// ── Audio device descriptor ───────────────────────────────────────────────────
struct AudioDevice {
    QString id;           // WASAPI device ID (e.g. "{0.0.0.00000000}.{...}")
    QString name;         // Human-readable device name
    bool    isDefault;    // true = system default device
};

// ── Per-channel audio level snapshot ─────────────────────────────────────────
struct AudioLevel {
    int     channel;      // 0-based channel index
    float   magnitude;    // RMS level in dB  (DYNAMIC: obs_volmeter callback)
    float   peak;         // Peak level in dB  (DYNAMIC: obs_volmeter callback)
    float   inputPeak;    // Input peak in dB  (DYNAMIC: obs_volmeter callback)
    bool    muted;        // current mute state
};

// ── Audio mixer channel configuration ────────────────────────────────────────
struct AudioChannel {
    int     index;        // 0-based
    QString name;         // display name (e.g. "Desktop Audio", "Mic/Aux")
    QString sourceTypeId; // "wasapi_output_capture" or "wasapi_input_capture"
    QString deviceId;     // WASAPI device ID ("default" = system default)
    float   volume;       // 0.0 – 1.0
    bool    muted;
    bool    active;       // source created and running
};

class OBSAudioManager : public QObject
{
    Q_OBJECT

public:
    static OBSAudioManager &instance();

    // ── Device enumeration ────────────────────────────────────────────────

    /**
     * Enumerate all available WASAPI output (desktop audio) devices.
     * DYNAMIC: populated by creating a temporary wasapi_output_capture source
     * and reading its "device_id" property list.
     */
    QList<AudioDevice> outputDevices() const;

    /**
     * Enumerate all available WASAPI input (microphone) devices.
     * DYNAMIC: populated from wasapi_input_capture property list.
     */
    QList<AudioDevice> inputDevices() const;

    // ── Mixer channel management ──────────────────────────────────────────

    /**
     * Initialize the default audio channels (Desktop Audio + Mic/Aux).
     * Called once after OBSCore::init().
     */
    void initDefaultChannels();

    /**
     * Add an audio channel with the given device.
     * @param name       display name
     * @param typeId     "wasapi_output_capture" or "wasapi_input_capture"
     * @param deviceId   WASAPI device ID, or "default"
     */
    bool addChannel(const QString &name, const QString &typeId,
                    const QString &deviceId = "default");

    /**
     * Remove an audio channel by index.
     */
    void removeChannel(int index);

    /**
     * Return all configured audio channels.
     */
    QList<AudioChannel> channels() const { return m_channels; }

    // ── Per-channel controls ──────────────────────────────────────────────

    /** Set volume for a channel. @param vol 0.0 – 1.0 */
    void setVolume(int channelIndex, float vol);

    /** Toggle mute for a channel. */
    void setMuted(int channelIndex, bool muted);

    /** Return current volume for a channel. */
    float volume(int channelIndex) const;

    /** Return current mute state for a channel. */
    bool isMuted(int channelIndex) const;

    // ── Monitoring ────────────────────────────────────────────────────────

    /**
     * Enable audio monitoring for a channel (plays audio through headphones).
     * @param mode  0=off, 1=monitor only, 2=monitor+output
     */
    void setMonitoringMode(int channelIndex, int mode);

signals:
    /**
     * Emitted ~30fps per channel with current audio levels.
     * Connect to VUMeter::setLevel() in the Dashboard audio mixer.
     * DYNAMIC: driven by obs_volmeter_t callbacks.
     */
    void levelUpdated(const AudioLevel &level);

    /** Emitted when the channel list changes. */
    void channelsChanged();

private:
    explicit OBSAudioManager(QObject *parent = nullptr);
    ~OBSAudioManager() override;
    OBSAudioManager(const OBSAudioManager &) = delete;
    OBSAudioManager &operator=(const OBSAudioManager &) = delete;

    struct ChannelState {
        AudioChannel    config;
        obs_source     *source    = nullptr;
        obs_volmeter   *volmeter  = nullptr;
    };

    static void volmeterCallback(void *param,
                                  const float magnitude[MAX_AUDIO_CHANNELS],
                                  const float peak[MAX_AUDIO_CHANNELS],
                                  const float inputPeak[MAX_AUDIO_CHANNELS]);

    QList<ChannelState> m_states;
    QList<AudioChannel> m_channels;
};
