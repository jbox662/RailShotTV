#pragma once

#include "core/Types.h"
#include <QObject>
#include <QJsonObject>
#include <QString>

namespace railshot {

/// True for cue-sport boards (Pool sport; 8/9/10-ball are games, not sports).
inline bool isPoolSport(const QString& sport)
{
    return sport == QLatin1String("8ball") || sport == QLatin1String("pool")
           || sport == QLatin1String("9ball") || sport == QLatin1String("10ball")
           || sport == QLatin1String("snooker")
           || sport == QLatin1String("straight") || sport == QLatin1String("onepocket");
}

/// Object balls used by the selected pool game (cue ball not shown on rack).
inline int poolObjectBallCount(const QString& sport)
{
    if (sport == QLatin1String("9ball"))
        return 9;
    if (sport == QLatin1String("10ball"))
        return 10;
    // 8-ball, straight pool, one-pocket, banks, legacy "pool", snooker approx
    return 15;
}

/// True when the active board theme/layout draws a pocketed-ball rack strip.
/// Dock ball controls should only appear when this is true.
inline bool scoreboardShowsBallRack(const QString& sport, const QString& theme, const QString& layout)
{
    if (!isPoolSport(sport))
        return false;
    if (layout == QLatin1String("compact") || layout == QLatin1String("center"))
        return false;
    // Mosconi (broadcast/gold) and Felt (railshot/neon) draw racks; Clean/Slant do not.
    return theme == QLatin1String("broadcast") || theme == QLatin1String("gold")
           || theme == QLatin1String("railshot") || theme == QLatin1String("neon");
}

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
    /// Bitmask of pocketed balls 1–15 (bit 0 = ball 1).
    int pocketedMask = 0;

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

inline bool scoreboardShowsBallRack(const ScoreboardState& st)
{
    return scoreboardShowsBallRack(st.sport, st.theme, st.layout);
}

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
