#pragma once
#include <QWidget>

namespace railshot {
class EngineController;
class PreviewWidget;
class TransitionPanel;
class SceneListWidget;
class InputTilesWidget;
class AudioMixerWidget;
class BottomToolbar;

class DashboardPage : public QWidget {
    Q_OBJECT
public:
    explicit DashboardPage(EngineController* engine, QWidget* parent = nullptr);
private:
    EngineController* m_engine = nullptr;
    PreviewWidget* m_preview = nullptr;
    PreviewWidget* m_program = nullptr;
};

} // namespace railshot
