// OBSSourceManager.cpp
// Chromatic Command: scene/source management for RailShotTV.

#include "OBSSourceManager.h"
#include "OBSCore.h"

#include <obs.h>
#include <obs-data.h>

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

// ── Singleton ─────────────────────────────────────────────────────────────────
OBSSourceManager &OBSSourceManager::instance()
{
    static OBSSourceManager s_instance;
    return s_instance;
}

OBSSourceManager::OBSSourceManager(QObject *parent)
    : QObject(parent)
{}

// ── Internal helpers ──────────────────────────────────────────────────────────
obs_scene *OBSSourceManager::sceneAt(int index) const
{
    if (index < 0 || index >= m_scenes.size()) return nullptr;
    return m_scenes.at(index);
}

obs_sceneitem *OBSSourceManager::sceneItemById(int itemId) const
{
    obs_scene *scene = sceneAt(m_activeIndex);
    if (!scene) return nullptr;

    obs_sceneitem *found = nullptr;
    obs_scene_enum_items(scene, [](obs_scene_t*, obs_sceneitem_t *item, void *param) -> bool {
        auto *p = static_cast<std::pair<int, obs_sceneitem_t**>*>(param);
        if (obs_sceneitem_get_id(item) == p->first) {
            *p->second = item;
            return false;
        }
        return true;
    }, &std::make_pair(itemId, &found));

    return found;
}

// ── Scene management ──────────────────────────────────────────────────────────
bool OBSSourceManager::createScene(const QString &name)
{
    obs_scene_t *scene = obs_scene_create(name.toUtf8().constData());
    if (!scene) {
        qWarning() << "[OBSSourceManager] Failed to create scene:" << name;
        return false;
    }
    m_scenes.append(scene);
    if (m_scenes.size() == 1) switchToScene(0);
    emit scenesChanged();
    return true;
}

bool OBSSourceManager::deleteScene(int index)
{
    if (m_scenes.size() <= 1) {
        qWarning() << "[OBSSourceManager] Cannot delete the last scene";
        return false;
    }
    obs_scene_t *scene = sceneAt(index);
    if (!scene) return false;
    m_scenes.removeAt(index);
    obs_source_release(obs_scene_get_source(scene));
    if (m_activeIndex >= m_scenes.size()) m_activeIndex = m_scenes.size() - 1;
    switchToScene(m_activeIndex);
    emit scenesChanged();
    return true;
}

bool OBSSourceManager::renameScene(int index, const QString &newName)
{
    obs_scene_t *scene = sceneAt(index);
    if (!scene) return false;
    obs_source_set_name(obs_scene_get_source(scene), newName.toUtf8().constData());
    emit scenesChanged();
    return true;
}

bool OBSSourceManager::moveScene(int fromIndex, int toIndex)
{
    if (fromIndex < 0 || fromIndex >= m_scenes.size()) return false;
    if (toIndex   < 0 || toIndex   >= m_scenes.size()) return false;
    m_scenes.move(fromIndex, toIndex);
    if (m_activeIndex == fromIndex) m_activeIndex = toIndex;
    emit scenesChanged();
    return true;
}

void OBSSourceManager::switchToScene(int index)
{
    obs_scene_t *scene = sceneAt(index);
    if (!scene) return;
    m_activeIndex = index;
    obs_source_t *src = obs_scene_get_source(scene);
    obs_set_output_source(0, src);
    emit activeSceneChanged(index, QString::fromUtf8(obs_source_get_name(src)));
}

QList<SceneInfo> OBSSourceManager::scenes() const
{
    QList<SceneInfo> result;
    for (int i = 0; i < m_scenes.size(); ++i) {
        obs_source_t *src = obs_scene_get_source(m_scenes.at(i));
        SceneInfo info;
        info.id     = i;
        info.name   = QString::fromUtf8(obs_source_get_name(src));
        info.active = (i == m_activeIndex);
        result.append(info);
    }
    return result;
}

int OBSSourceManager::activeSceneIndex() const { return m_activeIndex; }

