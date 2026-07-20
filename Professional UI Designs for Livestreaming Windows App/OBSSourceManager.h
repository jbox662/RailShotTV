#pragma once
/**
 * OBSSourceManager — Scene and source management for RailShotTV
 *
 * Wraps the libobs scene/source API with Qt-friendly signals and a clean
 * interface for the UI layer (DashboardPage, SceneEditorPage).
 *
 * Responsibilities:
 *  - Create, rename, delete, and reorder scenes
 *  - Add, remove, and reorder sources within a scene
 *  - Switch the active (program) scene
 *  - Enumerate available source types (capture cards, windows, images, etc.)
 *  - Provide source property access for the properties panel
 *
 * All obs_scene_t / obs_source_t / obs_sceneitem_t objects are managed
 * internally; callers work with integer IDs and QString names only.
 */

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QList>
#include <QPointF>
#include <QSizeF>

struct obs_scene;
struct obs_source;
struct obs_sceneitem;

// ── Data structs passed to the UI ─────────────────────────────────────────────

/** Describes one scene in the scene collection */
struct SceneInfo {
    int     id;       // internal index
    QString name;
    bool    active;   // true = currently on program output
};

/** Describes one source item within a scene */
struct SourceInfo {
    int     id;           // obs_sceneitem_id
    QString name;         // obs_source_get_name
    QString typeId;       // obs_source_get_id (e.g. "monitor_capture")
    QString typeLabel;    // human-readable type name
    bool    visible;      // obs_sceneitem_visible
    bool    locked;       // obs_sceneitem_locked
    double  volume;       // 0.0 – 1.0 (for audio sources)
};

/**
 * Transform/geometry data for a scene item.
 * All positions and sizes are in canvas pixel coordinates.
 * Scale is a multiplier (1.0 = 100 %).
 */
struct SceneItemTransform {
    float posX     = 0.f;   // obs_sceneitem_get_pos
    float posY     = 0.f;
    float scaleX   = 1.f;   // obs_sceneitem_get_scale
    float scaleY   = 1.f;
    float rotation = 0.f;   // degrees, obs_sceneitem_get_rot
    int   cropTop  = 0;     // obs_sceneitem_get_crop
    int   cropRight= 0;
    int   cropBottom=0;
    int   cropLeft = 0;
    float opacity  = 1.f;   // obs_sceneitem_get_blending_method (approximated)
};

/** Available source type for the "Add Source" dialog */
struct SourceTypeInfo {
    QString id;           // obs_source_get_id
    QString displayName;  // obs_source_get_display_name
    QString category;     // "Video Capture", "Audio Capture", "Image", etc.
};

class OBSSourceManager : public QObject
{
    Q_OBJECT

public:
    static OBSSourceManager &instance();

    // ── Scene management ──────────────────────────────────────────────────

    /** Create a new empty scene with the given name. */
    bool createScene(const QString &name);

    /** Delete the scene at the given index. Cannot delete the last scene. */
    bool deleteScene(int index);

    /** Rename a scene. */
    bool renameScene(int index, const QString &newName);

    /** Move a scene up or down in the list. */
    bool moveScene(int fromIndex, int toIndex);

    /** Switch the program output to the scene at the given index. */
    void switchToScene(int index);

    /** Return all scenes in order. */
    QList<SceneInfo> scenes() const;

    /** Return the index of the currently active scene. */
    int activeSceneIndex() const;

    // ── Source management ─────────────────────────────────────────────────

    /**
     * Add a source to the active scene.
     * @param typeId   libobs source type ID (e.g. "monitor_capture", "dshow_input")
     * @param name     display name for the source
     * @param settings initial settings (optional)
     * @return true on success
     */
    bool addSource(const QString &typeId, const QString &name,
                   const QVariantMap &settings = {});

    /**
     * Convenience overload: parse a JSON string into obs_data_t and add the
     * source.  The JSON string is the same format stored in
     * OverlayTemplate::defaultSettingsJson.
     */
    bool addSourceFromJson(const QString &typeId, const QString &name,
                           const QString &settingsJson);

