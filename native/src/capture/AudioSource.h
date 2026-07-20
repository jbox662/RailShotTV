#pragma once

#include "capture/IVideoSource.h"
#include "audio/AudioTypes.h"
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>

struct ID3D11Texture2D;

namespace railshot {

class WasapiCapture;
class AudioGraph;

/// Audio-only scene source with a dark placeholder texture for Preview selection.
class AudioSource : public IVideoSource {
public:
    AudioSource(QString id, QString name, AudioDeviceKind kind, QString deviceId, AudioGraph* graph);
    ~AudioSource() override;

    QString id() const override { return m_id; }
    QString name() const override { return m_name; }
    bool start(ID3D11Device* device, QString* error = nullptr) override;
    void stop() override;
    bool isRunning() const override { return m_running.load(); }
    bool acquireLatest(VideoFrame& out) override;
    QSize size() const override { return {320, 180}; }

private:
    bool ensureTexture(QString* error);

    QString m_id;
    QString m_name;
    AudioDeviceKind m_kind;
    QString m_deviceId;
    AudioGraph* m_graph = nullptr;
    std::unique_ptr<WasapiCapture> m_capture;
    ID3D11Device* m_device = nullptr;
    ID3D11Texture2D* m_texture = nullptr;
    std::atomic<bool> m_running{false};
    mutable std::mutex m_mutex;
    qint64 m_ptsUs = 0;
};

} // namespace railshot
