#include <QtTest>
#include "capture/OverlayRenderer.h"
#include "overlays/ReplayBuffer.h"
#include "encoding/SoftwareEncoder.h"
#include "encoding/AacEncoder.h"
#include <QDir>
#include <QFile>
#include <QImage>
#include <cmath>

using namespace railshot;

class TestCreatorFeatures : public QObject {
    Q_OBJECT
private slots:
    void overlayRendersTransparent();
    void replaySavesMkv();
};

void TestCreatorFeatures::overlayRendersTransparent()
{
    OverlayRenderer r;
    QJsonObject state;
    state.insert(QStringLiteral("playerA"), QStringLiteral("Ada"));
    state.insert(QStringLiteral("playerB"), QStringLiteral("Bea"));
    state.insert(QStringLiteral("scoreA"), 3);
    state.insert(QStringLiteral("scoreB"), 2);
    const QImage board = r.renderScoreboard(state, 640, 360);
    QVERIFY(!board.isNull());
    QCOMPARE(board.format(), QImage::Format_ARGB32);
    // Corner should remain transparent.
    QCOMPARE(qAlpha(board.pixel(0, 0)), 0);

    const QImage lower = r.renderLowerThird(QStringLiteral("Host"), QStringLiteral("Live"), 640, 360);
    QVERIFY(!lower.isNull());
    const QImage alert = r.renderAlert(QStringLiteral("Follow"), QStringLiteral("thanks"), 640, 360);
    QVERIFY(!alert.isNull());
}

void TestCreatorFeatures::replaySavesMkv()
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

    SoftwareEncoder video;
    AacEncoder audio;
    QString err;
    QVERIFY2(video.open(profile, &err), qPrintable(err));
    QVERIFY2(audio.open(profile, &err), qPrintable(err));

    ReplayBuffer replay;
    replay.setCapacitySeconds(10);
    replay.setCodecConfig(profile, video.extradata(), audio.extradata(),
                          audio.sampleRate(), audio.channels());

    for (int i = 0; i < 30; ++i) {
        QImage img(profile.width, profile.height, QImage::Format_ARGB32);
        img.fill(Qt::darkGreen);
        EncodedPacket vpkt;
        if (video.encodeImage(img, i * 33333, vpkt) && !vpkt.data.isEmpty())
            replay.pushVideo(vpkt);

        AudioBuffer buf;
        buf.channels = 2;
        buf.sampleRate = 48000;
        buf.samples.assign(480 * 2, 0.05f);
        for (int k = 0; k < 3; ++k) {
            QVector<EncodedPacket> apkts;
            QVERIFY(audio.encode(buf, apkts));
            for (const auto& ap : apkts)
                replay.pushAudio(ap);
        }
    }
    QVector<EncodedPacket> flushed;
    video.flush(flushed);
    for (const auto& p : flushed) replay.pushVideo(p);
    flushed.clear();
    audio.flush(flushed);
    for (const auto& p : flushed) replay.pushAudio(p);

    const QString path = QDir::temp().filePath(QStringLiteral("railshot-replay-test.mkv"));
    QFile::remove(path);
    QVERIFY2(replay.saveReplay(path, &err), qPrintable(err));
    QFileInfo info(path);
    QVERIFY2(info.size() > 512, "replay mkv too small");
    QFile::remove(path);
}

QTEST_MAIN(TestCreatorFeatures)
#include "test_creator_features.moc"
