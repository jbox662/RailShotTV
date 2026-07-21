#include "scoreboard/ScoreboardModel.h"

namespace railshot {

QJsonObject ScoreboardState::toJson() const
{
    return QJsonObject{
        {QStringLiteral("sport"), sport},
        {QStringLiteral("playerA"), playerA},
        {QStringLiteral("playerB"), playerB},
        {QStringLiteral("scoreA"), scoreA},
        {QStringLiteral("scoreB"), scoreB},
        {QStringLiteral("raceTo"), raceTo},
        {QStringLiteral("layout"), layout},
        {QStringLiteral("theme"), theme},
        {QStringLiteral("colorA"), colorA},
        {QStringLiteral("colorB"), colorB},
        {QStringLiteral("textColor"), textColor},
        {QStringLiteral("bgColor"), bgColor},
        {QStringLiteral("clockRunning"), clockRunning},
        {QStringLiteral("clockSeconds"), clockSeconds},
    };
}

ScoreboardState ScoreboardState::fromJson(const QJsonObject& o)
{
    ScoreboardState s;
    s.sport = o.value(QStringLiteral("sport")).toString(s.sport);
    s.playerA = o.value(QStringLiteral("playerA")).toString(s.playerA);
    s.playerB = o.value(QStringLiteral("playerB")).toString(s.playerB);
    s.scoreA = o.value(QStringLiteral("scoreA")).toInt();
    s.scoreB = o.value(QStringLiteral("scoreB")).toInt();
    s.raceTo = o.value(QStringLiteral("raceTo")).toInt(7);
    s.layout = o.value(QStringLiteral("layout")).toString(s.layout);
    s.theme = o.value(QStringLiteral("theme")).toString(s.theme);
    s.colorA = o.value(QStringLiteral("colorA")).toString(s.colorA);
    s.colorB = o.value(QStringLiteral("colorB")).toString(s.colorB);
    s.textColor = o.value(QStringLiteral("textColor")).toString();
    s.bgColor = o.value(QStringLiteral("bgColor")).toString();
    s.clockRunning = o.value(QStringLiteral("clockRunning")).toBool();
    s.clockSeconds = o.value(QStringLiteral("clockSeconds")).toInt();
    return s;
}

ScoreboardModel::ScoreboardModel(QObject* parent)
    : QObject(parent)
{
}

void ScoreboardModel::setState(const ScoreboardState& s)
{
    m_state = s;
    emit changed(m_state);
}

void ScoreboardModel::incrementA(int delta)
{
    m_state.scoreA = qMax(0, m_state.scoreA + delta);
    emit changed(m_state);
}

void ScoreboardModel::incrementB(int delta)
{
    m_state.scoreB = qMax(0, m_state.scoreB + delta);
    emit changed(m_state);
}

void ScoreboardModel::resetScores()
{
    m_state.scoreA = 0;
    m_state.scoreB = 0;
    emit changed(m_state);
}

void ScoreboardModel::startClock() { m_state.clockRunning = true; emit changed(m_state); }
void ScoreboardModel::stopClock() { m_state.clockRunning = false; emit changed(m_state); }

void ScoreboardModel::tickClock()
{
    if (!m_state.clockRunning) return;
    m_state.clockSeconds++;
    emit changed(m_state);
}

} // namespace railshot
