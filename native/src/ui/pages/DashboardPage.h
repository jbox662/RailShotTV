#pragma once
#include <QWidget>
#include <QFrame>
#include <QByteArray>

class QResizeEvent;
class QEvent;
class QHideEvent;
class QMainWindow;
class QDockWidget;

namespace railshot {
class EngineController;
class PreviewWidget;
class AudioMixerWidget;
class SourcePropertiesWidget;
class BottomToolbar;
class InputTilesWidget;
class MultiCorderPanel;
class PlayListPanel;

class DashboardPage : public QWidget {
    Q_OBJECT
public:
    explicit DashboardPage(EngineController* engine, QWidget* parent = nullptr);
    ~DashboardPage() override;
    void setBasicMode(bool on);
    void resetDockLayout();

signals:
    void openSceneEditorRequested();

protected:
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private:
    void openDrawer();
    void closeDrawer();
    void layoutDrawer();
    void setMultiCorderOpen(bool open);
    void setPlayListOpen(bool open);
    void layoutSidePanels();

    QDockWidget* makeDock(const QString& title, const QString& objectName, QWidget* content,
                          const QString& accent);
    void applyDefaultDockLayout();
    void saveDockState();
    void restoreDockState();
    void scheduleSaveDockState();

    EngineController* m_engine = nullptr;
    PreviewWidget* m_preview = nullptr;
    PreviewWidget* m_program = nullptr;
    AudioMixerWidget* m_mixer = nullptr;
    SourcePropertiesWidget* m_props = nullptr;
    BottomToolbar* m_toolbar = nullptr;
    InputTilesWidget* m_tiles = nullptr;
    QFrame* m_drawerBackdrop = nullptr;
    MultiCorderPanel* m_multi = nullptr;
    PlayListPanel* m_playlist = nullptr;

    QMainWindow* m_dockHost = nullptr;
    QDockWidget* m_scenesDock = nullptr;
    QDockWidget* m_sourcesDock = nullptr;
    QDockWidget* m_mixerDock = nullptr;
    QByteArray m_defaultDockState;
    bool m_dockStateReady = false;
};

} // namespace railshot
