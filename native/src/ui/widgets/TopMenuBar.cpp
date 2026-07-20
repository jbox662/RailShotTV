#include "ui/widgets/TopMenuBar.h"
#include "core/EngineController.h"
#include "ui/Theme.h"
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>

namespace railshot {

TopMenuBar::TopMenuBar(EngineController* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setFixedHeight(36);
    setStyleSheet(QStringLiteral(
        "background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #1A1D22, stop:1 #141619);"
        "border-bottom: 1px solid #2A2D35;"));
    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(10, 0, 10, 0);
    row->setSpacing(4);

    auto* brand = new QLabel(this);
    brand->setObjectName(QStringLiteral("brandWordmark"));
    brand->setText(QStringLiteral(
        "<span style='font-family:\"Bebas Neue\",\"Arial Narrow\",sans-serif; font-size:17px; letter-spacing:1px;'>"
        "<span style='color:#F0F0F0;font-weight:700;'>RAILSHOT</span>"
        "<span style='color:#FF5A2C;font-weight:700;'> TV</span></span>"));
    row->addWidget(brand);
    row->addSpacing(14);

    auto addBtn = [&](const QString& text, auto slot) {
        auto* b = new QPushButton(text, this);
        b->setCursor(Qt::PointingHandCursor);
        b->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #2A2D35, stop:1 #1E2128);"
            "  border: 1px solid #3A3D45; border-radius: 3px; padding: 3px 10px;"
            "  color: #C8CAD0; font-size: 11px; font-weight: 600;"
            "}"
            "QPushButton:hover { border-color:#4F9EFF; color:#fff; }"));
        connect(b, &QPushButton::clicked, this, slot);
        row->addWidget(b);
        return b;
    };
    addBtn(QStringLiteral("Preset"), [] {});
    addBtn(QStringLiteral("New"), [this] { emit newProject(); });
    addBtn(QStringLiteral("Open"), [this] { emit openProject(); });
    addBtn(QStringLiteral("Save"), [this] { emit saveProject(); });
    addBtn(QStringLiteral("Save As"), [this] { emit saveProject(); });
    addBtn(QStringLiteral("Settings"), [this] { emit openSettings(); });
    row->addStretch();

    auto* fullscreen = new QPushButton(QStringLiteral("Fullscreen"), this);
    fullscreen->setStyleSheet(QStringLiteral(
        "QPushButton { background:transparent; border:1px solid #2A2D35; color:#808898;"
        "  border-radius:3px; padding:3px 8px; font-size:11px; }"
        "QPushButton:hover { color:#fff; border-color:#4F9EFF; }"));
    row->addWidget(fullscreen);

    m_status = new QLabel(QStringLiteral("1080p59.94  ·  CPU —  ·  — FPS"), this);
    m_status->setObjectName(QStringLiteral("mono"));
    m_status->setStyleSheet(QStringLiteral(
        "font-family:'JetBrains Mono','Consolas',monospace; font-size:10px; color:#606878; padding-left:8px;"));
    row->addWidget(m_status);

    connect(m_engine, &EngineController::telemetryUpdated, this, [this](const TelemetrySnapshot& s) {
        m_status->setText(QStringLiteral("1080p59.94  ·  CPU %1%  ·  %2 FPS  ·  Drop %3")
                              .arg(int(s.cpuPercent)).arg(int(s.fpsRender)).arg(s.droppedFrames));
    });
}

} // namespace railshot
