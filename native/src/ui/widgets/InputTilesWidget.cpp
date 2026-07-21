#include "ui/widgets/InputTilesWidget.h"
#include "ui/widgets/AddSourceDialog.h"
#include "ui/widgets/FiltersDialog.h"
#include "ui/widgets/TransformDialog.h"
#include "ui/widgets/InteractDialog.h"
#include "core/EngineController.h"
#include "core/SceneGraph.h"
#include "core/Types.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QFrame>
#include <QLabel>
#include <QToolButton>
#include <QPushButton>
#include <QMenu>
#include <QAction>
#include <QMouseEvent>
#include <QCursor>
#include <QStyle>
#include <QInputDialog>
#include <QLineEdit>
#include <QColor>
#include <QPixmap>
#include <QIcon>
#include <QJsonArray>
#include <functional>

namespace railshot {

namespace {

QString typeIconGlyph(SourceType t)
{
    switch (t) {
    case SourceType::Browser: return QStringLiteral("🌐");
    case SourceType::Camera: return QStringLiteral("📷");
    case SourceType::Display: return QStringLiteral("🖥");
    case SourceType::Window: return QStringLiteral("🗔");
    case SourceType::Game: return QStringLiteral("🎮");
    case SourceType::Image: return QStringLiteral("🖼");
    case SourceType::Text: return QStringLiteral("T");
    case SourceType::Media: return QStringLiteral("▶");
    case SourceType::Ndi: return QStringLiteral("📡");
    case SourceType::Color: return QStringLiteral("◼");
    case SourceType::AudioInput: return QStringLiteral("🎤");
    case SourceType::AudioOutput: return QStringLiteral("🔊");
    case SourceType::Scoreboard: return QStringLiteral("🏆");
    case SourceType::LowerThird: return QStringLiteral("▭");
    case SourceType::Alert: return QStringLiteral("⚠");
    default: return QStringLiteral("◇");
    }
}

const QStringList& obsColorTags()
{
    static const QStringList k = {
        QStringLiteral("#1A1A1A"), QStringLiteral("#EF4444"), QStringLiteral("#22C55E"),
        QStringLiteral("#FBBF24"), QStringLiteral("#EC4899"), QStringLiteral("#3B82F6"),
        QStringLiteral("#A855F7"), QStringLiteral("#22D3EE"),
    };
    return k;
}

QString sourceBarStyle(const QString& colorHex, bool /*selected*/)
{
    QColor c(colorHex.isEmpty() ? QStringLiteral("#4F9EFF") : colorHex);
    if (!c.isValid())
        c = QColor(QStringLiteral("#4F9EFF"));
    const QString accent = c.name();
    const QString tint = QStringLiteral("rgba(%1,%2,%3,38)").arg(c.red()).arg(c.green()).arg(c.blue());
    const QString tintSel = QStringLiteral("rgba(%1,%2,%3,72)").arg(c.red()).arg(c.green()).arg(c.blue());
    return QStringLiteral(
               "QFrame#sourceBar{"
               "  background:%1; border:1px solid transparent; border-left:3px solid %2; border-radius:2px;"
               "}"
               "QFrame#sourceBar[selected=\"true\"]{"
               "  background:%3; border:1px solid #4F9EFF; border-left:3px solid %2;"
               "}"
               "QFrame#sourceBar:hover{background:%3;}")
        .arg(tint, accent, tintSel);
}

/// OBS SourceTreeItem-style row: [icon] name ………… [eye] [lock]
class SourceBar : public QFrame {
public:
    QString sourceId;
    std::function<void()> onSelect;
    std::function<void()> onConfigure;

