#pragma once

#include <QString>
#include <QJsonObject>

namespace railshot {

/// Provider-specific adapters for YouTube / Twitch / Facebook chat & auth.
class PlatformAdapters {
public:
    static QString authorizeUrl(const QString& platform, const QString& clientId, const QString& redirectUri);
    static QJsonObject parseTokenResponse(const QString& platform, const QByteArray& body);
};

} // namespace railshot
