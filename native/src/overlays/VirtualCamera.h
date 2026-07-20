#pragma once

#include <QObject>
#include <atomic>

struct ID3D11Texture2D;

namespace railshot {

/// Publishes program frames to shared memory and, on Windows 11, registers a system webcam via MF.
class VirtualCamera : public QObject {
    Q_OBJECT
public:
    explicit VirtualCamera(QObject* parent = nullptr);
    ~VirtualCamera() override;
    bool start(int width, int height, QString* error = nullptr);
    void stop();
    bool isRunning() const { return m_running.load(); }
    void submitFrame(ID3D11Texture2D* texture);

signals:
    void started();
    void stopped();
    void errorOccurred(const QString& message);

private:
    std::atomic<bool> m_running{false};
    int m_width = 1920;
    int m_height = 1080;
#ifdef _WIN32
    void* m_mapping = nullptr;
    void* m_view = nullptr;
    void* m_mutex = nullptr;
    void* m_mfCamera = nullptr; // IMFVirtualCamera*
#endif
};

} // namespace railshot
