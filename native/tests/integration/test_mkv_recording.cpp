#include <QtTest>
#include "encoding/SoftwareEncoder.h"
#include "encoding/AacEncoder.h"
#include "recording/MkvRecorder.h"
#include <QDir>
#include <QFile>
#include <QImage>
#include <cmath>

using namespace railshot;

class TestMkvRecording : public QObject {
    Q_OBJECT
private slots:
    void encodesPlayableMkv();
};

void TestMkvRecording::encodesPlayableMkv()
{
#if !RAILSHOT_HAS_FFMPEG
    QSKIP("FFmpeg not linked");
#endif
    OutputProfile profile;
    profile.width = 320;
    profile.height = 180;
    profile.fps = 30.0;
    profile.videoBitrateKbps = 500;
    profile.audioBitrateKbps = 96;
    profile.keyframeIntervalSec = 1;
    profile.encoderPreference = QStringLiteral("software");

    SoftwareEncoder video;
    AacEncoder audio;
    QString err;
    QVERIFY2(video.open(profile, &err), qPrintable(err));
    QVERIFY2(audio.open(profile, &err), qPrintable(err));

    const QString path = QDir::temp().filePath(QStringLiteral("railshot-smoke.mkv"));
    QFile::remove(path);

    MkvRecorder recorder;
    QVERIFY2(recorder.open(path, profile, video.extradata(), audio.extradata(),
                           audio.sampleRate(), audio.channels(), &err),
             qPrintable(err));

    // ~1 second of video + audio
    for (int i = 0; i < 30; ++i) {
        QImage img(profile.width, profile.height, QImage::Format_ARGB32);
        img.fill(QColor::fromHsv((i * 12) % 360, 200, 220));
        EncodedPacket vpkt;
        if (video.encodeImage(img, i * 33333, vpkt) && !vpkt.data.isEmpty())
            QVERIFY(recorder.writeVideo(vpkt));

        AudioBuffer buf;
        buf.channels = 2;
        buf.sampleRate = 48000;
        buf.ptsUs = i * 33333;
        buf.samples.resize(static_cast<size_t>(480 * 2)); // 10ms
        for (int s = 0; s < 480; ++s) {
            const float t = static_cast<float>((i * 480 + s) / 48000.0);
            const float sample = 0.2f * std::sin(2.0f * 3.14159265f * 440.0f * t);
            buf.samples[static_cast<size_t>(s * 2)] = sample;
            buf.samples[static_cast<size_t>(s * 2 + 1)] = sample;
        }
        // Feed 3x10ms to approach AAC frame size faster each video tick
        for (int k = 0; k < 3; ++k) {
            buf.ptsUs = (i * 3 + k) * 10000;
            QVector<EncodedPacket> apkts;
            QVERIFY(audio.encode(buf, apkts));
            for (const auto& ap : apkts)
                QVERIFY(recorder.writeAudio(ap));
        }
    }

    QVector<EncodedPacket> flushed;
    QVERIFY(video.flush(flushed));
    for (const auto& p : flushed)
        QVERIFY(recorder.writeVideo(p));
    flushed.clear();
    QVERIFY(audio.flush(flushed));
    for (const auto& p : flushed)
        QVERIFY(recorder.writeAudio(p));

    recorder.close();
    video.close();
    audio.close();

    QFileInfo info(path);
    QVERIFY2(info.exists(), "MKV file missing");
    QVERIFY2(info.size() > 1024, qPrintable(QStringLiteral("MKV too small: %1").arg(info.size())));
    QFile::remove(path);
}

QTEST_MAIN(TestMkvRecording)
#include "test_mkv_recording.moc"
