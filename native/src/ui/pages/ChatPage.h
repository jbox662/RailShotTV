#pragma once
#include <QWidget>

namespace railshot {

class ChatService;

class ChatPage : public QWidget {
    Q_OBJECT
public:
    explicit ChatPage(ChatService* chat, QWidget* parent = nullptr);
};

} // namespace railshot
