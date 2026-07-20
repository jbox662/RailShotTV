#include "ui/widgets/SceneListWidget.h"
#include "core/EngineController.h"
#include "core/SceneGraph.h"
#include <QMenu>
#include <QInputDialog>
#include <QLineEdit>
#include <QLinearGradient>
#include <QBrush>
#include <QAction>

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
        "  outline: none;"
        "}"
        "QListWidget::item {"
        "  padding: 10px 12px;"
        "  border-bottom: 1px solid #15181E;"
        "  border-left: 3px solid transparent;"
        "  color: #8892A4;"
        "  font-weight: 600;"
        "  font-size: 12px;"
        "}"
        "QListWidget::item:hover { background: #12151A; }"
        "QListWidget::item:selected { background: transparent; }"));

    connect(this, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        m_engine->setPreviewScene(item->data(Qt::UserRole).toString());
    });
    connect(this, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
        m_engine->setProgramScene(item->data(Qt::UserRole).toString());
    });
    connect(m_engine->sceneGraph(), &SceneGraph::projectChanged, this, &SceneListWidget::refresh);
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        auto* item = itemAt(pos);
        QMenu menu;
        menu.addAction(QStringLiteral("Add Scene"), this, [this] {
            m_engine->sceneGraph()->mutate([](Project& p) { p.addScene(); });
        });
        if (item) {
            const QString id = item->data(Qt::UserRole).toString();
            menu.addAction(QStringLiteral("Set Preview"), this, [this, id] { m_engine->setPreviewScene(id); });
            menu.addAction(QStringLiteral("Set Program"), this, [this, id] { m_engine->setProgramScene(id); });
            menu.addAction(QStringLiteral("Rename"), this, [this, id, item] {
                bool ok = false;
                const QString name = QInputDialog::getText(this, QStringLiteral("Rename Scene"),
                                                          QStringLiteral("Name"), QLineEdit::Normal,
                                                          item->text(), &ok);
                if (ok && !name.trimmed().isEmpty())
                    m_engine->sceneGraph()->mutate([&](Project& p) { p.renameScene(id, name.trimmed()); });
            });
            menu.addAction(QStringLiteral("Duplicate"), this, [this, id] {
                m_engine->sceneGraph()->mutate([&](Project& p) { p.duplicateScene(id); });
            });
            auto* del = menu.addAction(QStringLiteral("Delete"));
            connect(del, &QAction::triggered, this, [this, id] {
                m_engine->sceneGraph()->mutate([&](Project& p) { p.removeScene(id); });
            });
        }
        menu.exec(mapToGlobal(pos));
    });
    refresh();
}

void SceneListWidget::refresh()
{
    clear();
    const auto p = m_engine->projectSnapshot();
    if (p.scenes.isEmpty()) {
        auto* empty = new QListWidgetItem(QStringLiteral("No scenes yet. Click + to add one."), this);
        empty->setFlags(Qt::NoItemFlags);
        empty->setForeground(QColor(QStringLiteral("#404450")));
        return;
    }
    for (const auto& sc : p.scenes) {
        auto* item = new QListWidgetItem(sc.name, this);
        item->setData(Qt::UserRole, sc.id);
        QLinearGradient g(0, 0, 0, 1);
        g.setCoordinateMode(QGradient::ObjectBoundingMode);
        if (sc.id == p.programSceneId) {
            g.setColorAt(0, QColor(QStringLiteral("#2A0A0A")));
            g.setColorAt(1, QColor(QStringLiteral("#1A0808")));
            item->setBackground(QBrush(g));
            item->setForeground(QColor(QStringLiteral("#FF8A6A")));
            item->setData(Qt::UserRole + 1, QStringLiteral("program"));
            item->setToolTip(QStringLiteral("Program"));
        } else if (sc.id == p.previewSceneId) {
            g.setColorAt(0, QColor(QStringLiteral("#0A1A0A")));
            g.setColorAt(1, QColor(QStringLiteral("#081208")));
            item->setBackground(QBrush(g));
            item->setForeground(QColor(QStringLiteral("#6EE7A0")));
            item->setData(Qt::UserRole + 1, QStringLiteral("preview"));
            item->setToolTip(QStringLiteral("Preview"));
        } else if (sc.id == p.activeSceneId) {
            g.setColorAt(0, QColor(QStringLiteral("#0D1A2A")));
            g.setColorAt(1, QColor(QStringLiteral("#0A1220")));
            item->setBackground(QBrush(g));
            item->setForeground(QColor(QStringLiteral("#C8DAFF")));
            item->setData(Qt::UserRole + 1, QStringLiteral("active"));
        } else {
            item->setForeground(QColor(QStringLiteral("#8892A4")));
            item->setData(Qt::UserRole + 1, QStringLiteral("idle"));
        }
    }
}

} // namespace railshot
