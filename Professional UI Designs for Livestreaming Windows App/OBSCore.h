#pragma once
/**
 * OBSCore — libobs lifecycle singleton for RailShotTV
 *
 * Responsibilities:
 *  - obs_startup() / obs_shutdown()
 *  - Video (obs_video_info) and audio (obs_audio_info) format init
 *  - Plugin directory loading (obs_load_all_modules)
 *  - Providing the active scene collection
 *  - Emitting Qt signals for stats updates (bitrate, CPU, dropped frames)
 *
 * Usage:
 *   OBSCore::instance().init(videoInfo, audioInfo);
 *   OBSCore::instance().shutdown();
 */

#include <QObject>
#include <QTimer>
#include <QString>
#include <QStringList>

// Forward-declare libobs types so this header compiles without obs headers
// in translation units that only need the Qt signals.
struct obs_video_info;
struct obs_audio_info;
struct obs_source;
struct obs_scene;
struct obs_output;
struct obs_encoder;
struct obs_service;

// ── Stats snapshot emitted every second ──────────────────────────────────────
struct StreamStats {
    double  cpuUsage        = 0.0;   // percent 0–100  (DYNAMIC: obs_get_cpu_usage)
    double  memUsageMB      = 0.0;   // megabytes       (DYNAMIC: obs_get_memory_usage)
    uint64_t outputBytes    = 0;     // total bytes sent (DYNAMIC: obs_output_get_total_bytes)
    uint32_t droppedFrames  = 0;     // (DYNAMIC: obs_output_get_frames_dropped)
    uint32_t totalFrames    = 0;     // (DYNAMIC: obs_output_get_total_frames)
    double  kbps            = 0.0;   // computed from outputBytes delta
    bool    streaming       = false; // (DYNAMIC: obs_output_active)
    bool    recording       = false; // (DYNAMIC: obs_output_active)
    uint64_t streamDuration = 0;     // seconds since stream start
};

class OBSCore : public QObject
{
    Q_OBJECT

public:
    static OBSCore &instance();

    // ── Lifecycle ─────────────────────────────────────────────────────────
    /**
     * Initialize libobs.
     * Must be called once before any other obs_* calls.
     * @param locale  e.g. "en-US"
     * @param pluginDir  path to the folder containing obs plugins (.dll)
     *                   Defaults to the executable's directory.
     * @return true on success
     */
    bool init(const QString &locale = "en-US",
              const QString &pluginDir = QString());

    /**
     * Shut down libobs cleanly.
     * Stops all outputs, releases all sources, calls obs_shutdown().
     */
    void shutdown();

    bool isInitialized() const { return m_initialized; }

    // ── Video / Audio format ──────────────────────────────────────────────
    /**
     * Reset the video output format.
     * Call after the user changes resolution or FPS in Settings.
     * @param width   output width  (e.g. 1920)
     * @param height  output height (e.g. 1080)
     * @param fpsNum  FPS numerator   (e.g. 60)
     * @param fpsDen  FPS denominator (e.g. 1)
     */
    bool resetVideo(uint32_t width  = 1920,
                    uint32_t height = 1080,
                    uint32_t fpsNum = 60,
                    uint32_t fpsDen = 1);

    /**
     * Reset the audio output format.
     * @param sampleRate  e.g. 48000
     * @param speakers    channel layout (e.g. SPEAKERS_STEREO)
     */
    bool resetAudio(uint32_t sampleRate = 48000);

    // ── Scene collection ──────────────────────────────────────────────────
    /**
     * Create a new named scene and add it to the collection.
     * @return the new obs_scene_t* (caller does NOT own — OBSCore manages lifetime)
     */
    obs_scene *createScene(const QString &name);

    /**
     * Switch the program output to the given scene.
     * Triggers a transition if one is active.
     */
    void setActiveScene(obs_scene *scene);
    obs_scene *activeScene() const { return m_activeScene; }

    /**
     * Return the list of all scene names in order.
     * DYNAMIC: populated from obs_frontend_get_scenes() in Phase 3.
     */
    QStringList sceneNames() const;

    // ── Streaming output ──────────────────────────────────────────────────
    /**
     * Configure the RTMP service with the given server URL and stream key.
     * Must be called before startStreaming().
     */
    void configureRtmpService(const QString &server, const QString &key);

    /**
     * Start the RTMP stream output.
     * Requires: resetVideo(), resetAudio(), configureRtmpService() called first.
     * @return true if the output started successfully
     */
    bool startStreaming();

    /**
     * Stop the RTMP stream output.
     */
    void stopStreaming();

    bool isStreaming() const;

    // ── Recording output ──────────────────────────────────────────────────
    /**
     * Configure the recording output path and container format.
     * @param path    directory to save recordings
     * @param format  "mkv", "mp4", "mov", "flv"
     */
    void configureRecording(const QString &path, const QString &format = "mkv");

    bool startRecording();
    void stopRecording();
    bool isRecording() const;

    // ── Encoder configuration ─────────────────────────────────────────────
    /**
     * Set the video encoder.
     * @param encoderId  libobs encoder ID string, e.g.:
     *   "jim_nvenc"         — NVENC H.264 (NVIDIA)
     *   "jim_hevc_nvenc"    — NVENC HEVC (NVIDIA)
     *   "obs_qsv11"         — QuickSync H.264 (Intel)
     *   "amd_amf_h264"      — AMF H.264 (AMD)
     *   "obs_x264"          — x264 software
     * @param bitrate   video bitrate in kbps
     * @param rateControl "CBR", "VBR", or "CQP"
     * @param keyframeInterval  keyframe interval in seconds (0 = auto)
     */
    void configureVideoEncoder(const QString &encoderId,
                                uint32_t bitrate        = 8000,
                                const QString &rateControl = "CBR",
                                uint32_t keyframeInterval  = 2);

    /**
     * Set the audio encoder bitrate.
     * @param bitrate  audio bitrate in kbps (e.g. 320)
     */
    void configureAudioEncoder(uint32_t bitrate = 320);

    // ── Stats ─────────────────────────────────────────────────────────────
    const StreamStats &lastStats() const { return m_lastStats; }

signals:
    /** Emitted every second with fresh stream/system stats. */
    void statsUpdated(const StreamStats &stats);

    /** Emitted when streaming state changes. */
    void streamingStarted();
    void streamingStopped();

    /** Emitted when recording state changes. */
    void recordingStarted();
    void recordingStopped();

    /** Emitted when a scene is switched. */
    void activeSceneChanged(const QString &sceneName);

private:
    explicit OBSCore(QObject *parent = nullptr);
    ~OBSCore() override;
    OBSCore(const OBSCore &) = delete;
    OBSCore &operator=(const OBSCore &) = delete;

    void onStatsTick();
    void loadPlugins(const QString &pluginDir);

    bool          m_initialized  = false;
    obs_scene    *m_activeScene  = nullptr;
    obs_output   *m_streamOutput = nullptr;
    obs_output   *m_recOutput    = nullptr;
    obs_encoder  *m_videoEncoder = nullptr;
    obs_encoder  *m_audioEncoder = nullptr;
    obs_service  *m_rtmpService  = nullptr;

    QTimer        m_statsTimer;
    StreamStats   m_lastStats;
    uint64_t      m_prevOutputBytes = 0;
    QElapsedTimer m_streamTimer;
};
