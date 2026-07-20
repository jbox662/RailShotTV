#pragma once

#include "capture/IVideoSource.h"
#include <QString>
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

struct ID3D11Texture2D;
struct ID3D11DeviceContext;

namespace railshot {

/// NDI receive source. Dynamically loads Processing.NDI.Lib when present.
class NdiSource : public IVideoSource {
public:
    NdiSource(QString id, QString name, QString sourceName);
    ~NdiSource() override;

    QString id() const override { return m_id; }
    QString name() const override { return m_name; }
    bool start(ID3D11Device* device, QString* error = nullptr) override;
    void stop() override;
    bool isRunning() const override { return m_running.load(); }
    bool acquireLatest(VideoFrame& out) override;
    QSize size() const override { return {m_width, m_height}; }

    static QStringList discoverSources(int waitMs = 1000);

private:
    void receiveLoop();
    bool uploadBgra(const uint8_t* bgra, int stride, int w, int h);

    QString m_id, m_name, m_sourceName;
    ID3D11Device* m_device = nullptr;
    ID3D11DeviceContext* m_context = nullptr;
    ID3D11Texture2D* m_texture = nullptr;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_stop{false};
    mutable std::mutex m_mutex;
    std::thread m_thread;
    int m_width = 0, m_height = 0;
};

} // namespace railshot