// ── Source management ─────────────────────────────────────────────────────────
bool OBSSourceManager::addSource(const QString &typeId, const QString &name,
                                  const QVariantMap &settings)
{
    obs_scene_t *scene = sceneAt(m_activeIndex);
    if (!scene) return false;

    obs_data_t *obsSettings = obs_data_create();
    for (auto it = settings.constBegin(); it != settings.constEnd(); ++it) {
        const QByteArray key = it.key().toUtf8();
        switch (it.value().typeId()) {
            case QMetaType::QString:
                obs_data_set_string(obsSettings, key.constData(),
                    it.value().toString().toUtf8().constData()); break;
            case QMetaType::Int:
            case QMetaType::LongLong:
                obs_data_set_int(obsSettings, key.constData(), it.value().toLongLong()); break;
            case QMetaType::Double:
                obs_data_set_double(obsSettings, key.constData(), it.value().toDouble()); break;
            case QMetaType::Bool:
                obs_data_set_bool(obsSettings, key.constData(), it.value().toBool()); break;
            default: break;
        }
    }

    obs_source_t *source = obs_source_create(
        typeId.toUtf8().constData(), name.toUtf8().constData(), obsSettings, nullptr);
    obs_data_release(obsSettings);
    if (!source) {
        qWarning() << "[OBSSourceManager] Failed to create source type:" << typeId;
        return false;
    }
    obs_sceneitem_t *item = obs_scene_add(scene, source);
    obs_source_release(source);
    if (!item) {
        qWarning() << "[OBSSourceManager] Failed to add source to scene";
        return false;
    }
    emit sourcesChanged();
    return true;
}

bool OBSSourceManager::addSourceFromJson(const QString &typeId,
                                          const QString &name,
                                          const QString &settingsJson)
{
    obs_scene_t *scene = sceneAt(m_activeIndex);
    if (!scene) return false;

    obs_data_t *obsSettings = nullptr;
    if (!settingsJson.isEmpty() && settingsJson != "{}") {
        obsSettings = obs_data_create_from_json(settingsJson.toUtf8().constData());
    }
    if (!obsSettings) obsSettings = obs_data_create();

    obs_source_t *source = obs_source_create(
        typeId.toUtf8().constData(), name.toUtf8().constData(), obsSettings, nullptr);
    obs_data_release(obsSettings);
    if (!source) {
        qWarning() << "[OBSSourceManager] addSourceFromJson: failed to create" << typeId << name;
        return false;
    }
    obs_sceneitem_t *item = obs_scene_add(scene, source);
    obs_source_release(source);
    if (!item) {
        qWarning() << "[OBSSourceManager] addSourceFromJson: obs_scene_add failed";
        return false;
    }
    emit sourcesChanged();
    return true;
}

bool OBSSourceManager::addSourceAt(const QString &typeId,
                                    const QString &name,
                                    const QString &settingsJson,
                                    QPointF normPos,
                                    QSizeF  normSize)
{
    obs_scene_t *scene = sceneAt(m_activeIndex);
    if (!scene) return false;

    obs_video_info ovi{};
    const bool hasVideo = obs_get_video_info(&ovi);
    const float canvasW = hasVideo ? static_cast<float>(ovi.base_width)  : 1920.f;
    const float canvasH = hasVideo ? static_cast<float>(ovi.base_height) : 1080.f;

    obs_data_t *obsSettings = nullptr;
    if (!settingsJson.isEmpty() && settingsJson != "{}") {
        obsSettings = obs_data_create_from_json(settingsJson.toUtf8().constData());
    }
    if (!obsSettings) obsSettings = obs_data_create();

    obs_source_t *source = obs_source_create(
        typeId.toUtf8().constData(), name.toUtf8().constData(), obsSettings, nullptr);
    obs_data_release(obsSettings);
    if (!source) {
        qWarning() << "[OBSSourceManager] addSourceAt: failed to create" << typeId << name;
        return false;
    }
    obs_sceneitem_t *item = obs_scene_add(scene, source);
    obs_source_release(source);
    if (!item) {
        qWarning() << "[OBSSourceManager] addSourceAt: obs_scene_add failed";
        return false;
    }

    vec2 pos{};
    pos.x = static_cast<float>(normPos.x()) * canvasW;
    pos.y = static_cast<float>(normPos.y()) * canvasH;
    obs_sceneitem_set_pos(item, &pos);

    if (normSize.width() > 0.0 && normSize.height() > 0.0) {
        const uint32_t srcW = obs_source_get_width(source);
        const uint32_t srcH = obs_source_get_height(source);
        if (srcW > 0 && srcH > 0) {
            vec2 scale{};
            scale.x = static_cast<float>(normSize.width())  * canvasW / static_cast<float>(srcW);
            scale.y = static_cast<float>(normSize.height()) * canvasH / static_cast<float>(srcH);
            obs_sceneitem_set_scale(item, &scale);
        }
    }

    emit sourcesChanged();
    return true;
}

