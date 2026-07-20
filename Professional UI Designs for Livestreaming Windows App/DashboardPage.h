#pragma once
#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QTime>

class SceneModel;
class AudioChannelModel;
class LiveBadge;
class BitrateGraph;
class HealthBar;
class SceneCard;
class VUMeter;

/**
 * DashboardPage
 *
 * The main broadcast control screen. Layout (left→right, top→bottom):
 *
 *  ┌─────────────────────────────────────────────────────────────────┐
 *  │ TopBar: RAILSHOT TV | DASHBOARD | ● LIVE | timecode | sig | res │ 46px
 *  ├──────────────────────────────────────────┬──────────────────────┤
 *  │                                          │  STREAM STATUS       │
 *  │         PROGRAM OUTPUT                   │  ● LIVE on YouTube   │
 *  │         (QWidget, custom paint)          │  BITRATE  8,686 kbps │
 *  │         16:9 aspect ratio canvas         │  BitrateGraph        │
 *  │         with corner brackets             │  VIEWERS / UPTIME    │
 *  │         and center crosshair             │  STREAM HEALTH       │
 *  │                                          │  CPU/GPU/Network bars│
 *  ├──────────────────────────────────────────┤  ─────────────────── │
 *  │ Timecode strip (28px)                    │  [END STREAM]        │
 *  ├──────────────────────────────────────────┤                      │
 *  │ SCENES panel (scene cards, horizontal)   │                      │
 *  ├──────────────────────────────────────────┤                      │
 *  │ AUDIO MIXER (channel strips)             │                      │
 *  └──────────────────────────────────────────┴──────────────────────┘
 *
 * Right panel (StreamStatusPanel) is fixed at 240px wide.
 * Left column fills remaining width.
 */
class DashboardPage : public QWidget
{
    Q_OBJECT

public:
    explicit DashboardPage(QWidget *parent = nullptr);
    ~DashboardPage() override = default;

private slots:
    void onTimecodeTimer();
    void onBitrateTimer();
    void onSceneCardClicked(int index);

private:
    void setupUi();
    void buildTopBar(QWidget *parent);
    void buildProgramOutput(QWidget *parent);
    void buildTimecodeStrip(QWidget *parent);
    void buildScenesPanel(QWidget *parent);
    void buildAudioMixer(QWidget *parent);
    void buildStreamStatusPanel(QWidget *parent);

    // Models
    SceneModel        *m_sceneModel = nullptr;
    AudioChannelModel *m_audioModel = nullptr;

    // Top bar widgets
    LiveBadge *m_liveBadge      = nullptr;
    QLabel    *m_timecodeLabel  = nullptr;
    QLabel    *m_platformLabel  = nullptr;
    QLabel    *m_resLabel       = nullptr;

    // Timecode strip
    QLabel *m_tcStripLabel      = nullptr;
    QLabel *m_tcSceneLabel      = nullptr;
    QLabel *m_tcViewersLabel    = nullptr;
    QLabel *m_tcBitrateLabel    = nullptr;

    // Stream status panel
    QLabel       *m_bitrateValue   = nullptr;
    QLabel       *m_viewersValue   = nullptr;
    QLabel       *m_uptimeValue    = nullptr;
    BitrateGraph *m_bitrateGraph   = nullptr;
    HealthBar    *m_cpuBar         = nullptr;
    HealthBar    *m_gpuBar         = nullptr;
    HealthBar    *m_netBar         = nullptr;
    QLabel       *m_cpuPct         = nullptr;
    QLabel       *m_gpuPct         = nullptr;
    QLabel       *m_netStatus      = nullptr;

    // Scene cards
    QVector<SceneCard *> m_sceneCards;

    // VU meters
    QVector<VUMeter *> m_vuMeters;

    // Timers
    QTimer m_timecodeTimer;
    QTimer m_bitrateTimer;
    QTime  m_streamStart;

    // Static example values (DYNAMIC in Phase 3 — from libobs)
    int   m_viewerCount  = 2852;   // DYNAMIC: from platform API
    float m_bitrateSample = 8686.0f; // DYNAMIC: from libobs output stats
    int   m_cpuPercent   = 28;     // DYNAMIC: from QSysInfo / libobs
    int   m_gpuPercent   = 42;     // DYNAMIC: from GPU query
};
