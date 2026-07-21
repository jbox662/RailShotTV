#include "core/Project.h"
#include "core/Logger.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QSaveFile>
#include <QDir>
#include <QDateTime>

namespace railshot {

QJsonObject Project::toJson() const
{
    QJsonArray scenesArr;
    for (const auto& sc : scenes)
        scenesArr.append(sc.toJson());

    QJsonArray targetsArr;
    for (const auto& t : streamTargets)
        targetsArr.append(t.toJson());

    return QJsonObject{
        {QStringLiteral("schemaVersion"), schemaVersion},
        {QStringLiteral("name"), name},
        {QStringLiteral("scenes"), scenesArr},
        {QStringLiteral("activeSceneId"), activeSceneId},
        {QStringLiteral("previewSceneId"), previewSceneId},
        {QStringLiteral("programSceneId"), programSceneId},
        {QStringLiteral("output"), output.toJson()},
        {QStringLiteral("streamTargets"), targetsArr},
        {QStringLiteral("transition"), transitionTypeToString(transition)},
        {QStringLiteral("transitionMs"), transitionMs},
        {QStringLiteral("extras"), extras},
        {QStringLiteral("savedAt"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
    };
}

std::optional<Project> Project::fromJson(const QJsonObject& o, QString* error)
{
    const int ver = o.value(QStringLiteral("schemaVersion")).toInt(0);
    if (ver < 1 || ver > kProjectSchemaVersion) {
        if (error) *error = QStringLiteral("Unsupported project schema version %1").arg(ver);
        return std::nullopt;
    }

    Project p;
    p.schemaVersion = ver;
    p.name = o.value(QStringLiteral("name")).toString(QStringLiteral("Untitled Project"));
    p.activeSceneId = o.value(QStringLiteral("activeSceneId")).toString();
    p.previewSceneId = o.value(QStringLiteral("previewSceneId")).toString();
    p.programSceneId = o.value(QStringLiteral("programSceneId")).toString();
    p.output = OutputProfile::fromJson(o.value(QStringLiteral("output")).toObject());
    p.transition = transitionTypeFromString(o.value(QStringLiteral("transition")).toString());
    p.transitionMs = o.value(QStringLiteral("transitionMs")).toInt(500);
    p.extras = o.value(QStringLiteral("extras")).toObject();

    for (const auto& v : o.value(QStringLiteral("scenes")).toArray())
        p.scenes.append(SceneItem::fromJson(v.toObject()));
    for (const auto& v : o.value(QStringLiteral("streamTargets")).toArray())
        p.streamTargets.append(StreamTarget::fromJson(v.toObject()));

    p.ensureDefaults();
    return p;
}

bool Project::saveToFile(const QString& filePath, QString* error) const
{
    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        if (error) *error = file.errorString();
        return false;
    }
    // Backup previous version
    if (QFile::exists(filePath)) {
        const QString bak = filePath + QStringLiteral(".bak");
        QFile::remove(bak);
        QFile::copy(filePath, bak);
    }

    const QByteArray data = QJsonDocument(toJson()).toJson(QJsonDocument::Indented);
    if (file.write(data) != data.size()) {
        if (error) *error = QStringLiteral("Short write while saving project");
        return false;
    }
    if (!file.commit()) {
        if (error) *error = file.errorString();
        return false;
    }
    return true;
}

std::optional<Project> Project::loadFromFile(const QString& filePath, QString* error)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) *error = file.errorString();
        return std::nullopt;
    }
    QJsonParseError pe{};
    const auto doc = QJsonDocument::fromJson(file.readAll(), &pe);
    if (pe.error != QJsonParseError::NoError) {
        // Attempt recovery from .bak
        const QString bak = filePath + QStringLiteral(".bak");
        if (QFile::exists(bak)) {
            Logger::warn(QStringLiteral("Corrupt project — recovering from backup"));
            return loadFromFile(bak, error);
        }
        if (error) *error = pe.errorString();
        return std::nullopt;
    }
    auto p = fromJson(doc.object(), error);
    if (p) p->path = filePath;
    return p;
}

SceneItem* Project::findScene(const QString& id)
{
    for (auto& sc : scenes)
        if (sc.id == id) return &sc;
    return nullptr;
}

const SceneItem* Project::findScene(const QString& id) const
{
    for (const auto& sc : scenes)
        if (sc.id == id) return &sc;
    return nullptr;
}

