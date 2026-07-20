#pragma once
#include <QWidget>
#include "core/Types.h"

class QLabel;
class QPushButton;

namespace railshot {
class EngineController;

class StreamStatusWidget : public QWidget {
    Q_OBJECT
public:
    explicit StreamStatusWidget(EngineController* engine, QWidget* parent = nullptr);

public slots:
    void onTelemetry(const TelemetrySnapshot& snap);

private:
    EngineController* m_engine = nullptr;
    QLabel* m_state = nullptr;
    QLabel* m_bitrate = nullptr;
    QLabel* m_fps = nullptr;
    QLabel* m_dropped = nullptr;
    QLabel* m_drift = nullptr;
    QLabel* m_uptime = nullptr;
    QPushButton* m_end = nullptr;
};

} // namespace railshot
