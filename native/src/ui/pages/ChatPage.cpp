#include "ui/pages/ChatPage.h"
#include "ui/Theme.h"
#include "chat/ChatService.h"
#include "chat/PlatformAdapters.h"
#include "core/SecretStore.h"
#include "core/Types.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QInputDialog>
#include <QMessageBox>
#include <QLabel>
#include <QDesktopServices>
#include <QUrl>
#include <QUrlQuery>
#include <QSettings>
#include <QColor>
#include <QTabWidget>
#include <QCheckBox>
#include <QFrame>
#include <QScrollArea>
#include <QStyle>
#include <QTcpServer>
#include <QTcpSocket>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QPointer>
#include <QJsonObject>
#include <QHostAddress>
#include <QMenu>
#include <functional>

namespace railshot {

ChatPage::ChatPage(ChatService* chat, QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("chatPage"));
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    root->addWidget(theme::makePageHeader(QStringLiteral("Chat & Audience"), theme::PanelAccent::Violet, this));

    auto* body = new QHBoxLayout();
    body->setContentsMargins(0, 0, 0, 0);
    body->setSpacing(0);

    // ── Left: Connections ─────────────────────────────────────────────────
    auto* left = new QFrame(this);
    left->setObjectName(QStringLiteral("chatLeftRail"));
    left->setFixedWidth(220);
    auto* leftLay = new QVBoxLayout(left);
    leftLay->setContentsMargins(12, 12, 12, 12);
    leftLay->setSpacing(8);
    auto* connTitle = new QLabel(QStringLiteral("CONNECTIONS"), left);
    connTitle->setStyleSheet(QStringLiteral("color:#C084FC; font-weight:900; font-size:10px; letter-spacing:1.5px; background:transparent;"));
    leftLay->addWidget(connTitle);

    auto* platform = new QComboBox(left);
    platform->addItem(QStringLiteral("Twitch"), QStringLiteral("twitch"));
    platform->addItem(QStringLiteral("YouTube"), QStringLiteral("youtube"));
    platform->addItem(QStringLiteral("Facebook"), QStringLiteral("facebook"));
    leftLay->addWidget(platform);

    auto* channel = new QLineEdit(left);
    channel->setPlaceholderText(QStringLiteral("Channel / live id"));
    leftLay->addWidget(channel);

    auto* clientId = new QLineEdit(left);
    clientId->setPlaceholderText(QStringLiteral("OAuth client id"));
    QSettings s(QStringLiteral("RailShotTV"), QStringLiteral("RailShotTV"));
    clientId->setText(s.value(QStringLiteral("chat/clientId")).toString());
    leftLay->addWidget(clientId);

    auto* statusRow = new QHBoxLayout();
    auto* connDot = new QLabel(left);
    connDot->setObjectName(QStringLiteral("connDotDisconnected"));
    auto* connLbl = new QLabel(QStringLiteral("Disconnected"), left);
    connLbl->setStyleSheet(QStringLiteral("color:#606878; font-size:11px;"));
    statusRow->addWidget(connDot);
    statusRow->addWidget(connLbl, 1);
    leftLay->addLayout(statusRow);

    auto* oauthBtn = new QPushButton(QStringLiteral("Sign in with browser"), left);
    oauthBtn->setObjectName(QStringLiteral("chromeBtn"));
    auto* connectBtn = new QPushButton(QStringLiteral("Connect"), left);
    connectBtn->setObjectName(QStringLiteral("chromeBtnViolet"));
    leftLay->addWidget(oauthBtn);
    leftLay->addWidget(connectBtn);
    auto* pasteHint = new QLabel(QStringLiteral("Twitch: browser PKCE. Other platforms: paste token if needed."), left);
    pasteHint->setWordWrap(true);
    pasteHint->setStyleSheet(QStringLiteral("color:#606878; font-size:10px;"));
    leftLay->addWidget(pasteHint);

    auto* filterTitle = new QLabel(QStringLiteral("FILTER"), left);
    filterTitle->setStyleSheet(QStringLiteral("color:#8892A4; font-weight:800; font-size:10px; letter-spacing:1.5px; padding-top:12px;"));
    leftLay->addWidget(filterTitle);
    auto* filterAll = new QCheckBox(QStringLiteral("Show all platforms"), left);
    filterAll->setChecked(true);
    leftLay->addWidget(filterAll);
    QString* filterPlatform = new QString(); // empty = all

