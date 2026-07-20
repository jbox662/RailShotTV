#include "core/EngineController.h"
#include "capture/CaptureManager.h"
#include "compositor/D3D11Device.h"
#include "compositor/D3D11Compositor.h"
#include "compositor/TransitionEngine.h"
#include "audio/AudioGraph.h"
#include "output/OutputHub.h"
#include "telemetry/HealthTelemetry.h"
#include "scoreboard/ScoreboardModel.h"
#include "chat/ChatService.h"
#include "overlays/ReplayBuffer.h"
#include "overlays/VirtualCamera.h"
#include "recording/IsoRecordManager.h"
#include "core/Logger.h"
#include <QDateTime>
#include <QDir>
#include <QTimer>

namespace railshot {

EngineController::EngineController(QObject* parent)
    : QObject(parent)
{
    qRegisterMetaType<EncodedPacket>("railshot::EncodedPacket");
    m_sceneGraph = std::make_unique<SceneGraph>();
    m_settings = std::make_unique<SettingsStore>();
    m_telemetry = std::make_unique<HealthTelemetry>();
    m_capture = std::make_unique<CaptureManager>();
    m_audio = std::make_unique<AudioGraph>();
    m_outputs = std::make_unique<OutputHub>();
    m_scoreboard = std::make_unique<ScoreboardModel>();
    m_chat = std::make_unique<ChatService>();
    m_replay = std::make_unique<ReplayBuffer>();
    m_vcam = std::make_unique<VirtualCamera>();
    m_iso = new IsoRecordManager(this);

    connect(&m_telemetryTimer, &QTimer::timeout, this, &EngineController::onTelemetryTick);
    connect(&m_autosaveTimer, &QTimer::timeout, this, &EngineController::onAutosave);
    connect(m_telemetry.get(), &HealthTelemetry::updated, this, &EngineController::telemetryUpdated);
    connect(m_outputs.get(), &OutputHub::errorOccurred, this, &EngineController::errorOccurred);
    connect(m_scoreboard.get(), &ScoreboardModel::changed, this, &EngineController::onScoreboardChanged);

    connect(m_outputs.get(), &OutputHub::codecConfigReady, this,
            [this](const OutputProfile& profile, const QByteArray& vExtra, const QByteArray& aExtra,
                   int rate, int ch) {
                if (m_replay)
                    m_replay->setCodecConfig(profile, vExtra, aExtra, rate, ch);
            });
    connect(m_outputs.get(), &OutputHub::encodedPacket, this, [this](const EncodedPacket& pkt) {
        if (!m_replay) return;
        if (pkt.video) m_replay->pushVideo(pkt);
        else m_replay->pushAudio(pkt);
    });
}

EngineController::~EngineController()
{
    shutdown();
}

bool EngineController::initialize(QString* error)
{
    if (m_initialized) return true;

    m_d3d = std::make_unique<D3D11Device>();
    if (!m_d3d->initialize(error))
        return false;

    m_compositor = std::make_unique<D3D11Compositor>(m_d3d.get());
    const auto profile = m_settings->outputProfile();
    const int w = profile.width > 0 ? profile.width : kDefaultCanvasWidth;
    const int h = profile.height > 0 ? profile.height : kDefaultCanvasHeight;
    if (!m_compositor->initialize(w, h, error))
        return false;

    m_capture->setDevice(m_d3d->device());
    m_capture->setAudioGraph(m_audio.get());
    m_capture->setOverlayCanvasSize(w, h);
    if (!m_audio->initialize(m_settings->desktopDeviceId(), m_settings->micDeviceId(), error))
        return false;

    m_capture->setSourceAudioCallback([this](const QString& id, const QString& name, const AudioBuffer& buf) {
        if (!m_audio) return;
        m_audio->ensureChannel(id, name);
        m_audio->inject(id, buf);
    });
    connect(m_capture.get(), &CaptureManager::sourceStopped, this, [this](const QString& id) {
        if (m_audio) m_audio->removeChannel(id);
    });

    m_transition = new TransitionEngine(this);
    m_replay->setCapacitySeconds(30);

    m_audio->setOutputCallback([this](const AudioBuffer& buf) {
        if (m_outputs)
            m_outputs->submitAudio(buf);
        if (m_iso)
            m_iso->submitAudio(buf);
        if (m_telemetry)
            m_telemetry->setAvDriftMs(m_audio->clock().driftMs(buf.ptsUs));
    });

    auto* renderTimer = new QTimer(this);
    connect(renderTimer, &QTimer::timeout, this, [this] {
        const auto project = m_sceneGraph->snapshot();
        QVector<const SceneItem*> scenes;
        if (const auto* preview = project.findScene(project.previewSceneId))
            scenes.push_back(preview);
        else if (const auto* edit = project.findScene(project.editSceneId()))
            scenes.push_back(edit);
        if (const auto* program = project.findScene(project.programSceneId)) {
            bool listed = false;
            for (const auto* s : scenes) {
                if (s == program) { listed = true; break; }
            }
            if (!listed) scenes.push_back(program);
        }
        m_capture->syncWithScenes(scenes);
        m_capture->poll();

        float mix = 1.0f;
        TransitionType activeType = TransitionType::Cut;
        if (m_transition) {
            mix = m_transition->tick();
            activeType = m_transition->type();
        }

        // Preview paints the Preview scene; if cleared, fall back to edit scene (OBS current).
        const SceneItem* previewScene = project.findScene(project.previewSceneId);
        if (!previewScene)
            previewScene = project.findScene(project.editSceneId());
        if (previewScene)
            m_compositor->compose(*previewScene, m_capture->frameBus(), false, 1.0f);
        if (const auto* program = project.findScene(project.programSceneId)) {
            // Crossfade: compose new program at full, then blend hold on top
            const bool cross = m_transition && m_transition->isActive() && m_transition->isCrossfade()
                               && m_compositor->hasProgramHold();
            m_compositor->compose(*program, m_capture->frameBus(), true, cross ? 1.0f : mix);
            if (cross)
                m_compositor->blendProgramHold(mix, activeType);
        }

        const qint64 pts = m_audio->clock().nowUs();
        if (m_outputs && (m_outputs->isRecording() || m_outputs->isStreaming()))
            m_outputs->submitVideo(m_compositor->programTexture(), pts);

        if (m_vcam && m_vcam->isRunning())
            m_vcam->submitFrame(m_compositor->programTexture());

        tickIsoRecorders(pts);

        m_telemetry->noteRenderedFrame();
        if (m_outputs && m_outputs->isStreaming())
            m_telemetry->noteEncodedFrame();
    });
    renderTimer->start(16);

    m_telemetryTimer.start(500);
    m_autosaveTimer.start(30000);
    m_initialized = true;
    m_state = EngineState::Previewing;
    rebuildSourcesForActiveScenes();
    Logger::info(QStringLiteral("EngineController initialized"));
    return true;
}

void EngineController::shutdown()
{
    if (!m_initialized) return;
    stopStreaming();
    stopRecording();
    if (m_iso) m_iso->stopAll();
    if (m_vcam) m_vcam->stop();
    if (m_chat) m_chat->disconnectPlatform(QStringLiteral("twitch"));
    if (m_audio) m_audio->shutdown();
    if (m_capture) m_capture->detachAll();
    if (m_compositor) m_compositor->shutdown();
    if (m_d3d) m_d3d->shutdown();
    m_initialized = false;
}

Project EngineController::projectSnapshot() const
{
    return m_sceneGraph->snapshot();
}

bool EngineController::loadProject(const QString& path, QString* error)
{
    auto p = Project::loadFromFile(path, error);
    if (!p) return false;
    if (m_capture) m_capture->detachAll();
    m_sceneGraph->replace(*p);
    m_settings->setLastProjectPath(path);
    rebuildSourcesForActiveScenes();
    emit projectLoaded(path);
    return true;
}

bool EngineController::saveProject(const QString& path, QString* error)
{
    auto p = m_sceneGraph->snapshot();
    p.path = path;
    if (!p.saveToFile(path, error)) return false;
    m_sceneGraph->replace(p);
    m_settings->setLastProjectPath(path);
    emit projectSaved(path);
    return true;
}

bool EngineController::newProject()
{
    Project p;
    p.ensureDefaults();
    if (m_capture) m_capture->detachAll();
    if (m_iso) m_iso->stopAll();
    m_sceneGraph->replace(p);
    rebuildSourcesForActiveScenes();
    return true;
}

void EngineController::setPreviewScene(const QString& sceneId)
{
    m_sceneGraph->setPreviewSceneId(sceneId);
}

void EngineController::setProgramScene(const QString& sceneId)
{
    m_sceneGraph->setProgramSceneId(sceneId);
}

void EngineController::go(TransitionType type)
{
    auto p = m_sceneGraph->snapshot();
    if (p.previewSceneId.isEmpty()) {
        emit errorOccurred(QStringLiteral("Nothing in Preview"));
        return;
    }
    const TransitionType effective = type;
    if (m_transition) {
        m_transition->configure(effective, p.transitionMs);
        m_sceneGraph->setTransition(effective, p.transitionMs);
        emit transitionStarted(effective);
        if (effective == TransitionType::Cut) {
            m_sceneGraph->setProgramSceneId(p.previewSceneId);
            if (m_compositor) m_compositor->clearProgramHold();
            emit transitionFinished();
        } else if (m_transition->isCrossfade()) {
            // Snapshot current program, switch immediately, blend hold → new
            if (m_compositor) m_compositor->captureProgramHold();
            m_sceneGraph->setProgramSceneId(p.previewSceneId);
            connect(m_transition, &TransitionEngine::finished, this, [this] {
                if (m_compositor) m_compositor->clearProgramHold();
                emit transitionFinished();
            }, Qt::SingleShotConnection);
            m_transition->start();
        } else {
            // FTB: fade out old, swap, fade in new
            const QString target = p.previewSceneId;
            connect(m_transition, &TransitionEngine::phaseChanged, this, [this, target](TransitionEngine::Phase phase) {
                if (phase == TransitionEngine::Phase::In)
                    m_sceneGraph->setProgramSceneId(target);
            }, Qt::SingleShotConnection);
            connect(m_transition, &TransitionEngine::finished, this, &EngineController::transitionFinished, Qt::SingleShotConnection);
            m_transition->start();
        }
    } else {
        m_sceneGraph->setProgramSceneId(p.previewSceneId);
    }
}

void EngineController::setInputsPaused(bool paused)
{
    m_inputsPaused = paused;
    if (m_capture)
        m_capture->setPaused(paused);
}

void EngineController::setTransition(TransitionType type, int durationMs)
{
    m_sceneGraph->setTransition(type, durationMs);
    if (m_transition) m_transition->configure(type, durationMs);
}

QString EngineController::addSource(SourceType type, const QString& name, const QJsonObject& settings)
{
    QString id;
    m_sceneGraph->mutate([&](Project& p) {
        p.ensureDefaults();
        // OBS adds to the current (Preview) scene so the item paints immediately.
        const QString sceneId = p.editSceneId();
        if (sceneId.isEmpty())
            return;
        if (p.previewSceneId.isEmpty())
            p.previewSceneId = sceneId;
        if (p.activeSceneId.isEmpty())
            p.activeSceneId = sceneId;
        id = p.addSource(sceneId, type, name);
        if (auto* s = p.findSource(sceneId, id)) {
            if (type == SourceType::Scoreboard)
                s->settings = m_scoreboard->state().toJson();
            else if (type == SourceType::LowerThird) {
                s->settings.insert(QStringLiteral("title"), name.isEmpty() ? QStringLiteral("Lower Third") : name);
                s->settings.insert(QStringLiteral("subtitle"), QStringLiteral("RailShotTV"));
            } else if (type == SourceType::Alert) {
                s->settings.insert(QStringLiteral("title"), QStringLiteral("Alert"));
                s->settings.insert(QStringLiteral("body"), QStringLiteral("New follower"));
            }
            for (auto it = settings.begin(); it != settings.end(); ++it)
                s->settings.insert(it.key(), it.value());
            s->visible = true;
        }
    });
    if (!id.isEmpty())
        rebuildSourcesForActiveScenes();
    return id;
}

void EngineController::removeSource(const QString& sourceId)
{
    m_sceneGraph->mutate([&](Project& p) {
        // Remove from whichever scene owns it (usually the edit scene).
        for (auto& sc : p.scenes) {
            for (int i = 0; i < sc.sources.size(); ++i) {
                if (sc.sources[i].id == sourceId) {
                    sc.sources.removeAt(i);
                    return;
                }
            }
        }
    });
    m_capture->detachSource(sourceId);
}

void EngineController::setSourceVisible(const QString& sourceId, bool visible)
{
    m_sceneGraph->mutate([&](Project& p) {
        if (auto* s = p.findSourceAnywhere(sourceId))
            s->visible = visible;
    });
}

void EngineController::setSourceName(const QString& sourceId, const QString& name)
{
    m_sceneGraph->mutate([&](Project& p) {
        if (auto* s = p.findSourceAnywhere(sourceId))
            s->name = name;
    });
}

void EngineController::setSourceLocked(const QString& sourceId, bool locked)
{
    m_sceneGraph->mutate([&](Project& p) {
        if (auto* s = p.findSourceAnywhere(sourceId))
            s->locked = locked;
    });
}

void EngineController::moveSourceZOrder(const QString& sourceId, int delta)
{
    m_sceneGraph->mutate([&](Project& p) {
        // Prefer scene that owns the source
        for (const auto& sc : p.scenes) {
            for (const auto& src : sc.sources) {
                if (src.id == sourceId) {
                    p.moveSource(sc.id, sourceId, delta);
                    return;
                }
            }
        }
    });
}

void EngineController::setSelectedSourceId(const QString& sourceId)
{
    if (m_selectedSourceId == sourceId) return;
    m_selectedSourceId = sourceId;
    emit selectedSourceChanged(sourceId);
}

std::optional<SourceItem> EngineController::selectedSource() const
{
    if (m_selectedSourceId.isEmpty() || !m_sceneGraph) return std::nullopt;
    const auto p = m_sceneGraph->snapshot();
    if (const auto* src = p.findSourceAnywhere(m_selectedSourceId))
        return *src;
    return std::nullopt;
}

void EngineController::updateSourceTransform(const QString& sourceId, const Transform& t)
{
    m_sceneGraph->mutate([&](Project& p) {
        if (auto* s = p.findSourceAnywhere(sourceId))
            s->transform = t;
    });
}

void EngineController::updateSourceSettings(const QString& sourceId, const QJsonObject& settings)
{
    SourceItem updated;
    bool found = false;
    m_sceneGraph->mutate([&](Project& p) {
        if (auto* s = p.findSourceAnywhere(sourceId)) {
            s->settings = settings;
            updated = *s;
            found = true;
        }
    });
    if (found)
        m_capture->updateSource(updated);
}

void EngineController::onScoreboardChanged()
{
    pushScoreboardToProgram();
}

void EngineController::pushScoreboardToProgram()
{
    if (!m_scoreboard) return;
    const QJsonObject json = m_scoreboard->state().toJson();
    m_sceneGraph->mutate([&](Project& p) {
        for (auto& scene : p.scenes) {
            for (auto& src : scene.sources) {
                if (src.type == SourceType::Scoreboard)
                    src.settings = json;
            }
        }
    });
    // Refresh live textures
    const auto snap = m_sceneGraph->snapshot();
    for (const auto& scene : snap.scenes) {
        for (const auto& src : scene.sources) {
            if (src.type == SourceType::Scoreboard)
                m_capture->updateSource(src);
        }
    }
}

bool EngineController::startStreaming(const QString& targetId, QString* error)
{
    auto p = m_sceneGraph->snapshot();
    StreamTarget target;
    bool found = false;
    for (const auto& t : p.streamTargets) {
        if (t.id == targetId || targetId.isEmpty()) {
            target = t;
            found = true;
            if (!targetId.isEmpty()) break;
        }
    }
    if (!found) {
        target.id = newId(QStringLiteral("tgt"));
        target.name = QStringLiteral("Custom");
        target.platform = QStringLiteral("custom");
        target.rtmpUrl = QStringLiteral("rtmp://localhost/live");
    }
    return startStreamingTargets({target}, error);
}

bool EngineController::startStreamingTargets(const QVector<StreamTarget>& targets, QString* error)
{
    if (targets.isEmpty()) {
        if (error) *error = QStringLiteral("No stream targets");
        return false;
    }
    const auto p = m_sceneGraph->snapshot();
    const OutputProfile profile = p.output.width > 0 ? p.output : m_settings->outputProfile();
    if (!m_outputs->startStreaming(targets, profile, error))
        return false;
    m_telemetry->setStreaming(true);
    updateEngineState();
    return true;
}

void EngineController::stopStreaming()
{
    if (m_outputs) m_outputs->stopStreaming();
    m_telemetry->setStreaming(false);
    updateEngineState();
}

bool EngineController::startRecording(QString* error)
{
    auto profile = m_sceneGraph->snapshot().output;
    if (profile.width <= 0) profile = m_settings->outputProfile();
    if (profile.width <= 0) {
        profile.width = kDefaultCanvasWidth;
        profile.height = kDefaultCanvasHeight;
    }
    if (!m_outputs->startRecording(m_settings->recordingDirectory(), profile, error))
        return false;
    m_telemetry->setRecording(true);
    updateEngineState();
    return true;
}

void EngineController::stopRecording()
{
    if (m_outputs) m_outputs->stopRecording();
    m_telemetry->setRecording(false);
    updateEngineState();
}

bool EngineController::saveReplay(QString* error)
{
    if (!m_replay) {
        if (error) *error = QStringLiteral("Replay unavailable");
        return false;
    }
    QDir().mkpath(m_settings->recordingDirectory());
    const QString path = m_settings->recordingDirectory()
        + QStringLiteral("/RailShotTV-Replay-%1.mkv")
              .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss")));
    if (!m_replay->saveReplay(path, error))
        return false;
    emit replaySaved(path);
    return true;
}

bool EngineController::setVirtualCameraEnabled(bool enabled, QString* error)
{
    if (!m_vcam) {
        if (error) *error = QStringLiteral("Virtual camera unavailable");
        return false;
    }
    if (!enabled) {
        m_vcam->stop();
        return true;
    }
    auto profile = m_sceneGraph->snapshot().output;
    if (profile.width <= 0) profile = m_settings->outputProfile();
    return m_vcam->start(profile.width, profile.height, error);
}

TelemetrySnapshot EngineController::telemetrySnapshot() const
{
    return m_telemetry->snapshot();
}

QVector<AudioChannelState> EngineController::audioChannels() const
{
    return m_audio->channels();
}

void EngineController::onTelemetryTick()
{
    if (m_outputs && m_outputs->isStreaming()) {
        m_telemetry->setStreamState(m_outputs->streamState());
        m_telemetry->setStreamUptime(m_outputs->streamUptimeSec());
        m_telemetry->setBitrateKbps(m_outputs->bitrateKbps());
    }
    if (m_outputs && m_outputs->isRecording())
        m_telemetry->setRecordUptime(m_outputs->recordUptimeSec());
    m_telemetry->tick();
}

void EngineController::onAutosave()
{
    const auto path = m_settings->lastProjectPath();
    if (path.isEmpty()) return;
    QString err;
    saveProject(path, &err);
}

void EngineController::updateEngineState()
{
    emit engineStateChanged(m_telemetry->snapshot().state);
}

void EngineController::rebuildSourcesForActiveScenes()
{
    if (!m_capture || !m_sceneGraph) return;
    const auto project = m_sceneGraph->snapshot();
    QVector<const SceneItem*> scenes;
    if (const auto* preview = project.findScene(project.previewSceneId))
        scenes.push_back(preview);
    if (const auto* program = project.findScene(project.programSceneId)) {
        if (project.programSceneId != project.previewSceneId)
            scenes.push_back(program);
    }
    // Also keep active scene sources warm for editing
    if (const auto* active = project.findScene(project.activeSceneId)) {
        bool listed = false;
        for (const auto* s : scenes) {
            if (s == active) { listed = true; break; }
        }
        if (!listed) scenes.push_back(active);
    }
    m_capture->syncWithScenes(scenes);
}

void EngineController::tickIsoRecorders(qint64 ptsUs)
{
    if (!m_iso || !m_capture) return;
    const auto ids = m_capture->activeSourceIds();
    for (const auto& id : ids) {
        if (!m_iso->isArmed(id)) continue;
        auto frame = m_capture->frameBus().latest(id);
        if (frame && frame->texture)
            m_iso->submitVideo(id, frame->texture, ptsUs);
    }
}

bool EngineController::setSourceIsoRecording(const QString& sourceId, bool armed, QString* error)
{
    if (!m_iso) {
        if (error) *error = QStringLiteral("ISO recorder unavailable");
        return false;
    }
    if (!armed) {
        m_iso->stop(sourceId);
        return true;
    }
    auto profile = m_sceneGraph->snapshot().output;
    if (profile.width <= 0) profile = m_settings->outputProfile();
    if (profile.width <= 0) {
        profile.width = kDefaultCanvasWidth;
        profile.height = kDefaultCanvasHeight;
    }
    return m_iso->start(sourceId, m_settings->recordingDirectory(), profile, error);
}

bool EngineController::isSourceIsoRecording(const QString& sourceId) const
{
    return m_iso && m_iso->isArmed(sourceId);
}

bool EngineController::setExternalOutputEnabled(bool enabled, QString* error)
{
    // External = program feed out via virtual camera (Phase 2 path).
    return setVirtualCameraEnabled(enabled, error);
}

bool EngineController::externalOutputEnabled() const
{
    return m_vcam && m_vcam->isRunning();
}

} // namespace railshot
