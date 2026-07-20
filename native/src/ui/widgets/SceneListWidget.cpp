#include "ui/widgets/SceneListWidget.h"
#include "core/EngineController.h"
#include "core/SceneGraph.h"
#include <QMenu>
#include <QInputDialog>
#include <QLineEdit>
#include <QAction>
#include <QPainter>
#include <QStyledItemDelegate>

namespace railshot {

namespace {
class SceneItemDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        painter->save();
        const QString role = index.data(Qt::UserRole + 1).toString();
        QColor accent(QStringLiteral("#3A3F48"));
        QColor bg = QColor(QStringLiteral("#1A1D21"));
        QColor fg(QStringLiteral("#B8BCC4"));
        if (role == QLatin1String("program")) {
            accent = QColor(QStringLiteral("#C44A2A"));
            bg = QColor(QStringLiteral("#241A1A"));
            fg = QColor(QStringLiteral("#E8E8E8"));
        } else if (role == QLatin1String("preview")) {
            accent = QColor(QStringLiteral("#2E8B57"));
            bg = QColor(QStringLiteral("#1A221C"));
            fg = QColor(QStringLiteral("#E8E8E8"));
        } else if (role == QLatin1String("active")) {
            accent = QColor(QStringLiteral("#4A6FA5"));
            bg = QColor(QStringLiteral("#1C2028"));
            fg = QColor(QStringLiteral("#E8E8E8"));
        }
        if (option.state & QStyle::State_MouseOver && role.isEmpty())
            bg = QColor(QStringLiteral("#22262E"));
        painter->fillRect(option.rect, bg);
        painter->fillRect(QRect(option.rect.left(), option.rect.top(), 3, option.rect.height()), accent);

        const int num = index.data(Qt::UserRole + 2).toInt();
        const QString name = index.data(Qt::DisplayRole).toString();
        QFont f = option.font;
        f.setPointSize(10);
        f.setBold(false);
        painter->setFont(f);
        painter->setPen(fg);
        painter->drawText(option.rect.adjusted(10, 0, -8, 0), Qt::AlignVCenter | Qt::AlignLeft,
                          QStringLiteral("%1. %2").arg(num).arg(name));
        painter->restore();
    }
    QSize sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const override
    {
        return QSize(160, 26);
    }
};
} // namespace

SceneListWidget::SceneListWidget(EngineController* engine, QWidget* parent)
    : QListWidget(parent), m_engine(engine)
{
    setMinimumWidth(120);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setItemDelegate(new SceneItemDelegate(this));
    setUniformItemSizes(true);
    setSpacing(0);
    setFocusPolicy(Qt::StrongFocus);
    setStyleSheet(QStringLiteral(
        "QListWidget{background:#1A1D21;border:none;outline:none;}"
        "QListWidget::item{padding:0;min-height:26px;border-bottom:1px solid #242830;}"
        "QListWidget::item:hover{background:#22262E;}"
        "QListWidget::item:selected{background:transparent;}"));

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
        auto* empty = new QListWidgetItem(QStringLiteral("◌  No scenes yet — click +"), this);
        empty->setFlags(Qt::NoItemFlags);
        empty->setForeground(QColor(QStringLiteral("#606878")));
        return;
    }
    int n = 0;
    for (const auto& sc : p.scenes) {
        auto* item = new QListWidgetItem(sc.name, this);
        item->setData(Qt::UserRole, sc.id);
        item->setData(Qt::UserRole + 2, ++n);
        if (sc.id == p.programSceneId) {
            item->setData(Qt::UserRole + 1, QStringLiteral("program"));
            item->setToolTip(QStringLiteral("Program"));
        } else if (sc.id == p.previewSceneId) {
            item->setData(Qt::UserRole + 1, QStringLiteral("preview"));
            item->setToolTip(QStringLiteral("Preview"));
        } else if (sc.id == p.activeSceneId) {
            item->setData(Qt::UserRole + 1, QStringLiteral("active"));
        } else {
            item->setData(Qt::UserRole + 1, QStringLiteral("idle"));
        }
    }
}

} // namespace railshot
