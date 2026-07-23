#pragma once

#include "capture/IVideoSource.h"
#include <QStringList>
#include <QImage>
#include <QElapsedTimer>
#include <mutex>

struct ID3D11Texture2D;

namespace railshot {

/// OBS-style slideshow: cycle images with cut/fade/swipe + randomize.
class SlideshowSource : public IVideoSource {
public:
    SlideshowSource(QString id, QString name, QStringList paths, int intervalMs, bool loop,
                    QString transition, int transitionMs, bool randomize, int swipeDir);
    ~SlideshowSource() override;

    QString id() const override { return m_id; }
    QString name() const override { return m_name; }
    bool start(ID3D11Device* device, QString* error = nullptr) override;
    void stop() override;
    bool isRunning() const override { return m_running; }
    bool acquireLatest(VideoFrame& out) override;
    QSize size() const override { return {m_width, m_height}; }

    /// Manual step (hotkeys / UI). delta +1 next, -1 previous.
    void requestStep(int delta);

private:
    bool loadImage(int index, QImage* out, QString* error);
    bool uploadImage(const QImage& img);
    void beginAdvanceTo(int tryIdx);
    void beginAdvance();
    void tickTransition();
    int pickNextIndex() const;
    int pickPrevIndex() const;

    QString m_id, m_name;
    QStringList m_paths;
    int m_intervalMs = 5000;
    bool m_loop = true;
    QString m_transition = QStringLiteral("cut"); // cut|fade|swipe
    int m_transitionMs = 700;
    bool m_randomize = false;
    int m_swipeDir = 0; // 0=L→R, 1=R→L, 2=U→D, 3=D→U
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
    int m_pendingStep = 0; // applied on acquire thread
};

} // namespace railshot
