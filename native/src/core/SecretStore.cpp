#include "core/SecretStore.h"
#include "core/Logger.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <wincred.h>
#pragma comment(lib, "Advapi32.lib")
#endif

namespace railshot {

static QString targetName(const QString& secretId)
{
    return QStringLiteral("RailShotTV/") + secretId;
}

bool SecretStore::store(const QString& secretId, const QString& value, QString* error)
{
#ifdef _WIN32
    const QString target = targetName(secretId);
    CREDENTIALW cred{};
    cred.Type = CRED_TYPE_GENERIC;
    cred.TargetName = const_cast<LPWSTR>(reinterpret_cast<LPCWSTR>(target.utf16()));
    cred.CredentialBlobSize = static_cast<DWORD>(value.size() * sizeof(ushort));
    cred.CredentialBlob = reinterpret_cast<LPBYTE>(const_cast<ushort*>(value.utf16()));
    cred.Persist = CRED_PERSIST_LOCAL_MACHINE;
    cred.UserName = const_cast<LPWSTR>(L"RailShotTV");

    if (!CredWriteW(&cred, 0)) {
        if (error) *error = QStringLiteral("CredWrite failed: %1").arg(GetLastError());
        Logger::error(QStringLiteral("SecretStore::store failed for %1").arg(secretId));
        return false;
    }
    return true;
#else
    Q_UNUSED(secretId); Q_UNUSED(value);
    if (error) *error = QStringLiteral("SecretStore only supported on Windows");
    return false;
#endif
}

std::optional<QString> SecretStore::load(const QString& secretId, QString* error)
{
#ifdef _WIN32
    const QString target = targetName(secretId);
    PCREDENTIALW cred = nullptr;
    if (!CredReadW(reinterpret_cast<LPCWSTR>(target.utf16()), CRED_TYPE_GENERIC, 0, &cred)) {
        if (error) *error = QStringLiteral("CredRead failed: %1").arg(GetLastError());
        return std::nullopt;
    }
    QString value = QString::fromUtf16(
        reinterpret_cast<const char16_t*>(cred->CredentialBlob),
        static_cast<qsizetype>(cred->CredentialBlobSize / sizeof(char16_t)));
    CredFree(cred);
    return value;
#else
    Q_UNUSED(secretId);
    if (error) *error = QStringLiteral("SecretStore only supported on Windows");
    return std::nullopt;
#endif
}

bool SecretStore::remove(const QString& secretId, QString* error)
{
#ifdef _WIN32
    const QString target = targetName(secretId);
    if (!CredDeleteW(reinterpret_cast<LPCWSTR>(target.utf16()), CRED_TYPE_GENERIC, 0)) {
        if (error) *error = QStringLiteral("CredDelete failed: %1").arg(GetLastError());
        return false;
    }
    return true;
#else
    Q_UNUSED(secretId);
    if (error) *error = QStringLiteral("SecretStore only supported on Windows");
    return false;
#endif
}

QString SecretStore::makeStreamKeyId(const QString& targetId)
{
    return QStringLiteral("streamkey/") + targetId;
}

QString SecretStore::makeTokenId(const QString& platform, const QString& accountId)
{
    return QStringLiteral("token/%1/%2").arg(platform, accountId);
}

} // namespace railshot
