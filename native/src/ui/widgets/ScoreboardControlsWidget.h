#pragma once
#include <QWidget>

class QEvent;

namespace railshot {
class EngineController;

/// Live scoreboard dock: teams, scores, match clock — Sport/theme via Settings.
class ScoreboardControlsWidget : public QWidget {
    Q_OBJECT
public:
    explicit ScoreboardControlsWidget(EngineController* engine, QWidget* parent = nullptr);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    EngineController* m_engine = nullptr;
};

} // namespace railshot
