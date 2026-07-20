#pragma once

#include "audio/AudioTypes.h"
#include <QObject>
#include <QVector>
#include <atomic>
#include <mutex>
#include <thread>
#include <functional>

namespace railshot {

class WasapiCapture : public QObject {
    Q_OBJECT
public:
    explicit WasapiCapture(AudioDeviceKind kind, QString deviceId = {}, QObject* parent = nullptr);
    ~WasapiCapture() override;

    bool start(QString* error = nullptr);
    void stop();
    bool isRunning() const { return m_running.load(); }

    void setCallback(std::function<void(const AudioBuffer&)> cb);

    static QVector<AudioDeviceInfo> enumerate(AudioDeviceKind kind);

signals:
    void captureError(const QString& message);

private:
    void captureLoop();

    AudioDeviceKind m_kind;
    QString m_deviceId;
    std::atomic<bool> m_running{false};
    std::thread m_thread;
    std::mutex m_cbMutex;
    std::function<void(const AudioBuffer&)> m_callback;
};

} // namespace railshot
