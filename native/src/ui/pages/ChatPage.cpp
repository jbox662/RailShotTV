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
#include <QSettings>
#include <QColor>
#include <QTabWidget>
#include <QCheckBox>
#include <QFrame>
#include <QScrollArea>
#include <QStyle>

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

    auto* oauthBtn = new QPushButton(QStringLiteral("Open OAuth"), left);
    oauthBtn->setObjectName(QStringLiteral("chromeBtn"));
    auto* connectBtn = new QPushButton(QStringLiteral("Connect"), left);
    connectBtn->setObjectName(QStringLiteral("chromeBtnViolet"));
    leftLay->addWidget(oauthBtn);
    leftLay->addWidget(connectBtn);

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
    auto makeToggle = [&](const QString& label, const QString& color) {
        auto* b = new QPushButton(label, modTab);
        b->setCheckable(true);
        b->setToolTip(QStringLiteral("Requires platform moderation API (local UI state only for now)"));
        b->setStyleSheet(QStringLiteral(
            "QPushButton{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #2A2D35,stop:1 #1A1D22);"
            "border:1px solid %1;color:%1;padding:8px;border-radius:3px;font-weight:800;}"
            "QPushButton:checked{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 %120,stop:1 #12151A);"
            "border:2px solid %1;color:#FFFFFF;}").arg(color));
        modLay->addWidget(b);
        return b;
    };
    makeToggle(QStringLiteral("Slow Mode (local)"), QStringLiteral("#FBBF24"));
    makeToggle(QStringLiteral("Subscribers Only (local)"), QStringLiteral("#A855F7"));
    makeToggle(QStringLiteral("Emote Only (local)"), QStringLiteral("#22D3EE"));
    makeToggle(QStringLiteral("Mute Alerts (local)"), QStringLiteral("#FF5A2C"));
    auto* clearChat = new QPushButton(QStringLiteral("Clear Local Chat"), modTab);
    clearChat->setObjectName(QStringLiteral("chromeBtnBrand"));
    clearChat->setToolTip(QStringLiteral("Clears the local feed only"));
    connect(clearChat, &QPushButton::clicked, this, [list] { list->clear(); });
    modLay->addWidget(clearChat);
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
    connect(oauthBtn, &QPushButton::clicked, this, [platform, clientId] {
        QSettings s(QStringLiteral("RailShotTV"), QStringLiteral("RailShotTV"));
        s.setValue(QStringLiteral("chat/clientId"), clientId->text().trimmed());
        const QString cid = clientId->text().trimmed().isEmpty()
                                ? QStringLiteral("YOUR_CLIENT_ID")
                                : clientId->text().trimmed();
        const QString url = PlatformAdapters::authorizeUrl(platform->currentData().toString(), cid,
                                                           QStringLiteral("http://localhost"));
        if (url.isEmpty()) {
            QMessageBox::warning(nullptr, QStringLiteral("Chat"), QStringLiteral("Unknown platform"));
            return;
        }
        QDesktopServices::openUrl(QUrl(url));
    });

    connect(connectBtn, &QPushButton::clicked, this, [=] {
        bool ok = false;
        const QString token = QInputDialog::getText(this, QStringLiteral("OAuth Token"),
                                                    QStringLiteral("Paste access token:"),
                                                    QLineEdit::Password, {}, &ok);
        if (!ok || token.isEmpty()) return;
        connDot->setObjectName(QStringLiteral("connDotConnecting"));
        connLbl->setText(QStringLiteral("Connecting…"));
        connLbl->setStyleSheet(QStringLiteral("color:#FBBF24; font-size:11px;"));
        const QString plat = platform->currentData().toString();
        const QString secretId = SecretStore::makeTokenId(plat, QStringLiteral("default"));
        SecretStore::store(secretId, token);
        QString err;
        if (!chat->connectPlatform(plat, secretId, channel->text(), &err)) {
            connDot->setObjectName(QStringLiteral("connDotError"));
            connLbl->setText(QStringLiteral("Error"));
            connLbl->setStyleSheet(QStringLiteral("color:#FF5A2C; font-size:11px;"));
            QMessageBox::warning(this, QStringLiteral("Chat"), err);
            return;
        }
        connDot->setObjectName(QStringLiteral("connDotConnected"));
        connLbl->setText(QStringLiteral("Connected"));
        connLbl->setStyleSheet(QStringLiteral("color:#22C55E; font-size:11px;"));
        connDot->style()->unpolish(connDot);
        connDot->style()->polish(connDot);
    });

    connect(chat, &ChatService::messageReceived, this, [=](const ChatMessage& m) {
        if (!filterAll->isChecked() && !filterPlatform->isEmpty() && m.platform != *filterPlatform)
            return;
        QString accent = QStringLiteral("#C8CAD0");
        if (m.platform == QLatin1String("twitch")) accent = QStringLiteral("#9146FF");
        else if (m.platform == QLatin1String("youtube")) accent = QStringLiteral("#FF0000");
        else if (m.platform == QLatin1String("facebook")) accent = QStringLiteral("#1877F2");
        auto* item = new QListWidgetItem(QStringLiteral("[%1] %2: %3").arg(m.platform, m.user, m.text));
        item->setData(Qt::UserRole, m.platform);
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
        activity->addItem(QStringLiteral("%1 joined conversation").arg(m.user));
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
