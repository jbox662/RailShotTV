#pragma once
#include <QWidget>
#include <QPushButton>

namespace railshot {
class EngineController;

class BottomToolbar : public QWidget {
    Q_OBJECT
public:
    explicit BottomToolbar(EngineController* engine, QWidget* parent = nullptr);
signals:
    void addInputRequested();
    void goLiveRequested();
private:
    EngineController* m_engine = nullptr;
    QPushButton* m_streamBtn = nullptr;
    QPushButton* m_recordBtn = nullptr;
};

} // namespace railshot
