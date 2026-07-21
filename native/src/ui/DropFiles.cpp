#include "ui/DropFiles.h"
#include "core/EngineController.h"
#include "core/Types.h"
#include <QFileInfo>
#include <QJsonObject>

namespace railshot {

namespace {
bool isImageExt(const QString& ext)
{
    static const QStringList k = {
        QStringLiteral("png"), QStringLiteral("jpg"), QStringLiteral("jpeg"),
        QStringLiteral("bmp"), QStringLiteral("webp"), QStringLiteral("gif"),
        QStringLiteral("tif"), QStringLiteral("tiff")};
    return k.contains(ext);
}
bool isMediaExt(const QString& ext)
{
    static const QStringList k = {
        QStringLiteral("mp4"), QStringLiteral("mov"), QStringLiteral("mkv"),
        QStringLiteral("webm"), QStringLiteral("avi"), QStringLiteral("m4v"),
        QStringLiteral("mp3"), QStringLiteral("wav"), QStringLiteral("flac")};
    return k.contains(ext);
}
bool isHtmlExt(const QString& ext)
{
    return ext == QLatin1String("html") || ext == QLatin1String("htm");
}
} // namespace

int importDroppedUrls(EngineController* engine, const QList<QUrl>& urls, QString* summary)
{
    if (!engine || urls.isEmpty()) {
        if (summary) *summary = QStringLiteral("Nothing to import");
        return 0;
    }
    int created = 0;
    QStringList notes;
    QString lastId;

    for (const QUrl& url : urls) {
        if (url.isLocalFile()) {
            const QString path = url.toLocalFile();
            const QFileInfo fi(path);
            if (!fi.exists()) {
                notes << QStringLiteral("Missing: %1").arg(fi.fileName());
                continue;
            }
            const QString ext = fi.suffix().toLower();
            QJsonObject settings;
            SourceType type = SourceType::Unknown;
            QString name = fi.completeBaseName();

            if (isImageExt(ext)) {
                type = SourceType::Image;
                settings.insert(QStringLiteral("path"), path);
            } else if (isHtmlExt(ext)) {
                type = SourceType::Browser;
                settings.insert(QStringLiteral("url"), QUrl::fromLocalFile(path).toString());
                settings.insert(QStringLiteral("width"), 1280);
                settings.insert(QStringLiteral("height"), 720);
                settings.insert(QStringLiteral("fps"), 30);
            } else if (isMediaExt(ext)) {
                type = SourceType::Media;
                settings.insert(QStringLiteral("path"), path);
            } else {
                notes << QStringLiteral("Skipped: %1").arg(fi.fileName());
                continue;
            }
            lastId = engine->addSource(type, name, settings);
            ++created;
            notes << QStringLiteral("Added %1").arg(name);
        } else if (url.scheme().startsWith(QLatin1String("http"))) {
            QJsonObject settings;
            settings.insert(QStringLiteral("url"), url.toString());
            settings.insert(QStringLiteral("width"), 1280);
            settings.insert(QStringLiteral("height"), 720);
            settings.insert(QStringLiteral("fps"), 30);
            const QString name = url.host().isEmpty() ? QStringLiteral("Browser") : url.host();
            lastId = engine->addSource(SourceType::Browser, name, settings);
            ++created;
            notes << QStringLiteral("Added browser %1").arg(name);
        } else {
            notes << QStringLiteral("Skipped URL: %1").arg(url.toString());
        }
    }

    if (!lastId.isEmpty())
        engine->setSelectedSourceId(lastId);
    if (summary)
        *summary = notes.join(QStringLiteral(" · "));
    return created;
}

} // namespace railshot
