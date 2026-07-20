#pragma once

#include "audio/AudioTypes.h"
#include <QObject>
#include <atomic>
#include <mutex>
#include <thread>
#include <deque>

namespace railshot {

/// WASAPI render endpoint for headphone/speaker monitoring.
class AudioMonitor : public QObject {
    Q_OBJECT
public:
    explicit AudioMonitor(QObject* parent = nullptr);
    ~AudioMonitor() override;

    bool start(QString* error = nullptr);
    void stop();
    void push(const AudioBuffer& buffer);
    void setVolume(float v) { m_volume = v; }

private:
    void renderLoop();

    std::atomic<bool> m_running{false};
    std::atomic<float> m_volume{1.0f};
    std::thread m_thread;
    std::mutex m_mutex;
    std::deque<AudioBuffer> m_queue;
};

} // namespace railshot
