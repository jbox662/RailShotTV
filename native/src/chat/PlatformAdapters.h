#pragma once

#include <QString>
#include <QJsonObject>
#include <QByteArray>

namespace railshot {

/// Provider-specific adapters for YouTube / Twitch / Facebook chat & auth.
class PlatformAdapters {
public:
    static QString generateCodeVerifier();
    static QString codeChallengeS256(const QString& verifier);
    static QString generateState();

    static QString authorizeUrl(const QString& platform,
                                const QString& clientId,
                                const QString& redirectUri,
                                const QString& codeChallenge = {},
                                const QString& state = {});

    static QString tokenEndpoint(const QString& platform);
    static QByteArray tokenExchangeBody(const QString& platform,
                                        const QString& clientId,
                                        const QString& code,
                                        const QString& redirectUri,
                                        const QString& codeVerifier,
                                        const QString& clientSecret = {});

    static QJsonObject parseTokenResponse(const QString& platform, const QByteArray& body);
};

} // namespace railshot