    auto* statsTitle = new QLabel(QStringLiteral("SESSION"), left);
    statsTitle->setStyleSheet(QStringLiteral("color:#8892A4; font-weight:800; font-size:10px; letter-spacing:1.5px; padding-top:12px;"));
    leftLay->addWidget(statsTitle);
    auto* msgCount = new QLabel(QStringLiteral("Messages  0"), left);
    msgCount->setObjectName(QStringLiteral("mono"));
    msgCount->setStyleSheet(QStringLiteral("color:#606878; font-size:11px;"));
    leftLay->addWidget(msgCount);
    leftLay->addStretch();
    body->addWidget(left);

    // ── Center: feed ──────────────────────────────────────────────────────
    auto* center = new QWidget(this);
    auto* centerLay = new QVBoxLayout(center);
    centerLay->setContentsMargins(0, 0, 0, 0);
    centerLay->setSpacing(0);
    auto* tabs = new QTabWidget(center);
    tabs->setDocumentMode(true);
    tabs->setStyleSheet(QStringLiteral(
        "QTabBar::tab{background:#0F1114;color:#808898;padding:10px 14px;font-size:11px;font-weight:700;"
        "border-bottom:2px solid transparent;}"
        "QTabBar::tab:selected{color:#A855F7;border-bottom:2px solid #A855F7;}"));

    auto* chatTab = new QWidget(tabs);
    auto* chatLay = new QVBoxLayout(chatTab);
    chatLay->setContentsMargins(8, 8, 8, 8);
    auto* list = new QListWidget(chatTab);
    list->setStyleSheet(QStringLiteral(
        "QListWidget{background:#0A0C0F;border:1px solid #4A4D55;border-radius:4px;}"
        "QListWidget::item{padding:8px;border-bottom:1px solid #2A2D35;}"));
    chatLay->addWidget(list, 1);
    auto* sendRow = new QHBoxLayout();
    auto* input = new QLineEdit(chatTab);
    input->setPlaceholderText(QStringLiteral("Send a message…"));
    auto* send = new QPushButton(QStringLiteral("Send"), chatTab);
    send->setObjectName(QStringLiteral("chromeBtnPrimary"));
    sendRow->addWidget(input, 1);
    sendRow->addWidget(send);
    chatLay->addLayout(sendRow);
    tabs->addTab(chatTab, QStringLiteral("Live Chat"));

    auto* activity = new QListWidget(tabs);
    activity->setStyleSheet(QStringLiteral("background:#0A0C0F; border:none; color:#808898;"));
    activity->addItem(QStringLiteral("No activity yet"));
    tabs->addTab(activity, QStringLiteral("Activity"));

    auto* modTab = new QWidget(tabs);
    auto* modLay = new QVBoxLayout(modTab);
    modLay->setContentsMargins(16, 16, 16, 16);
    auto* modNote = new QLabel(QStringLiteral("Twitch Helix — re-auth after Wave 4 to pick up mod scopes."), modTab);
    modNote->setWordWrap(true);
    modNote->setStyleSheet(QStringLiteral("color:#606878; font-size:10px;"));
    modLay->addWidget(modNote);

    auto makeToggle = [&](const QString& label, const QString& color, const QString& tip) {
        auto* b = new QPushButton(label, modTab);
        b->setCheckable(true);
        b->setToolTip(tip);
        b->setStyleSheet(QStringLiteral(
            "QPushButton{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #2A2D35,stop:1 #1A1D22);"
            "border:1px solid %1;color:%1;padding:8px;border-radius:3px;font-weight:800;}"
            "QPushButton:checked{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 %120,stop:1 #12151A);"
            "border:2px solid %1;color:#FFFFFF;}").arg(color));
        modLay->addWidget(b);
        return b;
    };
    auto* slowBtn = makeToggle(QStringLiteral("Slow Mode (30s)"), QStringLiteral("#FBBF24"),
                               QStringLiteral("Twitch Helix: slow_mode 30s"));
    auto* subBtn = makeToggle(QStringLiteral("Subscribers Only"), QStringLiteral("#A855F7"),
                              QStringLiteral("Twitch Helix: subscriber_mode"));
    auto* emoteBtn = makeToggle(QStringLiteral("Emote Only"), QStringLiteral("#22D3EE"),
                                QStringLiteral("Twitch Helix: emote_mode"));
    auto* muteAlerts = makeToggle(QStringLiteral("Mute Alerts (local)"), QStringLiteral("#FF5A2C"),
                                  QStringLiteral("Hides alert-like lines in the local feed only"));
    bool* muteAlertsOn = new bool(false);