    /**
     * Add a source and immediately position its scene item.
     *
     * @param typeId       libobs source type ID
     * @param name         display name
     * @param settingsJson JSON settings string (from OverlayTemplate)
     * @param normPos      normalised position [0..1, 0..1] relative to canvas
     *                     (converted to pixel coords using the current output size)
     * @param normSize     normalised size [0..1, 0..1]; pass {0,0} for source default
     * @return true on success
     */
    bool addSourceAt(const QString &typeId, const QString &name,
                     const QString &settingsJson,
                     QPointF normPos, QSizeF normSize = {});

    /** Remove a source item from the active scene by its item ID. */
    bool removeSource(int itemId);

    /** Toggle visibility of a source item. */
    bool setSourceVisible(int itemId, bool visible);

    /** Toggle lock state of a source item. */
    bool setSourceLocked(int itemId, bool locked);

    /** Move a source item up or down in the z-order. */
    bool moveSource(int itemId, int delta);

    /** Return all sources in the active scene. */
    QList<SourceInfo> sourcesInActiveScene() const;

    /**
     * Return the SourceInfo for a single item by its scene-item ID.
     * Returns a default-constructed SourceInfo (id == -1) if not found.
     */
    SourceInfo sourceInfoById(int itemId) const;

    /**
     * Return the transform/geometry for a scene item.
     * Returns a default-constructed SceneItemTransform if not found.
     */
    SceneItemTransform sceneItemTransform(int itemId) const;

    /**
     * Apply a new transform to a scene item.
     * Only non-zero/non-negative fields are applied; pass -1 for fields to skip.
     */
    bool applySceneItemTransform(int itemId, const SceneItemTransform &t);

    // ── Source properties ─────────────────────────────────────────────────

    /**
     * Return the current settings for a source as a QVariantMap.
     * Keys match the libobs property names for the source type.
     * DYNAMIC: populated from obs_source_get_settings()
     */
    QVariantMap sourceSettings(int itemId) const;

    /**
     * Apply new settings to a source.
     * DYNAMIC: calls obs_source_update()
     */
    bool applySourceSettings(int itemId, const QVariantMap &settings);

    // ── Available source types ────────────────────────────────────────────

    /**
     * Return all source types available on this system.
     * Populated by enumerating obs_enum_source_types().
     * DYNAMIC: depends on which libobs plugins are loaded.
     */
    QList<SourceTypeInfo> availableSourceTypes() const;

    // ── Convenience: common source type IDs ──────────────────────────────
    // These are the libobs plugin IDs for the most common Windows sources.
    // Use these with addSource().
    static constexpr const char *SOURCE_MONITOR_CAPTURE = "monitor_capture";
    static constexpr const char *SOURCE_WINDOW_CAPTURE  = "window_capture";
    static constexpr const char *SOURCE_GAME_CAPTURE    = "game_capture";
    static constexpr const char *SOURCE_DSHOW_VIDEO     = "dshow_input";      // webcam / capture card
    static constexpr const char *SOURCE_WASAPI_INPUT    = "wasapi_input_capture";   // microphone
    static constexpr const char *SOURCE_WASAPI_OUTPUT   = "wasapi_output_capture";  // desktop audio
    static constexpr const char *SOURCE_IMAGE           = "image_source";
    static constexpr const char *SOURCE_COLOR           = "color_source";
    static constexpr const char *SOURCE_TEXT            = "text_gdiplus";     // Windows GDI+ text
    static constexpr const char *SOURCE_MEDIA           = "ffmpeg_source";    // video file playback
    static constexpr const char *SOURCE_BROWSER         = "browser_source";   // Chromium browser source

signals:
    /** Emitted when the scene list changes (add/remove/rename/reorder). */
    void scenesChanged();

    /** Emitted when the active scene switches. */
    void activeSceneChanged(int newIndex, const QString &name);

    /** Emitted when sources in the active scene change. */
    void sourcesChanged();

private:
    explicit OBSSourceManager(QObject *parent = nullptr);
    ~OBSSourceManager() override = default;
    OBSSourceManager(const OBSSourceManager &) = delete;
    OBSSourceManager &operator=(const OBSSourceManager &) = delete;

    obs_scene    *sceneAt(int index) const;
    obs_sceneitem *sceneItemById(int itemId) const;

    // Ordered list of scenes — obs_scene_t* are reference-counted by libobs
    QList<obs_scene *> m_scenes;
    int                m_activeIndex = 0;
};
