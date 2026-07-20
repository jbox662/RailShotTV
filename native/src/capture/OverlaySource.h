#pragma once

#include "capture/IVideoSource.h"
#include "core/Types.h"
#include <QJsonObject>
#include <mutex>
#include <atomic>

namespace railshot {

/// CPU-rendered overlay uploaded as a transparent D3D11 BGRA texture.
class OverlaySource : public IVideoSource {
public:
    OverlaySource(QString id, QString name, SourceType type, QJsonObject settings = {});
    ~OverlaySource() override;

    QString id() const override { return m_id; }
    QString name() const override { return m_name; }
    bool start(ID3D11Device* device, QString* error = nullptr) override;
    void stop() override;
    bool isRunning() const override { return m_running.load(); }
    bool acquireLatest(VideoFrame& out) override;
    QSize size() const override { return {m_width, m_height}; }

    void applySettings(const QJsonObject& settings);
    void setCanvasSize(int width, int height);

private:
    bool rebuildTexture(QString* error);

    QString m_id;
    QString m_name;
    SourceType m_type = SourceType::LowerThird;
    QJsonObject m_settings;
    ID3D11Device* m_device = nullptr;
    ID3D11Texture2D* m_texture = nullptr;
    std::atomic<bool> m_running{false};
    mutable std::mutex m_mutex;
    int m_width = 1920;
    int m_height = 1080;
};

} // namespace railshot
