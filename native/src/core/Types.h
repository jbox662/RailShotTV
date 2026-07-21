#pragma once

#include <QString>
#include <QUuid>
#include <QJsonObject>
#include <QJsonArray>
#include <QVector>
#include <QHash>
#include <QRectF>
#include <cstdint>
#include <optional>

namespace railshot {

inline constexpr int kProjectSchemaVersion = 1;
inline constexpr int kDefaultCanvasWidth = 1920;
inline constexpr int kDefaultCanvasHeight = 1080;
inline constexpr double kDefaultFps = 59.94;

enum class SourceType {
    Display,
    Camera,
    Browser,
    Text,
    Image,
    Alert,
    Scoreboard,
    LowerThird,
    Media,
    Ndi,
    Color,
    Window,
    Game,
    AudioInput,
    AudioOutput,
    Unknown
};

enum class TransitionType {
    Cut,
    Fade,
    Wipe,
    Merge,
    CubeZoom,
    FTB,
    // Wirecast-style catalog (each has a dedicated compositor path)
    Plane3D,
    Bands,
    ClockWipe,
    CrossBlur,
    CrossDissolve,
    Crosshair,
    RadialWipe,
    Swap,
    FlipOver,
    GridWipe,
    CurtainDrop,
    FadeToWhite,
    CircleWipe,
    Vacuum,
    WaveWipe,
    Push,
    WindshieldWipe,
    FlyOver,
    RgbChannels
};

/// True for two-phase fade-through-color takes (black or white).
bool transitionIsFtbStyle(TransitionType t);
/// True for single-pass A/B blends that use the program hold buffer.
bool transitionIsCrossfade(TransitionType t);
/// Shader mode id consumed by the transition pixel shader (0 = unused / Cut).
int transitionShaderMode(TransitionType t);

enum class EngineState {
    Idle,
    Previewing,
    Recording,
    Streaming,
    RecordingAndStreaming,
    Error
};

enum class ConnectionState {
    Disconnected,
    Connecting,
    Connected,
    Reconnecting,
    Failed
};

struct Transform {
    double x = 0.0;
    double y = 0.0;
    double w = 1.0;
    double h = 1.0;
    double opacity = 1.0;
    double rotation = 0.0;
    double cropLeft = 0.0;
    double cropRight = 0.0;
    double cropTop = 0.0;
    double cropBottom = 0.0;

    QJsonObject toJson() const;
    static Transform fromJson(const QJsonObject& o);
};

struct SourceItem {
    QString id;
    QString name;
    SourceType type = SourceType::Unknown;
    QString colorHex = QStringLiteral("#4F9EFF");
    bool visible = true;
    bool locked = false;
    Transform transform;
    QJsonObject settings; // type-specific configuration

    QJsonObject toJson() const;
    static SourceItem fromJson(const QJsonObject& o);
};

struct SceneItem {
    QString id;
    QString name;
    QVector<SourceItem> sources;

    QJsonObject toJson() const;
    static SceneItem fromJson(const QJsonObject& o);
};

/// OBS-style per-source monitoring (headphones vs stream/record).
enum class AudioMonitoringType {
    None = 0,            // stream/record only
    MonitorOnly = 1,     // headphones only
    MonitorAndOutput = 2 // both
};

struct AudioChannelState {
    QString id;
    QString name;
    float volume = 1.0f;      // 0..1 linear
    float pan = 0.0f;         // -1..1 balance
    float gainDb = 0.0f;      // trim dB (Adv Audio / fader math)
    bool muted = false;
    bool solo = false;
    bool forceMono = false;
    bool locked = false;
    int syncOffsetMs = 0;     // -2000..+2000
    AudioMonitoringType monitoring = AudioMonitoringType::MonitorAndOutput;
    quint8 trackMask = 0x01;  // bits 0..5 = tracks 1..6 (stream uses track 1)
    float peakL = 0.0f;
    float peakR = 0.0f;
    float rmsL = 0.0f;
    float rmsR = 0.0f;

    QJsonObject toJson() const;
    static AudioChannelState fromJson(const QJsonObject& o);
};

struct OutputProfile {
    int width = kDefaultCanvasWidth;
    int height = kDefaultCanvasHeight;
    double fps = kDefaultFps;
    int videoBitrateKbps = 6000;
    int audioBitrateKbps = 160;
    QString videoCodec = QStringLiteral("h264");
    QString audioCodec = QStringLiteral("aac");
    QString encoderPreference = QStringLiteral("auto"); // auto|software|mf|nvenc|amf|qsv
    QString rateControl = QStringLiteral("CBR");
    int keyframeIntervalSec = 2;

    QJsonObject toJson() const;
    static OutputProfile fromJson(const QJsonObject& o);
};

struct StreamTarget {
    QString id;
    QString platform;   // youtube|twitch|facebook|custom
    QString name;
    QString title;
    QString rtmpUrl;
    QString streamKeySecretId; // Credential Manager key id — never store raw key
    bool enabled = true;

    QJsonObject toJson() const;
    static StreamTarget fromJson(const QJsonObject& o);
};

struct TelemetrySnapshot {
    EngineState state = EngineState::Idle;
    ConnectionState streamState = ConnectionState::Disconnected;
    bool recording = false;
    bool streaming = false;
    double fpsRender = 0.0;
    double fpsEncode = 0.0;
    double cpuPercent = 0.0;
    double gpuPercent = 0.0;
    qint64 droppedFrames = 0;
    qint64 renderedFrames = 0;
    qint64 bitrateKbps = 0;
    double avDriftMs = 0.0;
    qint64 streamUptimeSec = 0;
    qint64 recordUptimeSec = 0;
    QString lastError;
};

QString sourceTypeToString(SourceType t);
SourceType sourceTypeFromString(const QString& s);
QString transitionTypeToString(TransitionType t);
TransitionType transitionTypeFromString(const QString& s);
QString newId(const QString& prefix = QStringLiteral("id"));

/// OBS_SOURCE_AUDIO equivalent — this source type can produce audio.
bool sourceTypeSupportsAudio(SourceType t);
/// OBS mixer visibility (incl. Browser “Control audio via OBS”).
bool sourceAppearsInAudioMixer(const SourceItem& src);

} // namespace railshot
