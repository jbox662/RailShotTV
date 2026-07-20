#include "ChatPage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QPushButton>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLineEdit>

static QLabel *uiLbl(const QString &t, const QString &c,
                     int sz = 11, bool bold = false, QWidget *p = nullptr)
{
    QLabel *l = new QLabel(t, p);
    l->setStyleSheet(QString("font-family:'DM Sans','Segoe UI',sans-serif;"
                             "font-size:%1px;color:%2;font-weight:%3;")
                         .arg(sz).arg(c).arg(bold ? "600" : "400"));
    return l;
}
static QLabel *bebasLbl(const QString &t, const QString &c, int sz = 18, QWidget *p = nullptr)
{
    QLabel *l = new QLabel(t, p);
    l->setStyleSheet(QString("font-family:'Bebas Neue','Impact',sans-serif;"
                             "font-size:%1px;color:%2;letter-spacing:1px;").arg(sz).arg(c));
    return l;
}
static QFrame *hSep(QWidget *p = nullptr)
{
    QFrame *f = new QFrame(p);
    f->setFrameShape(QFrame::HLine);
    f->setFixedHeight(1);
    f->setStyleSheet("background:#2A3350;border:none;");
    return f;
}

ChatPage::ChatPage(QWidget *parent) : QWidget(parent)
{
    setObjectName("ChatPage");
    setupUi();
}

void ChatPage::setupUi()
{
    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    QWidget *topBar = new QWidget(this);
    topBar->setObjectName("TopBar");
    topBar->setFixedHeight(46);
    buildTopBar(topBar);
    root->addWidget(topBar);

    QWidget *body = new QWidget(this);
    QHBoxLayout *bodyLayout = new QHBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);
    root->addWidget(body, 1);

    QWidget *chatPanel = new QWidget(body);
    chatPanel->setFixedWidth(340);
    buildChatPanel(chatPanel);
    bodyLayout->addWidget(chatPanel);

    QWidget *activityPanel = new QWidget(body);
    buildActivityPanel(activityPanel);
    bodyLayout->addWidget(activityPanel, 1);
}

void ChatPage::buildTopBar(QWidget *bar)
{
    QHBoxLayout *l = new QHBoxLayout(bar);
    l->setContentsMargins(16, 0, 16, 0);
    l->setSpacing(12);
    l->addWidget(bebasLbl("RAILSHOT", "#F8F8FF", 18, bar));
    l->addWidget(bebasLbl(" TV", "#FF5A2C", 18, bar));
    QFrame *sep = new QFrame(bar);
    sep->setFrameShape(QFrame::VLine);
    sep->setFixedWidth(1);
    sep->setStyleSheet("background:#2A3350;border:none;");
    l->addWidget(sep);
    l->addWidget(uiLbl("CHAT & AUDIENCE", "#8892A4", 11, true, bar));
    l->addStretch();
    l->addWidget(uiLbl("● LIVE", "#FF5A2C", 11, true, bar));
    l->addWidget(uiLbl("2,852 viewers", "#A0A0B8", 11, false, bar));
}

void ChatPage::buildChatPanel(QWidget *panel)
{
    panel->setStyleSheet("background:#1A2035;border-right:1px solid #2A3350;");
    QVBoxLayout *vl = new QVBoxLayout(panel);
    vl->setContentsMargins(0, 0, 0, 0);
    vl->setSpacing(0);

    // Header
    QWidget *header = new QWidget(panel);
    header->setFixedHeight(28);
    header->setStyleSheet("background:#1E2640;border-bottom:2px solid #A855F7;");
    QHBoxLayout *hl = new QHBoxLayout(header);
    hl->setContentsMargins(12, 0, 12, 0);
    hl->addWidget(uiLbl("LIVE CHAT", "#A0A0B8", 11, true, header));
    hl->addStretch();
    // Platform filter tabs
    for (const QString &t : {"All","YT","TW","FB"}) {
        QPushButton *btn = new QPushButton(t, header);
        btn->setObjectName(t == "All" ? "FilterBtnActive" : "FilterBtn");
        btn->setFixedSize(30, 20);
        hl->addWidget(btn);
    }
    vl->addWidget(header);

    // Chat list
    m_chatList = new QListWidget(panel);
    m_chatList->setObjectName("ChatList");
    m_chatList->setSpacing(2);
    m_chatList->setWordWrap(true);
    // Messages populated at runtime from platform WebSocket/IRC connections
    vl->addWidget(m_chatList, 1);

    vl->addWidget(hSep(panel));

    // Message input
    QWidget *inputArea = new QWidget(panel);
    inputArea->setFixedHeight(52);
    inputArea->setStyleSheet("background:#1E2640;");
    QHBoxLayout *il = new QHBoxLayout(inputArea);
    il->setContentsMargins(8, 8, 8, 8);
    il->setSpacing(6);
    m_messageInput = new QLineEdit(inputArea);
    m_messageInput->setObjectName("ChatInput");
    m_messageInput->setPlaceholderText("Send a message...");
    il->addWidget(m_messageInput, 1);
    QPushButton *sendBtn = new QPushButton("Send", inputArea);
    sendBtn->setObjectName("SendBtn");
    sendBtn->setFixedHeight(32);
    il->addWidget(sendBtn);
    vl->addWidget(inputArea);
}

void ChatPage::buildActivityPanel(QWidget *panel)
{
    panel->setStyleSheet("background:#161B2E;");
    QVBoxLayout *vl = new QVBoxLayout(panel);
    vl->setContentsMargins(0, 0, 0, 0);
    vl->setSpacing(0);

    QWidget *header = new QWidget(panel);
    header->setFixedHeight(28);
    header->setStyleSheet("background:#1A2035;border-bottom:2px solid #22D3EE;");
    QHBoxLayout *hl = new QHBoxLayout(header);
    hl->setContentsMargins(12, 0, 12, 0);
    hl->addWidget(uiLbl("ACTIVITY FEED", "#A0A0B8", 11, true, header));
    hl->addStretch();
    hl->addWidget(uiLbl("Donations · Subs · Raids", "#50506A", 10, false, header));
    vl->addWidget(header);

    m_activityList = new QListWidget(panel);
    m_activityList->setObjectName("ActivityList");
    m_activityList->setSpacing(4);
    m_activityList->setWordWrap(true);
    // Activity events populated at runtime from platform webhooks/APIs
    vl->addWidget(m_activityList, 1);
}