    auto pushTwitchSettings = [=] {
        if (!chat->isTwitchConnected()) {
            QMessageBox::information(this, QStringLiteral("Moderation"),
                                     QStringLiteral("Connect Twitch first (with mod scopes)."));
            return;
        }
        chat->setTwitchClientId(clientId->text().trimmed());
        TwitchChatSettings s;
        s.slowMode = slowBtn->isChecked();
        s.slowWaitSec = 30;
        s.subscriberMode = subBtn->isChecked();
        s.emoteMode = emoteBtn->isChecked();
        QString err;
        if (!chat->applyTwitchChatSettings(s, &err))
            QMessageBox::warning(this, QStringLiteral("Moderation"), err);
    };
    connect(slowBtn, &QPushButton::clicked, this, pushTwitchSettings);
    connect(subBtn, &QPushButton::clicked, this, pushTwitchSettings);
    connect(emoteBtn, &QPushButton::clicked, this, pushTwitchSettings);
    connect(muteAlerts, &QPushButton::toggled, this, [muteAlertsOn](bool on) { *muteAlertsOn = on; });

    auto* clearTwitch = new QPushButton(QStringLiteral("Clear Twitch Chat"), modTab);
    clearTwitch->setObjectName(QStringLiteral("chromeBtnBrand"));
    clearTwitch->setToolTip(QStringLiteral("Helix DELETE /moderation/chat"));
    connect(clearTwitch, &QPushButton::clicked, this, [=] {
        chat->setTwitchClientId(clientId->text().trimmed());
        QString err;
        if (!chat->clearTwitchChat(&err))
            QMessageBox::warning(this, QStringLiteral("Moderation"), err);
        else
            list->clear();
    });
    modLay->addWidget(clearTwitch);

    auto* clearLocal = new QPushButton(QStringLiteral("Clear Local Feed"), modTab);
    clearLocal->setObjectName(QStringLiteral("chromeBtn"));
    connect(clearLocal, &QPushButton::clicked, this, [list] { list->clear(); });
    modLay->addWidget(clearLocal);
    modLay->addStretch();
    tabs->addTab(modTab, QStringLiteral("Moderation"));

    centerLay->addWidget(tabs, 1);
    body->addWidget(center, 1);

    // ── Right: pinned / highlights ─────────────────────────────────────────
    auto* right = new QFrame(this);
    right->setObjectName(QStringLiteral("chatRightRail"));
    right->setFixedWidth(220);
    auto* rightLay = new QVBoxLayout(right);
    rightLay->setContentsMargins(12, 12, 12, 12);
    auto* pinTitle = new QLabel(QStringLiteral("PINNED"), right);
    pinTitle->setStyleSheet(QStringLiteral("color:#4F9EFF; font-weight:800; font-size:10px; letter-spacing:1.5px;"));
    rightLay->addWidget(pinTitle);
    auto* pinned = new QLabel(QStringLiteral("No pinned messages"), right);
    pinned->setWordWrap(true);
    pinned->setStyleSheet(QStringLiteral("color:#606878; font-size:11px;"));
    rightLay->addWidget(pinned);
    auto* hiTitle = new QLabel(QStringLiteral("HIGHLIGHTS"), right);
    hiTitle->setStyleSheet(QStringLiteral("color:#A855F7; font-weight:800; font-size:10px; letter-spacing:1.5px; padding-top:16px;"));
    rightLay->addWidget(hiTitle);
    auto* highlights = new QListWidget(right);
    highlights->setStyleSheet(QStringLiteral("background:transparent; border:none; color:#A0A8B8; font-size:11px;"));
    rightLay->addWidget(highlights, 1);
    auto* recentTitle = new QLabel(QStringLiteral("RECENT ACTIVITY"), right);
    recentTitle->setStyleSheet(QStringLiteral("color:#22D3EE; font-weight:800; font-size:10px; letter-spacing:1.5px;"));
    rightLay->addWidget(recentTitle);
    auto* recent = new QLabel(QStringLiteral("—"), right);
    recent->setStyleSheet(QStringLiteral("color:#606878; font-size:11px;"));
    rightLay->addWidget(recent);
    body->addWidget(right);

    root->addLayout(body, 1);

