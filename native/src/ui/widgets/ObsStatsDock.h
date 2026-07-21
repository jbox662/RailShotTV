#pragma once

#include "core/Types.h"
#include <QWidget>
#include <QLabel>

namespace railshot {
class EngineController;

/// Compact OBS-like stats dock (live telemetry).
class ObsStatsDock : public QWidget {
    Q_OBJECT
public:
    explicit ObsStatsDock(EngineController* engine, QWidget* parent = nullptr);

private slots:
    void refresh(const TelemetrySnapshot& s);

private:
    EngineController* m_engine = nullptr;
    QLabel* m_fps = nullptr;
    QLabel* m_cpu = nullptr;
    QLabel* m_gpu = nullptr;
    QLabel* m_dropped = nullptr;
    QLabel* m_bitrate = nullptr;
    QLabel* m_stream = nullptr;
    QLabel* m_record = nullptr;
    QLabel* m_encoder = nullptr;
    QLabel* m_state = nullptr;
};

} // namespace railshot
