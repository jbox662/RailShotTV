#include "compositor/TransitionEngine.h"

namespace railshot {

TransitionEngine::TransitionEngine(QObject* parent)
    : QObject(parent)
{
}

void TransitionEngine::configure(TransitionType type, int durationMs)
{
    m_type = type;
    m_durationMs = qMax(0, durationMs);
}

void TransitionEngine::start()
{
    if (m_type == TransitionType::Cut || m_durationMs <= 0) {
        m_active = false;
        m_phase = Phase::Idle;
        emit finished();
        return;
    }
    m_active = true;
    m_phase = Phase::Out;
    m_timer.restart();
    emit phaseChanged(m_phase);
}

float TransitionEngine::tick()
{
    if (!m_active) return 1.0f;
    const int half = (m_type == TransitionType::FTB) ? m_durationMs : m_durationMs / 2;
    const qint64 elapsed = m_timer.elapsed();

    if (m_phase == Phase::Out) {
        const float t = qBound(0.0f, 1.0f, float(elapsed) / float(qMax(1, half)));
        if (elapsed >= half) {
            m_phase = Phase::In;
            m_timer.restart();
            emit phaseChanged(m_phase);
            return 0.0f;
        }
        return 1.0f - t;
    }

    if (m_phase == Phase::In) {
        const float t = qBound(0.0f, 1.0f, float(elapsed) / float(qMax(1, half)));
        if (elapsed >= half) {
            m_active = false;
            m_phase = Phase::Idle;
            emit finished();
            return 1.0f;
        }
        return t;
    }
    return 1.0f;
}

} // namespace railshot
