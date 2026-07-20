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
        m_row->addWidget(theme::makeEmptyState(QStringLiteral("◇"),
            QStringLiteral("No inputs — click Add Input to begin"), this));
        return;
    }
    const QString selected = m_engine->selectedSourceId();
    int idx = 0;
    for (const auto& src : scene->sources) {
        auto* tile = new TileFrame(this);
        tile->sourceId = src.id;
        tile->setFixedWidth(180);
        tile->setCursor(Qt::PointingHandCursor);
        const bool isSel = src.id == selected;
        tile->setStyleSheet(QStringLiteral(
            "QFrame{background:#0D0F12; border-right:1px solid #1A1D24;"
            "%1}").arg(isSel ? QStringLiteral("border:1px solid #4F9EFF;") : QString()));

        tile->onSelect = [this, id = src.id] { m_engine->setSelectedSourceId(id); };
        tile->onConfigure = [this, id = src.id] {
            m_engine->setSelectedSourceId(id);
            emit configureSourceRequested(id);
        };

        auto* col = new QVBoxLayout(tile);
        col->setContentsMargins(0, 0, 0, 0);
        col->setSpacing(0);

        QString headerBg = QStringLiteral(
            "background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #22262E, stop:1 #1A1D22);"
            "color:#D0D2D8;");
        if (isSel) {
            headerBg = QStringLiteral(
                "background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #1A3A6A, stop:1 #122033);"
                "color:#C8DAFF;");
        }
        auto* header = new QLabel(QStringLiteral("%1  %2").arg(++idx).arg(src.name), tile);
        header->setStyleSheet(headerBg + QStringLiteral(
            "padding:4px 8px; font-weight:700; font-size:10px;"
            "border-bottom:1px solid #2A2D35;"));
        col->addWidget(header);

        auto* preview = new QLabel(sourceTypeToString(src.type).toUpper(), tile);
        preview->setAlignment(Qt::AlignCenter);
        preview->setFixedHeight(54);
        preview->setStyleSheet(QStringLiteral(
            "background:#080A0D; color:%1; font-weight:700; letter-spacing:0.5px; font-size:11px;")
                                   .arg(src.colorHex));
        col->addWidget(preview);

        auto* actions = new QHBoxLayout();
        actions->setContentsMargins(4, 4, 4, 4);
        actions->setSpacing(3);

        auto* gear = new QPushButton(QStringLiteral("⚙"), tile);
        gear->setFixedSize(26, 22);
        gear->setToolTip(QStringLiteral("Input Settings"));
        connect(gear, &QPushButton::clicked, this, [this, id = src.id] {
            m_engine->setSelectedSourceId(id);
            emit configureSourceRequested(id);
        });

        auto* vis = new QPushButton(src.visible ? QStringLiteral("👁") : QStringLiteral("–"), tile);
        vis->setFixedSize(26, 22);
        vis->setToolTip(QStringLiteral("Toggle visibility"));
        connect(vis, &QPushButton::clicked, this, [this, id = src.id, visFlag = src.visible] {
            m_engine->setSourceVisible(id, !visFlag);
        });

        auto* up = new QPushButton(QStringLiteral("↑"), tile);
        up->setFixedSize(22, 22);
        connect(up, &QPushButton::clicked, this, [this, id = src.id] {
            m_engine->moveSourceZOrder(id, 1);
        });
        auto* down = new QPushButton(QStringLiteral("↓"), tile);
        down->setFixedSize(22, 22);
        connect(down, &QPushButton::clicked, this, [this, id = src.id] {
            m_engine->moveSourceZOrder(id, -1);
        });

        auto* miniGo = new QPushButton(QStringLiteral("GO"), tile);
        miniGo->setFixedHeight(22);
        miniGo->setObjectName(QStringLiteral("goButton"));
        miniGo->setToolTip(QStringLiteral("Take transition"));
        connect(miniGo, &QPushButton::clicked, this, [this] { m_engine->go(); });

        auto* cut = new QPushButton(QStringLiteral("Cut"), tile);
        cut->setFixedHeight(22);
        connect(cut, &QPushButton::clicked, this, [this] {
            m_engine->go(TransitionType::Cut);
        });

        actions->addWidget(gear);
        actions->addWidget(vis);
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

        m_row->addWidget(tile);
    }
    m_row->addStretch();
}

} // namespace railshot
