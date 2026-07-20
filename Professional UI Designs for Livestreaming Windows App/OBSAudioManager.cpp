#include "OBSAudioManager.h"
#include "OBSCore.h"

#include <obs.h>
#include <obs-data.h>
#include <media-io/audio-io.h>

#include <QDebug>
#include <QMetaObject>

// ── Singleton ─────────────────────────────────────────────────────────────────
OBSAudioManager &OBSAudioManager::instance()
{
    static OBSAudioManager s_instance;
    return s_instance;
}

OBSAudioManager::OBSAudioManager(QObject *parent)
    : QObject(parent)
{}

OBSAudioManager::~OBSAudioManager()
{
    // Release all volmeters and sources
    for (auto &state : m_states) {
        if (state.volmeter) {
            obs_volmeter_detach_source(state.volmeter);
            obs_volmeter_destroy(state.volmeter);
        }
        if (state.source) {
            obs_source_release(state.source);
        }
    }
    m_states.clear();
}

// ── Device enumeration ────────────────────────────────────────────────────────
QList<AudioDevice> OBSAudioManager::outputDevices() const
{
    QList<AudioDevice> result;

    // Create a temporary source to enumerate its device property list
    obs_source_t *tmpSrc = obs_source_create("wasapi_output_capture",
                                              "__railshot_enum_out__", nullptr, nullptr);
    if (!tmpSrc) return result;

    obs_properties_t *props = obs_source_properties(tmpSrc);
    obs_property_t   *prop  = obs_properties_get(props, "device_id");

    if (prop) {
        // Add the "default" device first
        AudioDevice def;
        def.id        = "default";
        def.name      = "Default Desktop Audio";
        def.isDefault = true;
        result.append(def);

        size_t count = obs_property_list_item_count(prop);
        for (size_t i = 0; i < count; ++i) {
            const char *name = obs_property_list_item_name(prop, i);
            const char *val  = obs_property_list_item_string(prop, i);
            if (!name || !val) continue;

            AudioDevice dev;
            dev.id        = QString::fromUtf8(val);
            dev.name      = QString::fromUtf8(name);
            dev.isDefault = false;
            result.append(dev);
        }
    }

    obs_properties_destroy(props);
    obs_source_release(tmpSrc);
    return result;
}

QList<AudioDevice> OBSAudioManager::inputDevices() const
{
    QList<AudioDevice> result;

    obs_source_t *tmpSrc = obs_source_create("wasapi_input_capture",
                                              "__railshot_enum_in__", nullptr, nullptr);
    if (!tmpSrc) return result;

    obs_properties_t *props = obs_source_properties(tmpSrc);
    obs_property_t   *prop  = obs_properties_get(props, "device_id");

    if (prop) {
        AudioDevice def;
        def.id        = "default";
        def.name      = "Default Microphone";
        def.isDefault = true;
        result.append(def);

        size_t count = obs_property_list_item_count(prop);
        for (size_t i = 0; i < count; ++i) {
            const char *name = obs_property_list_item_name(prop, i);
            const char *val  = obs_property_list_item_string(prop, i);
            if (!name || !val) continue;

            AudioDevice dev;
            dev.id        = QString::fromUtf8(val);
            dev.name      = QString::fromUtf8(name);
            dev.isDefault = false;
            result.append(dev);
        }
    }

    obs_properties_destroy(props);
    obs_source_release(tmpSrc);
    return result;
}

// ── Channel management ────────────────────────────────────────────────────────
void OBSAudioManager::initDefaultChannels()
{
    // Channels are added by the user via Settings → Audio or detected from OBS.
    // Do not seed any default channels here — the mixer starts empty.
    qInfo() << "[OBSAudioManager] Audio manager ready (no default channels seeded)";
}

bool OBSAudioManager::addChannel(const QString &name, const QString &typeId,
                                   const QString &deviceId)
{
    int idx = m_states.size();
    if (idx >= 6) {
        qWarning() << "[OBSAudioManager] Maximum 6 audio channels supported";
        return false;
    }

    // Create the WASAPI source
    obs_data_t *settings = obs_data_create();
    obs_data_set_string(settings, "device_id", deviceId.toUtf8().constData());
    obs_data_set_bool(settings, "use_device_timing", false);

    QString sourceName = QString("RailShotTV_Audio_%1").arg(idx);
    obs_source_t *source = obs_source_create(
        typeId.toUtf8().constData(),
        sourceName.toUtf8().constData(),
        settings, nullptr);
    obs_data_release(settings);

    if (!source) {
        qWarning() << "[OBSAudioManager] Failed to create audio source:" << typeId;
        return false;
    }

    // Assign to a global audio channel (channels 1–6 in libobs)
    obs_set_output_source(static_cast<uint32_t>(idx + 1), source);

    // Create a volume meter for this source
    obs_volmeter_t *volmeter = obs_volmeter_create(OBS_FADER_LOG);
    obs_volmeter_attach_source(volmeter, source);
    obs_volmeter_add_callback(volmeter, OBSAudioManager::volmeterCallback, this);

    // Store state
    ChannelState state;
    state.config.index        = idx;
    state.config.name         = name;
    state.config.sourceTypeId = typeId;
    state.config.deviceId     = deviceId;
    state.config.volume       = 1.0f;
    state.config.muted        = false;
    state.config.active       = true;
    state.source   = source;
    state.volmeter = volmeter;

    m_states.append(state);
    m_channels.append(state.config);

    emit channelsChanged();
    return true;
}

