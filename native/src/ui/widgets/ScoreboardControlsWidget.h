#pragma once
#include <QWidget>

namespace railshot {
class EngineController;

/// Live scoreboard dock: teams, scores, match clock — Sport/theme via Settings.
class ScoreboardControlsWidget : public QWidget {
    Q_OBJECT
public:
    explicit ScoreboardControlsWidget(EngineController* engine, QWidget* parent = nullptr);

private:
    EngineController* m_engine = nullptr;
};

} // namespace railshot