    auto updatePlatformAccent = [platform, connectBtn, channel] {
        const QString id = platform->currentData().toString();
        QString color = QStringLiteral("#9146FF");
        if (id == QLatin1String("youtube")) {
            color = QStringLiteral("#FF0000");
            channel->setPlaceholderText(QStringLiteral("liveChatId or videoId"));
        } else if (id == QLatin1String("facebook")) {
            color = QStringLiteral("#1877F2");
            channel->setPlaceholderText(QStringLiteral("live video id"));
        } else {
            channel->setPlaceholderText(QStringLiteral("twitch channel"));
        }
        connectBtn->setStyleSheet(QStringLiteral(
            "QPushButton{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 %1,stop:1 #1A1E26);"
            "color:white;font-weight:800;border:1px solid %1;border-radius:3px;padding:7px 12px;}"
            "QPushButton:hover{border-color:#FFFFFF;}").arg(color));
    };
    connect(platform, QOverload<int>::of(&QComboBox::currentIndexChanged), this, updatePlatformAccent);
    updatePlatformAccent();

    int* count = new int(0);
    auto* nam = new QNetworkAccessManager(this);
    auto* oauthServer = new QTcpServer(this);
    auto* oauthVerifier = new QString();
    auto* oauthState = new QString();
    auto* oauthPlatform = new QString();
    auto* oauthClientId = new QString();
    constexpr quint16 kOAuthPort = 18765;
    const QString redirectUri = QStringLiteral("http://127.0.0.1:%1/callback").arg(kOAuthPort);

    auto applyConnected = [=] {
        connDot->setObjectName(QStringLiteral("connDotConnected"));
        connLbl->setText(QStringLiteral("Connected"));
        connLbl->setStyleSheet(QStringLiteral("color:#22C55E; font-size:11px;"));
        connDot->style()->unpolish(connDot);
        connDot->style()->polish(connDot);
    };
    auto applyError = [=](const QString& err) {
        connDot->setObjectName(QStringLiteral("connDotError"));
        connLbl->setText(QStringLiteral("Error"));
        connLbl->setStyleSheet(QStringLiteral("color:#FF5A2C; font-size:11px;"));
        connDot->style()->unpolish(connDot);
        connDot->style()->polish(connDot);
        QMessageBox::warning(this, QStringLiteral("Chat"), err);
    };
    auto connectWithToken = [=](const QString& plat, const QString& token) {
        if (token.isEmpty()) {
            applyError(QStringLiteral("Empty OAuth token"));
            return;
        }
        chat->setTwitchClientId(clientId->text().trimmed());
        const QString secretId = SecretStore::makeTokenId(plat, QStringLiteral("default"));
        SecretStore::store(secretId, token);
        QString err;
        if (!chat->connectPlatform(plat, secretId, channel->text(), &err)) {
            applyError(err);
            return;
        }
        applyConnected();
    };
    auto exchangeCode = [=](const QString& code) {
        const QString endpoint = PlatformAdapters::tokenEndpoint(*oauthPlatform);
        if (endpoint.isEmpty()) {
            applyError(QStringLiteral("Token exchange not supported for this platform — paste a token instead."));
            return;
        }
        QNetworkRequest req{QUrl(endpoint)};
        req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/x-www-form-urlencoded"));
        const QByteArray body = PlatformAdapters::tokenExchangeBody(
            *oauthPlatform, *oauthClientId, code, redirectUri, *oauthVerifier);
        QNetworkReply* reply = nam->post(req, body);
        QPointer<ChatPage> self(this);
        connect(reply, &QNetworkReply::finished, this, [=] {
            reply->deleteLater();
            if (!self) return;
            const QByteArray raw = reply->readAll();
            if (reply->error() != QNetworkReply::NoError) {
                applyError(QStringLiteral("Token exchange failed: %1").arg(QString::fromUtf8(raw)));
                return;
            }
            const QJsonObject obj = PlatformAdapters::parseTokenResponse(*oauthPlatform, raw);
            const QString token = obj.value(QStringLiteral("access_token")).toString();
            if (token.isEmpty()) {
                applyError(QStringLiteral("No access_token in response"));
                return;
            }
            connectWithToken(*oauthPlatform, token);
        });
    };

