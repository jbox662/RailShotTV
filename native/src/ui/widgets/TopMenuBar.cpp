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
    setStyleSheet(QStringLiteral("background:#141619; border-bottom:1px solid #2A2D35;"));
    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(8, 0, 8, 0);
    row->setSpacing(4);

    auto* brand = new QLabel(QStringLiteral("<span style='font-weight:800;'>RAILSHOT</span>"
                                            "<span style='color:#FF5A2C;font-weight:800;'>TV</span>"), this);
    row->addWidget(brand);
    row->addSpacing(12);

    auto addBtn = [&](const QString& text, auto slot) {
        auto* b = new QPushButton(text, this);
        connect(b, &QPushButton::clicked, this, slot);
        row->addWidget(b);
        return b;
    };
    addBtn(QStringLiteral("New"), [this] { emit newProject(); });
    addBtn(QStringLiteral("Open"), [this] { emit openProject(); });
    addBtn(QStringLiteral("Save"), [this] { emit saveProject(); });
    addBtn(QStringLiteral("Settings"), [this] { emit openSettings(); });
    row->addStretch();

    m_status = new QLabel(QStringLiteral("1080p59.94  |  CPU: —"), this);
    m_status->setObjectName(QStringLiteral("mono"));
    row->addWidget(m_status);

    connect(m_engine, &EngineController::telemetryUpdated, this, [this](const TelemetrySnapshot& s) {
        m_status->setText(QStringLiteral("1080p59.94  |  CPU: %1%  |  FPS: %2  |  Drop: %3")
                              .arg(int(s.cpuPercent)).arg(int(s.fpsRender)).arg(s.droppedFrames));
    });
}

} // namespace railshot
