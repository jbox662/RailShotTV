#pragma once

#include "capture/IVideoSource.h"
#include <QStringList>
#include <QImage>
#include <QElapsedTimer>
#include <mutex>

struct ID3D11Texture2D;

namespace railshot {

/// OBS-style slideshow: cycle images with optional fade + randomize.
class SlideshowSource : public IVideoSource {
public:
    SlideshowSource(QString id, QString name, QStringList paths, int intervalMs, bool loop,
                    QString transition, int transitionMs, bool randomize);
    ~SlideshowSource() override;

    QString id() const override { return m_id; }
    QString name() const override { return m_name; }
    bool start(ID3D11Device* device, QString* error = nullptr) override;
    void stop() override;
    bool isRunning() const override { return m_running; }
    bool acquireLatest(VideoFrame& out) override;
    QSize size() const override { return {m_width, m_height}; }

private:
    bool loadImage(int index, QImage* out, QString* error);
    bool uploadImage(const QImage& img);
    void beginAdvance();
    void tickFade();
    int pickNextIndex() const;

    QString m_id, m_name;
    QStringList m_paths;
    int m_intervalMs = 5000;
    bool m_loop = true;
    QString m_transition = QStringLiteral("cut"); // cut|fade
    int m_transitionMs = 700;
    bool m_randomize = false;
    int m_index = 0;
    ID3D11Device* m_device = nullptr;
    ID3D11Texture2D* m_texture = nullptr;
    bool m_running = false;
    mutable std::mutex m_mutex;
    int m_width = 0, m_height = 0;
    QElapsedTimer m_slideTimer;
    QElapsedTimer m_fadeTimer;
    bool m_fading = false;
    QImage m_fromImg;
    QImage m_toImg;
};

} // namespace railshot
