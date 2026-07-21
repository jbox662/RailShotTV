#include "core/ProfileCollectionStore.h"
#include "core/Types.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>

namespace railshot {

namespace {
QSettings appSettings()
{
    return QSettings(QStringLiteral("RailShotTV"), QStringLiteral("RailShotTV"));
}

QString sanitizeName(QString name)
{
    name = name.trimmed();
    name.replace(QRegularExpression(QStringLiteral("[\\\\/:*?\"<>|]")), QStringLiteral("_"));
    if (name.isEmpty())
        name = QStringLiteral("Untitled");
    return name;
}
} // namespace

QString profilesRootDir()
{
    const QString root = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                         + QStringLiteral("/profiles");
    QDir().mkpath(root);
    return root;
}

QString collectionsRootDir()
{
    const QString root = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                         + QStringLiteral("/collections");
    QDir().mkpath(root);
    return root;
}

QStringList listProfileNames()
{
    QStringList names;
    QDir dir(profilesRootDir());
    for (const auto& fi : dir.entryInfoList({QStringLiteral("*.json")}, QDir::Files, QDir::Name))
        names.append(fi.completeBaseName());
    names.removeDuplicates();
    names.sort(Qt::CaseInsensitive);
    return names;
}

std::optional<NamedOutputProfile> loadNamedProfile(const QString& name)
{
    const QString path = profilesRootDir() + QLatin1Char('/') + sanitizeName(name) + QStringLiteral(".json");
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return std::nullopt;
    const auto doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject())
        return std::nullopt;
    const auto o = doc.object();
    NamedOutputProfile p;
    p.name = o.value(QStringLiteral("name")).toString(name);
    p.profile = OutputProfile::fromJson(o.value(QStringLiteral("profile")).toObject());
    return p;
}

bool saveNamedProfile(const NamedOutputProfile& p, QString* error)
{
    const QString name = sanitizeName(p.name);
    const QString path = profilesRootDir() + QLatin1Char('/') + name + QStringLiteral(".json");
    QJsonObject o{
        {QStringLiteral("name"), name},
        {QStringLiteral("profile"), p.profile.toJson()},
    };
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error) *error = f.errorString();
        return false;
    }
    f.write(QJsonDocument(o).toJson(QJsonDocument::Indented));
    return true;
}

bool deleteNamedProfile(const QString& name, QString* error)
{
    const QString path = profilesRootDir() + QLatin1Char('/') + sanitizeName(name) + QStringLiteral(".json");
    if (!QFile::exists(path))
        return true;
    if (!QFile::remove(path)) {
        if (error) *error = QStringLiteral("Could not delete profile");
        return false;
    }
    if (currentProfileName() == sanitizeName(name))
        setCurrentProfileName(QString());
    return true;
}

bool duplicateNamedProfile(const QString& from, const QString& to, QString* error)
{
    auto src = loadNamedProfile(from);
    if (!src) {
        if (error) *error = QStringLiteral("Source profile not found");
        return false;
    }
    src->name = sanitizeName(to);
    return saveNamedProfile(*src, error);
}

QString currentProfileName()
{
    return appSettings().value(QStringLiteral("profiles/current")).toString();
}

void setCurrentProfileName(const QString& name)
{
    appSettings().setValue(QStringLiteral("profiles/current"), sanitizeName(name));
}

void ensureDefaultProfile(const OutputProfile& fallback)
{
    if (!listProfileNames().isEmpty())
        return;
    NamedOutputProfile p;
    p.name = QStringLiteral("Untitled");
    p.profile = fallback;
    if (p.profile.width <= 0) {
        p.profile.width = kDefaultCanvasWidth;
        p.profile.height = kDefaultCanvasHeight;
        p.profile.fps = kDefaultFps;
    }
    saveNamedProfile(p);
    setCurrentProfileName(p.name);
}

QStringList listCollectionNames()
{
    QStringList names;
    QDir dir(collectionsRootDir());
    for (const auto& fi : dir.entryInfoList({QStringLiteral("*.railshot.json"), QStringLiteral("*.json")},
                                            QDir::Files, QDir::Name)) {
        QString base = fi.completeBaseName();
        if (base.endsWith(QLatin1String(".railshot")))
            base.chop(9);
        names.append(base);
    }
    names.removeDuplicates();
    names.sort(Qt::CaseInsensitive);
    return names;
}

QString collectionFilePath(const QString& name)
{
    return collectionsRootDir() + QLatin1Char('/') + sanitizeName(name) + QStringLiteral(".railshot.json");
}

QString currentCollectionName()
{
    return appSettings().value(QStringLiteral("collections/current")).toString();
}

void setCurrentCollectionName(const QString& name)
{
    appSettings().setValue(QStringLiteral("collections/current"), sanitizeName(name));
}

bool deleteCollection(const QString& name, QString* error)
{
    const QString path = collectionFilePath(name);
    if (!QFile::exists(path))
        return true;
    if (!QFile::remove(path)) {
        if (error) *error = QStringLiteral("Could not delete collection");
        return false;
    }
    if (currentCollectionName() == sanitizeName(name))
        setCurrentCollectionName(QString());
    return true;
}

bool renameCollection(const QString& from, const QString& to, QString* error)
{
    const QString src = collectionFilePath(from);
    const QString dst = collectionFilePath(to);
    if (!QFile::exists(src)) {
        if (error) *error = QStringLiteral("Collection not found");
        return false;
    }
    if (QFile::exists(dst)) {
        if (error) *error = QStringLiteral("A collection with that name already exists");
        return false;
    }
    if (!QFile::rename(src, dst)) {
        if (error) *error = QStringLiteral("Rename failed");
        return false;
    }
    if (currentCollectionName() == sanitizeName(from))
        setCurrentCollectionName(to);
    return true;
}

bool duplicateCollection(const QString& from, const QString& to, QString* error)
{
    const QString src = collectionFilePath(from);
    const QString dst = collectionFilePath(to);
    if (!QFile::exists(src)) {
        if (error) *error = QStringLiteral("Collection not found");
        return false;
    }
    if (QFile::exists(dst)) {
        if (error) *error = QStringLiteral("A collection with that name already exists");
        return false;
    }
    if (!QFile::copy(src, dst)) {
        if (error) *error = QStringLiteral("Copy failed");
        return false;
    }
    return true;
}

} // namespace railshot
