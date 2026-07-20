#include "ui/widgets/InputTilesWidget.h"
#include "core/EngineController.h"
#include "core/SceneGraph.h"
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QFrame>

namespace railshot {

InputTilesWidget::InputTilesWidget(EngineController* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setMinimumHeight(100);
    setStyleSheet(QStringLiteral("background:#0A0C0F; border-top:1px solid #1A1D24;"));
    m_row = new QHBoxLayout(this);
    m_row->setContentsMargins(0, 0, 0, 0);
    m_row->setSpacing(0);
    connect(m_engine->sceneGraph(), &SceneGraph::projectChanged, this, &InputTilesWidget::refresh);
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
        auto* empty = new QLabel(QStringLiteral("No scene sources — click Add Input to create one"), this);
        empty->setAlignment(Qt::AlignCenter);
        empty->setStyleSheet(QStringLiteral(
            "color:#4A5060; font-size:12px; background:transparent; padding:16px;"));
        m_row->addWidget(empty);
        return;
    }
    int idx = 0;
    for (const auto& src : scene->sources) {
        auto* tile = new QFrame(this);
        tile->setFixedWidth(136);
        tile->setStyleSheet(QStringLiteral(
            "QFrame{background:#0D0F12; border-right:1px solid #1A1D24;}"));
        auto* col = new QVBoxLayout(tile);
        col->setContentsMargins(0, 0, 0, 0);
        col->setSpacing(0);
        auto* header = new QLabel(QStringLiteral("%1  %2").arg(++idx).arg(src.name), tile);
        header->setStyleSheet(QStringLiteral(
            "background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #22262E, stop:1 #1A1D22);"
            "padding:4px 8px; font-weight:700; font-size:10px; color:#D0D2D8;"
            "border-bottom:1px solid #2A2D35;"));
        col->addWidget(header);
        auto* preview = new QLabel(sourceTypeToString(src.type), tile);
        preview->setAlignment(Qt::AlignCenter);
        preview->setFixedHeight(54);
        preview->setStyleSheet(QStringLiteral(
            "background:#080A0D; color:%1; font-weight:700; letter-spacing:0.5px;").arg(src.colorHex));
        col->addWidget(preview);
        auto* actions = new QHBoxLayout();
        actions->setContentsMargins(4, 4, 4, 4);
        actions->setSpacing(4);
        auto* vis = new QPushButton(src.visible ? QStringLiteral("Show") : QStringLiteral("Hide"), tile);
        vis->setFixedHeight(22);
        connect(vis, &QPushButton::clicked, this, [this, id = src.id, visFlag = src.visible] {
            m_engine->setSourceVisible(id, !visFlag);
        });
        auto* del = new QPushButton(QStringLiteral("×"), tile);
        del->setFixedHeight(22);
        del->setFixedWidth(28);
        connect(del, &QPushButton::clicked, this, [this, id = src.id] { m_engine->removeSource(id); });
        actions->addWidget(vis, 1);
        actions->addWidget(del);
        col->addLayout(actions);
        m_row->addWidget(tile);
    }
    m_row->addStretch();
}

} // namespace railshot