bool OBSSourceManager::removeSource(int itemId)
{
    obs_sceneitem_t *item = sceneItemById(itemId);
    if (!item) return false;
    obs_sceneitem_remove(item);
    emit sourcesChanged();
    return true;
}

bool OBSSourceManager::setSourceVisible(int itemId, bool visible)
{
    obs_sceneitem_t *item = sceneItemById(itemId);
    if (!item) return false;
    obs_sceneitem_set_visible(item, visible);
    emit sourcesChanged();
    return true;
}

bool OBSSourceManager::setSourceLocked(int itemId, bool locked)
{
    obs_sceneitem_t *item = sceneItemById(itemId);
    if (!item) return false;
    obs_sceneitem_set_locked(item, locked);
    emit sourcesChanged();
    return true;
}

bool OBSSourceManager::moveSource(int itemId, int delta)
{
    obs_sceneitem_t *item = sceneItemById(itemId);
    if (!item) return false;
    obs_sceneitem_set_order_position(item,
        obs_sceneitem_get_order_position(item) + delta);
    emit sourcesChanged();
    return true;
}

QList<SourceInfo> OBSSourceManager::sourcesInActiveScene() const
{
    QList<SourceInfo> result;
    obs_scene_t *scene = sceneAt(m_activeIndex);
    if (!scene) return result;

    obs_scene_enum_items(scene, [](obs_scene_t*, obs_sceneitem_t *item, void *param) -> bool {
        auto *list = static_cast<QList<SourceInfo>*>(param);
        obs_source_t *src = obs_sceneitem_get_source(item);
        SourceInfo info;
        info.id        = static_cast<int>(obs_sceneitem_get_id(item));
        info.name      = QString::fromUtf8(obs_source_get_name(src));
        info.typeId    = QString::fromUtf8(obs_source_get_id(src));
        info.typeLabel = QString::fromUtf8(obs_source_get_display_name(obs_source_get_id(src)));
        info.visible   = obs_sceneitem_visible(item);
        info.locked    = obs_sceneitem_locked(item);
        info.volume    = static_cast<double>(obs_source_get_volume(src));
        list->prepend(info);
        return true;
    }, &result);

    return result;
}

// ── Single-item accessors ─────────────────────────────────────────────────────
SourceInfo OBSSourceManager::sourceInfoById(int itemId) const
{
    SourceInfo info;
    info.id = -1;
    obs_sceneitem_t *item = sceneItemById(itemId);
    if (!item) return info;

    obs_source_t *src = obs_sceneitem_get_source(item);
    info.id        = itemId;
    info.name      = QString::fromUtf8(obs_source_get_name(src));
    info.typeId    = QString::fromUtf8(obs_source_get_id(src));
    info.typeLabel = QString::fromUtf8(obs_source_get_display_name(obs_source_get_id(src)));
    info.visible   = obs_sceneitem_visible(item);
    info.locked    = obs_sceneitem_locked(item);
    info.volume    = static_cast<double>(obs_source_get_volume(src));
    return info;
}

SceneItemTransform OBSSourceManager::sceneItemTransform(int itemId) const
{
    SceneItemTransform t;
    obs_sceneitem_t *item = sceneItemById(itemId);
    if (!item) return t;

    vec2 pos{}, scale{};
    obs_sceneitem_get_pos(item, &pos);
    obs_sceneitem_get_scale(item, &scale);
    obs_sceneitem_crop crop{};
    obs_sceneitem_get_crop(item, &crop);

    t.posX       = pos.x;
    t.posY       = pos.y;
    t.scaleX     = scale.x;
    t.scaleY     = scale.y;
    t.rotation   = obs_sceneitem_get_rot(item);
    t.cropTop    = crop.top;
    t.cropRight  = crop.right;
    t.cropBottom = crop.bottom;
    t.cropLeft   = crop.left;
    return t;
}

