#include "ui/pages/AnalyticsPage.h"
#include "core/EngineController.h"
#include <QVBoxLayout>
#include <QLabel>

namespace railshot {

AnalyticsPage::AnalyticsPage(EngineController* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    auto* root = new QVBoxLayout(this);
    root->addWidget(new QLabel(QStringLiteral("ANALYTICS"), this));
    m_stats = new QLabel(QStringLiteral("Waiting for stream telemetry…"), this);
    m_stats->setObjectName(QStringLiteral("mono"));
    m_stats->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    root->addWidget(m_stats, 1);

    connect(engine, &EngineController::telemetryUpdated, this, [this](const TelemetrySnapshot& s) {
        m_stats->setText(QStringLiteral(
            "State: %1\nStreaming: %2\nRecording: %3\n"
            "Render FPS: %4\nEncode FPS: %5\nBitrate: %6 kbps\n"
            "Dropped: %7\nA/V drift: %8 ms\nCPU: %9%\n"
            "Stream uptime: %10s\nRecord uptime: %11s\nLast error: %12")
                             .arg(int(s.state))
                             .arg(s.streaming ? QStringLiteral("yes") : QStringLiteral("no"))
                             .arg(s.recording ? QStringLiteral("yes") : QStringLiteral("no"))
                             .arg(s.fpsRender, 0, 'f', 1)
                             .arg(s.fpsEncode, 0, 'f', 1)
                             .arg(s.bitrateKbps)
                             .arg(s.droppedFrames)
                             .arg(s.avDriftMs, 0, 'f', 1)
                             .arg(s.cpuPercent, 0, 'f', 1)
                             .arg(s.streamUptimeSec)
                             .arg(s.recordUptimeSec)
                             .arg(s.lastError.isEmpty() ? QStringLiteral("—") : s.lastError));
    });
}

} // namespace railshot
