#pragma once
#include <QWidget>

namespace railshot {
class EngineController;

/// Compact live scoreboard controls for the dashboard dock.
class ScoreboardControlsWidget : public QWidget {
    Q_OBJECT
public:
    explicit ScoreboardControlsWidget(EngineController* engine, QWidget* parent = nullptr);

private:
    EngineController* m_engine = nullptr;
};

} // namespace railshot
