#include "ui/widgets/SceneListWidget.h"
#include "core/EngineController.h"
#include "core/SceneGraph.h"
#include <QMenu>
#include <QInputDialog>

namespace railshot {

SceneListWidget::SceneListWidget(EngineController* engine, QWidget* parent)
    : QListWidget(parent), m_engine(engine)
{
    setFixedWidth(180);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setStyleSheet(QStringLiteral(
        "QListWidget {"
        "  background:#0A0C0F;"
        "  border: none;"
        "  border-right: 1px solid #1A1D24;"
        "  border-top: 2px solid #4F9EFF;"
        "}"
        "QListWidget::item {"
        "  padding: 10px 12px;"
        "  border-bottom: 1px solid #15181E;"
        "  border-left: 3px solid transparent;"
        "  color: #C8CAD0;"
        "  font-weight: 600;"
        "}"
        "QListWidget::item:selected {"
        "  background: #122033;"
        "  color: #4F9EFF;"
        "  border-left: 3px solid #4F9EFF;"
        "}"
        "QListWidget::item:hover { background: #12151A; }"));

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
        if (sc.id == p.programSceneId) {
            item->setForeground(QColor(QStringLiteral("#FF5A2C")));
            item->setToolTip(QStringLiteral("Program"));
        } else if (sc.id == p.previewSceneId) {
            item->setForeground(QColor(QStringLiteral("#22C55E")));
            item->setToolTip(QStringLiteral("Preview"));
        }
    }
}

} // namespace railshot
