#pragma once
#include <QWidget>
#include "scoreboard/ScoreboardModel.h"

namespace railshot {

class EngineController;

class ScoreboardPage : public QWidget {
    Q_OBJECT
public:
    explicit ScoreboardPage(EngineController* engine, QWidget* parent = nullptr);
};

} // namespace railshot
