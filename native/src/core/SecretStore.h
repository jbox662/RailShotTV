#pragma once

#include <QString>
#include <optional>

namespace railshot {

/// Windows Credential Manager backed secret store.
/// Stream keys / OAuth tokens MUST go here — never into project JSON.
class SecretStore {
public:
    static bool store(const QString& secretId, const QString& value, QString* error = nullptr);
    static std::optional<QString> load(const QString& secretId, QString* error = nullptr);
    static bool remove(const QString& secretId, QString* error = nullptr);
    static QString makeStreamKeyId(const QString& targetId);
    static QString makeTokenId(const QString& platform, const QString& accountId);
};

} // namespace railshot
