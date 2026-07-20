#include <QtTest>
#include "encoding/MfH264Encoder.h"
#include "encoding/EncoderFactory.h"
#include "recording/MkvRecorder.h"
#include <QDir>
#include <QFile>
#include <QImage>

using namespace railshot;

class TestMfEncoder : public QObject {
    Q_OBJECT
private slots:
    void opensAndEncodesOrFallsBack();
};

void TestMfEncoder::opensAndEncodesOrFallsBack()
{
    OutputProfile profile;
    profile.width = 320;
    profile.height = 180;
    profile.fps = 30.0;
    profile.videoBitrateKbps = 800;
    profile.audioBitrateKbps = 96;
    profile.keyframeIntervalSec = 1;
    profile.encoderPreference = QStringLiteral("auto");

    QString selected;
    auto enc = EncoderFactory::createVideo(profile, &selected);
    QVERIFY2(enc != nullptr, "No video encoder available");
    QVERIFY(!selected.isEmpty());

    QImage img(profile.width, profile.height, QImage::Format_ARGB32);
    img.fill(Qt::blue);

    bool gotPacket = false;
    for (int i = 0; i < 45; ++i) {
        EncodedPacket pkt;
        if (enc->encodeImage(img, i * 33333, pkt) && !pkt.data.isEmpty()) {
            gotPacket = true;
            break;
        }
    }
    QVERIFY2(gotPacket, "Encoder produced no packets");

    // If MF path is active, extradata should eventually exist for muxing.
    // Software openh264 also provides extradata with GLOBAL_HEADER.
    QVERIFY2(!enc->extradata().isEmpty() || selected.contains(QStringLiteral("Software")),
             "Missing codec extradata");

    enc->close();
}

QTEST_MAIN(TestMfEncoder)
#include "test_mf_encoder.moc"
