#include "ui/widgets/ObsStatusBarWidget.h"
#include "core/EngineController.h"
#include "overlays/ReplayBuffer.h"
#include <QHBoxLayout>

namespace railshot {

namespace {
QLabel* makePill(QWidget* parent, const QString& objectName)
{
    auto* l = new QLabel(QStringLiteral("—"), parent);
    l->setObjectName(objectName);
    l->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    return l;
}
} // namespace

ObsStatusBarWidget::ObsStatusBarWidget(EngineController* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setObjectName(QStringLiteral("obsStatusBar"));
    setFixedHeight(26);
    setStyleSheet(QStringLiteral(
        "QWidget#obsStatusBar{"
        "  background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #14171C,stop:1 #0A0C0F);"
        "  border-top:1px solid #3A3D45;"
        "}"
        "QWidget#obsStatusBar QLabel{"
        "  font-family:'JetBrains Mono','Consolas',monospace;"
        "  font-size:10px; color:#8892A4; padding:0 8px; border-right:1px solid #2A2D35;"
        "}"
        "QWidget#obsStatusBar QLabel#statusMsg{border-right:none; color:#606878;}"
        "QWidget#obsStatusBar QLabel#statusFps{color:#86EFAC;}"
        "QWidget#obsStatusBar QLabel#statusBitrate{color:#7AB8FF;}"
        "QWidget#obsStatusBar QLabel#statusStream{color:#FFB08A;}"
        "QWidget#obsStatusBar QLabel#statusRecord{color:#FECACA;}"));

    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(4, 0, 4, 0);
    row->setSpacing(0);

    m_fps = makePill(this, QStringLiteral("statusFps"));
    m_cpu = makePill(this, QStringLiteral("statusCpu"));
    m_dropped = makePill(this, QStringLiteral("statusDropped"));
    m_bitrate = makePill(this, QStringLiteral("statusBitrate"));
    m_streamTimer = makePill(this, QStringLiteral("statusStream"));
    m_recordTimer = makePill(this, QStringLiteral("statusRecord"));
    m_replay = makePill(this, QStringLiteral("statusReplay"));
    m_message = makePill(this, QStringLiteral("statusMsg"));

    row->addWidget(m_fps);
    row->addWidget(m_cpu);
    row->addWidget(m_dropped);
    row->addWidget(m_bitrate);
    row->addWidget(m_streamTimer);
    row->addWidget(m_recordTimer);
    row->addWidget(m_replay);
    row->addWidget(m_message, 1);

    connect(m_engine, &EngineController::telemetryUpdated, this, &ObsStatusBarWidget::refresh);
    connect(m_engine, &EngineController::errorOccurred, this, [this](const QString& msg) {
        m_message->setText(msg);
        m_message->setStyleSheet(QStringLiteral("color:#F87171; border-right:none;"));
    });
    refresh(m_engine->telemetrySnapshot());
}

QString ObsStatusBarWidget::formatUptime(qint64 sec)
{
    const qint64 h = sec / 3600;
    const qint64 m = (sec % 3600) / 60;
    const qint64 s = sec % 60;
    if (h > 0)
        return QStringLiteral("%1:%2:%3")
            .arg(h).arg(m, 2, 10, QLatin1Char('0')).arg(s, 2, 10, QLatin1Char('0'));
    return QStringLiteral("%1:%2")
        .arg(m, 2, 10, QLatin1Char('0')).arg(s, 2, 10, QLatin1Char('0'));
}

void ObsStatusBarWidget::refresh(const TelemetrySnapshot& s)
{
    m_fps->setText(QStringLiteral("FPS %1").arg(s.fpsRender, 0, 'f', 1));
    m_cpu->setText(QStringLiteral("CPU %1%").arg(int(s.cpuPercent)));
    const double dropPct = (s.renderedFrames > 0)
        ? (100.0 * double(s.droppedFrames) / double(s.renderedFrames + s.droppedFrames))
        : 0.0;
    m_dropped->setText(QStringLiteral("Dropped %1 (%2%)")
                           .arg(s.droppedFrames).arg(dropPct, 0, 'f', 1));
    m_bitrate->setText(s.streaming ? QStringLiteral("%1 kbps").arg(s.bitrateKbps)
                                   : QStringLiteral("— kbps"));
    m_streamTimer->setText(s.streaming
                               ? QStringLiteral("LIVE %1").arg(formatUptime(s.streamUptimeSec))
                               : QStringLiteral("Stream —"));
    m_recordTimer->setText(s.recording
                               ? QStringLiteral("REC %1").arg(formatUptime(s.recordUptimeSec))
                               : QStringLiteral("Rec —"));
    if (m_engine->replayBuffer()) {
        const qint64 us = m_engine->replayBuffer()->bufferedDurationUs();
        m_replay->setText(QStringLiteral("Replay %1/%2s")
                              .arg(us / 1000000)
                              .arg(m_engine->replayBuffer()->capacitySeconds()));
    } else {
        m_replay->setText(QStringLiteral("Replay —"));
    }
    if (!s.lastError.isEmpty()) {
        m_message->setText(s.lastError);
        m_message->setStyleSheet(QStringLiteral("color:#F87171; border-right:none;"));
    } else if (s.streaming) {
        m_message->setText(QStringLiteral("Streaming"));
        m_message->setStyleSheet(QStringLiteral("color:#FFB08A; border-right:none;"));
    } else if (s.recording) {
        m_message->setText(QStringLiteral("Recording"));
        m_message->setStyleSheet(QStringLiteral("color:#FECACA; border-right:none;"));
    } else {
        m_message->setText(QStringLiteral("Ready"));
        m_message->setStyleSheet(QStringLiteral("color:#606878; border-right:none;"));
    }
}

} // namespace railshot
