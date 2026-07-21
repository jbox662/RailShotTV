#include "ui/widgets/ObsStatsDock.h"
#include "core/EngineController.h"
#include <QFormLayout>
#include <QVBoxLayout>

namespace railshot {

namespace {
QString fmtUptime(qint64 sec)
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

QString connLabel(ConnectionState s)
{
    switch (s) {
    case ConnectionState::Connected: return QStringLiteral("Connected");
    case ConnectionState::Connecting: return QStringLiteral("Connecting");
    case ConnectionState::Reconnecting: return QStringLiteral("Reconnecting");
    case ConnectionState::Failed: return QStringLiteral("Failed");
    default: return QStringLiteral("Disconnected");
    }
}
} // namespace

ObsStatsDock::ObsStatsDock(EngineController* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setObjectName(QStringLiteral("obsStatsDock"));
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(10, 8, 10, 8);
    root->setSpacing(6);

    auto* form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignLeft);
    form->setHorizontalSpacing(12);
    form->setVerticalSpacing(4);

    auto add = [&](const QString& name) -> QLabel* {
        auto* v = new QLabel(QStringLiteral("—"), this);
        v->setStyleSheet(QStringLiteral(
            "font-family:'JetBrains Mono','Consolas',monospace; font-size:11px; color:#D0D2D8;"));
        auto* lab = new QLabel(name, this);
        lab->setStyleSheet(QStringLiteral("color:#8892A4; font-size:11px;"));
        form->addRow(lab, v);
        return v;
    };

    m_fps = add(QStringLiteral("FPS"));
    m_cpu = add(QStringLiteral("CPU"));
    m_gpu = add(QStringLiteral("GPU"));
    m_dropped = add(QStringLiteral("Dropped"));
    m_bitrate = add(QStringLiteral("Bitrate"));
    m_stream = add(QStringLiteral("Stream"));
    m_record = add(QStringLiteral("Record"));
    m_encoder = add(QStringLiteral("Encoder"));
    m_state = add(QStringLiteral("Status"));

    root->addLayout(form);
    root->addStretch();

    if (m_engine) {
        connect(m_engine, &EngineController::telemetryUpdated, this, &ObsStatsDock::refresh);
        refresh(m_engine->telemetrySnapshot());
    }
}

void ObsStatsDock::refresh(const TelemetrySnapshot& s)
{
    m_fps->setText(QStringLiteral("%1").arg(s.fpsRender, 0, 'f', 1));
    m_cpu->setText(QStringLiteral("%1%").arg(int(s.cpuPercent)));
    m_gpu->setText(s.gpuPercent > 0.0 ? QStringLiteral("%1%").arg(int(s.gpuPercent))
                                      : QStringLiteral("—"));
    const double dropPct = (s.renderedFrames > 0)
        ? (100.0 * double(s.droppedFrames) / double(s.renderedFrames + s.droppedFrames))
        : 0.0;
    m_dropped->setText(QStringLiteral("%1 (%2%)").arg(s.droppedFrames).arg(dropPct, 0, 'f', 1));
    m_bitrate->setText(s.streaming ? QStringLiteral("%1 kbps").arg(s.bitrateKbps)
                                   : QStringLiteral("—"));
    m_stream->setText(s.streaming
                          ? QStringLiteral("%1 · %2").arg(fmtUptime(s.streamUptimeSec),
                                                          connLabel(s.streamState))
                          : QStringLiteral("Idle"));
    m_record->setText(s.recording ? fmtUptime(s.recordUptimeSec) : QStringLiteral("Idle"));

    QString enc = QStringLiteral("—");
    // Encoder name is on OutputHub when live; fall back to preference.
    if (m_engine && m_engine->settings()) {
        enc = m_engine->settings()->outputProfile().encoderPreference;
        if (enc.isEmpty()) enc = QStringLiteral("auto");
    }
    m_encoder->setText(enc);

    if (!s.lastError.isEmpty())
        m_state->setText(s.lastError);
    else if (s.streaming && s.recording)
        m_state->setText(QStringLiteral("Streaming + Recording"));
    else if (s.streaming)
        m_state->setText(QStringLiteral("Streaming"));
    else if (s.recording)
        m_state->setText(QStringLiteral("Recording"));
    else
        m_state->setText(QStringLiteral("Ready"));
}

} // namespace railshot