    connect(oauthServer, &QTcpServer::newConnection, this, [=] {
        while (QTcpSocket* sock = oauthServer->nextPendingConnection()) {
            connect(sock, &QTcpSocket::readyRead, this, [=] {
                const QByteArray req = sock->readAll();
                const QString line = QString::fromUtf8(req.split('\n').value(0));
                QString path = line.section(QLatin1Char(' '), 1, 1);
                if (path.startsWith(QLatin1Char('/'))) {
                    const QUrl url(QStringLiteral("http://127.0.0.1") + path);
                    QUrlQuery q(url);
                    const QString state = q.queryItemValue(QStringLiteral("state"));
                    const QString code = q.queryItemValue(QStringLiteral("code"));
                    const QString err = q.queryItemValue(QStringLiteral("error"));
                    const QByteArray html =
                        "<html><body style='font-family:sans-serif;background:#12151C;color:#E0E2E8;"
                        "padding:40px;text-align:center'><h2>RailShotTV</h2>"
                        "<p>You can close this window and return to the app.</p></body></html>";
                    sock->write("HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n"
                                "Connection: close\r\nContent-Length: "
                                + QByteArray::number(html.size()) + "\r\n\r\n" + html);
                    sock->disconnectFromHost();
                    oauthServer->close();
                    if (!err.isEmpty()) {
                        applyError(QStringLiteral("OAuth error: %1").arg(err));
                        return;
                    }
                    if (!state.isEmpty() && state != *oauthState) {
                        applyError(QStringLiteral("OAuth state mismatch"));
                        return;
                    }
                    if (code.isEmpty()) {
                        applyError(QStringLiteral("OAuth callback missing code"));
                        return;
                    }
                    connLbl->setText(QStringLiteral("Exchanging token…"));
                    exchangeCode(code);
                }
            });
            connect(sock, &QTcpSocket::disconnected, sock, &QObject::deleteLater);
        }
    });

