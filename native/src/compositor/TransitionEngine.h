#pragma once

#include "core/Types.h"
#include <QObject>
#include <QElapsedTimer>
#include <functional>

namespace railshot {

class TransitionEngine : public QObject {
    Q_OBJECT
public:
    explicit TransitionEngine(QObject* parent = nullptr);

    void configure(TransitionType type, int durationMs);
    TransitionType type() const { return m_type; }
    int durationMs() const { return m_durationMs; }

    /// Begin transition from current program to target scene.
    void start();
    bool isActive() const { return m_active; }

    /// Returns mix 0..1 for out/in phases. Call each frame.
    float tick();

    enum class Phase { Idle, Out, In };
    Phase phase() const { return m_phase; }

signals:
    void finished();
    void phaseChanged(Phase phase);

private:
    TransitionType m_type = TransitionType::Cut;
    int m_durationMs = 500;
    bool m_active = false;
    Phase m_phase = Phase::Idle;
    QElapsedTimer m_timer;
};

} // namespace railshot
