#include "core/Types.h"
#include <QUuid>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>

namespace railshot {

QString newId(const QString& prefix)
{
    return prefix + QLatin1Char('_') + QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
}

QString sourceTypeToString(SourceType t)
{
    switch (t) {
    case SourceType::Display: return QStringLiteral("display");
    case SourceType::Camera: return QStringLiteral("camera");
    case SourceType::Browser: return QStringLiteral("browser");
    case SourceType::Text: return QStringLiteral("text");
    case SourceType::Image: return QStringLiteral("image");
    case SourceType::Alert: return QStringLiteral("alert");
    case SourceType::Scoreboard: return QStringLiteral("scoreboard");
    case SourceType::LowerThird: return QStringLiteral("lowerthird");
    case SourceType::Media: return QStringLiteral("media");
    case SourceType::Ndi: return QStringLiteral("ndi");
    case SourceType::Color: return QStringLiteral("color");
    case SourceType::Window: return QStringLiteral("window");
    case SourceType::Game: return QStringLiteral("game");
    case SourceType::AudioInput: return QStringLiteral("audio_input");
    case SourceType::AudioOutput: return QStringLiteral("audio_output");
    default: return QStringLiteral("unknown");
    }
}

SourceType sourceTypeFromString(const QString& s)
{
    if (s == QLatin1String("display")) return SourceType::Display;
    if (s == QLatin1String("camera")) return SourceType::Camera;
    if (s == QLatin1String("browser")) return SourceType::Browser;
    if (s == QLatin1String("text")) return SourceType::Text;
    if (s == QLatin1String("image")) return SourceType::Image;
    if (s == QLatin1String("alert")) return SourceType::Alert;
    if (s == QLatin1String("scoreboard")) return SourceType::Scoreboard;
    if (s == QLatin1String("lowerthird") || s == QLatin1String("lower-third")) return SourceType::LowerThird;
    if (s == QLatin1String("media")) return SourceType::Media;
    if (s == QLatin1String("ndi")) return SourceType::Ndi;
    if (s == QLatin1String("color")) return SourceType::Color;
    if (s == QLatin1String("window")) return SourceType::Window;
    if (s == QLatin1String("game")) return SourceType::Game;
    if (s == QLatin1String("audio_input") || s == QLatin1String("audioinput")) return SourceType::AudioInput;
    if (s == QLatin1String("audio_output") || s == QLatin1String("audiooutput")) return SourceType::AudioOutput;
    return SourceType::Unknown;
}

QString transitionTypeToString(TransitionType t)
{
    switch (t) {
    case TransitionType::Cut: return QStringLiteral("Cut");
    case TransitionType::Fade: return QStringLiteral("Fade");
    case TransitionType::Wipe: return QStringLiteral("Wipe");
    case TransitionType::Merge: return QStringLiteral("Merge");
    case TransitionType::CubeZoom: return QStringLiteral("CubeZoom");
    case TransitionType::FTB: return QStringLiteral("FTB");
    case TransitionType::Plane3D: return QStringLiteral("Plane3D");
    case TransitionType::Bands: return QStringLiteral("Bands");
    case TransitionType::ClockWipe: return QStringLiteral("ClockWipe");
    case TransitionType::CrossBlur: return QStringLiteral("CrossBlur");
    case TransitionType::CrossDissolve: return QStringLiteral("CrossDissolve");
    case TransitionType::Crosshair: return QStringLiteral("Crosshair");
    case TransitionType::RadialWipe: return QStringLiteral("RadialWipe");
    case TransitionType::Swap: return QStringLiteral("Swap");
    case TransitionType::FlipOver: return QStringLiteral("FlipOver");
    case TransitionType::GridWipe: return QStringLiteral("GridWipe");
    case TransitionType::CurtainDrop: return QStringLiteral("CurtainDrop");
    case TransitionType::FadeToWhite: return QStringLiteral("FadeToWhite");
    case TransitionType::CircleWipe: return QStringLiteral("CircleWipe");
    case TransitionType::Vacuum: return QStringLiteral("Vacuum");
    case TransitionType::WaveWipe: return QStringLiteral("WaveWipe");
    case TransitionType::Push: return QStringLiteral("Push");
    case TransitionType::WindshieldWipe: return QStringLiteral("WindshieldWipe");
    case TransitionType::FlyOver: return QStringLiteral("FlyOver");
    case TransitionType::RgbChannels: return QStringLiteral("RgbChannels");
    }
    return QStringLiteral("Cut");
}

TransitionType transitionTypeFromString(const QString& s)
{
    if (s == QLatin1String("Fade")) return TransitionType::Fade;
    if (s == QLatin1String("Wipe")) return TransitionType::Wipe;
    if (s == QLatin1String("Merge")) return TransitionType::Merge;
    if (s == QLatin1String("CubeZoom")) return TransitionType::CubeZoom;
    if (s == QLatin1String("FTB") || s == QLatin1String("FadeToBlack")) return TransitionType::FTB;
    if (s == QLatin1String("Plane3D") || s == QLatin1String("3D Plane")) return TransitionType::Plane3D;
    if (s == QLatin1String("Bands")) return TransitionType::Bands;
    if (s == QLatin1String("ClockWipe") || s == QLatin1String("Clock Wipe")) return TransitionType::ClockWipe;
    if (s == QLatin1String("CrossBlur") || s == QLatin1String("Cross Blur")) return TransitionType::CrossBlur;
    if (s == QLatin1String("CrossDissolve") || s == QLatin1String("Cross Dissolve")
        || s == QLatin1String("Smooth"))
        return TransitionType::CrossDissolve;
    if (s == QLatin1String("Crosshair")) return TransitionType::Crosshair;
    if (s == QLatin1String("RadialWipe") || s == QLatin1String("Radial Wipe")) return TransitionType::RadialWipe;
    if (s == QLatin1String("Swap")) return TransitionType::Swap;
    if (s == QLatin1String("FlipOver") || s == QLatin1String("Flip Over")) return TransitionType::FlipOver;
    if (s == QLatin1String("GridWipe") || s == QLatin1String("Grid Wipe")) return TransitionType::GridWipe;
    if (s == QLatin1String("CurtainDrop") || s == QLatin1String("Curtain Drop Wipe"))
        return TransitionType::CurtainDrop;
    if (s == QLatin1String("FadeToWhite") || s == QLatin1String("Fade to White"))
        return TransitionType::FadeToWhite;
    if (s == QLatin1String("CircleWipe") || s == QLatin1String("Circle Wipe")) return TransitionType::CircleWipe;
    if (s == QLatin1String("Vacuum")) return TransitionType::Vacuum;
    if (s == QLatin1String("WaveWipe") || s == QLatin1String("Wave Wipe")) return TransitionType::WaveWipe;
    if (s == QLatin1String("Push")) return TransitionType::Push;
    if (s == QLatin1String("WindshieldWipe") || s == QLatin1String("Windshield Wipe"))
        return TransitionType::WindshieldWipe;
    if (s == QLatin1String("FlyOver") || s == QLatin1String("Fly Over")) return TransitionType::FlyOver;
    if (s == QLatin1String("RgbChannels") || s == QLatin1String("RGB Channels"))
        return TransitionType::RgbChannels;
    return TransitionType::Cut;
}

bool transitionIsFtbStyle(TransitionType t)
{
    return t == TransitionType::FTB || t == TransitionType::FadeToWhite;
}

bool transitionIsCrossfade(TransitionType t)
{
    return t != TransitionType::Cut && !transitionIsFtbStyle(t);
}

int transitionShaderMode(TransitionType t)
{
    switch (t) {
    case TransitionType::Cut: return 0;
    case TransitionType::Fade:
    case TransitionType::CrossDissolve: return 1;
    case TransitionType::Wipe: return 2;
    case TransitionType::Merge: return 3;
    case TransitionType::CubeZoom: return 4;
    case TransitionType::Plane3D: return 5;
    case TransitionType::Bands: return 6;
    case TransitionType::ClockWipe: return 7;
    case TransitionType::CrossBlur: return 8;
    case TransitionType::Crosshair: return 9;
    case TransitionType::RadialWipe: return 10;
    case TransitionType::Swap: return 11;
    case TransitionType::FlipOver: return 12;
    case TransitionType::GridWipe: return 13;
    case TransitionType::CurtainDrop: return 14;
    case TransitionType::CircleWipe: return 15;
    case TransitionType::Vacuum: return 16;
    case TransitionType::WaveWipe: return 17;
    case TransitionType::Push: return 18;
    case TransitionType::WindshieldWipe: return 19;
    case TransitionType::FlyOver: return 20;
    case TransitionType::RgbChannels: return 21;
    case TransitionType::FTB:
    case TransitionType::FadeToWhite: return 0;
    }
    return 1;
}

bool sourceTypeSupportsAudio(SourceType t)
{
    // Match OBS output_flags OBS_SOURCE_AUDIO (not video-only types).
    switch (t) {
    case SourceType::Camera:      // dshow / MF: video+audio
    case SourceType::Window:      // window capture can include app audio
    case SourceType::Game:        // game capture can include app audio
    case SourceType::Media:       // ffmpeg/vlc media
    case SourceType::Ndi:
    case SourceType::AudioInput:
    case SourceType::AudioOutput:
    case SourceType::Browser:     // CEF browser (gated by controlAudioViaObs)
        return true;
    case SourceType::Display:     // display capture is video-only; Desktop Audio is separate
    case SourceType::Image:
    case SourceType::Text:
    case SourceType::Color:
    case SourceType::Alert:
    case SourceType::Scoreboard:
    case SourceType::LowerThird:
    case SourceType::Unknown:
    default:
        return false;
    }
}

bool sourceAppearsInAudioMixer(const SourceItem& src)
{
    if (!sourceTypeSupportsAudio(src.type))
        return false;
    // OBS Browser: mixer strip only when "Control audio via OBS" is enabled (default off).
    if (src.type == SourceType::Browser)
        return src.settings.value(QStringLiteral("controlAudioViaObs")).toBool(false);
    return true;
}

QJsonObject Transform::toJson() const
{
    return QJsonObject{
        {QStringLiteral("x"), x},
        {QStringLiteral("y"), y},
        {QStringLiteral("w"), w},
        {QStringLiteral("h"), h},
        {QStringLiteral("opacity"), opacity},
        {QStringLiteral("rotation"), rotation},
        {QStringLiteral("cropLeft"), cropLeft},
        {QStringLiteral("cropRight"), cropRight},
        {QStringLiteral("cropTop"), cropTop},
        {QStringLiteral("cropBottom"), cropBottom},
    };
}

Transform Transform::fromJson(const QJsonObject& o)
{
    Transform t;
    t.x = o.value(QStringLiteral("x")).toDouble(0.0);
    t.y = o.value(QStringLiteral("y")).toDouble(0.0);
    t.w = o.value(QStringLiteral("w")).toDouble(1.0);
    t.h = o.value(QStringLiteral("h")).toDouble(1.0);
    t.opacity = o.value(QStringLiteral("opacity")).toDouble(1.0);
    t.rotation = o.value(QStringLiteral("rotation")).toDouble(0.0);
    t.cropLeft = o.value(QStringLiteral("cropLeft")).toDouble(0.0);
    t.cropRight = o.value(QStringLiteral("cropRight")).toDouble(0.0);
    t.cropTop = o.value(QStringLiteral("cropTop")).toDouble(0.0);
    t.cropBottom = o.value(QStringLiteral("cropBottom")).toDouble(0.0);
    return t;
}

QJsonObject SourceItem::toJson() const
{
    return QJsonObject{
        {QStringLiteral("id"), id},
        {QStringLiteral("name"), name},
        {QStringLiteral("type"), sourceTypeToString(type)},
        {QStringLiteral("colorHex"), colorHex},
        {QStringLiteral("visible"), visible},
        {QStringLiteral("locked"), locked},
        {QStringLiteral("transform"), transform.toJson()},
        {QStringLiteral("settings"), settings},
    };
}

SourceItem SourceItem::fromJson(const QJsonObject& o)
{
    SourceItem s;
    s.id = o.value(QStringLiteral("id")).toString(newId(QStringLiteral("src")));
    s.name = o.value(QStringLiteral("name")).toString(QStringLiteral("Source"));
    s.type = sourceTypeFromString(o.value(QStringLiteral("type")).toString());
    s.colorHex = o.value(QStringLiteral("colorHex")).toString(QStringLiteral("#4F9EFF"));
    s.visible = o.value(QStringLiteral("visible")).toBool(true);
    s.locked = o.value(QStringLiteral("locked")).toBool(false);
    s.transform = Transform::fromJson(o.value(QStringLiteral("transform")).toObject());
    s.settings = o.value(QStringLiteral("settings")).toObject();
    return s;
}

QJsonObject SceneItem::toJson() const
{
    QJsonArray arr;
    for (const auto& src : sources)
        arr.append(src.toJson());
    return QJsonObject{
        {QStringLiteral("id"), id},
        {QStringLiteral("name"), name},
        {QStringLiteral("sources"), arr},
    };
}

SceneItem SceneItem::fromJson(const QJsonObject& o)
{
    SceneItem sc;
    sc.id = o.value(QStringLiteral("id")).toString(newId(QStringLiteral("scn")));
    sc.name = o.value(QStringLiteral("name")).toString(QStringLiteral("Scene"));
    const auto arr = o.value(QStringLiteral("sources")).toArray();
    for (const auto& v : arr)
        sc.sources.append(SourceItem::fromJson(v.toObject()));
    return sc;
}

QJsonObject AudioChannelState::toJson() const
{
    return QJsonObject{
        {QStringLiteral("id"), id},
        {QStringLiteral("name"), name},
        {QStringLiteral("volume"), double(volume)},
        {QStringLiteral("pan"), double(pan)},
        {QStringLiteral("gainDb"), double(gainDb)},
        {QStringLiteral("muted"), muted},
        {QStringLiteral("solo"), solo},
        {QStringLiteral("forceMono"), forceMono},
        {QStringLiteral("locked"), locked},
        {QStringLiteral("syncOffsetMs"), syncOffsetMs},
        {QStringLiteral("monitoring"), int(monitoring)},
        {QStringLiteral("trackMask"), int(trackMask)},
        {QStringLiteral("gateEnabled"), gateEnabled},
        {QStringLiteral("gateOpenDb"), double(gateOpenDb)},
        {QStringLiteral("gateAttackMs"), double(gateAttackMs)},
        {QStringLiteral("gateHoldMs"), double(gateHoldMs)},
        {QStringLiteral("gateReleaseMs"), double(gateReleaseMs)},
        {QStringLiteral("compEnabled"), compEnabled},
        {QStringLiteral("compThresholdDb"), double(compThresholdDb)},
        {QStringLiteral("compRatio"), double(compRatio)},
        {QStringLiteral("compAttackMs"), double(compAttackMs)},
        {QStringLiteral("compReleaseMs"), double(compReleaseMs)},
        {QStringLiteral("compMakeupDb"), double(compMakeupDb)},
    };
}

AudioChannelState AudioChannelState::fromJson(const QJsonObject& o)
{
    AudioChannelState s;
    s.id = o.value(QStringLiteral("id")).toString();
    s.name = o.value(QStringLiteral("name")).toString(s.id);
    s.volume = float(o.value(QStringLiteral("volume")).toDouble(1.0));
    s.pan = float(o.value(QStringLiteral("pan")).toDouble(0.0));
    s.gainDb = float(o.value(QStringLiteral("gainDb")).toDouble(0.0));
    s.muted = o.value(QStringLiteral("muted")).toBool(false);
    s.solo = o.value(QStringLiteral("solo")).toBool(false);
    s.forceMono = o.value(QStringLiteral("forceMono")).toBool(false);
    s.locked = o.value(QStringLiteral("locked")).toBool(false);
    s.syncOffsetMs = o.value(QStringLiteral("syncOffsetMs")).toInt(0);
    s.monitoring = AudioMonitoringType(o.value(QStringLiteral("monitoring")).toInt(int(AudioMonitoringType::MonitorAndOutput)));
    s.trackMask = quint8(o.value(QStringLiteral("trackMask")).toInt(0x01));
    s.gateEnabled = o.value(QStringLiteral("gateEnabled")).toBool(false);
    s.gateOpenDb = float(o.value(QStringLiteral("gateOpenDb")).toDouble(-32.0));
    s.gateAttackMs = float(o.value(QStringLiteral("gateAttackMs")).toDouble(25.0));
    s.gateHoldMs = float(o.value(QStringLiteral("gateHoldMs")).toDouble(200.0));
    s.gateReleaseMs = float(o.value(QStringLiteral("gateReleaseMs")).toDouble(150.0));
    s.compEnabled = o.value(QStringLiteral("compEnabled")).toBool(false);
    s.compThresholdDb = float(o.value(QStringLiteral("compThresholdDb")).toDouble(-18.0));
    s.compRatio = float(o.value(QStringLiteral("compRatio")).toDouble(4.0));
    s.compAttackMs = float(o.value(QStringLiteral("compAttackMs")).toDouble(6.0));
    s.compReleaseMs = float(o.value(QStringLiteral("compReleaseMs")).toDouble(60.0));
    s.compMakeupDb = float(o.value(QStringLiteral("compMakeupDb")).toDouble(0.0));
    return s;
}

QJsonObject OutputProfile::toJson() const
{
    return QJsonObject{
        {QStringLiteral("width"), width},
        {QStringLiteral("height"), height},
        {QStringLiteral("fps"), fps},
        {QStringLiteral("videoBitrateKbps"), videoBitrateKbps},
        {QStringLiteral("audioBitrateKbps"), audioBitrateKbps},
        {QStringLiteral("videoCodec"), videoCodec},
        {QStringLiteral("audioCodec"), audioCodec},
        {QStringLiteral("encoderPreference"), encoderPreference},
        {QStringLiteral("rateControl"), rateControl},
        {QStringLiteral("keyframeIntervalSec"), keyframeIntervalSec},
    };
}

OutputProfile OutputProfile::fromJson(const QJsonObject& o)
{
    OutputProfile p;
    p.width = o.value(QStringLiteral("width")).toInt(kDefaultCanvasWidth);
    p.height = o.value(QStringLiteral("height")).toInt(kDefaultCanvasHeight);
    p.fps = o.value(QStringLiteral("fps")).toDouble(kDefaultFps);
    p.videoBitrateKbps = o.value(QStringLiteral("videoBitrateKbps")).toInt(6000);
    p.audioBitrateKbps = o.value(QStringLiteral("audioBitrateKbps")).toInt(160);
    p.videoCodec = o.value(QStringLiteral("videoCodec")).toString(QStringLiteral("h264"));
    p.audioCodec = o.value(QStringLiteral("audioCodec")).toString(QStringLiteral("aac"));
    p.encoderPreference = o.value(QStringLiteral("encoderPreference")).toString(QStringLiteral("auto"));
    p.rateControl = o.value(QStringLiteral("rateControl")).toString(QStringLiteral("CBR"));
    p.keyframeIntervalSec = o.value(QStringLiteral("keyframeIntervalSec")).toInt(2);
    return p;
}

QJsonObject StreamTarget::toJson() const
{
    return QJsonObject{
        {QStringLiteral("id"), id},
        {QStringLiteral("platform"), platform},
        {QStringLiteral("name"), name},
        {QStringLiteral("title"), title},
        {QStringLiteral("rtmpUrl"), rtmpUrl},
        {QStringLiteral("streamKeySecretId"), streamKeySecretId},
        {QStringLiteral("enabled"), enabled},
    };
}

StreamTarget StreamTarget::fromJson(const QJsonObject& o)
{
    StreamTarget t;
    t.id = o.value(QStringLiteral("id")).toString(newId(QStringLiteral("tgt")));
    t.platform = o.value(QStringLiteral("platform")).toString(QStringLiteral("custom"));
    t.name = o.value(QStringLiteral("name")).toString(QStringLiteral("Stream"));
    t.title = o.value(QStringLiteral("title")).toString();
    t.rtmpUrl = o.value(QStringLiteral("rtmpUrl")).toString();
    t.streamKeySecretId = o.value(QStringLiteral("streamKeySecretId")).toString();
    t.enabled = o.value(QStringLiteral("enabled")).toBool(true);
    return t;
}

} // namespace railshot
