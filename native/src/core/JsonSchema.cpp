#include "core/JsonSchema.h"
#include "core/Types.h"

namespace railshot {

bool JsonSchema::validateProject(const QJsonObject& o, QString* error)
{
    if (!o.contains(QStringLiteral("schemaVersion"))) {
        if (error) *error = QStringLiteral("Missing schemaVersion");
        return false;
    }
    if (!o.contains(QStringLiteral("scenes")) || !o.value(QStringLiteral("scenes")).isArray()) {
        if (error) *error = QStringLiteral("Missing scenes array");
        return false;
    }
    return true;
}

QJsonObject JsonSchema::migrateProject(const QJsonObject& o, int fromVersion, int toVersion)
{
    QJsonObject out = o;
    // Future migrations go here (v1 -> v2 ...).
    Q_UNUSED(fromVersion);
    Q_UNUSED(toVersion);
    out.insert(QStringLiteral("schemaVersion"), kProjectSchemaVersion);
    return out;
}

} // namespace railshot