void OBSAudioManager::removeChannel(int index)
{
    if (index < 0 || index >= m_states.size()) return;

    ChannelState &state = m_states[index];
    if (state.volmeter) {
        obs_volmeter_detach_source(state.volmeter);
        obs_volmeter_destroy(state.volmeter);
        state.volmeter = nullptr;
    }
    if (state.source) {
        obs_set_output_source(static_cast<uint32_t>(index + 1), nullptr);
        obs_source_release(state.source);
        state.source = nullptr;
    }
    m_states.removeAt(index);
    m_channels.removeAt(index);
    emit channelsChanged();
}

// ── Per-channel controls ──────────────────────────────────────────────────────
void OBSAudioManager::setVolume(int channelIndex, float vol)
{
    if (channelIndex < 0 || channelIndex >= m_states.size()) return;
    vol = qBound(0.0f, vol, 1.0f);
    m_states[channelIndex].config.volume = vol;
    m_channels[channelIndex].volume      = vol;
    if (m_states[channelIndex].source) {
        obs_source_set_volume(m_states[channelIndex].source, vol);
    }
}

void OBSAudioManager::setMuted(int channelIndex, bool muted)
{
    if (channelIndex < 0 || channelIndex >= m_states.size()) return;
    m_states[channelIndex].config.muted = muted;
    m_channels[channelIndex].muted      = muted;
    if (m_states[channelIndex].source) {
        obs_source_set_muted(m_states[channelIndex].source, muted);
    }
}

float OBSAudioManager::volume(int channelIndex) const
{
    if (channelIndex < 0 || channelIndex >= m_channels.size()) return 0.0f;
    return m_channels.at(channelIndex).volume;
}

bool OBSAudioManager::isMuted(int channelIndex) const
{
    if (channelIndex < 0 || channelIndex >= m_channels.size()) return false;
    return m_channels.at(channelIndex).muted;
}

void OBSAudioManager::setMonitoringMode(int channelIndex, int mode)
{
    if (channelIndex < 0 || channelIndex >= m_states.size()) return;
    if (!m_states[channelIndex].source) return;

    obs_monitoring_type obsMode;
    switch (mode) {
        case 1:  obsMode = OBS_MONITORING_TYPE_MONITOR_ONLY;   break;
        case 2:  obsMode = OBS_MONITORING_TYPE_MONITOR_AND_OUTPUT; break;
        default: obsMode = OBS_MONITORING_TYPE_NONE;           break;
    }
    obs_source_set_monitoring_type(m_states[channelIndex].source, obsMode);
}

// ── Volmeter callback ─────────────────────────────────────────────────────────
// Called from the libobs audio thread — must marshal to Qt main thread.
void OBSAudioManager::volmeterCallback(void *param,
                                        const float magnitude[MAX_AUDIO_CHANNELS],
                                        const float peak[MAX_AUDIO_CHANNELS],
                                        const float inputPeak[MAX_AUDIO_CHANNELS])
{
    auto *self = static_cast<OBSAudioManager *>(param);

    // Find which channel this volmeter belongs to
    // (libobs passes the same param for all channels of the same source)
    // We use the first audio channel (stereo L) as the representative level.
    AudioLevel level;
    level.channel   = -1;  // resolved below
    level.magnitude = magnitude[0];
    level.peak      = peak[0];
    level.inputPeak = inputPeak[0];
    level.muted     = false;

    // Find the channel index by matching the volmeter pointer
    // (We pass `this` as param, so we need to iterate to find the right channel)
    // This is called per-source, so we emit for all channels and let the UI filter.
    QMetaObject::invokeMethod(self, [self, level]() mutable {
        for (int i = 0; i < self->m_states.size(); ++i) {
            // Emit for each channel — the VUMeter widget filters by channel index
            AudioLevel l = level;
            l.channel = i;
            l.muted   = self->m_channels.value(i).muted;
            emit self->levelUpdated(l);
        }
    }, Qt::QueuedConnection);
}
