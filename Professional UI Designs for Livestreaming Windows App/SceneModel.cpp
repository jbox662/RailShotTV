#include "SceneModel.h"

SceneModel::SceneModel(QObject *parent)
    : QObject(parent)
{
    // Scenes populated at runtime from OBS via loadFromOBS() / addScene()
    m_activeIndex = -1;
}

void SceneModel::setActiveScene(int index)
{
    if (index < 0 || index >= m_scenes.size()) return;
    if (m_activeIndex == index) return;
    m_activeIndex = index;
    emit activeSceneChanged(index);
}

void SceneModel::addScene(const QString &name)
{
    m_scenes.append({ name, {} });
    emit scenesChanged();
}

void SceneModel::removeScene(int index)
{
    if (index < 0 || index >= m_scenes.size()) return;
    m_scenes.removeAt(index);
    if (m_activeIndex >= m_scenes.size())
        m_activeIndex = m_scenes.size() - 1;
    emit scenesChanged();
}

void SceneModel::renameScene(int index, const QString &name)
{
    if (index < 0 || index >= m_scenes.size()) return;
    m_scenes[index].name = name;
    emit scenesChanged();
}
