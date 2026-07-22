#include "core/EngineController.h"
#include "capture/CaptureManager.h"
#include "capture/OverlaySource.h"
#include "capture/MediaSource.h"
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
#include "recording/RemuxUtil.h"
#include "core/RecordingPath.h"
#include "core/Logger.h"
#include <QDateTime>
#include <QDir>
#include <QHash>
#include <QImage>
#include <QTimer>
#include <QJsonArray>
#include <QThread>
#include <algorithm>

namespace railshot {

namespace {

void appendUniqueScene(QVector<const SceneItem*>& scenes, const SceneItem* sc)
{
    if (!sc) return;
    for (const auto* s : scenes) {
        if (s == sc) return;
    }
    scenes.push_back(sc);
}

/// Pull in scenes referenced by Scene-as-source so nested capture stays warm.
void collectNestedScenes(const Project& project, QVector<const SceneItem*>& scenes)
{
    for (int pass = 0; pass < 4; ++pass) {
        const int n = scenes.size();
        for (int i = 0; i < n; ++i) {
            const SceneItem* sc = scenes[i];
            if (!sc) continue;
            for (const auto& src : sc->sources) {
                if (src.type != SourceType::Scene || !src.visible) continue;
                const QString nid = src.settings.value(QStringLiteral("sceneId")).toString();
                if (nid.isEmpty() || nid == sc->id) continue;
                appendUniqueScene(scenes, project.findScene(nid));
            }
        }
    }
}

} // namespace

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
    connect(m_outputs.get(), &OutputHub::reconnecting, this, [this](int attempt) {
        emit errorOccurred(QStringLiteral("Reconnecting to RTMP (attempt %1)…").arg(attempt));
    });
    connect(m_outputs.get(), &OutputHub::recordingStopped, this, [this](const QString& path) {
        if (path.isEmpty() || !m_settings) return;
        const auto ui = m_settings->uiState();
        if (!ui.value(QStringLiteral("autoRemuxToMp4")).toBool())
            return;
        const QString out = defaultRemuxOutputPath(path);
        // Run off the UI/engine tick thread so stopRecording stays snappy.
        QThread* worker = QThread::create([this, path, out] {
            QString err;
            if (!remuxCopy(path, out, &err)) {
                QMetaObject::invokeMethod(this, [this, err] {
                    emit errorOccurred(err.isEmpty() ? QStringLiteral("Auto-remux failed") : err);
                }, Qt::QueuedConnection);
            } else {
                Logger::info(QStringLiteral("Auto-remux complete: %1").arg(out));
            }
        });
        connect(worker, &QThread::finished, worker, &QObject::deleteLater);
        worker->start();
    });
    connect(m_scoreboard.get(), &ScoreboardModel::changed, this, &EngineController::onScoreboardChanged);

