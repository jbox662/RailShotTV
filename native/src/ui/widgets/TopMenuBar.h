#pragma once
#include <QWidget>
class QLabel;
class QPushButton;
class QMenu;

namespace railshot {
class EngineController;

class TopMenuBar : public QWidget {
    Q_OBJECT
public:
    explicit TopMenuBar(EngineController* engine, QWidget* parent = nullptr);
    void setBasicMode(bool on);
    bool basicMode() const { return m_basicMode; }
    QMenu* docksMenu() const { return m_docksMenu; }

signals:
    void openProject();
    void saveProject();
    void saveProjectAs();
    void openLastProject();
    void newProject();
    void openSettings();
    void openAdvAudio();
    /// program=true → Program feed; fullscreen=true → fullscreen projector.
    void openProjector(bool program, bool fullscreen);
    void toggleShortcuts();
    void basicModeChanged(bool on);
    /// Fired just before the Docks menu opens so Dashboard can refill toggles.
    void docksMenuAboutToShow(QMenu* menu);

private:
    EngineController* m_engine = nullptr;
    QLabel* m_status = nullptr;
    QPushButton* m_fullscreen = nullptr;
    QPushButton* m_pauseInputs = nullptr;
    QPushButton* m_basic = nullptr;
    QPushButton* m_help = nullptr;
    QMenu* m_docksMenu = nullptr;
    bool m_fullscreenOn = false;
    bool m_inputsPaused = false;
    bool m_basicMode = false;
};

} // namespace railshot
