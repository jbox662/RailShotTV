#pragma once

#include "capture/IVideoSource.h"
#include "audio/AudioTypes.h"
#include <QString>
#include <atomic>
#include <functional>
#include <mutex>
#include <thread>

struct ID3D11Texture2D;
struct ID3D11DeviceContext;

namespace railshot {

/// File media source (video/image). Uses FFmpeg when available; falls back to still image load.
class MediaSource : public IVideoSource {
public:
    MediaSource(QString id, QString name, QString filePath, bool loop = true);
    ~MediaSource() override;

    QString id() const override { return m_id; }
    QString name() const override { return m_name; }
    bool start(ID3D11Device* device, QString* error = nullptr) override;
    void stop() override;
    bool isRunning() const override { return m_running.load(); }
    bool acquireLatest(VideoFrame& out) override;
    QSize size() const override { return {m_width, m_height}; }

    void setPath(const QString& path);
    void setAudioCallback(std::function<void(const AudioBuffer&)> cb);

private:
    bool startStill(QString* error);
    bool startFfmpeg(QString* error);
    void decodeLoop();
    bool uploadFrame(const uint8_t* bgra, int stride, int w, int h);
    void emitAudio(const float* interleaved, int frames, int channels, int sampleRate);

    QString m_id, m_name, m_filePath;
    bool m_loop = true;
    ID3D11Device* m_device = nullptr;
    ID3D11DeviceContext* m_context = nullptr;
    ID3D11Texture2D* m_texture = nullptr;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_stop{false};
    mutable std::mutex m_mutex;
    std::mutex m_audioCbMutex;
    std::function<void(const AudioBuffer&)> m_audioCb;
    std::thread m_thread;
    int m_width = 0, m_height = 0;
};

} // namespace railshot
