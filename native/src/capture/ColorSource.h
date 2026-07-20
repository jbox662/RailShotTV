#pragma once

#include "capture/IVideoSource.h"
#include <QColor>
#include <mutex>

struct ID3D11Texture2D;

namespace railshot {

class ColorSource : public IVideoSource {
public:
    ColorSource(QString id, QString name, QColor color, int width = 1920, int height = 1080);
    ~ColorSource() override;

    QString id() const override { return m_id; }
    QString name() const override { return m_name; }
    bool start(ID3D11Device* device, QString* error = nullptr) override;
    void stop() override;
    bool isRunning() const override { return m_running; }
    bool acquireLatest(VideoFrame& out) override;
    QSize size() const override { return {m_width, m_height}; }

    void setColor(const QColor& color);

private:
    bool rebuildTexture(QString* error);

    QString m_id, m_name;
    QColor m_color;
    ID3D11Device* m_device = nullptr;
    ID3D11Texture2D* m_texture = nullptr;
    bool m_running = false;
    mutable std::mutex m_mutex;
    int m_width = 1920, m_height = 1080;
};

} // namespace railshot
