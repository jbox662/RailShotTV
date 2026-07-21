#pragma once

#include "core/Types.h"
#include <QString>
#include <QStringList>
#include <optional>

namespace railshot {

/// Named output profile (OBS-style) stored under AppData + QSettings index.
struct NamedOutputProfile {
    QString name;
    OutputProfile profile;
};

QString profilesRootDir();
QStringList listProfileNames();
std::optional<NamedOutputProfile> loadNamedProfile(const QString& name);
bool saveNamedProfile(const NamedOutputProfile& p, QString* error = nullptr);
bool deleteNamedProfile(const QString& name, QString* error = nullptr);
bool duplicateNamedProfile(const QString& from, const QString& to, QString* error = nullptr);
QString currentProfileName();
void setCurrentProfileName(const QString& name);
/// Ensure at least "Untitled" exists from current settings profile.
void ensureDefaultProfile(const OutputProfile& fallback);

QString collectionsRootDir();
QStringList listCollectionNames();
QString collectionFilePath(const QString& name);
QString currentCollectionName();
void setCurrentCollectionName(const QString& name);
bool deleteCollection(const QString& name, QString* error = nullptr);
bool renameCollection(const QString& from, const QString& to, QString* error = nullptr);
bool duplicateCollection(const QString& from, const QString& to, QString* error = nullptr);

} // namespace railshot
