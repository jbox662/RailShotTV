#pragma once

#include "capture/IVideoSource.h"
#include <QString>
#include <QVector>
#include <QPair>
#include <atomic>
#include <thread>
#include <mutex>
#include <memory>

struct IMFSourceReader;
struct ID3D11Texture2D;
struct ID3D11Device;

namespace railshot {

class MediaFoundationCamera : public IVideoSource {
public:
    MediaFoundationCamera(QString id, QString name, QString deviceId = {});
    ~MediaFoundationCamera() override;

    QString id() const override { return m_id; }
    QString name() const override { return m_name; }
    bool start(ID3D11Device* device, QString* error = nullptr) override;
    void stop() override;
    bool isRunning() const override { return m_running.load(); }
    bool acquireLatest(VideoFrame& out) override;
    QSize size() const override { return {m_width, m_height}; }

    static QVector<QPair<QString, QString>> enumerateDevices(); // id, name

private:
    void captureLoop();

    QString m_id;
    QString m_name;
    QString m_deviceId;
    ID3D11Device* m_device = nullptr;
    std::atomic<bool> m_running{false};
    std::thread m_thread;
    mutable std::mutex m_frameMutex;
    ID3D11Texture2D* m_texture = nullptr;
    qint64 m_ptsUs = 0;
    int m_width = 1280;
    int m_height = 720;
};

} // namespace railshot
