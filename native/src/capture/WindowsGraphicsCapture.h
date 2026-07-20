#pragma once

#include "capture/IVideoSource.h"
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>

struct ID3D11Texture2D;
struct ID3D11Device;
struct ID3D11DeviceContext;

namespace railshot {

/// Windows.Graphics.Capture for monitor (by index) or window (by HWND).
/// Window targets fall back to PrintWindow/BitBlt when WGC is unavailable.
class WindowsGraphicsCapture : public IVideoSource {
public:
    enum class TargetKind { Monitor, Window };

    WindowsGraphicsCapture(QString id, QString name, TargetKind kind = TargetKind::Monitor,
                           int monitorIndex = 0, quintptr hwnd = 0);
    WindowsGraphicsCapture(QString id, QString name, quintptr hwnd);
    ~WindowsGraphicsCapture() override;

    QString id() const override { return m_id; }
    QString name() const override { return m_name; }
    bool start(ID3D11Device* device, QString* error = nullptr) override;
    void stop() override;
    bool isRunning() const override { return m_running.load(); }
    bool acquireLatest(VideoFrame& out) override;
    QSize size() const override { return {m_width, m_height}; }

    TargetKind targetKind() const { return m_kind; }
    int monitorIndex() const { return m_monitorIndex; }
    quintptr hwnd() const { return m_hwnd; }

    static bool isSupported();

private:
    void captureLoop();
    void signalStartResult(bool ok, const QString& error);
    bool ensureTexture(int width, int height, QString* error = nullptr);
    bool resolveTargetSize(int* width, int* height, QString* error) const;
    bool captureLoopWgc(QString* initError);
    void captureLoopBitBlt();

    QString m_id;
    QString m_name;
    TargetKind m_kind = TargetKind::Monitor;
    int m_monitorIndex = 0;
    quintptr m_hwnd = 0;
    ID3D11Device* m_device = nullptr;
    ID3D11DeviceContext* m_context = nullptr;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_useWgc{false};
    std::thread m_thread;
    mutable std::mutex m_frameMutex;
    ID3D11Texture2D* m_texture = nullptr;
    qint64 m_ptsUs = 0;
    int m_width = 1920;
    int m_height = 1080;

    std::mutex m_startMutex;
    std::condition_variable m_startCv;
    bool m_startSignaled = false;
    bool m_startOk = false;
    QString m_startError;
};

} // namespace railshot
