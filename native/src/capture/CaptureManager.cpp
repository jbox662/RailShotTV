#include "capture/CaptureManager.h"
#include "capture/MediaFoundationCamera.h"
#include "capture/WindowsGraphicsCapture.h"
#include "capture/DesktopDuplicationCapture.h"
#include "capture/ImageSource.h"
#include "capture/TextSource.h"
#include "capture/OverlaySource.h"
#include "capture/BrowserSource.h"
#include "capture/ColorSource.h"
#include "capture/MediaSource.h"
#include "capture/NdiSource.h"
#include "capture/AudioSource.h"
#include "audio/AudioGraph.h"
#include "core/Logger.h"
#include <QColor>
#include <QSet>

namespace railshot {

CaptureManager::CaptureManager(QObject* parent)
    : QObject(parent)
{
}

CaptureManager::~CaptureManager()
{
    detachAll();
}

void CaptureManager::setDevice(ID3D11Device* device)
{
    m_device = device;
}

void CaptureManager::setAudioGraph(AudioGraph* graph)
{
    m_audioGraph = graph;
}

void CaptureManager::setSourceAudioCallback(
    std::function<void(const QString&, const QString&, const AudioBuffer&)> cb)
{
    m_audioCb = std::move(cb);
}

void CaptureManager::wireSourceAudio(IVideoSource* src, const SourceItem& source)
{
    if (!src || !m_audioCb) return;
    const QString id = source.id;
    const QString name = source.name;
    auto forward = [this, id, name](const AudioBuffer& buf) {
        if (m_audioCb) m_audioCb(id, name, buf);
    };
    if (auto* media = dynamic_cast<MediaSource*>(src)) {
        media->setAudioCallback(forward);
        return;
    }
    if (auto* ndi = dynamic_cast<NdiSource*>(src)) {
        ndi->setAudioCallback(forward);
    }
}

std::unique_ptr<IVideoSource> CaptureManager::createSource(const SourceItem& source)
{
    switch (source.type) {
    case SourceType::Camera:
        return std::make_unique<MediaFoundationCamera>(
            source.id, source.name, source.settings.value(QStringLiteral("deviceId")).toString());
    case SourceType::Display: {
        const int monitor = source.settings.value(QStringLiteral("monitorIndex")).toInt(0);
        return std::make_unique<DesktopDuplicationCapture>(source.id, source.name, monitor);
    }
    case SourceType::Window:
    case SourceType::Game: {
        const quintptr hwnd = static_cast<quintptr>(
            source.settings.value(QStringLiteral("hwnd")).toVariant().toULongLong());
        return std::make_unique<WindowsGraphicsCapture>(
            source.id, source.name, WindowsGraphicsCapture::TargetKind::Window, 0, hwnd);
    }
    case SourceType::Image:
        return std::make_unique<ImageSource>(
            source.id, source.name, source.settings.value(QStringLiteral("path")).toString());
    case SourceType::Text:
        return std::make_unique<TextSource>(
            source.id, source.name, source.settings.value(QStringLiteral("text")).toString(source.name));
    case SourceType::Scoreboard:
    case SourceType::LowerThird:
    case SourceType::Alert:
        return std::make_unique<OverlaySource>(source.id, source.name, source.type, source.settings);
    case SourceType::Browser:
        return std::make_unique<BrowserSource>(source.id, source.name, source.settings);
    case SourceType::Color: {
        const QString hex = source.settings.value(QStringLiteral("color")).toString(QStringLiteral("#1A2035"));
        const int w = source.settings.value(QStringLiteral("width")).toInt(1920);
        const int h = source.settings.value(QStringLiteral("height")).toInt(1080);
        return std::make_unique<ColorSource>(source.id, source.name, QColor(hex), w, h);
    }
    case SourceType::Media: {
        const QString path = source.settings.value(QStringLiteral("path")).toString();
        const bool loop = source.settings.value(QStringLiteral("loop")).toBool(true);
        const bool local = source.settings.value(QStringLiteral("isLocalFile")).toBool(true);
        const QString ffopts = source.settings.value(QStringLiteral("ffmpegOptions")).toString();
        return std::make_unique<MediaSource>(source.id, source.name, path, loop, local, ffopts);
    }
    case SourceType::Ndi: {
        const QString ndi = source.settings.value(QStringLiteral("ndiName")).toString(source.name);
        return std::make_unique<NdiSource>(source.id, source.name, ndi);
    }
    case SourceType::AudioInput:
        return std::make_unique<AudioSource>(
            source.id, source.name, AudioDeviceKind::Capture,
            source.settings.value(QStringLiteral("deviceId")).toString(), m_audioGraph);
    case SourceType::AudioOutput:
        return std::make_unique<AudioSource>(
            source.id, source.name, AudioDeviceKind::Loopback,
            source.settings.value(QStringLiteral("deviceId")).toString(), m_audioGraph);
    default:
        return std::make_unique<TextSource>(source.id, source.name, source.name);
    }
}

bool CaptureManager::attachSource(const SourceItem& source, QString* error)
{
    if (m_sources.contains(source.id)) {
        updateSource(source);
        return true;
    }
    auto src = createSource(source);
    if (!src) {
        if (error) *error = QStringLiteral("Unsupported source type");
        return false;
    }
    wireSourceAudio(src.get(), source);
    if (!src->start(m_device, error))
        return false;
    // Apply text extras after start
    if (auto* text = dynamic_cast<TextSource*>(src.get())) {
        text->setText(source.settings.value(QStringLiteral("text")).toString(source.name));
        text->setFontSize(source.settings.value(QStringLiteral("fontSize")).toInt(48));
    }
    const QString id = source.id;
    m_sources.insert(id, std::shared_ptr<IVideoSource>(src.release()));
    emit sourceStarted(id);
    Logger::info(QStringLiteral("Attached source %1").arg(id));
    return true;
}

void CaptureManager::reattach(const SourceItem& source)
{
    detachSource(source.id);
    QString err;
    if (!attachSource(source, &err))
        emit sourceError(source.id, err);
}

void CaptureManager::updateSource(const SourceItem& source)
{
    auto* src = this->source(source.id);
    if (!src) return;

    // Identity changes require full re-attach
    auto needsReattach = [&]() -> bool {
        switch (source.type) {
        case SourceType::Camera:
            return dynamic_cast<MediaFoundationCamera*>(src) == nullptr; // type swap
        case SourceType::Display:
            return dynamic_cast<DesktopDuplicationCapture*>(src) == nullptr;
        case SourceType::Window:
        case SourceType::Game:
            return dynamic_cast<WindowsGraphicsCapture*>(src) == nullptr;
        case SourceType::Image:
            return dynamic_cast<ImageSource*>(src) == nullptr;
        case SourceType::Ndi:
            return dynamic_cast<NdiSource*>(src) == nullptr;
        case SourceType::AudioInput:
        case SourceType::AudioOutput:
            return dynamic_cast<AudioSource*>(src) == nullptr;
        default:
            return false;
        }
    };
    Q_UNUSED(needsReattach);

    // Compare identity settings stored by checking recreate for known keys via detach/attach
    // Store last settings on first update by re-creating when key fields change:
    static thread_local QHash<QString, QJsonObject> s_last;
    const QJsonObject& cur = source.settings;
    const QJsonObject prev = s_last.value(source.id);

    auto keyChanged = [&](const char* k) {
        return prev.value(QLatin1String(k)) != cur.value(QLatin1String(k));
    };

    bool reattach = false;
    if (!prev.isEmpty()) {
        switch (source.type) {
        case SourceType::Camera:
            reattach = keyChanged("deviceId");
            break;
        case SourceType::Display:
            reattach = keyChanged("monitorIndex");
            break;
        case SourceType::Window:
        case SourceType::Game:
            reattach = keyChanged("hwnd") || keyChanged("windowTitle");
            break;
        case SourceType::Image:
            reattach = keyChanged("path");
            break;
        case SourceType::Media:
            reattach = keyChanged("path") || keyChanged("loop") || keyChanged("isLocalFile")
                       || keyChanged("ffmpegOptions");
            break;
        case SourceType::Ndi:
            reattach = keyChanged("ndiName");
            break;
        case SourceType::AudioInput:
        case SourceType::AudioOutput:
            reattach = keyChanged("deviceId");
            break;
        case SourceType::Color:
            reattach = keyChanged("width") || keyChanged("height");
            break;
        default:
            break;
        }
    }
    s_last.insert(source.id, cur);

    if (reattach) {
        this->reattach(source);
        return;
    }

    if (auto* overlay = dynamic_cast<OverlaySource*>(src)) {
        overlay->applySettings(source.settings);
        return;
    }
    if (auto* browser = dynamic_cast<BrowserSource*>(src)) {
        browser->applySettings(source.settings);
        return;
    }
    if (auto* text = dynamic_cast<TextSource*>(src)) {
        text->setText(source.settings.value(QStringLiteral("text")).toString(source.name));
        text->setFontSize(source.settings.value(QStringLiteral("fontSize")).toInt(48));
        return;
    }
    if (auto* color = dynamic_cast<ColorSource*>(src)) {
        color->setColor(QColor(source.settings.value(QStringLiteral("color")).toString()));
        return;
    }
    if (auto* media = dynamic_cast<MediaSource*>(src)) {
        media->setPath(source.settings.value(QStringLiteral("path")).toString());
        media->setLoop(source.settings.value(QStringLiteral("loop")).toBool(true));
        media->setLocalFile(source.settings.value(QStringLiteral("isLocalFile")).toBool(true));
        media->setFfmpegOptions(source.settings.value(QStringLiteral("ffmpegOptions")).toString());
    }
}

void CaptureManager::setOverlayCanvasSize(int width, int height)
{
    for (auto it = m_sources.begin(); it != m_sources.end(); ++it) {
        if (auto* overlay = dynamic_cast<OverlaySource*>(it.value().get()))
            overlay->setCanvasSize(width, height);
    }
}

void CaptureManager::detachSource(const QString& sourceId)
{
    auto it = m_sources.find(sourceId);
    if (it == m_sources.end()) return;
    it.value()->stop();
    m_sources.erase(it);
    m_bus.remove(sourceId);
    emit sourceStopped(sourceId);
}

void CaptureManager::detachAll()
{
    const auto ids = m_sources.keys();
    for (const auto& id : ids)
        detachSource(id);
}

void CaptureManager::syncWithScene(const SceneItem& scene)
{
    const SceneItem* ptr = &scene;
    syncWithScenes({ptr});
}

void CaptureManager::syncWithScenes(const QVector<const SceneItem*>& scenes)
{
    QSet<QString> wanted;
    QHash<QString, SourceItem> items;
    for (const SceneItem* scene : scenes) {
        if (!scene) continue;
        for (const auto& src : scene->sources) {
            if (!src.visible) continue;
            wanted.insert(src.id);
            items.insert(src.id, src);
        }
    }
    // Pinned sources (Properties dialog) stay attached even if not in sync set.
    for (auto it = m_pinned.cbegin(); it != m_pinned.cend(); ++it) {
        wanted.insert(it.key());
        if (!items.contains(it.key()))
            items.insert(it.key(), it.value());
    }
    for (auto it = items.cbegin(); it != items.cend(); ++it) {
        if (!m_sources.contains(it.key())) {
            QString err;
            if (!attachSource(it.value(), &err))
                emit sourceError(it.key(), err);
        } else {
            updateSource(it.value());
        }
    }
    const auto existing = m_sources.keys();
    for (const auto& id : existing) {
        if (!wanted.contains(id))
            detachSource(id);
    }
}

void CaptureManager::pinSource(const SourceItem& source)
{
    m_pinned.insert(source.id, source);
    QString err;
    if (!attachSource(source, &err))
        emit sourceError(source.id, err);
}

void CaptureManager::unpinSource(const QString& sourceId)
{
    m_pinned.remove(sourceId);
}

bool CaptureManager::pollSource(const QString& sourceId)
{
    if (m_paused) return false;
    auto* src = source(sourceId);
    if (!src) return false;
    VideoFrame frame;
    if (!src->acquireLatest(frame) || !frame.texture)
        return false;
    m_bus.publish(sourceId, frame);
    return true;
}

IVideoSource* CaptureManager::source(const QString& id) const
{
    auto it = m_sources.find(id);
    return it == m_sources.end() ? nullptr : it.value().get();
}

QVector<QString> CaptureManager::activeSourceIds() const
{
    return m_sources.keys();
}

void CaptureManager::poll()
{
    if (m_paused) return;
    for (auto it = m_sources.begin(); it != m_sources.end(); ++it) {
        VideoFrame frame;
        if (it.value()->acquireLatest(frame))
            m_bus.publish(it.key(), frame);
    }
}

} // namespace railshot
