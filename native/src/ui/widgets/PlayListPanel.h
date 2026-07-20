#pragma once
#include <QFrame>
#include <QListWidget>

namespace railshot {
class EngineController;

class PlayListPanel : public QFrame {
    Q_OBJECT
public:
    explicit PlayListPanel(EngineController* engine, QWidget* parent = nullptr);
    void refresh();

signals:
    void closeRequested();

private:
    EngineController* m_engine = nullptr;
    QListWidget* m_list = nullptr;
};

} // namespace railshot