SourceItem* Project::findSource(const QString& sceneId, const QString& sourceId)
{
    auto* sc = findScene(sceneId);
    if (!sc) return nullptr;
    for (auto& src : sc->sources)
        if (src.id == sourceId) return &src;
    return nullptr;
}

QString Project::editSceneId() const
{
    if (!previewSceneId.isEmpty() && findScene(previewSceneId))
        return previewSceneId;
    if (!activeSceneId.isEmpty() && findScene(activeSceneId))
        return activeSceneId;
    if (!scenes.isEmpty())
        return scenes.first().id;
    return {};
}

SourceItem* Project::findSourceInEditScene(const QString& sourceId)
{
    return findSource(editSceneId(), sourceId);
}

const SourceItem* Project::findSourceAnywhere(const QString& sourceId) const
{
    for (const auto& sc : scenes) {
        for (const auto& src : sc.sources) {
            if (src.id == sourceId)
                return &src;
        }
    }
    return nullptr;
}

SourceItem* Project::findSourceAnywhere(const QString& sourceId)
{
    for (auto& sc : scenes) {
        for (auto& src : sc.sources) {
            if (src.id == sourceId)
                return &src;
        }
    }
    return nullptr;
}

void Project::ensureDefaults()
{
    if (scenes.isEmpty()) {
        SceneItem sc;
        sc.id = newId(QStringLiteral("scn"));
        sc.name = QStringLiteral("Scene 1");
        scenes.append(sc);
        activeSceneId = sc.id;
        previewSceneId = sc.id;
    }
    if (activeSceneId.isEmpty() || !findScene(activeSceneId))
        activeSceneId = scenes.first().id;
    // Keep a valid Preview target so newly added sources are visible (OBS current-scene).
    if (previewSceneId.isEmpty() || !findScene(previewSceneId))
        previewSceneId = activeSceneId;
}

QString Project::addScene(const QString& name)
{
    SceneItem sc;
    sc.id = newId(QStringLiteral("scn"));
    sc.name = name.isEmpty() ? QStringLiteral("Scene %1").arg(scenes.size() + 1) : name;
    scenes.append(sc);
    activeSceneId = sc.id;
    previewSceneId = sc.id;
    return sc.id;
}

bool Project::removeScene(const QString& id)
{
    const int idx = [&] {
        for (int i = 0; i < scenes.size(); ++i)
            if (scenes[i].id == id) return i;
        return -1;
    }();
    if (idx < 0) return false;
    scenes.removeAt(idx);
    if (scenes.isEmpty())
        ensureDefaults();
    if (activeSceneId == id)
        activeSceneId = scenes.first().id;
    if (previewSceneId == id)
        previewSceneId = activeSceneId;
    if (programSceneId == id)
        programSceneId.clear();
    return true;
}

QString Project::duplicateScene(const QString& id)
{
    const auto* src = findScene(id);
    if (!src) return {};
    SceneItem copy = *src;
    copy.id = newId(QStringLiteral("scn"));
    copy.name = src->name + QStringLiteral(" (copy)");
    for (auto& s : copy.sources)
        s.id = newId(QStringLiteral("src"));
    scenes.append(copy);
    activeSceneId = copy.id;
    previewSceneId = copy.id;
    return copy.id;
}

bool Project::renameScene(const QString& id, const QString& name)
{
    auto* sc = findScene(id);
    if (!sc || name.trimmed().isEmpty()) return false;
    sc->name = name.trimmed();
    return true;
}

bool Project::reorderScenes(int from, int to)
{
    if (from < 0 || to < 0 || from >= scenes.size() || to >= scenes.size() || from == to)
        return false;
    scenes.move(from, to);
    return true;
}

