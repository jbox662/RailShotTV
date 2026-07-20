#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <memory>

class QKeyEvent;
class QCloseEvent;
class QFrame;

namespace railshot {

class EngineController;
class SidebarRail;
class TopMenuBar;
class HotkeyDispatcher;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(EngineController* engine, QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private:
    void navigateTo(const QString& pageId);
    void openProjectDialog();
    void saveProjectDialog();
    void saveProjectAsDialog();
    void openLastProject();
    void updateLiveChrome(bool streaming);

    EngineController* m_engine = nullptr;
    SidebarRail* m_sidebar = nullptr;
    TopMenuBar* m_top = nullptr;
    QStackedWidget* m_stack = nullptr;
    HotkeyDispatcher* m_hotkeys = nullptr;
    QFrame* m_liveTopBorder = nullptr;
};

} // namespace railshot
