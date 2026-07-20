#include "capture/FrameBus.h"

namespace railshot {

void FrameBus::publish(const QString& sourceId, const VideoFrame& frame)
{
    QMutexLocker lock(&m_mutex);
    m_latest.insert(sourceId, frame);
}

std::optional<VideoFrame> FrameBus::latest(const QString& sourceId) const
{
    QMutexLocker lock(&m_mutex);
    auto it = m_latest.constFind(sourceId);
    if (it == m_latest.cend()) return std::nullopt;
    return *it;
}

void FrameBus::remove(const QString& sourceId)
{
    QMutexLocker lock(&m_mutex);
    m_latest.remove(sourceId);
}

void FrameBus::clear()
{
    QMutexLocker lock(&m_mutex);
    m_latest.clear();
}

QVector<QString> FrameBus::sourceIds() const
{
    QMutexLocker lock(&m_mutex);
    return m_latest.keys().toVector();
}

} // namespace railshot
