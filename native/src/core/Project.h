#pragma once

#include "core/Types.h"
#include <QObject>
#include <QString>
#include <QVector>
#include <optional>

namespace railshot {

class Project {
public:
    int schemaVersion = kProjectSchemaVersion;
    QString name = QStringLiteral("Untitled Project");
    QString path;
    QVector<SceneItem> scenes;
    QString activeSceneId;
    QString previewSceneId;
    QString programSceneId;
    OutputProfile output;
    QVector<StreamTarget> streamTargets;
    TransitionType transition = TransitionType::Cut;
    int transitionMs = 500;
    QJsonObject extras;

    QJsonObject toJson() const;
    static std::optional<Project> fromJson(const QJsonObject& o, QString* error = nullptr);

    bool saveToFile(const QString& filePath, QString* error = nullptr) const;
    static std::optional<Project> loadFromFile(const QString& filePath, QString* error = nullptr);

    SceneItem* findScene(const QString& id);
    const SceneItem* findScene(const QString& id) const;
    SourceItem* findSource(const QString& sceneId, const QString& sourceId);

    QString addScene(const QString& name = QString());
    bool removeScene(const QString& id);
    QString duplicateScene(const QString& id);
    bool renameScene(const QString& id, const QString& name);
    bool reorderScenes(int from, int to);

    QString addSource(const QString& sceneId, SourceType type, const QString& name);
    bool removeSource(const QString& sceneId, const QString& sourceId);
    bool updateSource(const QString& sceneId, const SourceItem& source);
    bool moveSource(const QString& sceneId, const QString& sourceId, int delta);

    void ensureDefaults();
};

} // namespace railshot
