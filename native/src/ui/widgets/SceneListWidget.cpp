#include "ui/widgets/SceneListWidget.h"
#include "core/EngineController.h"
#include <QContextMenuEvent>
#include <QMenu>
#include <QInputDialog>

namespace railshot {

SceneListWidget::SceneListWidget(EngineController* engine, QWidget* parent)
    : QListWidget(parent), m_engine(engine)
{
    setFixedWidth(180);
    setSelectionMode(QAbstractItemView::SingleSelection);
    connect(this, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        m_engine->setPreviewScene(item->data(Qt::UserRole).toString());
    });
    connect(m_engine->sceneGraph(), &SceneGraph::projectChanged, this, &SceneListWidget::refresh);
    refresh();
}

void SceneListWidget::refresh()
{
    clear();
    const auto p = m_engine->projectSnapshot();
    for (const auto& sc : p.scenes) {
        auto* item = new QListWidgetItem(sc.name, this);
        item->setData(Qt::UserRole, sc.id);
        if (sc.id == p.programSceneId)
            item->setForeground(QColor(QStringLiteral("#FF5A2C")));
        else if (sc.id == p.previewSceneId)
            item->setForeground(QColor(QStringLiteral("#22C55E")));
    }
}

} // namespace railshot
