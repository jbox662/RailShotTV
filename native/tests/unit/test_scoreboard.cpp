#include <QtTest>
#include "scoreboard/ScoreboardModel.h"

using namespace railshot;

class TestScoreboard : public QObject {
    Q_OBJECT
private slots:
    void increments();
    void reset();
};

void TestScoreboard::increments()
{
    ScoreboardModel m;
    m.incrementA(2);
    m.incrementB(1);
    QCOMPARE(m.state().scoreA, 2);
    QCOMPARE(m.state().scoreB, 1);
}

void TestScoreboard::reset()
{
    ScoreboardModel m;
    m.incrementA(5);
    m.resetScores();
    QCOMPARE(m.state().scoreA, 0);
    QCOMPARE(m.state().scoreB, 0);
}

QTEST_MAIN(TestScoreboard)
#include "test_scoreboard.moc"
