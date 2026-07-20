#pragma once
/**
 * OBSStatsMonitor — Real-time stream statistics feed for RailShotTV
 *
 * Polls obs_get_stats() and obs_output_get_* on a 1-second QTimer.
 * Emits statsUpdated() which the Dashboard, Analytics, and top-bar widgets
 * connect to for live KPI display.
 *
 * All values marked DYNAMIC must come from this class — never hardcode them.
 * Values marked STATIC are UI labels that do not change at runtime.
 */

#include <QObject>
#include <QTimer>

// ── Stats snapshot ────────────────────────────────────────────────────────────
struct StreamStats {
    // Output stats (DYNAMIC — from obs_output_get_*)
    uint64_t totalFrames        = 0;   // total frames rendered
    uint64_t droppedFrames      = 0;   // frames dropped by encoder
    uint64_t skippedFrames      = 0;   // frames skipped by renderer
    double   droppedFramesPct   = 0.0; // dropped / total * 100
    uint64_t totalBytes         = 0;   // total bytes sent
    uint64_t bitrate            = 0;   // current bitrate in kbps (DYNAMIC)
    double   congestion         = 0.0; // 0.0–1.0 network congestion estimate

    // Render stats (DYNAMIC — from obs_get_stats())
    double   renderFps          = 0.0; // actual render FPS (DYNAMIC)
    double   renderAvgTime      = 0.0; // avg render time in ms
    double   renderMissedFrames = 0.0; // missed render frames %

    // Encoder stats (DYNAMIC)
    double   encoderAvgTime     = 0.0; // avg encode time in ms
    double   encoderOverloaded  = 0.0; // encoder overload %

    // System stats (DYNAMIC — from obs_get_cpu_usage / os_get_sys_free_mem)
    double   cpuUsage           = 0.0; // CPU usage % (DYNAMIC)
    uint64_t memUsageMB         = 0;   // memory usage in MB (DYNAMIC)
    uint64_t diskSpaceMB        = 0;   // free disk space in MB (DYNAMIC)

    // Stream uptime (DYNAMIC — computed from stream start time)
    int      uptimeSeconds      = 0;   // seconds since stream started (DYNAMIC)

    // Stream state flags (DYNAMIC)
    bool     streaming          = false;
    bool     recording          = false;
    bool     replayBufferActive = false;
};

class OBSStatsMonitor : public QObject
{
    Q_OBJECT

public:
    static OBSStatsMonitor &instance();

    /** Start polling at the given interval (default 1000ms). */
    void start(int intervalMs = 1000);

    /** Stop polling. */
    void stop();

    /** Return the most recent stats snapshot. */
    const StreamStats &lastStats() const { return m_lastStats; }

signals:
    /**
     * Emitted every poll interval with a fresh stats snapshot.
     * Connect to:
     *   - BitrateGraph::addSample()  (Dashboard bitrate sparkline)
     *   - DashboardPage::onStats()   (top-bar KPI labels)
     *   - AnalyticsPage::onStats()   (analytics KPI cards)
     */
    void statsUpdated(const StreamStats &stats);

private slots:
    void poll();

private:
    explicit OBSStatsMonitor(QObject *parent = nullptr);
    ~OBSStatsMonitor() override = default;
    OBSStatsMonitor(const OBSStatsMonitor &) = delete;
    OBSStatsMonitor &operator=(const OBSStatsMonitor &) = delete;

    QTimer      m_timer;
    StreamStats m_lastStats;
    QDateTime   m_streamStartTime;
};
