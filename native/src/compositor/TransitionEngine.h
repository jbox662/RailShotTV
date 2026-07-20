#pragma once

#include "core/Types.h"
#include <QObject>
#include <QElapsedTimer>

namespace railshot {

class TransitionEngine : public QObject {
    Q_OBJECT
public:
    explicit TransitionEngine(QObject* parent = nullptr);

    void configure(TransitionType type, int durationMs);
    TransitionType type() const { return m_type; }
    int durationMs() const { return m_durationMs; }

    void start();
    bool isActive() const { return m_active; }

    /// Progress 0..1 for crossfade types. For FTB: Out fades 1→0 then In 0→1.
    float tick();

    enum class Phase { Idle, Out, In, Cross };
    Phase phase() const { return m_phase; }
    bool isCrossfade() const { return m_type != TransitionType::Cut && m_type != TransitionType::FTB; }

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