    SourceBar(QWidget* parent = nullptr)
        : QFrame(parent)
    {
        setObjectName(QStringLiteral("sourceBar"));
        setCursor(Qt::PointingHandCursor);
        setFixedHeight(32);
        setMouseTracking(true);
    }

protected:
    void mousePressEvent(QMouseEvent* e) override
    {
        if (e->button() == Qt::LeftButton && onSelect)
            onSelect();
        QFrame::mousePressEvent(e);
    }
    void mouseDoubleClickEvent(QMouseEvent* e) override
    {
        if (onConfigure)
            onConfigure();
        QFrame::mouseDoubleClickEvent(e);
    }
};

QToolButton* makeToolBtn(const QString& text, const QString& tip, QWidget* parent)
{
    auto* b = new QToolButton(parent);
    b->setText(text);
    b->setToolTip(tip);
    b->setAutoRaise(true);
    b->setFixedSize(28, 28);
    b->setCursor(Qt::PointingHandCursor);
    b->setStyleSheet(QStringLiteral(
        "QToolButton{background:transparent;color:#C8CAD0;border:none;font-size:14px;font-weight:700;}"
        "QToolButton:hover{color:#FFFFFF;background:#2A2D35;border-radius:3px;}"
        "QToolButton:disabled{color:#505860;}"));
    return b;
}

} // namespace

InputTilesWidget::InputTilesWidget(EngineController* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setObjectName(QStringLiteral("inputTiles"));
    setMinimumWidth(180);
    setStyleSheet(QStringLiteral(
        "QWidget#inputTiles{background:#1A1E24;border:none;}"
        "QScrollArea{background:transparent;border:none;}"
        "QWidget#sourcesListHost{background:#1A1E24;border:none;}"
        "QLabel{background:transparent;border:none;}"));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    m_scroll = new QScrollArea(this);
    m_scroll->setWidgetResizable(true);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scroll->setFrameShape(QFrame::NoFrame);

    m_listHost = new QWidget(m_scroll);
    m_listHost->setObjectName(QStringLiteral("sourcesListHost"));
    m_listLay = new QVBoxLayout(m_listHost);
    m_listLay->setContentsMargins(4, 4, 4, 4);
    m_listLay->setSpacing(2);
    m_listLay->addStretch(1);
    m_scroll->setWidget(m_listHost);
    root->addWidget(m_scroll, 1);

    auto* tools = new QWidget(this);
    tools->setFixedHeight(36);
    tools->setStyleSheet(QStringLiteral(
        "background:#14181E; border-top:1px solid #2A2D35;"));
    auto* toolsLay = new QHBoxLayout(tools);
    toolsLay->setContentsMargins(6, 4, 6, 4);
    toolsLay->setSpacing(2);

    auto* addBtn = makeToolBtn(QStringLiteral("+"), QStringLiteral("Add Source"), tools);
    m_addBtn = addBtn;
    auto* remBtn = makeToolBtn(QStringLiteral("🗑"), QStringLiteral("Remove Source"), tools);
    auto* propsBtn = makeToolBtn(QStringLiteral("⚙"), QStringLiteral("Properties"), tools);
    auto* upBtn = makeToolBtn(QStringLiteral("▲"), QStringLiteral("Move Up"), tools);
    auto* downBtn = makeToolBtn(QStringLiteral("▼"), QStringLiteral("Move Down"), tools);

    connect(addBtn, &QToolButton::clicked, this, &InputTilesWidget::onAddClicked);
    connect(remBtn, &QToolButton::clicked, this, &InputTilesWidget::onRemove);
    connect(propsBtn, &QToolButton::clicked, this, &InputTilesWidget::onProperties);
    connect(upBtn, &QToolButton::clicked, this, &InputTilesWidget::onMoveUp);
    connect(downBtn, &QToolButton::clicked, this, &InputTilesWidget::onMoveDown);

    toolsLay->addWidget(addBtn);
    toolsLay->addWidget(remBtn);
    toolsLay->addSpacing(8);
    toolsLay->addWidget(propsBtn);
    toolsLay->addSpacing(8);
    toolsLay->addWidget(upBtn);
    toolsLay->addWidget(downBtn);
    toolsLay->addStretch(1);
    root->addWidget(tools);

    connect(m_engine->sceneGraph(), &SceneGraph::projectChanged, this, &InputTilesWidget::refresh);
    connect(m_engine, &EngineController::selectedSourceChanged, this, [this](const QString&) { refresh(); });
    refresh();
}

void InputTilesWidget::onAddClicked()
{
    QPoint pos = QCursor::pos();
    if (m_addBtn)
        pos = m_addBtn->mapToGlobal(QPoint(0, m_addBtn->height()));
    const SourceType type = showAddSourceTypeMenu(this, pos);
    if (type != SourceType::Unknown)
        emit addSourceTypeRequested(type);
}

void InputTilesWidget::onRemove()
{
    const QString id = m_engine->selectedSourceId();
    if (!id.isEmpty())
        m_engine->removeSource(id);
}

void InputTilesWidget::onProperties()
{
    const QString id = m_engine->selectedSourceId();
    if (!id.isEmpty())
        emit configureSourceRequested(id);
}

void InputTilesWidget::onMoveUp()
{
    const QString id = m_engine->selectedSourceId();
    if (!id.isEmpty())
        m_engine->moveSourceZOrder(id, 1);
}

void InputTilesWidget::onMoveDown()
{
    const QString id = m_engine->selectedSourceId();
    if (!id.isEmpty())
        m_engine->moveSourceZOrder(id, -1);
}

void InputTilesWidget::refresh()
{
    while (QLayoutItem* child = m_listLay->takeAt(0)) {
        if (child->widget())
            child->widget()->deleteLater();
        delete child;
    }

    const auto p = m_engine->projectSnapshot();
    const QString sceneId = p.editSceneId();
    const auto* scene = p.findScene(sceneId);
    const QString selected = m_engine->selectedSourceId();

    if (!scene || scene->sources.isEmpty()) {
        auto* empty = new QLabel(QStringLiteral("No sources — click + to add"), m_listHost);
        empty->setAlignment(Qt::AlignCenter);
        empty->setWordWrap(true);
        empty->setStyleSheet(QStringLiteral(
            "color:#8892A4; font-size:11px; padding:24px 12px;"));
        m_listLay->addWidget(empty);
        m_listLay->addStretch(1);
        return;
    }

    for (int i = scene->sources.size() - 1; i >= 0; --i) {
        const auto& src = scene->sources[i];
        auto* bar = new SourceBar(m_listHost);
        bar->sourceId = src.id;
        const bool isSel = src.id == selected;
        bar->setProperty("selected", isSel);
        bar->setStyleSheet(sourceBarStyle(src.colorHex, isSel));
        bar->style()->unpolish(bar);
        bar->style()->polish(bar);

        bar->onSelect = [this, id = src.id] { m_engine->setSelectedSourceId(id); };
        bar->onConfigure = [this, id = src.id] {
            m_engine->setSelectedSourceId(id);
            emit configureSourceRequested(id);
        };

        auto* lay = new QHBoxLayout(bar);
        lay->setContentsMargins(8, 0, 6, 0);
        lay->setSpacing(8);

        auto* icon = new QLabel(typeIconGlyph(src.type), bar);
        icon->setFixedWidth(22);
        icon->setAlignment(Qt::AlignCenter);
        icon->setStyleSheet(QStringLiteral("font-size:13px; color:%1;")
                                .arg(src.colorHex.isEmpty() ? QStringLiteral("#C8CAD0") : src.colorHex));

        auto* name = new QLabel(src.name, bar);
        name->setStyleSheet(QStringLiteral(
            "color:#E8ECF4; font-family:'DM Sans'; font-size:12px; font-weight:600;"));
        if (!src.visible)
            name->setStyleSheet(QStringLiteral(
                "color:#606878; font-family:'DM Sans'; font-size:12px; font-weight:600;"));

        auto* vis = new QToolButton(bar);
        vis->setText(src.visible ? QStringLiteral("👁") : QStringLiteral("◌"));
        vis->setToolTip(src.visible ? QStringLiteral("Hide") : QStringLiteral("Show"));
        vis->setAutoRaise(true);
        vis->setFixedSize(24, 24);
        vis->setCursor(Qt::PointingHandCursor);
        vis->setStyleSheet(QStringLiteral(
            "QToolButton{background:transparent;border:none;font-size:12px;}"
            "QToolButton:hover{background:#2A2D35;border-radius:3px;}"));
        connect(vis, &QToolButton::clicked, this, [this, id = src.id, v = src.visible] {
            m_engine->setSourceVisible(id, !v);
        });

        auto* lock = new QToolButton(bar);
        lock->setText(src.locked ? QStringLiteral("🔒") : QStringLiteral("🔓"));
        lock->setToolTip(src.locked ? QStringLiteral("Unlock") : QStringLiteral("Lock"));
        lock->setAutoRaise(true);
        lock->setFixedSize(24, 24);
        lock->setCursor(Qt::PointingHandCursor);
        lock->setStyleSheet(QStringLiteral(
            "QToolButton{background:transparent;border:none;font-size:12px;}"
            "QToolButton:hover{background:#2A2D35;border-radius:3px;}"));
        connect(lock, &QToolButton::clicked, this, [this, id = src.id, l = src.locked] {
            m_engine->setSourceLocked(id, !l);
        });

        lay->addWidget(icon);
        lay->addWidget(name, 1);
        lay->addWidget(vis);
        lay->addWidget(lock);

        bar->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(bar, &QWidget::customContextMenuRequested, this,
                [this, bar, src](const QPoint& pos) {
            const QString id = src.id;
            m_engine->setSelectedSourceId(id);

            QMenu menu(this);
            menu.setStyleSheet(QStringLiteral(
                "QMenu{background:#1A1E26;color:#E8ECF4;border:1px solid #2A3140;}"
                "QMenu::item:selected{background:#2A3140;}"));

            menu.addAction(QStringLiteral("Rename"), this, [this, id, name = src.name] {
                bool ok = false;
                const QString n = QInputDialog::getText(
                    this, QStringLiteral("Rename Source"), QStringLiteral("Name:"),
                    QLineEdit::Normal, name, &ok);
                if (ok && !n.trimmed().isEmpty())
                    m_engine->setSourceName(id, n.trimmed());
            });
            menu.addAction(QStringLiteral("Properties"), this, [this, id] {
                emit configureSourceRequested(id);
            });
            menu.addAction(QStringLiteral("Filters"), this, [this, id] {
                FiltersDialog dlg(m_engine, id, window());
                dlg.exec();
            });
            menu.addAction(QStringLiteral("Interact…"), this, [this, id] {
                InteractDialog dlg(m_engine, id, window());
                dlg.exec();
            });

            auto* transform = menu.addMenu(QStringLiteral("Transform"));
            transform->addAction(QStringLiteral("Edit Transform…"), this, [this, id] {
                TransformDialog dlg(m_engine, id, window());
                dlg.exec();
            });
            transform->addAction(QStringLiteral("Copy Transform"), this, [this, id] {
                if (const auto* s = m_engine->projectSnapshot().findSourceAnywhere(id))
                    TransformDialog::copyTransform(s->transform);
            });
            auto* pasteX = transform->addAction(QStringLiteral("Paste Transform"));
            pasteX->setEnabled(TransformDialog::hasClipboard());
            connect(pasteX, &QAction::triggered, this, [this, id] {
                TransformDialog::pasteOnto(m_engine, id);
            });
            transform->addSeparator();
            transform->addAction(QStringLiteral("Reset Transform"), this, [this, id] {
                TransformDialog::resetTransform(m_engine, id);
            });
            transform->setEnabled(!src.locked);

            menu.addSeparator();
            auto* order = menu.addMenu(QStringLiteral("Order"));
            order->addAction(QStringLiteral("Move Up"), this, [this, id] {
                m_engine->moveSourceZOrder(id, 1);
            });
            order->addAction(QStringLiteral("Move Down"), this, [this, id] {
                m_engine->moveSourceZOrder(id, -1);
            });
            order->addAction(QStringLiteral("Move to Top"), this, [this, id] {
                m_engine->moveSourceZOrderExtreme(id, true);
            });
            order->addAction(QStringLiteral("Move to Bottom"), this, [this, id] {
                m_engine->moveSourceZOrderExtreme(id, false);
            });

            menu.addSeparator();
            menu.addAction(src.visible ? QStringLiteral("Hide") : QStringLiteral("Show"), this,
                           [this, id, v = src.visible] { m_engine->setSourceVisible(id, !v); });
            menu.addAction(src.locked ? QStringLiteral("Unlock") : QStringLiteral("Lock"), this,
                           [this, id, l = src.locked] { m_engine->setSourceLocked(id, !l); });

            menu.addSeparator();
            menu.addAction(QStringLiteral("Duplicate"), this, [this, id] {
                m_engine->duplicateSource(id);
            });
            menu.addAction(QStringLiteral("Copy Filters"), this, [this, id] {
                if (const auto* s = m_engine->projectSnapshot().findSourceAnywhere(id))
                    FiltersDialog::copyFilters(s->settings.value(QStringLiteral("filters")).toArray());
            });
            auto* pasteF = menu.addAction(QStringLiteral("Paste Filters"));
            pasteF->setEnabled(FiltersDialog::hasFilterClipboard());
            connect(pasteF, &QAction::triggered, this, [this, id] {
                FiltersDialog::pasteOnto(m_engine, id);
            });

            menu.addSeparator();
            auto* colorMenu = menu.addMenu(QStringLiteral("Set Color"));
            for (const QString& hex : obsColorTags()) {
                auto* act = colorMenu->addAction(hex);
                QPixmap px(14, 14);
                px.fill(QColor(hex));
                act->setIcon(QIcon(px));
                connect(act, &QAction::triggered, this, [this, id, hex] {
                    m_engine->setSourceColor(id, hex);
                });
            }
            colorMenu->addSeparator();
            colorMenu->addAction(QStringLiteral("Reset (type default)"), this, [this, id, t = src.type] {
                QString hex = QStringLiteral("#4F9EFF");
                switch (t) {
                case SourceType::Camera: hex = QStringLiteral("#22C55E"); break;
                case SourceType::Display: hex = QStringLiteral("#4F9EFF"); break;
                case SourceType::Window: hex = QStringLiteral("#38BDF8"); break;
                case SourceType::Game: hex = QStringLiteral("#A3E635"); break;
                case SourceType::AudioInput: hex = QStringLiteral("#F472B6"); break;
                case SourceType::AudioOutput: hex = QStringLiteral("#FB7185"); break;
                case SourceType::Browser: hex = QStringLiteral("#22D3EE"); break;
                case SourceType::Text: hex = QStringLiteral("#A855F7"); break;
                case SourceType::Image: hex = QStringLiteral("#FBBF24"); break;
                case SourceType::Alert:
                case SourceType::Scoreboard: hex = QStringLiteral("#FF5A2C"); break;
                default: break;
                }
                m_engine->setSourceColor(id, hex);
            });

            menu.addSeparator();
            auto* rem = menu.addAction(QStringLiteral("Remove"));
            connect(rem, &QAction::triggered, this, [this, id] { m_engine->removeSource(id); });

            menu.exec(bar->mapToGlobal(pos));
        });

        m_listLay->addWidget(bar);
    }
    m_listLay->addStretch(1);
}

} // namespace railshot
