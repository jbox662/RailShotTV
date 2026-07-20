#pragma once

#include <QString>
#include <QJsonObject>
#include <optional>

namespace railshot {

class JsonSchema {
public:
    static bool validateProject(const QJsonObject& o, QString* error = nullptr);
    static QJsonObject migrateProject(const QJsonObject& o, int fromVersion, int toVersion);
};

} // namespace railshot
