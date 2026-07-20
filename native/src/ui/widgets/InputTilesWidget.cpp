#include "ui/widgets/InputTilesWidget.h"
#include "core/EngineController.h"
#include "core/SceneGraph.h"
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QMenu>
#include <QMouseEvent>
#include <QAction>
#include <functional>

namespace railshot {

namespace {
class TileFrame : public QFrame {
public:
    using QFrame::QFrame;
    QString sourceId;
    std::function<void()> onSelect;
    std::function<void()> onConfigure;

protected:
    void mousePressEvent(QMouseEvent* e) override
    {
        if (e->button() == Qt::LeftButton && onSelect) onSelect();
        QFrame::mousePressEvent(e);
    }
    void mouseDoubleClickEvent(QMouseEvent* e) override
    {
        if (onConfigure) onConfigure();
        QFrame::mouseDoubleClickEvent(e);
    }
};

QString chromeBtnStyle()
{
    return QStringLiteral(
        "QPushButton{background:transparent;border:none;color:#6B7280;font-size:11px;padding:1px;}"
        "QPushButton:hover{color:#E5E7EB;}");
}
} // namespace

InputTilesWidget::InputTilesWidget(EngineController* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setMinimumHeight(118);
    setStyleSheet(QStringLiteral("background:#1A1D21; border:none;"));
    m_row = new QHBoxLayout(this);
    m_row->setContentsMargins(0, 0, 0, 0);
    m_row->setSpacing(0);
    connect(m_engine->sceneGraph(), &SceneGraph::projectChanged, this, &InputTilesWidget::refresh);
    connect(m_engine, &EngineController::selectedSourceChanged, this, [this](const QString&) { refresh(); });
    refresh();
}

void InputTilesWidget::refresh()
{
    while (QLayoutItem* child = m_row->takeAt(0)) {
        if (child->widget()) child->widget()->deleteLater();
        delete child;
    }
    const auto p = m_engine->projectSnapshot();
    const auto* scene = p.findScene(p.activeSceneId);
    if (!scene || scene->sources.isEmpty()) {
        auto* empty = new QWidget(this);
        auto* emptyLay = new QVBoxLayout(empty);
        emptyLay->setContentsMargins(16, 12, 16, 12);
        emptyLay->setSpacing(6);
        auto* hint = new QLabel(QStringLiteral("No sources"), empty);
        hint->setAlignment(Qt::AlignCenter);
        hint->setStyleSheet(QStringLiteral(
            "color:#8B919C; font-size:12px; font-weight:500; background:transparent;"));
        auto* sub = new QLabel(QStringLiteral("Use Add Input in the toolbar"), empty);
        sub->setAlignment(Qt::AlignCenter);
        sub->setWordWrap(true);
        sub->setStyleSheet(QStringLiteral("color:#5A6070; font-size:11px; background:transparent;"));
        emptyLay->addStretch(1);
        emptyLay->addWidget(hint);
        emptyLay->addWidget(sub);
        emptyLay->addStretch(1);
        m_row->addWidget(empty, 1);
        return;
    }
    const QString selected = m_engine->selectedSourceId();
    const bool isInProgram = !p.activeSceneId.isEmpty() && p.activeSceneId == p.programSceneId;
    const bool isInPreview = !p.activeSceneId.isEmpty() && p.activeSceneId == p.previewSceneId;
    int idx = 0;
    for (const auto& src : scene->sources) {
        auto* tile = new TileFrame(this);
        tile->sourceId = src.id;
        tile->setMinimumWidth(160);
        tile->setMaximumWidth(200);
        tile->setCursor(Qt::PointingHandCursor);
        const bool isSel = src.id == selected;

        QString bodyBg = QStringLiteral("#1A1D21");
        QString accent = QStringLiteral("#2F333A");
        if (isInProgram) {
            bodyBg = QStringLiteral("#241A1A");
            accent = QStringLiteral("#C44A2A");
        } else if (isInPreview) {
            bodyBg = QStringLiteral("#1A221C");
            accent = QStringLiteral("#2E8B57");
        } else if (isSel) {
            bodyBg = QStringLiteral("#1C2028");
            accent = QStringLiteral("#4A6FA5");
        }
        tile->setStyleSheet(QStringLiteral(
                                "QFrame{background:%1;border:none;border-bottom:1px solid #242830;"
                                "border-left:3px solid %2;}")
                                .arg(bodyBg, accent));

        tile->onSelect = [this, id = src.id] { m_engine->setSelectedSourceId(id); };
        tile->onConfigure = [this, id = src.id] {
            m_engine->setSelectedSourceId(id);
            emit configureSourceRequested(id);
        };

        auto* col = new QVBoxLayout(tile);
        col->setContentsMargins(0, 0, 0, 0);
        col->setSpacing(0);

        auto* header = new QWidget(tile);
        header->setFixedHeight(22);
        header->setStyleSheet(QStringLiteral(
            "background:#22262E; border-bottom:1px solid #2F333A;"));
        auto* headerLay = new QHBoxLayout(header);
        headerLay->setContentsMargins(6, 0, 4, 0);
        headerLay->setSpacing(4);

        auto* name = new QLabel(QStringLiteral("%1. %2").arg(++idx).arg(src.name), header);
        name->setStyleSheet(QStringLiteral(
            "color:#D0D4DC;font-weight:500;font-size:11px;background:transparent;"));
        headerLay->addWidget(name, 1);

        auto* gear = new QPushButton(QStringLiteral("⚙"), header);
        gear->setFixedSize(18, 18);
        gear->setCursor(Qt::PointingHandCursor);
        gear->setToolTip(QStringLiteral("Input Settings"));
        gear->setStyleSheet(chromeBtnStyle());
        connect(gear, &QPushButton::clicked, this, [this, id = src.id] {
            m_engine->setSelectedSourceId(id);
            emit configureSourceRequested(id);
        });

        auto* vis = new QPushButton(src.visible ? QStringLiteral("👁") : QStringLiteral("–"), header);
        vis->setFixedSize(18, 18);
        vis->setCursor(Qt::PointingHandCursor);
        vis->setToolTip(QStringLiteral("Toggle visibility"));
        vis->setStyleSheet(chromeBtnStyle());
        connect(vis, &QPushButton::clicked, this, [this, id = src.id, visFlag = src.visible] {
            m_engine->setSourceVisible(id, !visFlag);
        });

        auto* del = new QPushButton(QStringLiteral("✕"), header);
        del->setFixedSize(18, 18);
        del->setCursor(Qt::PointingHandCursor);
        del->setToolTip(QStringLiteral("Remove"));
        del->setStyleSheet(chromeBtnStyle());
        connect(del, &QPushButton::clicked, this, [this, id = src.id] { m_engine->removeSource(id); });

        headerLay->addWidget(gear);
        headerLay->addWidget(vis);
        headerLay->addWidget(del);
        col->addWidget(header);

        auto* preview = new QLabel(sourceTypeToString(src.type), tile);
        preview->setAlignment(Qt::AlignCenter);
        preview->setMinimumHeight(60);
        preview->setMaximumHeight(80);
        preview->setStyleSheet(QStringLiteral(
            "background:#141618; color:#6B7280; font-weight:500; letter-spacing:0px; font-size:11px;"
            "border:none; opacity:%1;")
                                   .arg(src.visible ? QStringLiteral("1") : QStringLiteral("0.4")));
        col->addWidget(preview, 1);

        auto* actions = new QHBoxLayout();
        actions->setContentsMargins(4, 4, 4, 4);
        actions->setSpacing(4);

        auto* up = new QPushButton(QStringLiteral("◀"), tile);
        up->setObjectName(QStringLiteral("toolbarChromeBtn"));
        up->setFixedSize(22, 20);
        up->setToolTip(QStringLiteral("Move up in z-order"));
        connect(up, &QPushButton::clicked, this, [this, id = src.id] {
            m_engine->moveSourceZOrder(id, 1);
        });
        auto* down = new QPushButton(QStringLiteral("▶"), tile);
        down->setObjectName(QStringLiteral("toolbarChromeBtn"));
        down->setFixedSize(22, 20);
        down->setToolTip(QStringLiteral("Move down in z-order"));
        connect(down, &QPushButton::clicked, this, [this, id = src.id] {
            m_engine->moveSourceZOrder(id, -1);
        });

        auto* miniGo = new QPushButton(QStringLiteral("GO"), tile);
        miniGo->setFixedHeight(20);
        miniGo->setObjectName(QStringLiteral("tileMiniGo"));
        miniGo->setToolTip(QStringLiteral("Take transition"));
        connect(miniGo, &QPushButton::clicked, this, [this] { m_engine->go(); });

        auto* cut = new QPushButton(QStringLiteral("Cut"), tile);
        cut->setFixedHeight(20);
        cut->setObjectName(QStringLiteral("tileMiniCut"));
        connect(cut, &QPushButton::clicked, this, [this] {
            m_engine->go(TransitionType::Cut);
        });

        actions->addWidget(up);
        actions->addWidget(down);
        actions->addStretch();
        actions->addWidget(miniGo);
        actions->addWidget(cut);
        col->addLayout(actions);

        tile->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(tile, &QWidget::customContextMenuRequested, this,
                [this, tile, id = src.id, visible = src.visible](const QPoint& pos) {
            QMenu menu;
            menu.addAction(QStringLiteral("Input Settings"), this, [this, id] {
                m_engine->setSelectedSourceId(id);
                emit configureSourceRequested(id);
            });
            menu.addAction(visible ? QStringLiteral("Hide") : QStringLiteral("Show"), this, [this, id, visible] {
                m_engine->setSourceVisible(id, !visible);
            });
            menu.addSeparator();
            auto* rem = menu.addAction(QStringLiteral("Remove"));
            connect(rem, &QAction::triggered, this, [this, id] { m_engine->removeSource(id); });
            menu.exec(tile->mapToGlobal(pos));
        });

        m_row->addWidget(tile, 1);
    }
    m_row->addStretch();
}

} // namespace railshot
