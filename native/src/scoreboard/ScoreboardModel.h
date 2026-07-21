#pragma once

#include "core/Types.h"
#include <QObject>
#include <QJsonObject>

namespace railshot {

struct ScoreboardState {
    QString sport = QStringLiteral("8ball");
    QString playerA = QStringLiteral("Player A");
    QString playerB = QStringLiteral("Player B");
    int scoreA = 0;
    int scoreB = 0;
    int raceTo = 7;
    QString layout = QStringLiteral("standard");
    QString theme = QStringLiteral("railshot");
    QString colorA = QStringLiteral("#FF5A2C");
    QString colorB = QStringLiteral("#4F9EFF");
    /// Optional overrides (empty = use theme defaults).
    QString textColor;
    QString bgColor;
    bool clockRunning = false;
    int clockSeconds = 0;

    /// Billiards: 0 = none, 1 = player A at table, 2 = player B.
    int activeSide = 1;

    /// Baseball state
    int balls = 0;
    int strikes = 0;
    int outs = 0;
    int inning = 1;
    bool topHalf = true;
    bool onFirst = false;
    bool onSecond = false;
    bool onThird = false;

    /// Basketball / soccer period (1-based). Tennis uses scoreA/B as games.
    int period = 1;

    QJsonObject toJson() const;
    static ScoreboardState fromJson(const QJsonObject& o);
};

class ScoreboardModel : public QObject {
    Q_OBJECT
public:
    explicit ScoreboardModel(QObject* parent = nullptr);
    ScoreboardState state() const { return m_state; }
    void setState(const ScoreboardState& s);
    void incrementA(int delta = 1);
    void incrementB(int delta = 1);
    void resetScores();
    void startClock();
    void stopClock();
    void tickClock();

signals:
    void changed(const ScoreboardState& state);

private:
    ScoreboardState m_state;
};

} // namespace railshot
