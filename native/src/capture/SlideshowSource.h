#pragma once

#include "capture/IVideoSource.h"
#include <QStringList>
#include <QElapsedTimer>
#include <mutex>

struct ID3D11Texture2D;

namespace railshot {

/// Minimal OBS-style slideshow: cycle image files on an interval (hard cut).
class SlideshowSource : public IVideoSource {
public:
    SlideshowSource(QString id, QString name, QStringList paths, int intervalMs, bool loop);
    ~SlideshowSource() override;

    QString id() const override { return m_id; }
    QString name() const override { return m_name; }
    bool start(ID3D11Device* device, QString* error = nullptr) override;
    void stop() override;
    bool isRunning() const override { return m_running; }
    bool acquireLatest(VideoFrame& out) override;
    QSize size() const override { return {m_width, m_height}; }

private:
    bool loadIndex(int index, QString* error);
    void advanceIfNeeded();

    QString m_id, m_name;
    QStringList m_paths;
    int m_intervalMs = 5000;
    bool m_loop = true;
    int m_index = 0;
    ID3D11Device* m_device = nullptr;
    ID3D11Texture2D* m_texture = nullptr;
    bool m_running = false;
    mutable std::mutex m_mutex;
    int m_width = 0, m_height = 0;
    QElapsedTimer m_timer;
};

} // namespace railshot
