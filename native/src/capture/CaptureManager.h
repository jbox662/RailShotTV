#pragma once

#include "capture/IVideoSource.h"
#include "capture/FrameBus.h"
#include "core/Types.h"
#include <QObject>
#include <QHash>
#include <memory>

struct ID3D11Device;

namespace railshot {

class CaptureManager : public QObject {
    Q_OBJECT
public:
    explicit CaptureManager(QObject* parent = nullptr);
    ~CaptureManager() override;

    void setDevice(ID3D11Device* device);
    FrameBus& frameBus() { return m_bus; }

    bool attachSource(const SourceItem& source, QString* error = nullptr);
    void updateSource(const SourceItem& source);
    void setOverlayCanvasSize(int width, int height);
    void detachSource(const QString& sourceId);
    void detachAll();
    void syncWithScene(const SceneItem& scene);

    IVideoSource* source(const QString& id) const;
    QVector<QString> activeSourceIds() const;

    /// Poll all sources into the frame bus (call from render thread).
    void poll();

signals:
    void sourceStarted(const QString& id);
    void sourceStopped(const QString& id);
    void sourceError(const QString& id, const QString& message);

private:
    std::unique_ptr<IVideoSource> createSource(const SourceItem& source);

    ID3D11Device* m_device = nullptr;
    FrameBus m_bus;
    QHash<QString, std::shared_ptr<IVideoSource>> m_sources;
};

} // namespace railshot
