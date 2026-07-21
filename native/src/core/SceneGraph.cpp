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

void SceneGraph::replace(const Project& project)
{
    {
        QMutexLocker lock(&m_mutex);
        m_project = project;
        m_project.ensureDefaults();
    }
    emit projectChanged();
}

void SceneGraph::mutate(const std::function<void(Project&)>& fn)
{
    {
        QMutexLocker lock(&m_mutex);
        fn(m_project);
    }
    emit projectChanged();
}

void SceneGraph::mutateSilent(const std::function<void(Project&)>& fn)
{
    QMutexLocker lock(&m_mutex);
    fn(m_project);
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
