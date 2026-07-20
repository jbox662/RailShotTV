#pragma once
#include <QFrame>
#include <QVBoxLayout>

namespace railshot {
class EngineController;

class MultiCorderPanel : public QFrame {
    Q_OBJECT
public:
    explicit MultiCorderPanel(EngineController* engine, QWidget* parent = nullptr);
    void refresh();

signals:
    void closeRequested();

private:
    EngineController* m_engine = nullptr;
    QVBoxLayout* m_list = nullptr;
};

} // namespace railshot
