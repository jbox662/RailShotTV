#include <QtTest>
#include "audio/AudioMeter.h"

using namespace railshot;

class TestAudioMeter : public QObject {
    Q_OBJECT
private slots:
    void peaksAndRms();
};

void TestAudioMeter::peaksAndRms()
{
    AudioMeter m;
    float samples[] = {0.5f, -0.25f, 0.8f, 0.1f};
    m.process(samples, 2, 2);
    QVERIFY(m.peakL() >= 0.79f);
    QVERIFY(m.peakR() >= 0.24f);
    QVERIFY(m.rmsL() > 0.0f);
}

QTEST_MAIN(TestAudioMeter)
#include "test_audio_meter.moc"
