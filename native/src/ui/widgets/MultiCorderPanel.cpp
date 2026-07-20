#include "ui/widgets/MultiCorderPanel.h"
#include "core/EngineController.h"
#include "core/SceneGraph.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>

namespace railshot {

MultiCorderPanel::MultiCorderPanel(EngineController* engine, QWidget* parent)
    : QFrame(parent), m_engine(engine)
{
    setObjectName(QStringLiteral("multiCorderPanel"));
    setFixedWidth(320);
    setStyleSheet(QStringLiteral(
        "QFrame#multiCorderPanel {"
        "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #13151A, stop:1 #0F1114);"
        "  border-left: 1px solid #2A2D35;"
        "}"));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* header = new QWidget(this);
    header->setFixedHeight(36);
    header->setStyleSheet(QStringLiteral("background:#0F1114; border-bottom:1px solid #2A2D35;"));
    auto* h = new QHBoxLayout(header);
    h->setContentsMargins(12, 0, 8, 0);
    auto* title = new QLabel(QStringLiteral("MULTICORDER"), header);
    title->setStyleSheet(QStringLiteral(
        "color:#93C5FD; font-weight:800; font-size:11px; letter-spacing:1.5px; background:transparent;"));
    auto* close = new QPushButton(QStringLiteral("✕"), header);
    close->setFixedSize(28, 24);
    connect(close, &QPushButton::clicked, this, &MultiCorderPanel::closeRequested);
    h->addWidget(title);
    h->addStretch();
    h->addWidget(close);
    root->addWidget(header);

    auto* hint = new QLabel(QStringLiteral("Per-source record arms (UI). Master Record uses the bottom toolbar."), this);
    hint->setWordWrap(true);
    hint->setStyleSheet(QStringLiteral("color:#606878; font-size:10px; padding:10px 12px;"));
    root->addWidget(hint);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet(QStringLiteral("background:transparent; border:none;"));
    auto* host = new QWidget(scroll);
    m_list = new QVBoxLayout(host);
    m_list->setContentsMargins(8, 4, 8, 8);
    m_list->setSpacing(4);
    scroll->setWidget(host);
    root->addWidget(scroll, 1);

    connect(engine->sceneGraph(), &SceneGraph::projectChanged, this, &MultiCorderPanel::refresh);
    refresh();
}

void MultiCorderPanel::refresh()
{
    while (QLayoutItem* child = m_list->takeAt(0)) {
        if (child->widget()) child->widget()->deleteLater();
        delete child;
    }
    const auto p = m_engine->projectSnapshot();
    const auto* scene = p.findScene(p.activeSceneId);
    if (!scene || scene->sources.isEmpty()) {
        auto* empty = new QLabel(QStringLiteral("No sources in active scene"), this);
        empty->setStyleSheet(QStringLiteral("color:#606878; padding:16px;"));
        empty->setAlignment(Qt::AlignCenter);
        m_list->addWidget(empty);
        m_list->addStretch();
        return;
    }
    for (const auto& src : scene->sources) {
        auto* row = new QFrame(this);
        row->setStyleSheet(QStringLiteral(
            "QFrame{background:#0A0C0F; border:1px solid #2A2D35; border-radius:4px;}"));
        auto* lay = new QHBoxLayout(row);
        lay->setContentsMargins(10, 8, 10, 8);
        auto* name = new QLabel(src.name, row);
        name->setStyleSheet(QStringLiteral("color:#D0D2D8; font-weight:600; background:transparent;"));
        auto* type = new QLabel(sourceTypeToString(src.type), row);
        type->setStyleSheet(QStringLiteral("color:#606878; font-size:10px; background:transparent;"));
        auto* rec = new QPushButton(QStringLiteral("● REC"), row);
        rec->setCheckable(true);
        rec->setFixedWidth(72);
        rec->setStyleSheet(QStringLiteral(
            "QPushButton{background:#1A1D22;border:1px solid #3A3D45;color:#8892A4;font-weight:800;font-size:10px;}"
            "QPushButton:checked{background:#2A0A0A;border-color:#EF4444;color:#EF4444;}"));
        connect(rec, &QPushButton::toggled, this, [rec](bool on) {
            rec->setText(on ? QStringLiteral("■ ARM") : QStringLiteral("● REC"));
        });
        lay->addWidget(name, 1);
        lay->addWidget(type);
        lay->addWidget(rec);
        m_list->addWidget(row);
    }
    m_list->addStretch();
}

} // namespace railshot
