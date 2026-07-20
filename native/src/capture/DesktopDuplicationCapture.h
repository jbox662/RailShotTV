#pragma once

#include "capture/IVideoSource.h"
#include <atomic>
#include <mutex>
#include <thread>

struct IDXGIOutputDuplication;
struct ID3D11Texture2D;
struct ID3D11Device;
struct ID3D11DeviceContext;

namespace railshot {

/// DXGI Desktop Duplication API fallback for full-monitor capture.
class DesktopDuplicationCapture : public IVideoSource {
public:
    DesktopDuplicationCapture(QString id, QString name, int outputIndex = 0);
    ~DesktopDuplicationCapture() override;

    QString id() const override { return m_id; }
    QString name() const override { return m_name; }
    bool start(ID3D11Device* device, QString* error = nullptr) override;
    void stop() override;
    bool isRunning() const override { return m_running.load(); }
    bool acquireLatest(VideoFrame& out) override;
    QSize size() const override { return {m_width, m_height}; }

private:
    void captureLoop();
    bool recreateDuplication(QString* error);

    QString m_id;
    QString m_name;
    int m_outputIndex = 0;
    ID3D11Device* m_device = nullptr;
    ID3D11DeviceContext* m_context = nullptr;
    IDXGIOutputDuplication* m_duplication = nullptr;
    ID3D11Texture2D* m_texture = nullptr;
    std::atomic<bool> m_running{false};
    std::thread m_thread;
    mutable std::mutex m_frameMutex;
    qint64 m_ptsUs = 0;
    int m_width = 1920;
    int m_height = 1080;
};

} // namespace railshot
