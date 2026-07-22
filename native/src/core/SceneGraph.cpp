#include "core/SceneGraph.h"

namespace railshot {

SceneGraph::SceneGraph(QObject* parent)
    : QObject(parent)
{
    m_project.ensureDefaults();
}

Project SceneGraph::snapshot() const
{
    QMutexLocker lock(&m_mutex);
    return m_project;
}

void SceneGraph::applyLocked(const Project& project)
{
    m_project = project;
    m_project.ensureDefaults();
    m_silentActive = false;
}

void SceneGraph::replace(const Project& project)
{
    {
        QMutexLocker lock(&m_mutex);
        applyLocked(project);
        m_history.clear();
    }
    emit historyChanged();
    emit projectChanged();
}

void SceneGraph::applySnapshot(const Project& project)
{
    {
        QMutexLocker lock(&m_mutex);
        applyLocked(project);
    }
    emit projectChanged();
}

void SceneGraph::mutate(const std::function<void(Project&)>& fn)
{
    bool histChanged = false;
    {
        QMutexLocker lock(&m_mutex);
        // If a silent drag was in progress, commit its undo point first.
        if (m_silentActive) {
            m_history.pushBefore(m_silentBefore);
            m_silentActive = false;
            histChanged = true;
        }
        const Project before = m_project;
        fn(m_project);
        m_history.pushBefore(before);
        histChanged = true;
    }
    if (histChanged)
        emit historyChanged();
    emit projectChanged();
}

void SceneGraph::mutateSilent(const std::function<void(Project&)>& fn)
{
    QMutexLocker lock(&m_mutex);
    if (!m_silentActive) {
        m_silentBefore = m_project;
        m_silentActive = true;
    }
    fn(m_project);
}

void SceneGraph::notifyProjectChanged()
{
    bool histChanged = false;
    {
        QMutexLocker lock(&m_mutex);
        if (m_silentActive) {
            m_history.pushBefore(m_silentBefore);
            m_silentActive = false;
            histChanged = true;
        }
    }
    if (histChanged)
        emit historyChanged();
    emit projectChanged();
}

bool SceneGraph::canUndo() const
{
    QMutexLocker lock(&m_mutex);
    return m_history.canUndo();
}

bool SceneGraph::canRedo() const
{
    QMutexLocker lock(&m_mutex);
    return m_history.canRedo();
}

void SceneGraph::clearHistory()
{
    {
        QMutexLocker lock(&m_mutex);
        m_history.clear();
        m_silentActive = false;
    }
    emit historyChanged();
}

bool SceneGraph::undo()
{
    Project next;
    {
        QMutexLocker lock(&m_mutex);
        if (!m_history.canUndo())
            return false;
        next = m_history.undo(m_project);
        applyLocked(next);
    }
    emit historyChanged();
    emit projectChanged();
    return true;
}

bool SceneGraph::redo()
{
    Project next;
    {
        QMutexLocker lock(&m_mutex);
        if (!m_history.canRedo())
            return false;
        next = m_history.redo(m_project);
        applyLocked(next);
    }
    emit historyChanged();
    emit projectChanged();
    return true;
}

QString SceneGraph::previewSceneId() const
{
    QMutexLocker lock(&m_mutex);
    return m_project.previewSceneId;
}

QString SceneGraph::programSceneId() const
{
    QMutexLocker lock(&m_mutex);
    return m_project.programSceneId;
}

void SceneGraph::setPreviewSceneId(const QString& id)
{
    {
        QMutexLocker lock(&m_mutex);
        m_project.previewSceneId = id;
        // Arming Preview also selects the edit scene (OBS current scene).
        // Clearing Preview must NOT wipe activeSceneId — sources still need a home.
        if (!id.isEmpty())
            m_project.activeSceneId = id;
        else if (m_project.activeSceneId.isEmpty() && !m_project.scenes.isEmpty())
            m_project.activeSceneId = m_project.scenes.first().id;
    }
    emit previewChanged(id);
    emit projectChanged();
}

void SceneGraph::setProgramSceneId(const QString& id)
{
    {
        QMutexLocker lock(&m_mutex);
        m_project.programSceneId = id;
    }
    emit programChanged(id);
    emit projectChanged();
}

void SceneGraph::setTransition(TransitionType type, int durationMs)
{
    {
        QMutexLocker lock(&m_mutex);
        m_project.transition = type;
        m_project.transitionMs = durationMs;
    }
    emit transitionChanged(type, durationMs);
}

} // namespace railshot
