#include "ui/pages/ChatPage.h"
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

namespace railshot {

ChatPage::ChatPage(ChatService* chat, QWidget* parent)
    : QWidget(parent)
{
    auto* root = new QVBoxLayout(this);
    auto* top = new QHBoxLayout();
    auto* platform = new QComboBox(this);
    platform->addItems({QStringLiteral("twitch"), QStringLiteral("youtube"), QStringLiteral("facebook")});
    auto* channel = new QLineEdit(this);
    channel->setPlaceholderText(QStringLiteral("twitch channel"));
    auto* oauthBtn = new QPushButton(QStringLiteral("Open OAuth"), this);
    auto* connectBtn = new QPushButton(QStringLiteral("Connect"), this);
    top->addWidget(platform);
    top->addWidget(channel, 1);
    top->addWidget(oauthBtn);
    top->addWidget(connectBtn);
    root->addLayout(top);

    auto* hint = new QLabel(this);
    auto updateHint = [hint, platform, channel] {
        if (platform->currentText() == QLatin1String("twitch")) {
            hint->setText(QStringLiteral("Twitch · Open OAuth, then paste the access token and connect with channel login."));
            channel->setPlaceholderText(QStringLiteral("twitch channel"));
        } else if (platform->currentText() == QLatin1String("youtube")) {
            hint->setText(QStringLiteral("YouTube · Open OAuth, paste token, set liveChatId or videoId."));
            channel->setPlaceholderText(QStringLiteral("liveChatId or videoId"));
        } else {
            hint->setText(QStringLiteral("Facebook · Open OAuth / Page token, paste live video id."));
            channel->setPlaceholderText(QStringLiteral("live video id"));
        }
    };
    updateHint();
    connect(platform, &QComboBox::currentTextChanged, this, updateHint);
    hint->setStyleSheet(QStringLiteral("color:#606878;"));
    root->addWidget(hint);

    auto* clientId = new QLineEdit(this);
    clientId->setPlaceholderText(QStringLiteral("OAuth client id (optional — stored locally)"));
    QSettings s(QStringLiteral("RailShotTV"), QStringLiteral("RailShotTV"));
    clientId->setText(s.value(QStringLiteral("chat/clientId")).toString());
    root->addWidget(clientId);

    auto* list = new QListWidget(this);
    root->addWidget(list, 1);

    auto* sendRow = new QHBoxLayout();
    auto* input = new QLineEdit(this);
    auto* send = new QPushButton(QStringLiteral("Send"), this);
    sendRow->addWidget(input, 1);
    sendRow->addWidget(send);
    root->addLayout(sendRow);

    connect(oauthBtn, &QPushButton::clicked, this, [platform, clientId] {
        QSettings s(QStringLiteral("RailShotTV"), QStringLiteral("RailShotTV"));
        s.setValue(QStringLiteral("chat/clientId"), clientId->text().trimmed());
        const QString cid = clientId->text().trimmed().isEmpty()
                                ? QStringLiteral("YOUR_CLIENT_ID")
                                : clientId->text().trimmed();
        const QString url = PlatformAdapters::authorizeUrl(platform->currentText(), cid,
                                                           QStringLiteral("http://localhost"));
        if (url.isEmpty()) {
            QMessageBox::warning(nullptr, QStringLiteral("Chat"), QStringLiteral("Unknown platform"));
            return;
        }
        QDesktopServices::openUrl(QUrl(url));
    });

    connect(connectBtn, &QPushButton::clicked, this, [chat, platform, channel, this] {
        bool ok = false;
        const QString token = QInputDialog::getText(this, QStringLiteral("OAuth Token"),
                                                    QStringLiteral("Paste access token:"),
                                                    QLineEdit::Password, {}, &ok);
        if (!ok || token.isEmpty()) return;
        const QString secretId = SecretStore::makeTokenId(platform->currentText(), QStringLiteral("default"));
        SecretStore::store(secretId, token);
        QString err;
        if (!chat->connectPlatform(platform->currentText(), secretId, channel->text(), &err))
            QMessageBox::warning(this, QStringLiteral("Chat"), err);
    });

    connect(chat, &ChatService::messageReceived, this, [list](const ChatMessage& m) {
        auto* item = new QListWidgetItem(QStringLiteral("[%1] %2: %3").arg(m.platform, m.user, m.text));
        if (m.text.contains(QLatin1Char('@')))
            item->setForeground(QColor(QStringLiteral("#F59E0B")));
        list->addItem(item);
        list->scrollToBottom();
    });

    connect(send, &QPushButton::clicked, this, [chat, platform, input] {
        chat->sendMessage(platform->currentText(), input->text());
        input->clear();
    });
}

} // namespace railshot
