#pragma once

#include "core/Types.h"
#include <QWidget>
#include <QLabel>

namespace railshot {
class EngineController;

/// OBS-style status strip: FPS, CPU, dropped frames, bitrate, stream/record timers.
class ObsStatusBarWidget : public QWidget {
    Q_OBJECT
public:
    explicit ObsStatusBarWidget(EngineController* engine, QWidget* parent = nullptr);

private:
    void refresh(const TelemetrySnapshot& s);
    static QString formatUptime(qint64 sec);

    EngineController* m_engine = nullptr;
    QLabel* m_fps = nullptr;
    QLabel* m_cpu = nullptr;
    QLabel* m_dropped = nullptr;
    QLabel* m_bitrate = nullptr;
    QLabel* m_streamTimer = nullptr;
    QLabel* m_recordTimer = nullptr;
    QLabel* m_replay = nullptr;
    QLabel* m_message = nullptr;
};

} // namespace railshot
