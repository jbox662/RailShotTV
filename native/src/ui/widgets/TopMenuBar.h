#pragma once
#include <QWidget>
class QLabel;

namespace railshot {
class EngineController;

class TopMenuBar : public QWidget {
    Q_OBJECT
public:
    explicit TopMenuBar(EngineController* engine, QWidget* parent = nullptr);
signals:
    void openProject();
    void saveProject();
    void newProject();
    void openSettings();
private:
    EngineController* m_engine = nullptr;
    QLabel* m_status = nullptr;
};

} // namespace railshot
