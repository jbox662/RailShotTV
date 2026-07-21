#include <QtTest>
#include "encoding/SoftwareEncoder.h"
#include "encoding/AacEncoder.h"
#include "streaming/RtmpOutput.h"
#include "output/OutputHub.h"
#include <QDir>
#include <QFile>
#include <QImage>
#include <cmath>

using namespace railshot;

class TestRtmpFlv : public QObject {
    Q_OBJECT
private slots:
    void writesLocalFlv();
    void outputHubRecordsAndStreamsToFlv();
};

void TestRtmpFlv::writesLocalFlv()
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

    SoftwareEncoder video;
    AacEncoder audio;
    QString err;
    QVERIFY2(video.open(profile, &err), qPrintable(err));
    QVERIFY2(audio.open(profile, &err), qPrintable(err));

    const QString path = QDir::temp().filePath(QStringLiteral("railshot-smoke.flv"));
    QFile::remove(path);

    RtmpOutput out;
    QVERIFY2(out.connectTo(path, {}, profile, video.extradata(), audio.extradata(),
                           audio.sampleRate(), audio.channels(), &err),
             qPrintable(err));

    for (int i = 0; i < 30; ++i) {
        QImage img(profile.width, profile.height, QImage::Format_ARGB32);
        img.fill(QColor::fromRgb(20, 40 + i * 2, 80));
        EncodedPacket vpkt;
        if (video.encodeImage(img, i * 33333, vpkt) && !vpkt.data.isEmpty())
            out.pushVideo(vpkt);

        AudioBuffer buf;
        buf.channels = 2;
        buf.sampleRate = 48000;
        buf.samples.resize(480 * 2);
        for (int s = 0; s < 480; ++s) {
            const float sample = 0.1f * std::sin(2.0f * 3.14159265f * 440.0f * (i * 480 + s) / 48000.0f);
            buf.samples[static_cast<size_t>(s * 2)] = sample;
            buf.samples[static_cast<size_t>(s * 2 + 1)] = sample;
        }
        for (int k = 0; k < 3; ++k) {
            QVector<EncodedPacket> apkts;
            QVERIFY(audio.encode(buf, apkts));
            for (const auto& ap : apkts)
                out.pushAudio(ap);
        }
    }

    QVector<EncodedPacket> flushed;
    video.flush(flushed);
    for (const auto& p : flushed) out.pushVideo(p);
    flushed.clear();
    audio.flush(flushed);
    for (const auto& p : flushed) out.pushAudio(p);

    QThread::msleep(200);
    out.disconnectFrom();
    video.close();
    audio.close();

    QFileInfo info(path);
    QVERIFY2(info.exists() && info.size() > 1024, qPrintable(QStringLiteral("FLV too small: %1").arg(info.size())));
    QFile::remove(path);
}

void TestRtmpFlv::outputHubRecordsAndStreamsToFlv()
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
    profile.encoderPreference = QStringLiteral("software");

    const QString dir = QDir::temp().filePath(QStringLiteral("railshot-hub-test"));
    QDir().mkpath(dir);
    const QString flv = dir + QStringLiteral("/live.flv");
    QFile::remove(flv);

    OutputHub hub;
    QString err;
    QVERIFY2(hub.startRecording(dir + QStringLiteral("/hub.mkv"), profile, &err), qPrintable(err));

    StreamTarget target;
    target.name = QStringLiteral("Local FLV");
    target.rtmpUrl = flv;
    QVERIFY2(hub.startStreaming(target, profile, &err), qPrintable(err));

    for (int i = 0; i < 24; ++i) {
        QImage img(profile.width, profile.height, QImage::Format_ARGB32);
        img.fill(Qt::darkCyan);
        hub.submitVideoImage(img, i * 33333);

        AudioBuffer buf;
        buf.channels = 2;
        buf.sampleRate = 48000;
        buf.samples.assign(480 * 2, 0.0f);
        hub.submitAudio(buf);
        QThread::msleep(5);
    }

    QThread::msleep(300);
    hub.stopStreaming();
    hub.stopRecording();

    QFileInfo flvInfo(flv);
    QVERIFY2(flvInfo.exists() && flvInfo.size() > 512, "local FLV missing/empty");

    // Clean recording mkv(s)
    const auto mkvs = QDir(dir).entryList({QStringLiteral("*.mkv")}, QDir::Files);
    QVERIFY(!mkvs.isEmpty());
    for (const auto& m : mkvs)
        QFile::remove(dir + QLatin1Char('/') + m);
    QFile::remove(flv);
}

QTEST_MAIN(TestRtmpFlv)
#include "test_rtmp_flv.moc"
