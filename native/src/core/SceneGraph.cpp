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
        m_project.activeSceneId = id;
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
