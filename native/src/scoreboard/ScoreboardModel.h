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
    bool clockRunning = false;
    int clockSeconds = 0;

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
