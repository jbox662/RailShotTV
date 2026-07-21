#include "app/Application.h"
#include "core/EngineController.h"
#include "core/Logger.h"
#include "core/ProfileCollectionStore.h"
#include "platform/windows/WinCrashHandler.h"
#include "platform/windows/ComInitializer.h"
#include "ui/MainWindow.h"
#include "ui/Theme.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QMessageBox>

namespace railshot {

Application::Application(int& argc, char** argv)
{
    // High-DPI
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    m_app = std::make_unique<QApplication>(argc, argv);
    m_app->setApplicationName(QStringLiteral("RailShotTV"));
    m_app->setOrganizationName(QStringLiteral("RailShotTV"));
    m_app->setApplicationVersion(QStringLiteral(RAILSHOT_VERSION));
}

Application::~Application()
{
    if (m_engine) m_engine->shutdown();
}

bool Application::bootstrap(QString* error)
{
    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    QDir().mkpath(dataDir + QStringLiteral("/logs"));
    QDir().mkpath(dataDir + QStringLiteral("/dumps"));
    QDir().mkpath(dataDir + QStringLiteral("/projects"));

    Logger::initFileLogging(dataDir + QStringLiteral("/logs"));
    WinCrashHandler::install(dataDir + QStringLiteral("/dumps"));

    ComInitializer com;
    if (!com.ok()) {
        if (error) *error = QStringLiteral("COM initialization failed");
        return false;
    }

    theme::registerFonts();
    m_app->setStyleSheet(theme::loadStyleSheet());
    m_app->setWindowIcon(theme::appIcon());

    m_engine = std::make_unique<EngineController>();
    if (!m_engine->initialize(error))
        return false;

    ensureDefaultProfile(m_engine->settings()->outputProfile());

    if (listCollectionNames().isEmpty()) {
        // Defer untitled save until after first project exists.
    }

    m_window = std::make_unique<MainWindow>(m_engine.get());
    m_window->show();

    // Load after MainWindow so Missing Files dialog can connect to projectLoaded.
    const auto last = m_engine->settings()->lastProjectPath();
    if (!last.isEmpty() && QFile::exists(last)) {
        QString err;
        m_engine->loadProject(last, &err);
    } else {
        m_engine->newProject();
    }

    if (listCollectionNames().isEmpty()) {
        QString err;
        const QString name = QStringLiteral("Untitled");
        m_engine->saveProject(collectionFilePath(name), &err);
        setCurrentCollectionName(name);
    }

    // Optional Virtual Camera auto-start
    if (m_engine->settings()->uiState().value(QStringLiteral("vcamStartOnLaunch")).toBool(false)) {
        QString err;
        m_engine->setVirtualCameraEnabled(true, &err);
    }

    Logger::info(QStringLiteral("RailShotTV %1 ready").arg(QStringLiteral(RAILSHOT_VERSION)));
    return true;
}

int Application::run()
{
    QString error;
    if (!bootstrap(&error)) {
        QMessageBox::critical(nullptr, QStringLiteral("RailShotTV"),
                              QStringLiteral("Failed to start:\n%1").arg(error));
        return 1;
    }
    return m_app->exec();
}

} // namespace railshot
