#pragma once

#include "core/Project.h"
#include <QObject>
#include <QMutex>
#include <functional>

namespace railshot {

/// Thread-safe ownership of the live Project / scene graph.
/// UI and engine workers exchange immutable snapshots.
class SceneGraph : public QObject {
    Q_OBJECT
public:
    explicit SceneGraph(QObject* parent = nullptr);

    Project snapshot() const;
    void replace(const Project& project);
    void mutate(const std::function<void(Project&)>& fn);
    /// Mutate without emitting projectChanged (live preview drag — OBS-smooth).
    void mutateSilent(const std::function<void(Project&)>& fn);
    void notifyProjectChanged() { emit projectChanged(); }

    QString previewSceneId() const;
    QString programSceneId() const;
    void setPreviewSceneId(const QString& id);
    void setProgramSceneId(const QString& id);
    void setTransition(TransitionType type, int durationMs);

signals:
    void projectChanged();
    void previewChanged(const QString& sceneId);
    void programChanged(const QString& sceneId);
    void transitionChanged(TransitionType type, int durationMs);

private:
    mutable QMutex m_mutex;
    Project m_project;
};

} // namespace railshot