    connect(m_outputs.get(), &OutputHub::codecConfigReady, this,
            [this](const OutputProfile& profile, const QByteArray& vExtra, const QByteArray& aExtra,
                   int rate, int ch) {
                if (m_replay)
                    m_replay->setCodecConfig(profile, vExtra, aExtra, rate, ch);
            });
    connect(m_outputs.get(), &OutputHub::encodedPacket, this, [this](const EncodedPacket& pkt) {
        if (!m_replay || !m_replay->isEnabled()) return;
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
    m_audio->applyChannelsFromJson(m_settings->audioChannels());
    connect(m_audio.get(), &AudioGraph::channelsChanged, this, [this] {
        if (m_settings && m_audio)
            m_settings->setAudioChannels(m_audio->channelsToJson());
    });

    m_capture->setSourceAudioCallback([this](const QString& id, const QString& name, const AudioBuffer& buf) {
        if (!m_audio) return;
        const auto snap = m_sceneGraph->snapshot();
        if (const auto* s = snap.findSourceAnywhere(id)) {
            if (!sourceAppearsInAudioMixer(*s))
                return; // OBS: no mixer / no OBS-routed audio for this source
        }
        const bool created = m_audio->ensureChannelEx(id, name);
        if (created) {
            if (const auto* s = snap.findSourceAnywhere(id))
                syncSourceAudioToGraph(id, s->name, s->settings);
            else
                m_audio->ensureChannel(id, name);
        }
        m_audio->inject(id, buf);
    });
    connect(m_capture.get(), &CaptureManager::sourceStopped, this, [this](const QString& id) {
        // Keep mixer strip while the source still exists in the project (OBS behavior).
        if (!m_audio || !m_sceneGraph) return;
        if (m_sceneGraph->snapshot().findSourceAnywhere(id))
            return;
        m_audio->removeChannel(id);
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
        auto project = m_sceneGraph->snapshot();
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
        collectNestedScenes(project, scenes);
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
            m_compositor->compose(*previewScene, m_capture->frameBus(), false, 1.0f, 0.02f, 0.02f, 0.03f, &project);

        const bool stinger = m_transition && m_transition->isActive() && transitionIsStinger(activeType);
        if (stinger) {
            const float pot = float(project.extras.value(QStringLiteral("stingerPoint")).toInt(50)) / 100.f;
            if (!m_stingerSwapped && mix >= pot) {
                m_sceneGraph->setProgramSceneId(m_stingerToSceneId);
                m_stingerSwapped = true;
                project = m_sceneGraph->snapshot();
            }
            const SceneItem* show = nullptr;
            if (m_stingerSwapped)
                show = project.findScene(project.programSceneId);
            else
                show = project.findScene(m_stingerFromSceneId);
            if (!show)
                show = project.findScene(project.programSceneId);
            if (show)
                m_compositor->compose(*show, m_capture->frameBus(), true, 1.0f, 0.02f, 0.02f, 0.03f, &project);
            if (m_stingerMedia) {
                VideoFrame vf;
                if (m_stingerMedia->acquireLatest(vf) && vf.texture)
                    m_compositor->drawFullscreenOverlay(vf.texture, 1.0f, true);
            }
        } else if (const auto* program = project.findScene(project.programSceneId)) {
            // Crossfade: compose new program at full, then blend hold on top
            const bool cross = m_transition && m_transition->isActive() && m_transition->isCrossfade()
                               && m_compositor->hasProgramHold();
            float clearR = 0.02f, clearG = 0.02f, clearB = 0.03f;
            if (m_transition && m_transition->isActive() && transitionIsFtbStyle(activeType)) {
                if (activeType == TransitionType::FadeToWhite) {
                    clearR = clearG = clearB = 1.0f;
                } else {
                    clearR = clearG = clearB = 0.0f;
                }
            }
            m_compositor->compose(*program, m_capture->frameBus(), true, cross ? 1.0f : mix,
                                  clearR, clearG, clearB, &project);
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
    syncMixerChannelsFromProject();
    if (m_compositor)
        m_compositor->setWipeDirection(wipeDirection());
    Logger::info(QStringLiteral("EngineController initialized"));
    return true;
}

void EngineController::shutdown()
{
    if (!m_initialized) return;
    stopStreaming();
    stopRecording();
    stopStingerMedia();
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
    if (m_audio) {
        const auto arr = p->extras.value(QStringLiteral("audioChannels")).toArray();
        if (!arr.isEmpty()) {
            m_audio->applyChannelsFromJson(arr);
            if (m_settings)
                m_settings->setAudioChannels(arr);
        }
    }
    rebuildSourcesForActiveScenes();
    syncMixerChannelsFromProject();
    if (m_compositor)
        m_compositor->setWipeDirection(wipeDirection());
    emit projectLoaded(path);
    return true;
}

bool EngineController::saveProject(const QString& path, QString* error)
{
    auto p = m_sceneGraph->snapshot();
    p.path = path;
    if (m_audio) {
        auto extras = p.extras;
        extras.insert(QStringLiteral("audioChannels"), m_audio->channelsToJson());
        p.extras = extras;
        if (m_settings)
            m_settings->setAudioChannels(m_audio->channelsToJson());
    }
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
    syncMixerChannelsFromProject();
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

void EngineController::swapPreviewProgram()
{
    m_sceneGraph->mutate([&](Project& p) {
        const QString a = p.previewSceneId;
        const QString b = p.programSceneId;
        p.previewSceneId = b;
        p.programSceneId = a;
        if (!p.previewSceneId.isEmpty())
            p.activeSceneId = p.previewSceneId;
    });
    rebuildSourcesForActiveScenes();
}

void EngineController::stopStingerMedia()
{
    if (m_stingerMedia) {
        m_stingerMedia->stop();
        m_stingerMedia.reset();
    }
    m_stingerSwapped = false;
    m_stingerFromSceneId.clear();
    m_stingerToSceneId.clear();
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
            stopStingerMedia();
            m_sceneGraph->setProgramSceneId(p.previewSceneId);
            if (m_compositor) m_compositor->clearProgramHold();
            emit transitionFinished();
        } else if (transitionIsStinger(effective)) {
            stopStingerMedia();
            m_stingerFromSceneId = p.programSceneId;
            m_stingerToSceneId = p.previewSceneId;
            m_stingerSwapped = false;
            const QString path = p.extras.value(QStringLiteral("stingerPath")).toString();
            if (path.isEmpty()) {
                emit errorOccurred(QStringLiteral("Stinger: set a media file in the Take panel"));
                // Fall back to cut
                m_sceneGraph->setProgramSceneId(p.previewSceneId);
                emit transitionFinished();
                return;
            }
            if (m_d3d && m_d3d->device()) {
                m_stingerMedia = std::make_unique<MediaSource>(
                    QStringLiteral("stinger_take"), QStringLiteral("Stinger"), path, false, true);
                QString err;
                if (!m_stingerMedia->start(m_d3d->device(), &err)) {
                    Logger::warn(QStringLiteral("Stinger open failed: %1").arg(err));
                    m_stingerMedia.reset();
                    m_sceneGraph->setProgramSceneId(p.previewSceneId);
                    emit transitionFinished();
                    return;
                }
            }
            connect(m_transition, &TransitionEngine::finished, this, [this] {
                stopStingerMedia();
                if (m_compositor) m_compositor->clearProgramHold();
                emit transitionFinished();
            }, Qt::SingleShotConnection);
            m_transition->start();
        } else if (m_transition->isCrossfade()) {
            stopStingerMedia();
            // Snapshot current program, switch immediately, blend hold → new
            if (m_compositor) m_compositor->captureProgramHold();
            m_sceneGraph->setProgramSceneId(p.previewSceneId);
            connect(m_transition, &TransitionEngine::finished, this, [this] {
                if (m_compositor) m_compositor->clearProgramHold();
                emit transitionFinished();
            }, Qt::SingleShotConnection);
            m_transition->start();
        } else {
            stopStingerMedia();
            // FTB / Fade to White: fade out old, swap, fade in new
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

void EngineController::setWipeDirection(int direction)
{
    const int d = std::clamp(direction, 0, 3);
    m_sceneGraph->mutate([&](Project& p) {
        p.extras.insert(QStringLiteral("wipeDirection"), d);
    });
    if (m_compositor)
        m_compositor->setWipeDirection(d);
}

int EngineController::wipeDirection() const
{
    if (!m_sceneGraph) return 0;
    return m_sceneGraph->snapshot().extras.value(QStringLiteral("wipeDirection")).toInt(0);
}

void EngineController::setStudioMode(bool enabled)
{
    if (m_studioMode == enabled) return;
    m_studioMode = enabled;
    emit studioModeChanged(enabled);
}

void EngineController::setReplayBufferEnabled(bool enabled)
{
    if (!m_replay) return;
    m_replay->setEnabled(enabled);
    emit replayBufferEnabledChanged(m_replay->isEnabled());
}

bool EngineController::replayBufferEnabled() const
{
    return m_replay && m_replay->isEnabled();
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
    if (!id.isEmpty()) {
        rebuildSourcesForActiveScenes();
        syncMixerChannelsFromProject();
    }
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
    if (m_audio)
        m_audio->removeChannel(sourceId);
    syncMixerChannelsFromProject();
}

QString EngineController::duplicateSource(const QString& sourceId)
{
    QString newIdOut;
    m_sceneGraph->mutate([&](Project& p) {
        for (const auto& sc : p.scenes) {
            for (const auto& src : sc.sources) {
                if (src.id == sourceId) {
                    newIdOut = p.duplicateSource(sc.id, sourceId);
                    return;
                }
            }
        }
    });
    if (!newIdOut.isEmpty()) {
        rebuildSourcesForActiveScenes();
        syncMixerChannelsFromProject();
        setSelectedSourceId(newIdOut);
    }
    return newIdOut;
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
    syncMixerChannelsFromProject();
}

void EngineController::setSourceLocked(const QString& sourceId, bool locked)
{
    m_sceneGraph->mutate([&](Project& p) {
        if (auto* s = p.findSourceAnywhere(sourceId))
            s->locked = locked;
    });
}

void EngineController::setSourceColor(const QString& sourceId, const QString& colorHex)
{
    if (sourceId.isEmpty() || colorHex.isEmpty()) return;
    m_sceneGraph->mutate([&](Project& p) {
        if (auto* s = p.findSourceAnywhere(sourceId))
            s->colorHex = colorHex;
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

void EngineController::moveSourceZOrderExtreme(const QString& sourceId, bool toTop)
{
    m_sceneGraph->mutate([&](Project& p) {
        for (const auto& sc : p.scenes) {
            for (const auto& src : sc.sources) {
                if (src.id == sourceId) {
                    p.moveSourceToExtreme(sc.id, sourceId, toTop);
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

void EngineController::updateSourceTransform(const QString& sourceId, const Transform& t, bool notify)
{
    if (notify) {
        m_sceneGraph->mutate([&](Project& p) {
            if (auto* s = p.findSourceAnywhere(sourceId))
                s->transform = t;
        });
    } else {
        m_sceneGraph->mutateSilent([&](Project& p) {
            if (auto* s = p.findSourceAnywhere(sourceId))
                s->transform = t;
        });
    }
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
    if (found) {
        m_capture->updateSource(updated);
        syncSourceAudioToGraph(sourceId, updated.name, updated.settings);
        syncMixerChannelsFromProject();
    }
}

void EngineController::syncSourceAudioToGraph(const QString& sourceId, const QString& name, const QJsonObject& settings)
{
    if (!m_audio || sourceId.isEmpty())
        return;
    SourceItem probe;
    probe.id = sourceId;
    probe.name = name;
    probe.settings = settings;
    if (const auto* s = m_sceneGraph->snapshot().findSourceAnywhere(sourceId))
        probe.type = s->type;
    if (!sourceAppearsInAudioMixer(probe)) {
        m_audio->removeChannel(sourceId);
        return;
    }
    if (!settings.contains(QStringLiteral("audioVolume")) && !settings.contains(QStringLiteral("audioMute"))) {
        m_audio->ensureChannel(sourceId, name);
        return;
    }
    m_audio->ensureChannel(sourceId, name);
    auto state = m_audio->channelState(sourceId);
    state.id = sourceId;
    if (!name.isEmpty())
        state.name = name;
    if (settings.contains(QStringLiteral("audioVolume"))) {
        const float pct = float(settings.value(QStringLiteral("audioVolume")).toInt(100));
        state.volume = std::clamp(pct / 100.f, 0.f, 20.f);
    }
    if (settings.contains(QStringLiteral("audioMute")))
        state.muted = settings.value(QStringLiteral("audioMute")).toBool(false);
    m_audio->setChannelState(sourceId, state);
}

void EngineController::syncMixerChannelsFromProject()
{
    if (!m_audio || !m_sceneGraph)
        return;
    QHash<QString, QString> keep;
    const auto project = m_sceneGraph->snapshot();
    for (const auto& sc : project.scenes) {
        for (const auto& src : sc.sources) {
            if (sourceAppearsInAudioMixer(src))
                keep.insert(src.id, src.name);
        }
    }
    m_audio->syncDynamicChannels(keep);
}

void EngineController::onScoreboardChanged()
{
    pushScoreboardToProgram();
}

void EngineController::pushScoreboardToProgram()
{
    if (!m_scoreboard) return;
    const QJsonObject json = m_scoreboard->state().toJson();
    const auto snap = m_sceneGraph->snapshot();
    const int cw = snap.output.width > 0 ? snap.output.width : 1920;
    const int ch = snap.output.height > 0 ? snap.output.height : 1080;
    m_sceneGraph->mutate([&](Project& p) {
        for (auto& scene : p.scenes) {
            for (auto& src : scene.sources) {
                if (src.type != SourceType::Scoreboard)
                    continue;
                src.settings = json;
                // Lock aspect so the compact bug texture isn't squashed into a wide strip
                Transform t = src.transform;
                const double w = t.w > 0.01 ? t.w : 0.85;
                const double h = scoreboardNormHeight(w, cw, ch);
                if (qAbs(t.h - h) > 0.002 || qAbs(t.w - w) > 0.002) {
                    const double bottom = t.y + t.h;
                    t.w = w;
                    t.h = h;
                    // Keep bottom edge if it was a lower-third placement
                    if (bottom > 0.7)
                        t.y = qMax(0.0, bottom - h);
                    src.transform = t;
                }
            }
        }
    });
    // Refresh live textures
    const auto after = m_sceneGraph->snapshot();
    for (const auto& scene : after.scenes) {
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
    const auto ui = m_settings->uiState();
    m_outputs->setNetworkOptions(
        ui.value(QStringLiteral("reconnectEnabled")).toBool(true),
        ui.value(QStringLiteral("reconnectMaxAttempts")).toInt(0),
        ui.value(QStringLiteral("reconnectBaseMs")).toInt(500),
        ui.value(QStringLiteral("streamDelaySec")).toInt(0));
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
    const QString dir = m_settings->recordingDirectory();
    QDir().mkpath(dir);
    const auto ui = m_settings->uiState();
    const QString pattern = ui.value(QStringLiteral("recordingFilenamePattern")).toString(
        defaultRecordingFilenamePattern());
    const QString path = buildRecordingFilePath(dir, pattern);
    if (!m_outputs->startRecording(path, profile, error))
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

bool EngineController::takeScreenshot(bool program, QString* pathOut, QString* error)
{
    if (!m_compositor) {
        if (error) *error = QStringLiteral("Compositor unavailable");
        return false;
    }
    const QImage img = program ? m_compositor->readbackProgram() : m_compositor->readbackPreview();
    if (img.isNull()) {
        if (error) *error = QStringLiteral("Screenshot readback failed");
        return false;
    }
    QDir().mkpath(m_settings->recordingDirectory());
    const QString path = m_settings->recordingDirectory()
        + QStringLiteral("/RailShotTV-%1-%2.png")
              .arg(program ? QStringLiteral("Program") : QStringLiteral("Preview"),
                   QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss")));
    if (!img.save(path, "PNG")) {
        if (error) *error = QStringLiteral("Failed to write %1").arg(path);
        return false;
    }
    if (pathOut) *pathOut = path;
    emit screenshotSaved(path);
    Logger::info(QStringLiteral("Screenshot saved: %1").arg(path));
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
    if (m_audio && m_settings)
        m_settings->setAudioChannels(m_audio->channelsToJson());
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
    collectNestedScenes(project, scenes);
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
