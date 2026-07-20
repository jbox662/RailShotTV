#pragma once

#include "capture/IVideoSource.h"
#include "core/Types.h"
#include <QJsonObject>
#include <QProcess>
#include <mutex>
#include <atomic>

namespace railshot {

/// Browser overlay source: isolated helper process → shared-memory BGRA frames (ADR-004).
class BrowserSource : public IVideoSource {
public:
    BrowserSource(QString id, QString name, QJsonObject settings = {});
    ~BrowserSource() override;

    QString id() const override { return m_id; }
    QString name() const override { return m_name; }
    bool start(ID3D11Device* device, QString* error = nullptr) override;
    void stop() override;
    bool isRunning() const override { return m_running.load(); }
    bool acquireLatest(VideoFrame& out) override;
    QSize size() const override { return {m_width, m_height}; }

    void applySettings(const QJsonObject& settings);

private:
    bool openSharedMemory(QString* error);
    void closeSharedMemory();
    bool ensureHelper(QString* error);
    bool uploadLatest(QString* error);
    QString helperExecutable() const;
    QString mappingName() const;

    QString m_id;
    QString m_name;
    QJsonObject m_settings;
    ID3D11Device* m_device = nullptr;
    ID3D11Texture2D* m_texture = nullptr;
    std::atomic<bool> m_running{false};
    mutable std::mutex m_mutex;
    int m_width = 1280;
    int m_height = 720;
    quint64 m_lastFrameIndex = 0;
    QProcess m_helper;

#ifdef _WIN32
    void* m_mapping = nullptr;
    void* m_view = nullptr;
    void* m_mutexHandle = nullptr;
#endif
};

} // namespace railshot
