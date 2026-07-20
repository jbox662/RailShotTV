#include "ui/widgets/MultiCorderPanel.h"
#include "ui/Theme.h"
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
        "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #16181E, stop:1 #0F1114);"
        "  border-left: 2px solid #FF5A2C;"
        "}"));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* header = new QWidget(this);
    header->setObjectName(QStringLiteral("multiCorderHeader"));
    header->setFixedHeight(36);
    header->setStyleSheet(
        QStringLiteral("QWidget#multiCorderHeader{background:#0F1114; border-bottom:1px solid #3A3D45;}")
        + theme::panelHeaderStyle(theme::PanelAccent::Brand));
    auto* h = new QHBoxLayout(header);
    h->setContentsMargins(12, 0, 8, 0);
    auto* title = new QLabel(QStringLiteral("MULTICORDER"), header);
    title->setStyleSheet(QStringLiteral(
        "color:#FF8C42; font-weight:900; font-size:11px; letter-spacing:1.5px; background:transparent;"));
    auto* close = new QPushButton(QStringLiteral("✕"), header);
    close->setObjectName(QStringLiteral("chromeBtn"));
    close->setFixedSize(26, 22);
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
        m_list->addWidget(theme::makeEmptyState(QStringLiteral("▣"),
            QStringLiteral("No sources in active scene"), this));
        m_list->addStretch();
        return;
    }
    for (const auto& src : scene->sources) {
        auto* row = new QFrame(this);
        row->setStyleSheet(QStringLiteral(
            "QFrame{background:#0A0C0F; border:1px solid #4A4D55; border-radius:4px;}"));
        auto* lay = new QHBoxLayout(row);
        lay->setContentsMargins(10, 8, 10, 8);
        auto* name = new QLabel(src.name, row);
        name->setStyleSheet(QStringLiteral("color:#E0E2E8; font-weight:700; background:transparent;"));
        auto* type = new QLabel(sourceTypeToString(src.type), row);
        type->setStyleSheet(QStringLiteral("color:#8892A4; font-size:10px; background:transparent;"));
        auto* rec = new QPushButton(QStringLiteral("● REC"), row);
        rec->setCheckable(true);
        rec->setFixedWidth(72);
        rec->setStyleSheet(QStringLiteral(
            "QPushButton{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #2A2D35,stop:1 #1A1D22);"
            "border:1px solid #5A5E68;color:#A0A8B8;font-weight:800;font-size:10px;}"
            "QPushButton:checked{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #5A1010,stop:1 #2A0A0A);"
            "border:2px solid #EF4444;color:#FECACA;}"));
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
