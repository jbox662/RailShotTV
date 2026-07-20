#include "chat/PlatformAdapters.h"
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QByteArray>

namespace railshot {
namespace {

QString randomUrlSafe(int bytes)
{
    static const char alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~";
    QString out;
    out.reserve(bytes);
    auto* rng = QRandomGenerator::global();
    for (int i = 0; i < bytes; ++i)
        out.append(QLatin1Char(alphabet[rng->bounded(static_cast<int>(sizeof(alphabet) - 1))]));
    return out;
}

QString base64Url(const QByteArray& raw)
{
    return QString::fromLatin1(raw.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}

} // namespace

QString PlatformAdapters::generateCodeVerifier()
{
    return randomUrlSafe(64);
}

QString PlatformAdapters::codeChallengeS256(const QString& verifier)
{
    const QByteArray hash = QCryptographicHash::hash(verifier.toUtf8(), QCryptographicHash::Sha256);
    return base64Url(hash);
}

QString PlatformAdapters::generateState()
{
    return randomUrlSafe(24);
}

QString PlatformAdapters::authorizeUrl(const QString& platform,
                                       const QString& clientId,
                                       const QString& redirectUri,
                                       const QString& codeChallenge,
                                       const QString& state)
{
    QUrl url;
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("client_id"), clientId);
    q.addQueryItem(QStringLiteral("redirect_uri"), redirectUri);
    q.addQueryItem(QStringLiteral("response_type"), QStringLiteral("code"));
    if (!state.isEmpty())
        q.addQueryItem(QStringLiteral("state"), state);
    if (!codeChallenge.isEmpty()) {
        q.addQueryItem(QStringLiteral("code_challenge"), codeChallenge);
        q.addQueryItem(QStringLiteral("code_challenge_method"), QStringLiteral("S256"));
    }

    if (platform == QLatin1String("twitch")) {
        url = QUrl(QStringLiteral("https://id.twitch.tv/oauth2/authorize"));
        q.addQueryItem(QStringLiteral("scope"),
                       QStringLiteral("chat:read chat:edit moderator:manage:chat_settings "
                                      "moderator:manage:chat_messages moderator:manage:banned_users"));
    } else if (platform == QLatin1String("youtube")) {
        url = QUrl(QStringLiteral("https://accounts.google.com/o/oauth2/v2/auth"));
        q.addQueryItem(QStringLiteral("scope"), QStringLiteral("https://www.googleapis.com/auth/youtube.force-ssl"));
        q.addQueryItem(QStringLiteral("access_type"), QStringLiteral("offline"));
        q.addQueryItem(QStringLiteral("prompt"), QStringLiteral("consent"));
    } else if (platform == QLatin1String("facebook")) {
        url = QUrl(QStringLiteral("https://www.facebook.com/v19.0/dialog/oauth"));
        q.addQueryItem(QStringLiteral("scope"), QStringLiteral("pages_messaging"));
    } else {
        return {};
    }
    url.setQuery(q);
    return url.toString();
}

QString PlatformAdapters::tokenEndpoint(const QString& platform)
{
    if (platform == QLatin1String("twitch"))
        return QStringLiteral("https://id.twitch.tv/oauth2/token");
    if (platform == QLatin1String("youtube"))
        return QStringLiteral("https://oauth2.googleapis.com/token");
    if (platform == QLatin1String("facebook"))
        return QStringLiteral("https://graph.facebook.com/v19.0/oauth/access_token");
    return {};
}

QByteArray PlatformAdapters::tokenExchangeBody(const QString& platform,
                                               const QString& clientId,
                                               const QString& code,
                                               const QString& redirectUri,
                                               const QString& codeVerifier,
                                               const QString& clientSecret)
{
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("client_id"), clientId);
    q.addQueryItem(QStringLiteral("code"), code);
    q.addQueryItem(QStringLiteral("grant_type"), QStringLiteral("authorization_code"));
    q.addQueryItem(QStringLiteral("redirect_uri"), redirectUri);
    if (!codeVerifier.isEmpty())
        q.addQueryItem(QStringLiteral("code_verifier"), codeVerifier);
    if (!clientSecret.isEmpty())
        q.addQueryItem(QStringLiteral("client_secret"), clientSecret);
    Q_UNUSED(platform);
    return q.query(QUrl::FullyEncoded).toUtf8();
}

QJsonObject PlatformAdapters::parseTokenResponse(const QString& platform, const QByteArray& body)
{
    Q_UNUSED(platform);
    return QJsonDocument::fromJson(body).object();
}

} // namespace railshot
