#pragma once
#include <QDialog>

namespace railshot {
class EngineController;

/// Sport, layout, theme, and other scoreboard presentation options.
class ScoreboardSettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit ScoreboardSettingsDialog(EngineController* engine, QWidget* parent = nullptr);

private:
    EngineController* m_engine = nullptr;
};

} // namespace railshot
