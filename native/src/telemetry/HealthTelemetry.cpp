#include "telemetry/HealthTelemetry.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace railshot {

HealthTelemetry::HealthTelemetry(QObject* parent)
    : QObject(parent)
{
    m_fpsTimer.start();
}

void HealthTelemetry::setStreaming(bool v) { m_snap.streaming = v; }
void HealthTelemetry::setRecording(bool v) { m_snap.recording = v; }
void HealthTelemetry::setStreamState(ConnectionState s) { m_snap.streamState = s; }
void HealthTelemetry::setEngineState(EngineState s) { m_snap.state = s; }
void HealthTelemetry::noteRenderedFrame() { m_framesThisSecond++; m_snap.renderedFrames++; }
void HealthTelemetry::noteEncodedFrame() { m_encodeThisSecond++; }
void HealthTelemetry::noteDroppedFrame() { m_snap.droppedFrames++; }
void HealthTelemetry::setBitrateKbps(qint64 kbps) { m_snap.bitrateKbps = kbps; }
void HealthTelemetry::setAvDriftMs(double ms) { m_snap.avDriftMs = ms; }
void HealthTelemetry::setStreamUptime(qint64 sec) { m_snap.streamUptimeSec = sec; }
void HealthTelemetry::setRecordUptime(qint64 sec) { m_snap.recordUptimeSec = sec; }
void HealthTelemetry::setLastError(const QString& err) { m_snap.lastError = err; }

TelemetrySnapshot HealthTelemetry::snapshot() const
{
    return m_snap;
}

void HealthTelemetry::tick()
{
    if (m_fpsTimer.elapsed() >= 1000) {
        m_snap.fpsRender = double(m_framesThisSecond.exchange(0));
        m_snap.fpsEncode = double(m_encodeThisSecond.exchange(0));
        m_fpsTimer.restart();
    }

#ifdef _WIN32
    FILETIME idle{}, kernel{}, user{};
    if (GetSystemTimes(&idle, &kernel, &user)) {
        // Rough busy estimate — refined in soak builds with PDH counters.
        static ULARGE_INTEGER prevIdle{}, prevKernel{}, prevUser{};
        ULARGE_INTEGER i{}, k{}, u{};
        i.LowPart = idle.dwLowDateTime; i.HighPart = idle.dwHighDateTime;
        k.LowPart = kernel.dwLowDateTime; k.HighPart = kernel.dwHighDateTime;
        u.LowPart = user.dwLowDateTime; u.HighPart = user.dwHighDateTime;
        const ULONGLONG idleDiff = i.QuadPart - prevIdle.QuadPart;
        const ULONGLONG totalDiff = (k.QuadPart - prevKernel.QuadPart) + (u.QuadPart - prevUser.QuadPart);
        if (totalDiff > 0)
            m_snap.cpuPercent = 100.0 * (1.0 - double(idleDiff) / double(totalDiff));
        prevIdle = i; prevKernel = k; prevUser = u;
    }
#endif

    if (m_snap.streaming && m_snap.recording)
        m_snap.state = EngineState::RecordingAndStreaming;
    else if (m_snap.streaming)
        m_snap.state = EngineState::Streaming;
    else if (m_snap.recording)
        m_snap.state = EngineState::Recording;
    else
        m_snap.state = EngineState::Previewing;

    emit updated(m_snap);
}

} // namespace railshot
