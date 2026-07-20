#pragma once

#include "capture/IVideoSource.h"
#include <QMutex>
#include <QHash>
#include <QVector>
#include <optional>

namespace railshot {

/// Bounded per-source latest-frame bus. Drops stale frames under backpressure.
class FrameBus {
public:
    void publish(const QString& sourceId, const VideoFrame& frame);
    std::optional<VideoFrame> latest(const QString& sourceId) const;
    void remove(const QString& sourceId);
    void clear();
    QVector<QString> sourceIds() const;

private:
    mutable QMutex m_mutex;
    QHash<QString, VideoFrame> m_latest;
};

} // namespace railshot
