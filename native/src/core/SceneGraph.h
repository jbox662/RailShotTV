#pragma once

#include "core/Project.h"
#include "core/UndoStack.h"
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
    /// Replace live project and clear undo/redo (New / Open).
    void replace(const Project& project);
    /// Replace without touching history (Save path sync, undo/redo apply).
    void applySnapshot(const Project& project);
    void mutate(const std::function<void(Project&)>& fn);
    /// Mutate without emitting projectChanged (live preview drag — OBS-smooth).
    void mutateSilent(const std::function<void(Project&)>& fn);
    /// End a silent drag: push undo point from drag start, then notify.
    void notifyProjectChanged();

    bool canUndo() const;
    bool canRedo() const;
    bool undo();
    bool redo();
    void clearHistory();

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
    void historyChanged();

private:
    void applyLocked(const Project& project);

    mutable QMutex m_mutex;
    Project m_project;
    UndoStack m_history;
    bool m_silentActive = false;
    Project m_silentBefore;
};

} // namespace railshot
