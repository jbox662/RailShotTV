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
        return std::make_unique<MediaSource>(source.id, source.name, path, loop);
    }
    case SourceType::Ndi: {
        const QString ndi = source.settings.value(QStringLiteral("ndiName")).toString(source.name);
        return std::make_unique<NdiSource>(source.id, source.name, ndi);
    }
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
    if (!src->start(m_device, error))
        return false;
    const QString id = source.id;
    m_sources.insert(id, std::shared_ptr<IVideoSource>(src.release()));
    emit sourceStarted(id);
    Logger::info(QStringLiteral("Attached source %1").arg(id));
    return true;
}

void CaptureManager::updateSource(const SourceItem& source)
{
    auto* src = this->source(source.id);
    if (!src) return;
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
        return;
    }
    if (auto* color = dynamic_cast<ColorSource*>(src)) {
        color->setColor(QColor(source.settings.value(QStringLiteral("color")).toString()));
        return;
    }
    if (auto* media = dynamic_cast<MediaSource*>(src)) {
        media->setPath(source.settings.value(QStringLiteral("path")).toString());
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

IVideoSource* CaptureManager::source(const QString& id) const
{
    auto it = m_sources.constFind(id);
    return it == m_sources.cend() ? nullptr : it.value().get();
}

QVector<QString> CaptureManager::activeSourceIds() const
{
    return m_sources.keys().toVector();
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
