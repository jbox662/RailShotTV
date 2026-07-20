#include "chat/ChatService.h"
#include "core/SecretStore.h"
#include "core/Logger.h"
#include "core/Types.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QUrlQuery>
#include <QEventLoop>
#include <QTimer>

namespace railshot {

ChatService::ChatService(QObject* parent)
    : QObject(parent)
{
    connect(&m_youtubePoll, &QTimer::timeout, this, &ChatService::pollYouTube);
    connect(&m_facebookPoll, &QTimer::timeout, this, &ChatService::pollFacebook);
}

ChatService::~ChatService()
{
    disconnectPlatform(QStringLiteral("twitch"));
    disconnectPlatform(QStringLiteral("youtube"));
    disconnectPlatform(QStringLiteral("facebook"));
}

void ChatService::appendSystem(const QString& platform, const QString& text)
{
    appendMessage(platform, QStringLiteral("system"), text);
}

void ChatService::appendMessage(const QString& platform, const QString& user, const QString& text, const QString& id)
{
    if (!id.isEmpty()) {
        if (m_seenIds.contains(id))
            return;
        m_seenIds.insert(id);
        if (m_seenIds.size() > 2000) {
            // Simple prune: clear and keep going (dup risk only across reconnect)
            m_seenIds.clear();
            m_seenIds.insert(id);
        }
    }
    ChatMessage m;
    m.id = id.isEmpty() ? newId(QStringLiteral("msg")) : id;
    m.platform = platform;
    m.user = user;
    m.text = text;
    m.ts = QDateTime::currentDateTimeUtc();
    m_messages.append(m);
    if (m_messages.size() > 500)
        m_messages.remove(0, m_messages.size() - 500);
    emit messageReceived(m);
}

bool ChatService::connectPlatform(const QString& platform,
                                  const QString& tokenSecretId,
                                  const QString& channel,
                                  QString* error)
{
    auto token = SecretStore::load(tokenSecretId);
    if (!token || token->isEmpty()) {
        if (error) *error = QStringLiteral("Missing OAuth token for %1").arg(platform);
        return false;
    }

    if (platform == QLatin1String("twitch")) {
        QString ch = channel.trimmed();
        if (ch.startsWith(QLatin1Char('#')))
            ch = ch.mid(1);
        if (ch.isEmpty()) {
            if (error) *error = QStringLiteral("Twitch channel name required");
            return false;
        }
        return connectTwitch(*token, ch.toLower(), error);
    }
    if (platform == QLatin1String("youtube")) {
        if (channel.trimmed().isEmpty()) {
            if (error) *error = QStringLiteral("YouTube liveChatId or videoId required");
            return false;
        }
        return connectYouTube(*token, channel.trimmed(), error);
    }
    if (platform == QLatin1String("facebook")) {
        if (channel.trimmed().isEmpty()) {
            if (error) *error = QStringLiteral("Facebook live video id required");
            return false;
        }
        return connectFacebook(*token, channel.trimmed(), error);
    }

    if (!m_connected.contains(platform))
        m_connected.append(platform);
    emit connectionChanged();
    appendSystem(platform, QStringLiteral("Connected to %1 (local echo mode)").arg(platform));
    return true;
}

bool ChatService::connectTwitch(const QString& token, const QString& channel, QString* error)
{
    disconnectPlatform(QStringLiteral("twitch"));

    QString nick;
    {
        QNetworkRequest req{QUrl(QStringLiteral("https://id.twitch.tv/oauth2/validate"))};
        QString auth = token;
        if (!auth.startsWith(QLatin1String("oauth:"), Qt::CaseInsensitive)
            && !auth.startsWith(QLatin1String("Bearer "), Qt::CaseInsensitive))
            auth = QStringLiteral("OAuth %1").arg(token);
        else if (auth.startsWith(QLatin1String("oauth:"), Qt::CaseInsensitive))
            auth = QStringLiteral("OAuth %1").arg(token.mid(6));
        req.setRawHeader("Authorization", auth.toUtf8());

        QEventLoop loop;
        QNetworkReply* reply = m_nam.get(req);
        QTimer::singleShot(8000, &loop, &QEventLoop::quit);
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();
        if (reply->error() == QNetworkReply::NoError) {
            const auto doc = QJsonDocument::fromJson(reply->readAll());
            nick = doc.object().value(QStringLiteral("login")).toString();
        }
        reply->deleteLater();
    }
    if (nick.isEmpty())
        nick = channel;

    m_twitchNick = nick.toLower();
    m_twitchChannel = channel.toLower();
    m_twitchToken = token;
    if (m_twitchToken.startsWith(QLatin1String("oauth:"), Qt::CaseInsensitive))
        m_twitchToken = m_twitchToken.mid(6);

    m_twitchSock = new QTcpSocket(this);
    connect(m_twitchSock, &QTcpSocket::connected, this, &ChatService::onTwitchReady);
    connect(m_twitchSock, &QTcpSocket::readyRead, this, &ChatService::onTwitchData);
    connect(m_twitchSock, &QTcpSocket::errorOccurred, this, &ChatService::onTwitchError);

    m_twitchSock->connectToHost(QStringLiteral("irc.chat.twitch.tv"), 6667);
    if (!m_twitchSock->waitForConnected(8000)) {
        if (error) *error = QStringLiteral("Twitch IRC connect failed: %1").arg(m_twitchSock->errorString());
        m_twitchSock->deleteLater();
        m_twitchSock = nullptr;
        return false;
    }

    if (!m_connected.contains(QLatin1String("twitch")))
        m_connected.append(QStringLiteral("twitch"));
    emit connectionChanged();
    return true;
}

void ChatService::onTwitchReady()
{
    if (!m_twitchSock) return;
    m_twitchSock->write(QByteArray("PASS oauth:") + m_twitchToken.toUtf8() + "\r\n");
    m_twitchSock->write(QByteArray("NICK ") + m_twitchNick.toUtf8() + "\r\n");
    m_twitchSock->write(QByteArray("USER ") + m_twitchNick.toUtf8() + " 8 * :" + m_twitchNick.toUtf8() + "\r\n");
    m_twitchSock->write("CAP REQ :twitch.tv/tags twitch.tv/commands\r\n");
    m_twitchSock->write(QByteArray("JOIN #") + m_twitchChannel.toUtf8() + "\r\n");
    Logger::info(QStringLiteral("Twitch IRC joined #%1 as %2").arg(m_twitchChannel, m_twitchNick));
    appendSystem(QStringLiteral("twitch"),
                 QStringLiteral("Joined #%1 as %2").arg(m_twitchChannel, m_twitchNick));
}

void ChatService::onTwitchData()
{
    if (!m_twitchSock) return;
    m_twitchBuf.append(m_twitchSock->readAll());
    while (true) {
        const int idx = m_twitchBuf.indexOf("\r\n");
        if (idx < 0) break;
        const QString line = QString::fromUtf8(m_twitchBuf.left(idx));
        m_twitchBuf.remove(0, idx + 2);
        parseTwitchLine(line);
    }
}

void ChatService::parseTwitchLine(const QString& line)
{
    if (line.startsWith(QLatin1String("PING "))) {
        if (m_twitchSock)
            m_twitchSock->write(QByteArray("PONG ") + line.mid(5).toUtf8() + "\r\n");
        return;
    }

    QString msg = line;
    if (msg.startsWith(QLatin1Char('@'))) {
        const int sp = msg.indexOf(QLatin1Char(' '));
        if (sp > 0) msg = msg.mid(sp + 1);
    }

    if (!msg.contains(QLatin1String(" PRIVMSG ")))
        return;
    const int bang = msg.indexOf(QLatin1Char('!'));
    const int priv = msg.indexOf(QLatin1String(" PRIVMSG "));
    const int colon = msg.indexOf(QLatin1String(" :"), priv);
    if (bang < 1 || priv < 0 || colon < 0) return;
    appendMessage(QStringLiteral("twitch"), msg.mid(1, bang - 1), msg.mid(colon + 2));
}

void ChatService::onTwitchError(QAbstractSocket::SocketError)
{
    if (!m_twitchSock) return;
    Logger::warn(QStringLiteral("Twitch IRC error: %1").arg(m_twitchSock->errorString()));
    appendSystem(QStringLiteral("twitch"), QStringLiteral("IRC error: %1").arg(m_twitchSock->errorString()));
}

bool ChatService::connectYouTube(const QString& token, const QString& liveChatOrVideoId, QString* error)
{
    disconnectPlatform(QStringLiteral("youtube"));
    m_youtubeToken = token;
    m_youtubePageToken.clear();

    // Heuristic: liveChatIds are long; videoIds are 11 chars-ish. Try as liveChatId first,
    // and if polling fails with notFound, resolve via videos.list.
    m_youtubeLiveChatId = liveChatOrVideoId;
    if (liveChatOrVideoId.size() <= 16) {
        resolveYouTubeLiveChatId(token, liveChatOrVideoId);
        if (m_youtubeLiveChatId.isEmpty()) {
            if (error) *error = QStringLiteral("Could not resolve YouTube liveChatId from videoId");
            return false;
        }
    }

    if (!m_connected.contains(QLatin1String("youtube")))
        m_connected.append(QStringLiteral("youtube"));
    emit connectionChanged();
    appendSystem(QStringLiteral("youtube"),
                 QStringLiteral("Polling live chat %1").arg(m_youtubeLiveChatId));
    m_youtubePoll.start(3000);
    pollYouTube();
    return true;
}

void ChatService::resolveYouTubeLiveChatId(const QString& token, const QString& videoId)
{
    QUrl url(QStringLiteral("https://www.googleapis.com/youtube/v3/videos"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("part"), QStringLiteral("liveStreamingDetails"));
    q.addQueryItem(QStringLiteral("id"), videoId);
    url.setQuery(q);

    QNetworkRequest req(url);
    req.setRawHeader("Authorization", QByteArray("Bearer ") + token.toUtf8());

    QEventLoop loop;
    QNetworkReply* reply = m_nam.get(req);
    QTimer::singleShot(8000, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    if (reply->error() == QNetworkReply::NoError) {
        const auto doc = QJsonDocument::fromJson(reply->readAll());
        const auto items = doc.object().value(QStringLiteral("items")).toArray();
        if (!items.isEmpty()) {
            m_youtubeLiveChatId = items.at(0).toObject()
                                      .value(QStringLiteral("liveStreamingDetails")).toObject()
                                      .value(QStringLiteral("activeLiveChatId")).toString();
        }
    }
    reply->deleteLater();
}

void ChatService::pollYouTube()
{
    if (m_youtubeLiveChatId.isEmpty() || m_youtubeToken.isEmpty())
        return;

    QUrl url(QStringLiteral("https://www.googleapis.com/youtube/v3/liveChat/messages"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("liveChatId"), m_youtubeLiveChatId);
    q.addQueryItem(QStringLiteral("part"), QStringLiteral("snippet,authorDetails"));
    if (!m_youtubePageToken.isEmpty())
        q.addQueryItem(QStringLiteral("pageToken"), m_youtubePageToken);
    url.setQuery(q);

    QNetworkRequest req(url);
    req.setRawHeader("Authorization", QByteArray("Bearer ") + m_youtubeToken.toUtf8());
    QNetworkReply* reply = m_nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (reply->error() == QNetworkReply::NoError) {
            const auto doc = QJsonDocument::fromJson(reply->readAll());
            const auto obj = doc.object();
            m_youtubePageToken = obj.value(QStringLiteral("nextPageToken")).toString();
            const int interval = obj.value(QStringLiteral("pollingIntervalMillis")).toInt(3000);
            m_youtubePoll.setInterval(qMax(1500, interval));
            const auto items = obj.value(QStringLiteral("items")).toArray();
            for (const auto& v : items) {
                const auto item = v.toObject();
                const QString id = item.value(QStringLiteral("id")).toString();
                const auto snip = item.value(QStringLiteral("snippet")).toObject();
                const auto author = item.value(QStringLiteral("authorDetails")).toObject();
                appendMessage(QStringLiteral("youtube"),
                              author.value(QStringLiteral("displayName")).toString(QStringLiteral("user")),
                              snip.value(QStringLiteral("displayMessage")).toString(),
                              id);
            }
        } else {
            Logger::warn(QStringLiteral("YouTube chat poll failed: %1").arg(reply->errorString()));
        }
        reply->deleteLater();
    });
}

bool ChatService::connectFacebook(const QString& token, const QString& liveVideoId, QString* error)
{
    Q_UNUSED(error);
    disconnectPlatform(QStringLiteral("facebook"));
    m_facebookToken = token;
    m_facebookLiveId = liveVideoId;
    if (!m_connected.contains(QLatin1String("facebook")))
        m_connected.append(QStringLiteral("facebook"));
    emit connectionChanged();
    appendSystem(QStringLiteral("facebook"),
                 QStringLiteral("Polling comments on live video %1").arg(liveVideoId));
    m_facebookPoll.start(4000);
    pollFacebook();
    return true;
}

void ChatService::pollFacebook()
{
    if (m_facebookLiveId.isEmpty() || m_facebookToken.isEmpty())
        return;

    QUrl url(QStringLiteral("https://graph.facebook.com/v19.0/%1/comments").arg(m_facebookLiveId));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("access_token"), m_facebookToken);
    q.addQueryItem(QStringLiteral("fields"), QStringLiteral("id,from,message,created_time"));
    q.addQueryItem(QStringLiteral("order"), QStringLiteral("chronological"));
    q.addQueryItem(QStringLiteral("limit"), QStringLiteral("25"));
    url.setQuery(q);

    QNetworkReply* reply = m_nam.get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (reply->error() == QNetworkReply::NoError) {
            const auto doc = QJsonDocument::fromJson(reply->readAll());
            const auto data = doc.object().value(QStringLiteral("data")).toArray();
            for (const auto& v : data) {
                const auto item = v.toObject();
                const QString id = item.value(QStringLiteral("id")).toString();
                const auto from = item.value(QStringLiteral("from")).toObject();
                appendMessage(QStringLiteral("facebook"),
                              from.value(QStringLiteral("name")).toString(QStringLiteral("user")),
                              item.value(QStringLiteral("message")).toString(),
                              id);
            }
        } else {
            Logger::warn(QStringLiteral("Facebook chat poll failed: %1").arg(reply->errorString()));
        }
        reply->deleteLater();
    });
}

