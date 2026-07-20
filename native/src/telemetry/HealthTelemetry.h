#pragma once

#include "core/Types.h"
#include <QObject>
#include <QElapsedTimer>
#include <atomic>

namespace railshot {

class HealthTelemetry : public QObject {
    Q_OBJECT
public:
    explicit HealthTelemetry(QObject* parent = nullptr);

    void setStreaming(bool v);
    void setRecording(bool v);
    void setStreamState(ConnectionState s);
    void setEngineState(EngineState s);
    void noteRenderedFrame();
    void noteEncodedFrame();
    void noteDroppedFrame();
    void setBitrateKbps(qint64 kbps);
    void setAvDriftMs(double ms);
    void setStreamUptime(qint64 sec);
    void setRecordUptime(qint64 sec);
    void setLastError(const QString& err);

    TelemetrySnapshot snapshot() const;
    void tick(); // update CPU estimate etc.

signals:
    void updated(const TelemetrySnapshot& snap);

private:
    mutable TelemetrySnapshot m_snap;
    QElapsedTimer m_fpsTimer;
    std::atomic<qint64> m_framesThisSecond{0};
    std::atomic<qint64> m_encodeThisSecond{0};
};

} // namespace railshot
