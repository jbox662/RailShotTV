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
        QColor accent(QStringLiteral("#2A2D35"));
        QColor bg = QColor(QStringLiteral("#0A0C0F"));
        QColor fg(QStringLiteral("#8892A4"));
        if (role == QLatin1String("program")) {
            accent = QColor(QStringLiteral("#FF5A2C"));
            bg = QColor(QStringLiteral("#2A0C0C"));
            fg = QColor(QStringLiteral("#FFB08A"));
        } else if (role == QLatin1String("preview")) {
            accent = QColor(QStringLiteral("#22C55E"));
            bg = QColor(QStringLiteral("#0C1E10"));
            fg = QColor(QStringLiteral("#86EFAC"));
        } else if (role == QLatin1String("active")) {
            accent = QColor(QStringLiteral("#4F9EFF"));
            bg = QColor(QStringLiteral("#0C1A30"));
            fg = QColor(QStringLiteral("#B8D4FF"));
        }
        if (option.state & QStyle::State_MouseOver && role.isEmpty())
            bg = QColor(QStringLiteral("#14181F"));
        painter->fillRect(option.rect, bg);
        painter->fillRect(QRect(option.rect.left(), option.rect.top(), 4, option.rect.height()), accent);

        const int num = index.data(Qt::UserRole + 2).toInt();
        const QString name = index.data(Qt::DisplayRole).toString();

        QRect badge(option.rect.left() + 10, option.rect.center().y() - 8, 16, 16);
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(accent.red(), accent.green(), accent.blue(), 70));
        painter->drawRoundedRect(badge, 3, 3);
        painter->setPen(QPen(accent, 1));
        painter->drawRoundedRect(badge, 3, 3);
        QFont f = option.font;
        f.setBold(true);
        f.setPointSize(8);
        painter->setFont(f);
        painter->setPen(fg);
        painter->drawText(badge, Qt::AlignCenter, QString::number(num));

        QFont nameFont = option.font;
        nameFont.setBold(true);
        nameFont.setPointSize(11);
        painter->setFont(nameFont);
        painter->setPen(fg);
        painter->drawText(option.rect.adjusted(32, 0, -8, 0), Qt::AlignVCenter | Qt::AlignLeft, name);
        painter->restore();
    }
    QSize sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const override
    {
        return QSize(160, 32);
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
        "QListWidget{background:#0A0C0F;border:none;outline:none;}"
        "QListWidget::item{padding:0;min-height:32px;border-bottom:1px solid #1E2228;}"
        "QListWidget::item:hover{background:#14181F;}"
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