void ChatService::disconnectPlatform(const QString& platform)
{
    if (platform == QLatin1String("twitch") || platform.isEmpty()) {
        if (m_twitchSock) {
            m_twitchSock->disconnectFromHost();
            m_twitchSock->deleteLater();
            m_twitchSock = nullptr;
        }
        m_twitchBuf.clear();
    }
    if (platform == QLatin1String("youtube") || platform.isEmpty()) {
        m_youtubePoll.stop();
        m_youtubeLiveChatId.clear();
        m_youtubePageToken.clear();
        m_youtubeToken.clear();
    }
    if (platform == QLatin1String("facebook") || platform.isEmpty()) {
        m_facebookPoll.stop();
        m_facebookLiveId.clear();
        m_facebookToken.clear();
    }
    if (platform.isEmpty())
        m_connected.clear();
    else
        m_connected.removeAll(platform);
    emit connectionChanged();
}

void ChatService::sendMessage(const QString& platform, const QString& text)
{
    if (text.trimmed().isEmpty()) return;
    if (!m_connected.contains(platform)) return;

    if (platform == QLatin1String("twitch") && m_twitchSock
        && m_twitchSock->state() == QAbstractSocket::ConnectedState) {
        m_twitchSock->write(QByteArray("PRIVMSG #") + m_twitchChannel.toUtf8()
                            + " :" + text.toUtf8() + "\r\n");
        appendMessage(platform, m_twitchNick, text);
        return;
    }

    if (platform == QLatin1String("youtube") && !m_youtubeLiveChatId.isEmpty()) {
        QUrl url(QStringLiteral("https://www.googleapis.com/youtube/v3/liveChat/messages"));
        QUrlQuery q;
        q.addQueryItem(QStringLiteral("part"), QStringLiteral("snippet"));
        url.setQuery(q);
        QNetworkRequest req(url);
        req.setRawHeader("Authorization", QByteArray("Bearer ") + m_youtubeToken.toUtf8());
        req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
        QJsonObject snip;
        snip.insert(QStringLiteral("liveChatId"), m_youtubeLiveChatId);
        snip.insert(QStringLiteral("type"), QStringLiteral("textMessageEvent"));
        QJsonObject textObj;
        textObj.insert(QStringLiteral("messageText"), text);
        snip.insert(QStringLiteral("textMessageDetails"), textObj);
        QJsonObject body;
        body.insert(QStringLiteral("snippet"), snip);
        m_nam.post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
        appendMessage(platform, QStringLiteral("you"), text);
        return;
    }

    if (platform == QLatin1String("facebook") && !m_facebookLiveId.isEmpty()) {
        QUrl url(QStringLiteral("https://graph.facebook.com/v19.0/%1/comments").arg(m_facebookLiveId));
        QUrlQuery q;
        q.addQueryItem(QStringLiteral("access_token"), m_facebookToken);
        q.addQueryItem(QStringLiteral("message"), text);
        url.setQuery(q);
        m_nam.post(QNetworkRequest(url), QByteArray());
        appendMessage(platform, QStringLiteral("you"), text);
        return;
    }

    appendMessage(platform, QStringLiteral("you"), text);
}

} // namespace railshot