    connect(oauthBtn, &QPushButton::clicked, this, [=] {
        QSettings s(QStringLiteral("RailShotTV"), QStringLiteral("RailShotTV"));
        const QString cid = clientId->text().trimmed();
        s.setValue(QStringLiteral("chat/clientId"), cid);
        if (cid.isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("Chat"),
                                 QStringLiteral("Enter your OAuth client id first."));
            return;
        }
        *oauthClientId = cid;
        *oauthPlatform = platform->currentData().toString();
        *oauthVerifier = PlatformAdapters::generateCodeVerifier();
        *oauthState = PlatformAdapters::generateState();
        const QString challenge = PlatformAdapters::codeChallengeS256(*oauthVerifier);
        if (oauthServer->isListening())
            oauthServer->close();
        if (!oauthServer->listen(QHostAddress::LocalHost, kOAuthPort)) {
            QMessageBox::warning(this, QStringLiteral("Chat"),
                                 QStringLiteral("Could not bind localhost:%1 for OAuth callback.")
                                     .arg(kOAuthPort));
            return;
        }
        const QString url = PlatformAdapters::authorizeUrl(*oauthPlatform, cid, redirectUri,
                                                           challenge, *oauthState);
        if (url.isEmpty()) {
            oauthServer->close();
            QMessageBox::warning(this, QStringLiteral("Chat"), QStringLiteral("Unknown platform"));
            return;
        }
        connDot->setObjectName(QStringLiteral("connDotConnecting"));
        connLbl->setText(QStringLiteral("Waiting for browser…"));
        connLbl->setStyleSheet(QStringLiteral("color:#FBBF24; font-size:11px;"));
        connDot->style()->unpolish(connDot);
        connDot->style()->polish(connDot);
        QDesktopServices::openUrl(QUrl(url));
    });

    connect(connectBtn, &QPushButton::clicked, this, [=] {
        const QString plat = platform->currentData().toString();
        const QString secretId = SecretStore::makeTokenId(plat, QStringLiteral("default"));
        auto existing = SecretStore::load(secretId);
        if (existing && !existing->isEmpty()) {
            connDot->setObjectName(QStringLiteral("connDotConnecting"));
            connLbl->setText(QStringLiteral("Connecting…"));
            connLbl->setStyleSheet(QStringLiteral("color:#FBBF24; font-size:11px;"));
            connectWithToken(plat, *existing);
            return;
        }
        bool ok = false;
        const QString token = QInputDialog::getText(this, QStringLiteral("OAuth Token"),
                                                    QStringLiteral("Paste access token (or use Sign in with browser):"),
                                                    QLineEdit::Password, {}, &ok);
        if (!ok || token.isEmpty()) return;
        connDot->setObjectName(QStringLiteral("connDotConnecting"));
        connLbl->setText(QStringLiteral("Connecting…"));
        connLbl->setStyleSheet(QStringLiteral("color:#FBBF24; font-size:11px;"));
        connectWithToken(plat, token);
    });

    connect(chat, &ChatService::messageReceived, this, [=](const ChatMessage& m) {
        if (!filterAll->isChecked() && !filterPlatform->isEmpty() && m.platform != *filterPlatform)
            return;
        const bool alertish = m.user == QLatin1String("system")
                              || m.text.contains(QStringLiteral("subscribed"), Qt::CaseInsensitive)
                              || m.text.contains(QStringLiteral("raid"), Qt::CaseInsensitive)
                              || m.text.startsWith(QLatin1Char('!'));
        if (*muteAlertsOn && alertish)
            return;
        QString accent = QStringLiteral("#C8CAD0");
        if (m.platform == QLatin1String("twitch")) accent = QStringLiteral("#9146FF");
        else if (m.platform == QLatin1String("youtube")) accent = QStringLiteral("#FF0000");
        else if (m.platform == QLatin1String("facebook")) accent = QStringLiteral("#1877F2");
        auto* item = new QListWidgetItem(QStringLiteral("[%1] %2: %3").arg(m.platform, m.user, m.text));
        item->setData(Qt::UserRole, m.platform);
        item->setData(Qt::UserRole + 1, m.user);
        item->setForeground(QColor(accent));
        if (m.text.contains(QLatin1Char('@')))
            item->setBackground(QColor(QStringLiteral("#FBBF2420")));
        list->addItem(item);
        list->scrollToBottom();
        ++(*count);
        msgCount->setText(QStringLiteral("Messages  %1").arg(*count));
        recent->setText(QStringLiteral("%1 · %2").arg(m.user, m.text.left(40)));
        if (m.text.contains(QStringLiteral("!"), Qt::CaseInsensitive) || m.text.size() > 80) {
            highlights->insertItem(0, QStringLiteral("%1: %2").arg(m.user, m.text.left(48)));
        }
        activity->addItem(QStringLiteral("%1 · %2").arg(m.user, m.text.left(48)));
    });

    list->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(list, &QListWidget::customContextMenuRequested, this, [=](const QPoint& pos) {
        auto* item = list->itemAt(pos);
        if (!item) return;
        if (item->data(Qt::UserRole).toString() != QLatin1String("twitch")) return;
        const QString user = item->data(Qt::UserRole + 1).toString();
        if (user.isEmpty() || user == QLatin1String("system") || user == QLatin1String("you"))
            return;
        QMenu menu(list);
        auto* t10 = menu.addAction(QStringLiteral("Timeout %1 (10m)").arg(user));
        auto* t1h = menu.addAction(QStringLiteral("Timeout %1 (1h)").arg(user));
        auto* ban = menu.addAction(QStringLiteral("Ban %1").arg(user));
        QAction* chosen = menu.exec(list->mapToGlobal(pos));
        if (!chosen) return;
        chat->setTwitchClientId(clientId->text().trimmed());
        QString err;
        int sec = 0;
        if (chosen == t10) sec = 600;
        else if (chosen == t1h) sec = 3600;
        if (!chat->timeoutTwitchUser(user, sec, &err))
            QMessageBox::warning(this, QStringLiteral("Moderation"), err);
    });

    connect(chat, &ChatService::moderationResult, this, [=](bool ok, const QString& detail) {
        activity->addItem(ok ? QStringLiteral("✓ %1").arg(detail)
                             : QStringLiteral("✗ %1").arg(detail));
        activity->scrollToBottom();
    });

    connect(filterAll, &QCheckBox::toggled, this, [=](bool all) {
        *filterPlatform = all ? QString() : platform->currentData().toString();
        for (int i = 0; i < list->count(); ++i) {
            auto* item = list->item(i);
            const QString plat = item->data(Qt::UserRole).toString();
            item->setHidden(!all && !plat.isEmpty() && plat != *filterPlatform);
        }
    });
    connect(platform, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int) {
        if (!filterAll->isChecked()) {
            *filterPlatform = platform->currentData().toString();
            for (int i = 0; i < list->count(); ++i) {
                auto* item = list->item(i);
                const QString plat = item->data(Qt::UserRole).toString();
                item->setHidden(!plat.isEmpty() && plat != *filterPlatform);
            }
        }
    });

    connect(send, &QPushButton::clicked, this, [chat, platform, input] {
        chat->sendMessage(platform->currentData().toString(), input->text());
        input->clear();
    });
}

} // namespace railshot
