#include "ui/widgets/SceneListWidget.h"
#include "core/EngineController.h"
#include "core/SceneGraph.h"
#include "core/Types.h"
#include <QMenu>
#include <QInputDialog>
#include <QLineEdit>
#include <QAction>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QDialog>
#include <QFormLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <QVBoxLayout>

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
            menu.addAction(QStringLiteral("Transition Override…"), this, [this, id] {
                const auto snap = m_engine->projectSnapshot();
                const auto* sc = snap.findScene(id);
                if (!sc) return;
                QDialog dlg(this);
                dlg.setWindowTitle(QStringLiteral("Transition Override — %1").arg(sc->name));
                auto* lay = new QVBoxLayout(&dlg);
                auto* form = new QFormLayout;
                auto* enable = new QCheckBox(QStringLiteral("Override transition when taking to this scene"), &dlg);
                enable->setChecked(sc->transitionOverride);
                auto* typeBox = new QComboBox(&dlg);
                const TransitionType catalog[] = {
                    TransitionType::Fade, TransitionType::CrossDissolve, TransitionType::Wipe,
                    TransitionType::Merge, TransitionType::Push, TransitionType::Cut,
                    TransitionType::FTB, TransitionType::FadeToWhite, TransitionType::Stinger,
                    TransitionType::CubeZoom, TransitionType::Swap, TransitionType::FlyOver,
                };
                for (auto t : catalog)
                    typeBox->addItem(transitionTypeToString(t), int(t));
                const int curIdx = typeBox->findData(int(sc->transition));
                typeBox->setCurrentIndex(curIdx >= 0 ? curIdx : 0);
                auto* ms = new QSpinBox(&dlg);
                ms->setRange(50, 10000);
                ms->setSuffix(QStringLiteral(" ms"));
                ms->setValue(sc->transitionMs > 0 ? sc->transitionMs : 500);
                form->addRow(enable);
                form->addRow(QStringLiteral("Type"), typeBox);
                form->addRow(QStringLiteral("Duration"), ms);
                lay->addLayout(form);
                auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
                lay->addWidget(buttons);
                connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
                connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
                if (dlg.exec() != QDialog::Accepted) return;
                m_engine->sceneGraph()->mutate([&](Project& p) {
                    if (auto* s = p.findScene(id)) {
                        s->transitionOverride = enable->isChecked();
                        s->transition = TransitionType(typeBox->currentData().toInt());
                        s->transitionMs = ms->value();
                    }
                });
            });
            menu.addAction(QStringLiteral("Clear Transition Override"), this, [this, id] {
                m_engine->sceneGraph()->mutate([&](Project& p) {
                    if (auto* s = p.findScene(id))
                        s->transitionOverride = false;
                });
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
