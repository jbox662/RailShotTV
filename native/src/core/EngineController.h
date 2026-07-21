#pragma once

#include "core/Types.h"
#include "core/SceneGraph.h"
#include "core/SettingsStore.h"
#include <QObject>
#include <QTimer>
#include <memory>
#include <optional>

namespace railshot {

class CaptureManager;
class D3D11Compositor;
class D3D11Device;
class AudioGraph;
class OutputHub;
class HealthTelemetry;
class ScoreboardModel;
class ChatService;
class ReplayBuffer;
class VirtualCamera;

class EngineController : public QObject {
    Q_OBJECT
public:
    explicit EngineController(QObject* parent = nullptr);
    ~EngineController() override;

    bool initialize(QString* error = nullptr);
    void shutdown();

    SceneGraph* sceneGraph() const { return m_sceneGraph.get(); }
    SettingsStore* settings() const { return m_settings.get(); }
    HealthTelemetry* telemetry() const { return m_telemetry.get(); }
    CaptureManager* capture() const { return m_capture.get(); }
    D3D11Compositor* compositor() const { return m_compositor.get(); }
    D3D11Device* graphicsDevice() const { return m_d3d.get(); }
    AudioGraph* audio() const { return m_audio.get(); }
    ScoreboardModel* scoreboard() const { return m_scoreboard.get(); }
    ChatService* chat() const { return m_chat.get(); }
    ReplayBuffer* replayBuffer() const { return m_replay.get(); }
    VirtualCamera* virtualCamera() const { return m_vcam.get(); }

    Project projectSnapshot() const;
    bool loadProject(const QString& path, QString* error = nullptr);
    bool saveProject(const QString& path, QString* error = nullptr);
    bool newProject();

    void setPreviewScene(const QString& sceneId);
    void setProgramScene(const QString& sceneId);
    void go(TransitionType type = TransitionType::Cut);
    void setTransition(TransitionType type, int durationMs);

    void setInputsPaused(bool paused);
    bool inputsPaused() const { return m_inputsPaused; }

    bool setSourceIsoRecording(const QString& sourceId, bool armed, QString* error = nullptr);
    bool isSourceIsoRecording(const QString& sourceId) const;
    bool setExternalOutputEnabled(bool enabled, QString* error = nullptr);
    bool externalOutputEnabled() const;

    QString addSource(SourceType type, const QString& name = {}, const QJsonObject& settings = {});
    void removeSource(const QString& sourceId);
    void setSourceVisible(const QString& sourceId, bool visible);
    void setSourceName(const QString& sourceId, const QString& name);
    void setSourceLocked(const QString& sourceId, bool locked);
    void moveSourceZOrder(const QString& sourceId, int delta);
    void updateSourceTransform(const QString& sourceId, const Transform& t);
    void updateSourceSettings(const QString& sourceId, const QJsonObject& settings);

    QString selectedSourceId() const { return m_selectedSourceId; }
    void setSelectedSourceId(const QString& sourceId);
    std::optional<SourceItem> selectedSource() const;

    bool startStreaming(const QString& targetId, QString* error = nullptr);
    bool startStreamingTargets(const QVector<StreamTarget>& targets, QString* error = nullptr);
    void stopStreaming();
    bool startRecording(QString* error = nullptr);
    void stopRecording();

    bool saveReplay(QString* error = nullptr);
    bool setVirtualCameraEnabled(bool enabled, QString* error = nullptr);
    /// Capture Preview or Program to PNG under the recording directory.
    bool takeScreenshot(bool program, QString* pathOut = nullptr, QString* error = nullptr);
    void pushScoreboardToProgram();

    TelemetrySnapshot telemetrySnapshot() const;
    QVector<AudioChannelState> audioChannels() const;

signals:
    void projectLoaded(const QString& path);
    void projectSaved(const QString& path);
    void engineStateChanged(EngineState state);
    void telemetryUpdated(const TelemetrySnapshot& snap);
    void errorOccurred(const QString& message);
    void transitionStarted(TransitionType type);
    void transitionFinished();
    void replaySaved(const QString& path);
    void screenshotSaved(const QString& path);
    void selectedSourceChanged(const QString& sourceId);

private slots:
    void onTelemetryTick();
    void onAutosave();
    void onScoreboardChanged();

private:
    void rebuildSourcesForActiveScenes();
    void updateEngineState();
    void tickIsoRecorders(qint64 ptsUs);
    void syncSourceAudioToGraph(const QString& sourceId, const QString& name, const QJsonObject& settings);

    std::unique_ptr<SceneGraph> m_sceneGraph;
    std::unique_ptr<SettingsStore> m_settings;
    std::unique_ptr<D3D11Device> m_d3d;
    std::unique_ptr<CaptureManager> m_capture;
    std::unique_ptr<D3D11Compositor> m_compositor;
    std::unique_ptr<AudioGraph> m_audio;
    std::unique_ptr<OutputHub> m_outputs;
    std::unique_ptr<HealthTelemetry> m_telemetry;
    std::unique_ptr<ScoreboardModel> m_scoreboard;
    std::unique_ptr<ChatService> m_chat;
    std::unique_ptr<ReplayBuffer> m_replay;
    std::unique_ptr<VirtualCamera> m_vcam;
    class TransitionEngine* m_transition = nullptr;

    QTimer m_telemetryTimer;
    QTimer m_autosaveTimer;
    EngineState m_state = EngineState::Idle;
    bool m_initialized = false;
    bool m_inputsPaused = false;
    QString m_selectedSourceId;
    class IsoRecordManager* m_iso = nullptr;
};

} // namespace railshot
