#pragma once
#include <QWidget>
#include <QFrame>
#include <QByteArray>
#include <QVector>

class QResizeEvent;
class QEvent;
class QHideEvent;
class QMainWindow;
class QDockWidget;
class QMenu;

namespace railshot {
class EngineController;
class PreviewWidget;
class AudioMixerWidget;
class SourcePropertiesWidget;
class BottomToolbar;
class InputTilesWidget;
class MultiCorderPanel;
class PlayListPanel;
class ScoreboardControlsWidget;
class SceneListWidget;
class SourceContextToolbar;
class TransitionPanel;
class ObsStatsDock;
class ExtraBrowserPanel;

class DashboardPage : public QWidget {
    Q_OBJECT
public:
    explicit DashboardPage(EngineController* engine, QWidget* parent = nullptr);
    ~DashboardPage() override;
    void setBasicMode(bool on);
    void resetDockLayout();
    /// OBS-style Docks menu: toggles + Add Browser Panel + Reset.
    void populateDocksMenu(QMenu* menu);

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
    void setStudioMode(bool enabled);

    QDockWidget* makeDock(const QString& title, const QString& objectName, QWidget* content,
                          const QString& accent);
    void applyDefaultDockLayout();
    void saveDockState();
    void restoreDockState();
    void scheduleSaveDockState();
    void restoreExtraBrowserPanels();
    void persistExtraBrowserPanels();
    void addExtraBrowserPanel(const QString& title = {}, const QString& url = {});
    QDockWidget* createExtraBrowserDock(const QString& panelId, const QString& title,
                                        const QString& url);

    EngineController* m_engine = nullptr;
    PreviewWidget* m_preview = nullptr;
    PreviewWidget* m_program = nullptr;
    TransitionPanel* m_transitions = nullptr;
    SourceContextToolbar* m_contextBar = nullptr;
    QWidget* m_previewColumn = nullptr;
    AudioMixerWidget* m_mixer = nullptr;
    SourcePropertiesWidget* m_props = nullptr;
    BottomToolbar* m_toolbar = nullptr;
    InputTilesWidget* m_tiles = nullptr;
    SceneListWidget* m_sceneList = nullptr;
    QFrame* m_drawerBackdrop = nullptr;
    MultiCorderPanel* m_multi = nullptr;
    PlayListPanel* m_playlist = nullptr;

    QMainWindow* m_dockHost = nullptr;
    QDockWidget* m_scenesDock = nullptr;
    QDockWidget* m_sourcesDock = nullptr;
    QDockWidget* m_mixerDock = nullptr;
    QDockWidget* m_scoreboardDock = nullptr;
    QDockWidget* m_statsDock = nullptr;
    QVector<QDockWidget*> m_browserDocks;
    ScoreboardControlsWidget* m_scoreboardControls = nullptr;
    ObsStatsDock* m_statsWidget = nullptr;
    QByteArray m_defaultDockState;
    bool m_dockStateReady = false;
};

} // namespace railshot
