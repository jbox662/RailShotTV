#include <QtTest>
#include "core/JsonSchema.h"
#include "core/Types.h"
#include <QJsonArray>

using namespace railshot;

class TestJsonSchema : public QObject {
    Q_OBJECT
private slots:
    void rejectsMissingScenes();
    void acceptsValid();
};

void TestJsonSchema::rejectsMissingScenes()
{
    QJsonObject o{{QStringLiteral("schemaVersion"), 1}};
    QString err;
    QVERIFY(!JsonSchema::validateProject(o, &err));
    QVERIFY(!err.isEmpty());
}

void TestJsonSchema::acceptsValid()
{
    QJsonObject o{
        {QStringLiteral("schemaVersion"), kProjectSchemaVersion},
        {QStringLiteral("scenes"), QJsonArray{}},
    };
    QVERIFY(JsonSchema::validateProject(o));
}

QTEST_MAIN(TestJsonSchema)
#include "test_json_schema.moc"
