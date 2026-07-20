#include "chat/PlatformAdapters.h"
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>

namespace railshot {

QString PlatformAdapters::authorizeUrl(const QString& platform, const QString& clientId, const QString& redirectUri)
{
    QUrl url;
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("client_id"), clientId);
    q.addQueryItem(QStringLiteral("redirect_uri"), redirectUri);
    q.addQueryItem(QStringLiteral("response_type"), QStringLiteral("code"));

    if (platform == QLatin1String("twitch")) {
        url = QUrl(QStringLiteral("https://id.twitch.tv/oauth2/authorize"));
        q.addQueryItem(QStringLiteral("scope"), QStringLiteral("chat:read chat:edit"));
    } else if (platform == QLatin1String("youtube")) {
        url = QUrl(QStringLiteral("https://accounts.google.com/o/oauth2/v2/auth"));
        q.addQueryItem(QStringLiteral("scope"), QStringLiteral("https://www.googleapis.com/auth/youtube.force-ssl"));
    } else if (platform == QLatin1String("facebook")) {
        url = QUrl(QStringLiteral("https://www.facebook.com/v19.0/dialog/oauth"));
        q.addQueryItem(QStringLiteral("scope"), QStringLiteral("pages_messaging"));
    } else {
        return {};
    }
    url.setQuery(q);
    return url.toString();
}

QJsonObject PlatformAdapters::parseTokenResponse(const QString& platform, const QByteArray& body)
{
    Q_UNUSED(platform);
    return QJsonDocument::fromJson(body).object();
}

} // namespace railshot
