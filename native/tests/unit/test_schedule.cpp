#include <QtTest>
#include "schedule/ScheduleModel.h"
#include "core/Types.h"

using namespace railshot;

class TestSchedule : public QObject {
    Q_OBJECT
private slots:
    void nextUpPicksSoonest();
};

void TestSchedule::nextUpPicksSoonest()
{
    ScheduleModel m;
    ScheduledEvent a;
    a.id = newId();
    a.title = QStringLiteral("Later");
    a.start = QDateTime::currentDateTimeUtc().addSecs(7200);
    ScheduledEvent b;
    b.id = newId();
    b.title = QStringLiteral("Soon");
    b.start = QDateTime::currentDateTimeUtc().addSecs(600);
    m.addEvent(a);
    m.addEvent(b);
    auto* next = m.nextUp();
    QVERIFY(next);
    QCOMPARE(next->title, QStringLiteral("Soon"));
}

QTEST_MAIN(TestSchedule)
#include "test_schedule.moc"
