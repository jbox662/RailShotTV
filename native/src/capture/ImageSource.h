#pragma once

#include "capture/IVideoSource.h"
#include <mutex>

struct ID3D11Texture2D;

namespace railshot {

class ImageSource : public IVideoSource {
public:
    ImageSource(QString id, QString name, QString filePath);
    ~ImageSource() override;

    QString id() const override { return m_id; }
    QString name() const override { return m_name; }
    bool start(ID3D11Device* device, QString* error = nullptr) override;
    void stop() override;
    bool isRunning() const override { return m_running; }
    bool acquireLatest(VideoFrame& out) override;
    QSize size() const override { return {m_width, m_height}; }

private:
    QString m_id, m_name, m_filePath;
    ID3D11Device* m_device = nullptr;
    ID3D11Texture2D* m_texture = nullptr;
    bool m_running = false;
    mutable std::mutex m_mutex;
    int m_width = 0, m_height = 0;
};

} // namespace railshot
