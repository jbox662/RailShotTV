#include "ui/widgets/StreamStatusWidget.h"
#include "core/EngineController.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

namespace railshot {

StreamStatusWidget::StreamStatusWidget(EngineController* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setFixedWidth(168);
    setStyleSheet(QStringLiteral("background:#0F1114; border-left:2px solid #FF4D1C;"));
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(10, 10, 10, 10);
    auto* title = new QLabel(QStringLiteral("STREAM STATUS"), this);
    title->setStyleSheet(QStringLiteral("color:#FF4D1C; font-weight:800; letter-spacing:1px; font-size:11px;"));
    root->addWidget(title);

    auto make = [this, root](const QString& label) {
        auto* l = new QLabel(label, this);
        l->setStyleSheet(QStringLiteral("color:#A0A0B8; font-size:11px;"));
        root->addWidget(l);
        return l;
    };
    m_state = make(QStringLiteral("Idle"));
    m_state->setStyleSheet(QStringLiteral("color:#F8F8FF; font-weight:700; font-size:14px;"));
    m_bitrate = make(QStringLiteral("Bitrate —"));
    m_fps = make(QStringLiteral("FPS —"));
    m_dropped = make(QStringLiteral("Dropped —"));
    m_drift = make(QStringLiteral("A/V drift —"));
    m_uptime = make(QStringLiteral("Uptime —"));
    root->addStretch();

    m_end = new QPushButton(QStringLiteral("END STREAM"), this);
    m_end->setStyleSheet(QStringLiteral(
        "QPushButton{background:#FF4D1C;color:white;font-weight:800;padding:8px;border:none;}"
        "QPushButton:disabled{background:#3A3A50;color:#808898;}"));
    m_end->setEnabled(false);
    connect(m_end, &QPushButton::clicked, this, [this] {
        if (m_engine) m_engine->stopStreaming();
    });
    root->addWidget(m_end);

    connect(engine, &EngineController::telemetryUpdated, this, &StreamStatusWidget::onTelemetry);
    onTelemetry(engine->telemetrySnapshot());
}

void StreamStatusWidget::onTelemetry(const TelemetrySnapshot& s)
{
    if (s.streaming) {
        m_state->setText(QStringLiteral("LIVE"));
        m_state->setStyleSheet(QStringLiteral("color:#FF4D1C; font-weight:800; font-size:16px;"));
        m_end->setEnabled(true);
    } else if (s.recording) {
        m_state->setText(QStringLiteral("REC"));
        m_state->setStyleSheet(QStringLiteral("color:#EF4444; font-weight:800; font-size:16px;"));
        m_end->setEnabled(false);
    } else {
        m_state->setText(QStringLiteral("Idle"));
        m_state->setStyleSheet(QStringLiteral("color:#F8F8FF; font-weight:700; font-size:14px;"));
        m_end->setEnabled(false);
    }
    m_bitrate->setText(QStringLiteral("Bitrate  %1 kbps").arg(s.bitrateKbps));
    m_fps->setText(QStringLiteral("Render  %1  Enc  %2")
                       .arg(s.fpsRender, 0, 'f', 1)
                       .arg(s.fpsEncode, 0, 'f', 1));
    m_dropped->setText(QStringLiteral("Dropped  %1").arg(s.droppedFrames));
    m_drift->setText(QStringLiteral("A/V drift  %1 ms").arg(s.avDriftMs, 0, 'f', 1));
    m_uptime->setText(QStringLiteral("Up  %1s").arg(s.streaming ? s.streamUptimeSec : s.recordUptimeSec));
}

} // namespace railshot
