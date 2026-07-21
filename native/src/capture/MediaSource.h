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

/// Media source (local file or network URL via FFmpeg). Still images use Qt when local.
class MediaSource : public IVideoSource {
public:
    MediaSource(QString id, QString name, QString input, bool loop = true,
                bool isLocalFile = true, QString ffmpegOptions = {});
    ~MediaSource() override;

    QString id() const override { return m_id; }
    QString name() const override { return m_name; }
    bool start(ID3D11Device* device, QString* error = nullptr) override;
    void stop() override;
    bool isRunning() const override { return m_running.load(); }
    bool acquireLatest(VideoFrame& out) override;
    QSize size() const override { return {m_width, m_height}; }

    void setPath(const QString& path);
    void setLoop(bool loop);
    void setLocalFile(bool local);
    void setFfmpegOptions(const QString& opts);
    void setAudioCallback(std::function<void(const AudioBuffer&)> cb);

    static bool looksLikeNetworkUrl(const QString& input);

private:
    bool startStill(QString* error);
    bool startFfmpeg(QString* error);
    void decodeLoop();
    bool uploadFrame(const uint8_t* bgra, int stride, int w, int h);
    void emitAudio(const float* interleaved, int frames, int channels, int sampleRate);
    bool useNetworkPath() const;

    QString m_id, m_name, m_filePath;
    bool m_loop = true;
    bool m_isLocalFile = true;
    QString m_ffmpegOptions;
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
