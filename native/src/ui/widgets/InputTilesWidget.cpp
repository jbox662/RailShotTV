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

QString chromeBtnStyle(bool hot)
{
    const QString c = hot ? QStringLiteral("#00000090") : QStringLiteral("#4F9EFF");
    return QStringLiteral(
               "QPushButton{background:transparent;border:none;color:%1;font-size:12px;font-weight:700;padding:1px;}"
               "QPushButton:hover{color:%2;}")
        .arg(c, hot ? QStringLiteral("#000") : QStringLiteral("#B8D4FF"));
}
} // namespace

InputTilesWidget::InputTilesWidget(EngineController* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setMinimumHeight(118);
    setStyleSheet(QStringLiteral(
        "background:#080A0D; border-top:2px solid #FF5A2C55; border-right:1px solid #2A2D35;"));
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
    // Prefer Preview scene (dock follows what you're editing), then active.
    const QString sceneId = !p.previewSceneId.isEmpty() ? p.previewSceneId
                            : !p.activeSceneId.isEmpty()  ? p.activeSceneId
                                                          : QString();
    const auto* scene = p.findScene(sceneId);
    if (!scene || scene->sources.isEmpty()) {
        auto* empty = new QWidget(this);
        auto* emptyLay = new QVBoxLayout(empty);
        emptyLay->setContentsMargins(16, 12, 16, 12);
        emptyLay->setSpacing(8);
        auto* icon = new QLabel(QStringLiteral("◇"), empty);
        icon->setAlignment(Qt::AlignCenter);
        icon->setStyleSheet(QStringLiteral("color:#FF5A2C66; font-size:28px; background:transparent;"));
        auto* hint = new QLabel(QStringLiteral("No inputs yet"), empty);
        hint->setAlignment(Qt::AlignCenter);
        hint->setStyleSheet(QStringLiteral(
            "color:#E0E2E8; font-family:'DM Sans'; font-size:13px; font-weight:700; background:transparent;"));
        auto* sub = new QLabel(QStringLiteral("Click Add Input in the toolbar to begin"), empty);
        sub->setAlignment(Qt::AlignCenter);
        sub->setWordWrap(true);
        sub->setStyleSheet(QStringLiteral("color:#8892A4; font-size:11px; background:transparent;"));
        emptyLay->addStretch(1);
        emptyLay->addWidget(icon);
        emptyLay->addWidget(hint);
        emptyLay->addWidget(sub);
        emptyLay->addStretch(1);
        m_row->addWidget(empty, 1);
        return;
    }
    const QString selected = m_engine->selectedSourceId();
    const bool isInProgram = !sceneId.isEmpty() && sceneId == p.programSceneId;
    const bool isInPreview = !sceneId.isEmpty() && sceneId == p.previewSceneId;
    int idx = 0;
    for (const auto& src : scene->sources) {
        auto* tile = new TileFrame(this);
        tile->sourceId = src.id;
        tile->setMinimumWidth(160);
        tile->setMaximumWidth(200);
        tile->setCursor(Qt::PointingHandCursor);
        const bool isSel = src.id == selected;

        QString bodyBg = QStringLiteral("#0D0F12");
        QString outline = QStringLiteral("border:1px solid #3A3D45; border-right:2px solid #2A2D35;");
        if (isInProgram) {
            bodyBg = QStringLiteral("#220C0C");
            outline = QStringLiteral("border:2px solid #FF5A2C;");
        } else if (isInPreview) {
            bodyBg = QStringLiteral("#0C1A0E");
            outline = QStringLiteral("border:2px solid #22C55E;");
        } else if (isSel) {
            bodyBg = QStringLiteral("#0C1830");
            outline = QStringLiteral("border:2px solid #4F9EFF;");
        }
        tile->setStyleSheet(QStringLiteral("QFrame{background:%1;%2}").arg(bodyBg, outline));

        tile->onSelect = [this, id = src.id] { m_engine->setSelectedSourceId(id); };
        tile->onConfigure = [this, id = src.id] {
            m_engine->setSelectedSourceId(id);
            emit configureSourceRequested(id);
        };

        auto* col = new QVBoxLayout(tile);
        col->setContentsMargins(0, 0, 0, 0);
        col->setSpacing(0);

        QString headerBg = QStringLiteral(
            "background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #2A2E36,stop:1 #181B22);");
        QString headerColor = QStringLiteral("#E0E2E8");
        const bool hot = isInProgram || isInPreview;
        if (isInProgram) {
            headerBg = QStringLiteral(
                "background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #FF8C42,stop:1 #FF5A2C);");
            headerColor = QStringLiteral("#000000");
        } else if (isInPreview) {
            headerBg = QStringLiteral(
                "background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #4ADE80,stop:1 #22C55E);");
            headerColor = QStringLiteral("#04140A");
        } else if (isSel) {
            headerBg = QStringLiteral(
                "background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #1A3A5A,stop:1 #142838);");
            headerColor = QStringLiteral("#B8D4FF");
        }

        auto* header = new QWidget(tile);
        header->setFixedHeight(24);
        header->setStyleSheet(headerBg + QStringLiteral("border-bottom:1px solid #4A4D55;"));
        auto* headerLay = new QHBoxLayout(header);
        headerLay->setContentsMargins(6, 0, 4, 0);
        headerLay->setSpacing(4);

        auto* name = new QLabel(QStringLiteral("%1  %2").arg(++idx).arg(src.name), header);
        name->setStyleSheet(QStringLiteral(
            "color:%1;font-weight:800;font-size:10px;background:transparent;").arg(headerColor));
        headerLay->addWidget(name, 1);

        auto* gear = new QPushButton(QStringLiteral("⚙"), header);
        gear->setFixedSize(18, 18);
        gear->setCursor(Qt::PointingHandCursor);
        gear->setToolTip(QStringLiteral("Input Settings"));
        gear->setStyleSheet(chromeBtnStyle(hot));
        connect(gear, &QPushButton::clicked, this, [this, id = src.id] {
            m_engine->setSelectedSourceId(id);
            emit configureSourceRequested(id);
        });

        auto* vis = new QPushButton(src.visible ? QStringLiteral("👁") : QStringLiteral("–"), header);
        vis->setFixedSize(18, 18);
        vis->setCursor(Qt::PointingHandCursor);
        vis->setToolTip(QStringLiteral("Toggle visibility"));
        vis->setStyleSheet(chromeBtnStyle(hot));
        connect(vis, &QPushButton::clicked, this, [this, id = src.id, visFlag = src.visible] {
            m_engine->setSourceVisible(id, !visFlag);
        });

        auto* del = new QPushButton(QStringLiteral("✕"), header);
        del->setFixedSize(18, 18);
        del->setCursor(Qt::PointingHandCursor);
        del->setToolTip(QStringLiteral("Remove"));
        del->setStyleSheet(chromeBtnStyle(hot));
        connect(del, &QPushButton::clicked, this, [this, id = src.id] { m_engine->removeSource(id); });

        headerLay->addWidget(gear);
        headerLay->addWidget(vis);
        headerLay->addWidget(del);
        col->addWidget(header);

        auto* preview = new QLabel(sourceTypeToString(src.type).toUpper(), tile);
        preview->setAlignment(Qt::AlignCenter);
        preview->setMinimumHeight(60);
        preview->setMaximumHeight(80);
        const QString typeColor = src.colorHex.isEmpty() ? QStringLiteral("#4F9EFF") : src.colorHex;
        preview->setStyleSheet(QStringLiteral(
            "background:#000000; color:%1; font-weight:900; letter-spacing:1px; font-size:11px;"
            "border-top:1px solid #1A1D24; border-bottom:1px solid #1A1D24;"
            "opacity:%2;")
                                   .arg(typeColor, src.visible ? QStringLiteral("1") : QStringLiteral("0.35")));
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
        miniGo->setStyleSheet(QStringLiteral(
            "QPushButton{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #4ADE80,stop:1 #16A34A);"
            "border:1px solid #86EFAC;color:#04140A;font-weight:900;font-size:9px;border-radius:3px;padding:0 8px;}"
            "QPushButton:hover{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #86EFAC,stop:1 #22C55E);}"));
        miniGo->setToolTip(QStringLiteral("Take transition"));
        connect(miniGo, &QPushButton::clicked, this, [this] { m_engine->go(); });

        auto* cut = new QPushButton(QStringLiteral("Cut"), tile);
        cut->setFixedHeight(20);
        cut->setObjectName(QStringLiteral("tileMiniCut"));
        cut->setStyleSheet(QStringLiteral(
            "QPushButton{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #2A2D35,stop:1 #1A1D22);"
            "border:1px solid #4F9EFF;color:#B8D4FF;font-weight:800;font-size:9px;border-radius:3px;padding:0 8px;}"
            "QPushButton:hover{border-color:#8AB4FF;color:#fff;}"));
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
