#include <QtTest>
#include "browser/BrowserIpc.h"
#include "vcam/VCamIpc.h"
#include "chat/PlatformAdapters.h"

using namespace railshot;

class TestPhase5 : public QObject {
    Q_OBJECT
private slots:
    void browserIpcBufferSize();
    void vcamIpcBufferSize();
    void platformAuthUrls();
};

void TestPhase5::browserIpcBufferSize()
{
    QCOMPARE(browser_ipc::kMagic, quint32(0x52534252));
    QVERIFY(browser_ipc::bufferBytes(1280, 720) > sizeof(browser_ipc::FrameHeader));
    QCOMPARE(browser_ipc::bufferBytes(100, 50),
             sizeof(browser_ipc::FrameHeader) + 100 * 50 * 4);
}

void TestPhase5::vcamIpcBufferSize()
{
    QCOMPARE(vcam_ipc::kMagic, quint32(0x52535643));
    QVERIFY(vcam_ipc::bufferBytes(1920, 1080) > sizeof(vcam_ipc::SharedHeader));
    QCOMPARE(vcam_ipc::bufferBytes(64, 36),
             sizeof(vcam_ipc::SharedHeader) + 64 * 36 * 4);
}

void TestPhase5::platformAuthUrls()
{
    QVERIFY(PlatformAdapters::authorizeUrl(QStringLiteral("twitch"), QStringLiteral("cid"),
                                           QStringLiteral("http://localhost")).contains(QStringLiteral("twitch.tv")));
    QVERIFY(PlatformAdapters::authorizeUrl(QStringLiteral("youtube"), QStringLiteral("cid"),
                                           QStringLiteral("http://localhost")).contains(QStringLiteral("google")));
    QVERIFY(PlatformAdapters::authorizeUrl(QStringLiteral("facebook"), QStringLiteral("cid"),
                                           QStringLiteral("http://localhost")).contains(QStringLiteral("facebook")));
}

QTEST_MAIN(TestPhase5)
#include "test_phase5.moc"
