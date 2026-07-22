#pragma once

#include "capture/IVideoSource.h"
#include "capture/FrameBus.h"
#include "audio/AudioTypes.h"
#include "core/Types.h"
#include <QObject>
#include <QHash>
#include <QVector>
#include <functional>
#include <memory>

struct ID3D11Device;

namespace railshot {

class CaptureManager : public QObject {
    Q_OBJECT
public:
    explicit CaptureManager(QObject* parent = nullptr);
    ~CaptureManager() override;

    void setDevice(ID3D11Device* device);
    void setAudioGraph(class AudioGraph* graph);
    FrameBus& frameBus() { return m_bus; }

    /// Called when Media/NDI sources produce audio (sourceId, display name, buffer).
    void setSourceAudioCallback(std::function<void(const QString&, const QString&, const AudioBuffer&)> cb);

    bool attachSource(const SourceItem& source, QString* error = nullptr);
    void updateSource(const SourceItem& source);
    void setOverlayCanvasSize(int width, int height);
    void detachSource(const QString& sourceId);
    void detachAll();
    void syncWithScene(const SceneItem& scene);
    /// Attach visible sources from all scenes; detach anything not listed.
    void syncWithScenes(const QVector<const SceneItem*>& scenes);

    /// Keep a source attached while Properties (etc.) is open, even if not in the
    /// active preview/program scene sync set.
    void pinSource(const SourceItem& source);
    void unpinSource(const QString& sourceId);

    IVideoSource* source(const QString& id) const;
    QVector<QString> activeSourceIds() const;

    void setPaused(bool paused) { m_paused = paused; }
    bool isPaused() const { return m_paused; }

    /// Poll all sources into the frame bus (call from render thread).
    void poll();
    /// Poll a single source into the frame bus (Properties preview).
    bool pollSource(const QString& sourceId);

    /// OBS "Restart when activated" for Media sources in a scene.
    void restartMediaOnActivate(const SceneItem& scene);

signals:
    void sourceStarted(const QString& id);
    void sourceStopped(const QString& id);
    void sourceError(const QString& id, const QString& message);
    void mediaEnded(const QString& id);

private:
    std::unique_ptr<IVideoSource> createSource(const SourceItem& source);
    void wireSourceAudio(IVideoSource* src, const SourceItem& source);
    void reattach(const SourceItem& source);

    ID3D11Device* m_device = nullptr;
    class AudioGraph* m_audioGraph = nullptr;
    FrameBus m_bus;
    QHash<QString, std::shared_ptr<IVideoSource>> m_sources;
    QHash<QString, SourceItem> m_pinned;
    bool m_paused = false;
    std::function<void(const QString&, const QString&, const AudioBuffer&)> m_audioCb;
};

} // namespace railshot
