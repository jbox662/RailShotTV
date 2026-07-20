#include "ui/widgets/PlayListPanel.h"
#include "ui/Theme.h"
#include "core/EngineController.h"
#include "core/SceneGraph.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QColor>

namespace railshot {

PlayListPanel::PlayListPanel(EngineController* engine, QWidget* parent)
    : QFrame(parent), m_engine(engine)
{
    setObjectName(QStringLiteral("playListPanel"));
    setFixedWidth(300);
    setStyleSheet(QStringLiteral(
        "QFrame#playListPanel {"
        "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #13151A, stop:1 #0F1114);"
        "  border-left: 1px solid #2A2D35;"
        "}"));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* header = new QWidget(this);
    header->setFixedHeight(36);
    header->setStyleSheet(
        QStringLiteral("background:#0F1114; border-bottom:1px solid #2A2D35;")
        + theme::panelHeaderStyle(theme::PanelAccent::Violet));
    auto* h = new QHBoxLayout(header);
    h->setContentsMargins(12, 0, 8, 0);
    auto* title = new QLabel(QStringLiteral("PLAYLIST"), header);
    title->setStyleSheet(QStringLiteral(
        "color:#D8B4FE; font-weight:800; font-size:11px; letter-spacing:1.5px; background:transparent;"));
    auto* close = new QPushButton(QStringLiteral("✕"), header);
    close->setFixedSize(28, 24);
    connect(close, &QPushButton::clicked, this, &PlayListPanel::closeRequested);
    h->addWidget(title);
    h->addStretch();
    h->addWidget(close);
    root->addWidget(header);

    m_list = new QListWidget(this);
    m_list->setStyleSheet(QStringLiteral(
        "QListWidget{background:#0A0C0F;border:none;}"
        "QListWidget::item{padding:10px 12px;border-bottom:1px solid #15181E;color:#C8CAD0;}"
        "QListWidget::item:selected{background:#1A1228;color:#D8B4FE;}"));
    connect(m_list, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        m_engine->setPreviewScene(item->data(Qt::UserRole).toString());
    });
    root->addWidget(m_list, 1);

    auto* go = new QPushButton(QStringLiteral("▶  GO (Preview → Program)"), this);
    go->setMinimumHeight(40);
    go->setCursor(Qt::PointingHandCursor);
    go->setStyleSheet(QStringLiteral(
        "QPushButton{background:#22C55E;color:#000;font-weight:800;border:none;border-radius:0;}"
        "QPushButton:hover{background:#4ADE80;}"));
    connect(go, &QPushButton::clicked, this, [this] {
        if (!m_engine->projectSnapshot().previewSceneId.isEmpty())
            m_engine->go();
    });
    root->addWidget(go);

    connect(engine->sceneGraph(), &SceneGraph::projectChanged, this, &PlayListPanel::refresh);
    refresh();
}

void PlayListPanel::refresh()
{
    m_list->clear();
    const auto p = m_engine->projectSnapshot();
    if (p.scenes.isEmpty()) {
        auto* empty = new QListWidgetItem(QStringLiteral("◌  No scenes — add one on Dashboard"), m_list);
        empty->setFlags(Qt::NoItemFlags);
        empty->setForeground(QColor(QStringLiteral("#606878")));
        return;
    }
    for (const auto& sc : p.scenes) {
        auto* item = new QListWidgetItem(sc.name, m_list);
        item->setData(Qt::UserRole, sc.id);
        if (sc.id == p.programSceneId)
            item->setForeground(QColor(QStringLiteral("#FF8A6A")));
        else if (sc.id == p.previewSceneId)
            item->setForeground(QColor(QStringLiteral("#6EE7A0")));
    }
}

} // namespace railshot
