#include "core/Types.h"
#include <QUuid>
#include <QDateTime>

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
    }
    return QStringLiteral("Cut");
}

TransitionType transitionTypeFromString(const QString& s)
{
    if (s == QLatin1String("Fade")) return TransitionType::Fade;
    if (s == QLatin1String("Wipe")) return TransitionType::Wipe;
    if (s == QLatin1String("Merge")) return TransitionType::Merge;
    if (s == QLatin1String("CubeZoom")) return TransitionType::CubeZoom;
    if (s == QLatin1String("FTB")) return TransitionType::FTB;
    return TransitionType::Cut;
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
