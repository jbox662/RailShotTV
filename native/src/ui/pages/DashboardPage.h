#pragma once
#include <QWidget>
#include <QFrame>

class QResizeEvent;
class QEvent;

namespace railshot {
class EngineController;
class PreviewWidget;
class AudioMixerWidget;
class SourcePropertiesWidget;
class BottomToolbar;
class InputTilesWidget;

class DashboardPage : public QWidget {
    Q_OBJECT
public:
    explicit DashboardPage(EngineController* engine, QWidget* parent = nullptr);

signals:
    void openSceneEditorRequested();

protected:
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void setMixerOpen(bool open);
    void openDrawer();
    void closeDrawer();
    void layoutDrawer();

    EngineController* m_engine = nullptr;
    PreviewWidget* m_preview = nullptr;
    PreviewWidget* m_program = nullptr;
    AudioMixerWidget* m_mixer = nullptr;
    SourcePropertiesWidget* m_props = nullptr;
    BottomToolbar* m_toolbar = nullptr;
    InputTilesWidget* m_tiles = nullptr;
    QFrame* m_drawerBackdrop = nullptr;
    bool m_mixerOpen = false;
};

} // namespace railshot