bool OBSSourceManager::applySceneItemTransform(int itemId,
                                                const SceneItemTransform &t)
{
    obs_sceneitem_t *item = sceneItemById(itemId);
    if (!item) return false;

    vec2 pos{};
    pos.x = t.posX;  pos.y = t.posY;
    obs_sceneitem_set_pos(item, &pos);

    vec2 scale{};
    scale.x = t.scaleX;  scale.y = t.scaleY;
    obs_sceneitem_set_scale(item, &scale);

    obs_sceneitem_set_rot(item, t.rotation);

    obs_sceneitem_crop crop{};
    crop.top    = t.cropTop;
    crop.right  = t.cropRight;
    crop.bottom = t.cropBottom;
    crop.left   = t.cropLeft;
    obs_sceneitem_set_crop(item, &crop);

    emit sourcesChanged();
    return true;
}

// ── Source properties ─────────────────────────────────────────────────────────
QVariantMap OBSSourceManager::sourceSettings(int itemId) const
{
    QVariantMap result;
    obs_sceneitem_t *item = sceneItemById(itemId);
    if (!item) return result;

    obs_source_t *src = obs_sceneitem_get_source(item);
    obs_data_t *data  = obs_source_get_settings(src);
    if (!data) return result;

    obs_data_item_t *di = obs_data_first(data);
    while (di) {
        const char *key = obs_data_item_get_name(di);
        switch (obs_data_item_gettype(di)) {
            case OBS_DATA_STRING:
                result[key] = QString::fromUtf8(obs_data_item_get_string(di)); break;
            case OBS_DATA_NUMBER:
                if (obs_data_item_numtype(di) == OBS_DATA_NUM_DOUBLE)
                    result[key] = obs_data_item_get_double(di);
                else
                    result[key] = static_cast<qlonglong>(obs_data_item_get_int(di));
                break;
            case OBS_DATA_BOOLEAN:
                result[key] = obs_data_item_get_bool(di); break;
            default: break;
        }
        obs_data_item_next(&di);
    }
    obs_data_release(data);
    return result;
}

bool OBSSourceManager::applySourceSettings(int itemId, const QVariantMap &settings)
{
    obs_sceneitem_t *item = sceneItemById(itemId);
    if (!item) return false;

    obs_source_t *src  = obs_sceneitem_get_source(item);
    obs_data_t   *data = obs_data_create();

    for (auto it = settings.constBegin(); it != settings.constEnd(); ++it) {
        const QByteArray key = it.key().toUtf8();
        switch (it.value().typeId()) {
            case QMetaType::QString:
                obs_data_set_string(data, key.constData(),
                    it.value().toString().toUtf8().constData()); break;
            case QMetaType::Int:
            case QMetaType::LongLong:
                obs_data_set_int(data, key.constData(), it.value().toLongLong()); break;
            case QMetaType::Double:
                obs_data_set_double(data, key.constData(), it.value().toDouble()); break;
            case QMetaType::Bool:
                obs_data_set_bool(data, key.constData(), it.value().toBool()); break;
            default: break;
        }
    }

    obs_source_update(src, data);
    obs_data_release(data);
    emit sourcesChanged();
    return true;
}

// ── Available source types ────────────────────────────────────────────────────
QList<SourceTypeInfo> OBSSourceManager::availableSourceTypes() const
{
    QList<SourceTypeInfo> result;
    const char *typeId = nullptr;
    size_t idx = 0;
    while (obs_enum_source_types(idx++, &typeId)) {
        if (!typeId) continue;
        uint32_t caps = obs_get_source_output_flags(typeId);
        SourceTypeInfo info;
        info.id          = QString::fromUtf8(typeId);
        info.displayName = QString::fromUtf8(obs_source_get_display_name(typeId));
        if (caps & OBS_SOURCE_VIDEO) {
            if      (info.id.contains("capture"))  info.category = "Capture";
            else if (info.id.contains("image"))    info.category = "Image/Media";
            else if (info.id.contains("browser"))  info.category = "Browser";
            else                                   info.category = "Video";
        } else if (caps & OBS_SOURCE_AUDIO) {
            info.category = "Audio";
        } else {
            info.category = "Other";
        }
        result.append(info);
    }
    return result;
}
