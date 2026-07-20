#include "ui/widgets/InputTilesWidget.h"
#include "ui/Theme.h"
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
    const QString c = hot ? QStringLiteral("#00000090") : QStringLiteral("#4F9EFF80");
    return QStringLiteral(
               "QPushButton{background:transparent;border:none;color:%1;font-size:11px;padding:1px;}"
               "QPushButton:hover{color:%2;}")
        .arg(c, hot ? QStringLiteral("#000") : QStringLiteral("#4F9EFF"));
}
} // namespace

InputTilesWidget::InputTilesWidget(EngineController* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setMinimumHeight(110);
    setStyleSheet(QStringLiteral("background:#0A0C0F; border-top:1px solid #1A1D24;"));
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
        auto* empty = theme::makeEmptyState(QStringLiteral("◇"),
            QStringLiteral("No inputs — click Add Input to begin"), this);
        empty->setMinimumHeight(100);
        empty->setStyleSheet(empty->styleSheet()
            + QStringLiteral("border:1px dashed #2A2D35; border-radius:4px; margin:8px;"));
        m_row->addWidget(empty);
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

        QString bodyBg = QStringLiteral("#0D0F12");
        QString outline;
        if (isInProgram) {
            bodyBg = QStringLiteral("#1A0A0A");
            outline = QStringLiteral("border:2px solid #FF5A2C60;");
        } else if (isInPreview) {
            bodyBg = QStringLiteral("#0A150A");
            outline = QStringLiteral("border:2px solid #22C55E60;");
        } else if (isSel) {
            bodyBg = QStringLiteral("#0A1220");
            outline = QStringLiteral("border:2px solid #4F9EFF60;");
        } else {
            outline = QStringLiteral("border-right:1px solid #2A2D35;");
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
            "background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #1E2128,stop:1 #181B22);");
        QString headerColor = QStringLiteral("#C0C2C8");
        const bool hot = isInProgram || isInPreview;
        if (isInProgram) {
            headerBg = QStringLiteral(
                "background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #FF5A2C,stop:1 #CC3A14);");
            headerColor = QStringLiteral("#000000");
        } else if (isInPreview) {
            headerBg = QStringLiteral(
                "background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #22C55E,stop:1 #16A34A);");
            headerColor = QStringLiteral("#000000");
        } else if (isSel) {
            headerBg = QStringLiteral(
                "background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #1A2A3A,stop:1 #141E2A);");
            headerColor = QStringLiteral("#C8DAFF");
        }

        auto* header = new QWidget(tile);
        header->setFixedHeight(22);
        header->setStyleSheet(headerBg + QStringLiteral("border-bottom:1px solid #2A2D35;"));
        auto* headerLay = new QHBoxLayout(header);
        headerLay->setContentsMargins(6, 0, 4, 0);
        headerLay->setSpacing(4);

        auto* name = new QLabel(QStringLiteral("%1  %2").arg(++idx).arg(src.name), header);
        name->setStyleSheet(QStringLiteral(
            "color:%1;font-weight:700;font-size:10px;background:transparent;").arg(headerColor));
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
        preview->setStyleSheet(QStringLiteral(
            "background:#000000; color:%1; font-weight:700; letter-spacing:0.5px; font-size:10px;"
            "opacity:%2;")
                                   .arg(src.colorHex, src.visible ? QStringLiteral("1") : QStringLiteral("0.35")));
        col->addWidget(preview, 1);

        auto* actions = new QHBoxLayout();
        actions->setContentsMargins(4, 3, 4, 3);
        actions->setSpacing(3);

        auto* up = new QPushButton(QStringLiteral("◀"), tile);
        up->setFixedSize(22, 20);
        up->setToolTip(QStringLiteral("Move up in z-order"));
        connect(up, &QPushButton::clicked, this, [this, id = src.id] {
            m_engine->moveSourceZOrder(id, 1);
        });
        auto* down = new QPushButton(QStringLiteral("▶"), tile);
        down->setFixedSize(22, 20);
        down->setToolTip(QStringLiteral("Move down in z-order"));
        connect(down, &QPushButton::clicked, this, [this, id = src.id] {
            m_engine->moveSourceZOrder(id, -1);
        });

        auto* miniGo = new QPushButton(QStringLiteral("GO"), tile);
        miniGo->setFixedHeight(20);
        miniGo->setObjectName(QStringLiteral("goButton"));
        miniGo->setToolTip(QStringLiteral("Take transition"));
        connect(miniGo, &QPushButton::clicked, this, [this] { m_engine->go(); });

        auto* cut = new QPushButton(QStringLiteral("Cut"), tile);
        cut->setFixedHeight(20);
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
