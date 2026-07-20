#pragma once
#include <QObject>
#include <QWebSocket>
#include <QTimer>
#include <QString>
#include <QStringList>

// ─────────────────────────────────────────────────────────────────────────────
// TwitchChatClient
//
// Connects to Twitch IRC over WSS (wss://irc-ws.chat.twitch.tv:443).
// Supports anonymous read-only access (no OAuth token required for public
// channels) and authenticated access for sending messages.
//
// Usage:
//   TwitchChatClient *client = new TwitchChatClient(this);
//   connect(client, &TwitchChatClient::messageReceived, this, &MyWidget::onMessage);
//   connect(client, &TwitchChatClient::statusChanged,   this, &MyWidget::onStatus);
//   client->connectToChannel("your_channel", "optional_oauth_token");
// ─────────────────────────────────────────────────────────────────────────────

struct TwitchMessage {
    QString username;
    QString displayName;
    QString color;          // hex color e.g. "#9146FF", empty if unset
    QString message;
    QString timestamp;      // HH:mm:ss
    bool    isMod    = false;
    bool    isSub    = false;
    bool    isVip    = false;
    bool    isBroadcaster = false;
    QString badgeInfo;      // raw badge-info tag value
    QString badges;         // raw badges tag value
    QString msgId;          // message-id for moderation
    QString channelId;
    QString userId;
};

struct TwitchEvent {
    enum Type {
        Sub, Resub, GiftSub, GiftSubCommunity,
        Raid, Ritual, BitsDonation, UserBanned, UserTimedOut
    };
    Type    type;
    QString username;
    QString systemMsg;
    QString message;        // optional user message (resub)
    int     months   = 0;   // sub streak/cumulative
    int     giftCount = 0;  // gift sub count
    int     bits     = 0;   // bits amount
    int     raidViewers = 0;
    QString timestamp;
};

class TwitchChatClient : public QObject {
    Q_OBJECT
public:
    enum Status { Disconnected, Connecting, Connected, Reconnecting, Error };
    Q_ENUM(Status)

    explicit TwitchChatClient(QObject *parent = nullptr);
    ~TwitchChatClient() override;

    // Connect to a channel. oauthToken is optional for read-only.
    // Format: "oauth:xxxxxxxxxxxxxxxxxxxxxx" or empty for anonymous.
    void connectToChannel(const QString &channel, const QString &oauthToken = {});
    void disconnectFromChannel();

    // Send a chat message (requires authenticated token with chat:edit scope)
    void sendMessage(const QString &message);

    // Moderation (requires channel:moderate scope)
    void banUser(const QString &username, const QString &reason = {});
    void timeoutUser(const QString &username, int seconds = 600);
    void clearChat();

    Status      status()  const { return m_status; }
    QString     channel() const { return m_channel; }
    bool        isConnected() const { return m_status == Connected; }

signals:
    void statusChanged(Status status, const QString &detail);
    void messageReceived(const TwitchMessage &msg);
    void eventReceived(const TwitchEvent &event);
    void viewerCountUpdated(int count);
    void chatCleared(const QString &username); // empty = full clear
    void messageDeleted(const QString &msgId);

private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString &raw);
    void onError(QAbstractSocket::SocketError error);
    void onPingTimer();
    void onReconnectTimer();

private:
    void        setStatus(Status s, const QString &detail = {});
    void        authenticate();
    void        joinChannel();
    void        parseLine(const QString &line);
    void        parsePrivmsg(const QMap<QString,QString> &tags,
                             const QString &prefix, const QString &params);
    void        parseUsernotice(const QMap<QString,QString> &tags,
                                const QString &prefix, const QString &params);
    void        parseClearmsg(const QMap<QString,QString> &tags);
    void        parseClearchat(const QMap<QString,QString> &tags,
                               const QString &params);
    QMap<QString,QString> parseTags(const QString &tagStr);
    QString     currentTimestamp() const;

    QWebSocket  *m_socket      = nullptr;
    QTimer      *m_pingTimer   = nullptr;
    QTimer      *m_reconnTimer = nullptr;
    Status       m_status      = Disconnected;
    QString      m_channel;
    QString      m_oauthToken;
    int          m_reconnectAttempts = 0;
    static constexpr int kMaxReconnects = 5;
    static constexpr int kPingIntervalMs  = 60000;  // 60 s
    static constexpr int kReconnectBaseMs = 2000;   // exponential backoff base
};
