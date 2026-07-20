#include "ui/widgets/InputTilesWidget.h"
#include "core/EngineController.h"
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QFrame>

namespace railshot {

InputTilesWidget::InputTilesWidget(EngineController* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setMinimumHeight(90);
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
        auto* empty = new QLabel(QStringLiteral("No inputs — click Add Input"), this);
        empty->setAlignment(Qt::AlignCenter);
        empty->setStyleSheet(QStringLiteral("color:#404450;"));
        m_row->addWidget(empty);
        return;
    }
    int idx = 0;
    for (const auto& src : scene->sources) {
        auto* tile = new QFrame(this);
        tile->setFixedWidth(130);
        tile->setStyleSheet(QStringLiteral("QFrame{background:#0A0C10;border-right:1px solid #1A1D24;}"));
        auto* col = new QVBoxLayout(tile);
        col->setContentsMargins(0, 0, 0, 0);
        col->setSpacing(0);
        auto* header = new QLabel(QStringLiteral("%1 %2").arg(++idx).arg(src.name), tile);
        header->setStyleSheet(QStringLiteral("background:#1E2128;padding:2px 6px;font-weight:700;font-size:10px;"));
        col->addWidget(header);
        auto* preview = new QLabel(sourceTypeToString(src.type), tile);
        preview->setAlignment(Qt::AlignCenter);
        preview->setFixedHeight(52);
        preview->setStyleSheet(QStringLiteral("background:#000;color:%1;").arg(src.colorHex));
        col->addWidget(preview);
        auto* actions = new QHBoxLayout();
        auto* vis = new QPushButton(src.visible ? QStringLiteral("👁") : QStringLiteral("✕"), tile);
        vis->setFixedHeight(22);
        connect(vis, &QPushButton::clicked, this, [this, id = src.id, visFlag = src.visible] {
            m_engine->setSourceVisible(id, !visFlag);
        });
        auto* del = new QPushButton(QStringLiteral("×"), tile);
        del->setFixedHeight(22);
        connect(del, &QPushButton::clicked, this, [this, id = src.id] { m_engine->removeSource(id); });
        actions->addWidget(vis);
        actions->addWidget(del);
        col->addLayout(actions);
        m_row->addWidget(tile);
    }
    m_row->addStretch();
}

} // namespace railshot
