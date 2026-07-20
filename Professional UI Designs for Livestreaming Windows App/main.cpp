#include <QApplication>
#include <QFontDatabase>
#include <QFile>
#include <QDir>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    // Enable high-DPI scaling (Qt 6 default, but explicit for clarity)
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);
    app.setApplicationName("RailShotTV");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("RailShotTV");
    app.setOrganizationDomain("railshottv.com");

    // ── Load bundled fonts ─────────────────────────────────────────────────
    // Place TTF files in resources/fonts/ and add to resources.qrc
    // QFontDatabase::addApplicationFont(":/fonts/BebasNeue-Regular.ttf");
    // QFontDatabase::addApplicationFont(":/fonts/DMSans-Regular.ttf");
    // QFontDatabase::addApplicationFont(":/fonts/DMSans-SemiBold.ttf");
    // QFontDatabase::addApplicationFont(":/fonts/JetBrainsMono-Regular.ttf");
    // Until fonts are bundled, the QSS fallback chain (Segoe UI) is used.

    // ── Load master QSS theme ──────────────────────────────────────────────
    QFile qssFile(":/qss/theme.qss");
    if (qssFile.open(QFile::ReadOnly | QFile::Text)) {
        app.setStyleSheet(QString::fromUtf8(qssFile.readAll()));
        qssFile.close();
    }

    MainWindow window;
    window.setWindowTitle("RailShotTV");
    window.resize(1440, 900);
    window.setMinimumSize(1024, 640);
    window.show();

    return app.exec();
}