QString Project::addSource(const QString& sceneId, SourceType type, const QString& name)
{
    auto* sc = findScene(sceneId);
    if (!sc) return {};
    SourceItem src;
    src.id = newId(QStringLiteral("src"));
    src.type = type;
    src.name = name.isEmpty() ? sourceTypeToString(type) : name;
    switch (type) {
    case SourceType::Camera: src.colorHex = QStringLiteral("#22C55E"); break;
    case SourceType::Display: src.colorHex = QStringLiteral("#4F9EFF"); break;
    case SourceType::Window: src.colorHex = QStringLiteral("#38BDF8"); break;
    case SourceType::Game: src.colorHex = QStringLiteral("#A3E635"); break;
    case SourceType::AudioInput: src.colorHex = QStringLiteral("#F472B6"); break;
    case SourceType::AudioOutput: src.colorHex = QStringLiteral("#FB7185"); break;
    case SourceType::Browser: src.colorHex = QStringLiteral("#22D3EE"); break;
    case SourceType::Text: src.colorHex = QStringLiteral("#A855F7"); break;
    case SourceType::Image: src.colorHex = QStringLiteral("#FBBF24"); break;
    case SourceType::Alert:
    case SourceType::Scoreboard: src.colorHex = QStringLiteral("#FF5A2C"); break;
    default: break;
    }
    // Default transforms matching React prototype
    switch (type) {
    case SourceType::Display:
    case SourceType::Window:
    case SourceType::Game:
        src.transform = {0, 0, 1, 1};
        break;
    case SourceType::Camera: src.transform = {0.05, 0.05, 0.4, 0.4}; break;
    case SourceType::Browser: src.transform = {0.1, 0.1, 0.8, 0.8}; break;
    case SourceType::Text: src.transform = {0.1, 0.7, 0.5, 0.12}; break;
    case SourceType::LowerThird: src.transform = {0.0, 0.72, 1.0, 0.2}; break;
    case SourceType::AudioInput:
    case SourceType::AudioOutput:
        src.transform = {0.35, 0.35, 0.3, 0.3};
        break;
    default: src.transform = {0.1, 0.1, 0.5, 0.5}; break;
    }
    sc->sources.append(src);
    return src.id;
}

bool Project::removeSource(const QString& sceneId, const QString& sourceId)
{
    auto* sc = findScene(sceneId);
    if (!sc) return false;
    for (int i = 0; i < sc->sources.size(); ++i) {
        if (sc->sources[i].id == sourceId) {
            sc->sources.removeAt(i);
            return true;
        }
    }
    return false;
}

bool Project::updateSource(const QString& sceneId, const SourceItem& source)
{
    auto* src = findSource(sceneId, source.id);
    if (!src) return false;
    *src = source;
    return true;
}

bool Project::moveSource(const QString& sceneId, const QString& sourceId, int delta)
{
    auto* sc = findScene(sceneId);
    if (!sc) return false;
    int idx = -1;
    for (int i = 0; i < sc->sources.size(); ++i)
        if (sc->sources[i].id == sourceId) { idx = i; break; }
    if (idx < 0) return false;
    const int dest = idx + delta;
    if (dest < 0 || dest >= sc->sources.size()) return false;
    sc->sources.swapItemsAt(idx, dest);
    return true;
}

QString Project::duplicateSource(const QString& sceneId, const QString& sourceId)
{
    auto* sc = findScene(sceneId);
    if (!sc) return {};
    int idx = -1;
    for (int i = 0; i < sc->sources.size(); ++i) {
        if (sc->sources[i].id == sourceId) {
            idx = i;
            break;
        }
    }
    if (idx < 0) return {};
    SourceItem copy = sc->sources[idx];
    copy.id = newId(QStringLiteral("src"));
    copy.name = copy.name + QStringLiteral(" (copy)");
    // Independent filter ids (OBS paste/duplicate behavior)
    const auto filters = copy.settings.value(QStringLiteral("filters")).toArray();
    if (!filters.isEmpty()) {
        QJsonArray next;
        for (const auto& v : filters) {
            QJsonObject o = v.toObject();
            o.insert(QStringLiteral("id"), newId(QStringLiteral("flt")));
            next.append(o);
        }
        copy.settings.insert(QStringLiteral("filters"), next);
    }
    sc->sources.insert(idx + 1, copy);
    return copy.id;
}

bool Project::moveSourceToExtreme(const QString& sceneId, const QString& sourceId, bool toTop)
{
    auto* sc = findScene(sceneId);
    if (!sc) return false;
    int idx = -1;
    for (int i = 0; i < sc->sources.size(); ++i) {
        if (sc->sources[i].id == sourceId) {
            idx = i;
            break;
        }
    }
    if (idx < 0) return false;
    if (toTop && idx == sc->sources.size() - 1) return true;
    if (!toTop && idx == 0) return true;
    SourceItem item = sc->sources.takeAt(idx);
    if (toTop)
        sc->sources.append(item);
    else
        sc->sources.prepend(item);
    return true;
}

} // namespace railshot
