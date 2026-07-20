#pragma once
/**
 * OBSOutputManager — Streaming and recording output controller for RailShotTV
 *
 * Manages the full output pipeline:
 *   Settings UI → OBSOutputManager → obs_output_t / obs_encoder_t / obs_service_t
 *
 * Streaming pipeline:
 *   obs_service_t (rtmp_custom / rtmp_common)
 *     └── obs_output_t (rtmp_output)
 *           ├── obs_encoder_t video (jim_nvenc / obs_qsv11 / amd_amf_h264 / obs_x264)
 *           └── obs_encoder_t audio (ffmpeg_aac)
 *
 * Recording pipeline:
 *   obs_output_t (ffmpeg_muxer / flv_output)
 *     ├── obs_encoder_t video (same as streaming, or separate)
 *     └── obs_encoder_t audio (ffmpeg_aac)
 *
 * All methods are called from the main Qt thread.
 * libobs callbacks are marshalled back to the Qt thread via QMetaObject::invokeMethod.
 */

#include <QObject>
#include <QString>

struct obs_output;
struct obs_encoder;
struct obs_service;

// ── Stream configuration ──────────────────────────────────────────────────────
struct StreamConfig {
    // Service
    QString server;          // RTMP URL, e.g. "rtmp://live.twitch.tv/app"
    QString streamKey;       // Stream key (never logged)
    bool    useAuth = false;
    QString username;
    QString password;

    // Video encoder
    QString encoderId       = "jim_nvenc";   // libobs encoder ID
    uint32_t videoBitrate   = 8000;          // kbps
    QString rateControl     = "CBR";
    uint32_t keyframeInterval = 2;           // seconds
    QString preset          = "Quality";
    QString profile         = "high";

    // Audio encoder
    uint32_t audioBitrate   = 320;           // kbps
};

// ── Recording configuration ───────────────────────────────────────────────────
struct RecordingConfig {
    QString outputPath;          // directory to save recordings
    QString format = "mkv";      // "mkv", "mp4", "mov", "flv", "ts"
    bool    separateEncoder = false;  // use a separate encoder from stream
    uint32_t videoBitrate   = 0;      // 0 = same as stream encoder
    bool    replayBuffer    = false;  // enable replay buffer
    uint32_t replaySeconds  = 30;     // replay buffer duration
};

class OBSOutputManager : public QObject
{
    Q_OBJECT

public:
    static OBSOutputManager &instance();

    // ── Stream output ─────────────────────────────────────────────────────

    /**
     * Apply stream configuration.
     * Can be called before or after startStream() — changes take effect
     * on the next startStream() call.
     */
    void applyStreamConfig(const StreamConfig &config);
    const StreamConfig &streamConfig() const { return m_streamConfig; }

    /**
     * Start the RTMP stream.
     * Requires: applyStreamConfig() called with valid server + key.
     * @return true if the output started (async — streamStarted signal fires on success)
     */
    bool startStream();

    /**
     * Stop the RTMP stream.
     */
    void stopStream();

    bool isStreaming() const;

    // ── Recording output ──────────────────────────────────────────────────

    void applyRecordingConfig(const RecordingConfig &config);
    const RecordingConfig &recordingConfig() const { return m_recConfig; }

    bool startRecording();
    void stopRecording();
    bool isRecording() const;

    // ── Replay buffer ─────────────────────────────────────────────────────

    bool startReplayBuffer();
    void stopReplayBuffer();
    void saveReplay();
    bool isReplayBufferActive() const;

    // ── Encoder availability ──────────────────────────────────────────────

    /**
     * Return the best available hardware encoder ID on this system.
     * Checks NVENC → QSV → AMF → falls back to obs_x264.
     * DYNAMIC: depends on GPU and loaded plugins.
     */
    static QString bestAvailableEncoder();

    /**
     * Return true if the given encoder ID is available on this system.
     * DYNAMIC: obs_get_encoder_codec() returns null for unavailable encoders.
     */
    static bool isEncoderAvailable(const QString &encoderId);

signals:
    void streamStarted();
    void streamStopped();
    void streamError(const QString &message);

    void recordingStarted();
    void recordingStopped();
    void recordingError(const QString &message);

    void replayBufferStarted();
    void replayBufferStopped();
    void replaySaved(const QString &filePath);

private:
    explicit OBSOutputManager(QObject *parent = nullptr);
    ~OBSOutputManager() override;
    OBSOutputManager(const OBSOutputManager &) = delete;
    OBSOutputManager &operator=(const OBSOutputManager &) = delete;

    void buildStreamPipeline();
    void buildRecordingPipeline();
    void teardownStreamPipeline();
    void teardownRecordingPipeline();

    // libobs output signal callbacks (called from obs render thread)
    static void onStreamStart(void *data, calldata_t *cd);
    static void onStreamStop(void *data, calldata_t *cd);
    static void onStreamError(void *data, calldata_t *cd);
    static void onRecordStart(void *data, calldata_t *cd);
    static void onRecordStop(void *data, calldata_t *cd);

    StreamConfig    m_streamConfig;
    RecordingConfig m_recConfig;

    obs_output  *m_streamOutput  = nullptr;
    obs_output  *m_recOutput     = nullptr;
    obs_output  *m_replayOutput  = nullptr;
    obs_encoder *m_videoEncoder  = nullptr;
    obs_encoder *m_audioEncoder  = nullptr;
    obs_encoder *m_recVideoEnc   = nullptr;  // separate recording encoder (optional)
    obs_encoder *m_recAudioEnc   = nullptr;
    obs_service *m_service       = nullptr;
};
