#include <QtTest>
#include "core/Project.h"

using namespace railshot;

class TestProject : public QObject {
    Q_OBJECT
private slots:
    void defaultsCreateScene();
    void addRemoveSource();
    void roundTripJson();
    void reorderScenes();
};

void TestProject::defaultsCreateScene()
{
    Project p;
    p.ensureDefaults();
    QVERIFY(!p.scenes.isEmpty());
    QVERIFY(!p.activeSceneId.isEmpty());
}

void TestProject::addRemoveSource()
{
    Project p;
    p.ensureDefaults();
    const QString sid = p.addSource(p.activeSceneId, SourceType::Camera, QStringLiteral("Cam"));
    QVERIFY(!sid.isEmpty());
    QCOMPARE(p.findScene(p.activeSceneId)->sources.size(), 1);
    QVERIFY(p.removeSource(p.activeSceneId, sid));
    QCOMPARE(p.findScene(p.activeSceneId)->sources.size(), 0);
}

void TestProject::roundTripJson()
{
    Project p;
    p.ensureDefaults();
    p.addSource(p.activeSceneId, SourceType::Text, QStringLiteral("Title"));
    const auto json = p.toJson();
    QString err;
    auto loaded = Project::fromJson(json, &err);
    QVERIFY2(loaded.has_value(), qPrintable(err));
    QCOMPARE(loaded->scenes.size(), p.scenes.size());
    QCOMPARE(loaded->scenes.first().sources.size(), 1);
}

void TestProject::reorderScenes()
{
    Project p;
    p.ensureDefaults();
    p.addScene(QStringLiteral("B"));
    QVERIFY(p.reorderScenes(0, 1));
    QCOMPARE(p.scenes.first().name, QStringLiteral("B"));
}

QTEST_MAIN(TestProject)
#include "test_project.moc"
