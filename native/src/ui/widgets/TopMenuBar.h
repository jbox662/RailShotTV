#pragma once
#include <QWidget>
class QLabel;
class QPushButton;

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
    void toggleShortcuts();
private:
    EngineController* m_engine = nullptr;
    QLabel* m_status = nullptr;
    QPushButton* m_fullscreen = nullptr;
    QPushButton* m_pauseInputs = nullptr;
    QPushButton* m_help = nullptr;
    bool m_fullscreenOn = false;
    bool m_inputsPaused = false;
};

} // namespace railshot
