#pragma once

#include <QObject>
#include <QVector>
#include <QDateTime>
#include <QNetworkAccessManager>
#include <QTcpSocket>
#include <QTimer>
#include <QSet>

namespace railshot {

struct ChatMessage {
    QString id;
    QString platform;
    QString user;
    QString text;
    QDateTime ts;
    bool pinned = false;
};

class ChatService : public QObject {
    Q_OBJECT
public:
    explicit ChatService(QObject* parent = nullptr);
    ~ChatService() override;

    /// Twitch: channel name. YouTube: liveChatId or videoId. Facebook: live video id.
    bool connectPlatform(const QString& platform,
                         const QString& tokenSecretId,
                         const QString& channel = {},
                         QString* error = nullptr);
    void disconnectPlatform(const QString& platform);
    void sendMessage(const QString& platform, const QString& text);
    QVector<ChatMessage> messages() const { return m_messages; }
    QStringList connectedPlatforms() const { return m_connected; }

signals:
    void messageReceived(const ChatMessage& msg);
    void connectionChanged();

private slots:
    void onTwitchReady();
    void onTwitchData();
    void onTwitchError(QAbstractSocket::SocketError err);
    void pollYouTube();
    void pollFacebook();

private:
    void appendSystem(const QString& platform, const QString& text);
    void appendMessage(const QString& platform, const QString& user, const QString& text, const QString& id = {});
    void parseTwitchLine(const QString& line);
    bool connectTwitch(const QString& token, const QString& channel, QString* error);
    bool connectYouTube(const QString& token, const QString& liveChatOrVideoId, QString* error);
    bool connectFacebook(const QString& token, const QString& liveVideoId, QString* error);
    void resolveYouTubeLiveChatId(const QString& token, const QString& videoId);

    QNetworkAccessManager m_nam;
    QVector<ChatMessage> m_messages;
    QStringList m_connected;
    QSet<QString> m_seenIds;

    QTcpSocket* m_twitchSock = nullptr;
    QString m_twitchNick;
    QString m_twitchChannel;
    QString m_twitchToken;
    QByteArray m_twitchBuf;

    QTimer m_youtubePoll;
    QString m_youtubeToken;
    QString m_youtubeLiveChatId;
    QString m_youtubePageToken;

    QTimer m_facebookPoll;
    QString m_facebookToken;
    QString m_facebookLiveId;
};

} // namespace railshot
