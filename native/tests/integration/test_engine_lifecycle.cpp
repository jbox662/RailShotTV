#include <QtTest>
#include "core/Project.h"
#include "core/SceneGraph.h"
#include "core/JsonSchema.h"
#include <QDir>
#include <QFile>

using namespace railshot;

/// Lightweight lifecycle test that does not require a GPU/Qt GUI session.
class TestEngineLifecycle : public QObject {
    Q_OBJECT
private slots:
    void sceneGraphMutations();
    void projectAutosaveRoundTrip();
};

void TestEngineLifecycle::sceneGraphMutations()
{
    SceneGraph g;
    g.mutate([](Project& p) {
        p.addScene(QStringLiteral("Cameras"));
        p.addSource(p.activeSceneId, SourceType::Camera, QStringLiteral("Wide"));
    });
    const auto snap = g.snapshot();
    QCOMPARE(snap.scenes.size(), 2); // default + Cameras
    QVERIFY(!snap.previewSceneId.isEmpty());
}

void TestEngineLifecycle::projectAutosaveRoundTrip()
{
    Project p;
    p.ensureDefaults();
    p.name = QStringLiteral("Soak");
    const QString path = QDir::temp().filePath(QStringLiteral("railshot-test-project.json"));
    QString err;
    QVERIFY2(p.saveToFile(path, &err), qPrintable(err));
    auto loaded = Project::loadFromFile(path, &err);
    QVERIFY2(loaded.has_value(), qPrintable(err));
    QCOMPARE(loaded->name, QStringLiteral("Soak"));
    QFile::remove(path);
    QFile::remove(path + QStringLiteral(".bak"));
}

QTEST_MAIN(TestEngineLifecycle)
#include "test_engine_lifecycle.moc"
