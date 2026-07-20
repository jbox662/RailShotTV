#include "ui/MainWindow.h"
#include "ui/Theme.h"
#include "ui/Motion.h"
#include "ui/HotkeyDispatcher.h"
#include "ui/widgets/SidebarRail.h"
#include "ui/widgets/TopMenuBar.h"
#include "ui/pages/DashboardPage.h"
#include "ui/pages/SettingsPage.h"
#include "ui/pages/ScoreboardPage.h"
#include "ui/pages/SchedulePage.h"
#include "ui/pages/ChatPage.h"
#include "ui/pages/AnalyticsPage.h"
#include "ui/pages/SceneEditorPage.h"
#include "ui/widgets/ShortcutsOverlay.h"
#include "ui/widgets/GoLiveDialog.h"
#include "core/EngineController.h"
#include "overlays/ReplayBuffer.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QMessageBox>
#include <QStatusBar>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QAbstractSpinBox>
#include <QFrame>

namespace railshot {

MainWindow::MainWindow(EngineController* engine, QWidget* parent)
    : QMainWindow(parent), m_engine(engine)
{
    setWindowTitle(QStringLiteral("RailShotTV"));
    resize(1440, 900);
    setMinimumSize(1100, 700);
    setObjectName(QStringLiteral("AppRoot"));

    auto* central = new QWidget(this);
    central->setObjectName(QStringLiteral("AppRoot"));
    auto* outer = new QVBoxLayout(central);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    m_liveTopBorder = new QFrame(central);
    m_liveTopBorder->setObjectName(QStringLiteral("liveTopBorder"));
    m_liveTopBorder->setFixedHeight(4);
    m_liveTopBorder->setVisible(false);
    outer->addWidget(m_liveTopBorder);

    auto* root = new QHBoxLayout();
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    m_sidebar = new SidebarRail(central);
    root->addWidget(m_sidebar);

    auto* mainCol = new QVBoxLayout();
    mainCol->setContentsMargins(0, 0, 0, 0);
    mainCol->setSpacing(0);
    m_top = new TopMenuBar(engine, central);
    mainCol->addWidget(m_top);

    m_stack = new QStackedWidget(central);
    m_stack->setObjectName(QStringLiteral("PageStack"));
    m_stack->addWidget(new DashboardPage(engine, m_stack));
    m_stack->addWidget(new ChatPage(engine->chat(), m_stack));
    m_stack->addWidget(new AnalyticsPage(engine, m_stack));
    m_stack->addWidget(new ScoreboardPage(engine, m_stack));
    auto* schedulePage = new SchedulePage(m_stack);
    m_stack->addWidget(schedulePage);
    m_stack->addWidget(new SettingsPage(engine, m_stack));
    auto* sceneEditor = new SceneEditorPage(engine, m_stack);
    m_stack->addWidget(sceneEditor);
    mainCol->addWidget(m_stack, 1);
    root->addLayout(mainCol, 1);
    outer->addLayout(root, 1);
    setCentralWidget(central);

    statusBar()->setStyleSheet(QStringLiteral(
        "background:#0F1114; color:#8892A4; border-top:1px solid #3A3D45;"));
    statusBar()->showMessage(QStringLiteral("Ready"));

    m_hotkeys = new HotkeyDispatcher(engine, this);

    connect(m_sidebar, &SidebarRail::navigate, this, &MainWindow::navigateTo);
    connect(m_top, &TopMenuBar::openProject, this, &MainWindow::openProjectDialog);

    if (auto* dash = qobject_cast<DashboardPage*>(m_stack->widget(0))) {
        connect(dash, &DashboardPage::openSceneEditorRequested, this, [this] {
            navigateTo(QStringLiteral("sceneeditor"));
        });
        connect(m_top, &TopMenuBar::basicModeChanged, dash, &DashboardPage::setBasicMode);
        dash->setBasicMode(m_top->basicMode());
    }
    connect(sceneEditor, &SceneEditorPage::backToDashboard, this, [this] {
        navigateTo(QStringLiteral("dashboard"));
    });
    connect(schedulePage, &SchedulePage::goLiveRequested, this, [this] {
        navigateTo(QStringLiteral("dashboard"));
        GoLiveDialog dlg(m_engine, this);
        dlg.exec();
    });
    connect(m_top, &TopMenuBar::toggleShortcuts, this, [this] {
        ShortcutsOverlay dlg(this);
        dlg.exec();
    });
    connect(m_top, &TopMenuBar::saveProject, this, &MainWindow::saveProjectDialog);
    connect(m_top, &TopMenuBar::newProject, this, [this] {
        QMessageBox box(this);
        box.setWindowTitle(QStringLiteral("New Project"));
        box.setIcon(QMessageBox::Warning);
        box.setText(QStringLiteral("Clear all scenes and start a new project?"));
        box.setInformativeText(QStringLiteral("Unsaved changes will be lost. This cannot be undone."));
        box.setStyleSheet(QStringLiteral(
            "QMessageBox{background:#1A1D22;}"
            "QLabel{color:#D0D2D8;}"
            "QPushButton{min-width:100px;padding:6px 14px;}"));
        auto* cancel = box.addButton(QStringLiteral("Cancel"), QMessageBox::RejectRole);
        auto* clear = box.addButton(QStringLiteral("Clear & Start New"), QMessageBox::DestructiveRole);
        clear->setStyleSheet(QStringLiteral(
            "QPushButton{background:#EF4444;color:white;font-weight:800;border:1px solid #F87171;}"));
        box.exec();
        if (box.clickedButton() != clear) {
            Q_UNUSED(cancel);
            return;
        }
        m_engine->newProject();
        statusBar()->showMessage(QStringLiteral("New project"), 2000);
    });
    connect(m_top, &TopMenuBar::openSettings, this, [this] { navigateTo(QStringLiteral("settings")); });
    connect(m_engine, &EngineController::errorOccurred, this, [this](const QString& msg) {
        statusBar()->showMessage(msg, 5000);
    });
    connect(m_engine, &EngineController::telemetryUpdated, this, [this](const TelemetrySnapshot& s) {
        updateLiveChrome(s.streaming);
        QString msg;
        if (s.streaming)
            msg = QStringLiteral("LIVE  %1 kbps  drift %2 ms").arg(s.bitrateKbps).arg(s.avDriftMs, 0, 'f', 1);
        else if (s.recording)
            msg = QStringLiteral("REC  %1s").arg(s.recordUptimeSec);
        else
            msg = QStringLiteral("Ready");
        if (m_engine->replayBuffer()) {
            const qint64 us = m_engine->replayBuffer()->bufferedDurationUs();
            msg += QStringLiteral("  ·  Replay %1s / %2s")
                       .arg(us / 1000000)
                       .arg(m_engine->replayBuffer()->capacitySeconds());
        }
        statusBar()->showMessage(msg);
    });
    connect(m_engine, &EngineController::replaySaved, this, [this](const QString& path) {
        statusBar()->showMessage(QStringLiteral("Replay saved: %1").arg(path), 4000);
    });
}

MainWindow::~MainWindow() = default;

void MainWindow::updateLiveChrome(bool streaming)
{
    if (m_liveTopBorder) {
        if (streaming) {
            if (!m_liveTopBorder->isVisible())
                motion::animateLiveBorderIn(m_liveTopBorder);
            else
                m_liveTopBorder->setVisible(true);
        } else {
            m_liveTopBorder->setVisible(false);
            m_liveTopBorder->setMaximumWidth(QWIDGETSIZE_MAX);
        }
    }
    if (m_sidebar) m_sidebar->setLive(streaming);
}

void MainWindow::navigateTo(const QString& pageId)
{
    if (pageId != QLatin1String("sceneeditor") && m_sidebar)
        m_sidebar->setActivePage(pageId);
    if (pageId == QLatin1String("dashboard")) m_stack->setCurrentIndex(0);
    else if (pageId == QLatin1String("chat")) m_stack->setCurrentIndex(1);
    else if (pageId == QLatin1String("analytics")) m_stack->setCurrentIndex(2);
    else if (pageId == QLatin1String("scoreboard")) m_stack->setCurrentIndex(3);
    else if (pageId == QLatin1String("schedule")) m_stack->setCurrentIndex(4);
    else if (pageId == QLatin1String("settings")) m_stack->setCurrentIndex(5);
    else if (pageId == QLatin1String("sceneeditor")) {
        m_stack->setCurrentIndex(6);
        if (auto* p = m_stack->currentWidget())
            p->setFocus(Qt::OtherFocusReason);
    }
    // Dashboard keeps the broadcast TopMenuBar; secondary pages use their own page headers.
    if (m_top)
        m_top->setVisible(pageId == QLatin1String("dashboard"));
}

void MainWindow::openProjectDialog()
{
    const auto path = QFileDialog::getOpenFileName(this, QStringLiteral("Open Project"),
                                                   {}, QStringLiteral("RailShot Project (*.railshot.json *.json)"));
    if (path.isEmpty()) return;
    QString err;
    if (!m_engine->loadProject(path, &err))
        QMessageBox::warning(this, QStringLiteral("Open"), err);
    else
        statusBar()->showMessage(QStringLiteral("Loaded %1").arg(path), 3000);
}

void MainWindow::saveProjectDialog()
{
    auto path = m_engine->settings()->lastProjectPath();
    if (path.isEmpty()) {
        path = QFileDialog::getSaveFileName(this, QStringLiteral("Save Project"),
                                            QStringLiteral("project.railshot.json"),
                                            QStringLiteral("RailShot Project (*.railshot.json)"));
    }
    if (path.isEmpty()) return;
    QString err;
    if (!m_engine->saveProject(path, &err))
        QMessageBox::warning(this, QStringLiteral("Save"), err);
    else
        statusBar()->showMessage(QStringLiteral("Saved %1").arg(path), 3000);
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    if (auto* w = focusWidget()) {
        if (qobject_cast<QLineEdit*>(w) || qobject_cast<QPlainTextEdit*>(w)
            || qobject_cast<QAbstractSpinBox*>(w) || w->inherits("QKeySequenceEdit")) {
            QMainWindow::keyPressEvent(event);
            return;
        }
    }
    if (event->modifiers() & Qt::ShiftModifier
        && (event->key() == Qt::Key_Question || event->key() == Qt::Key_Slash)) {
        ShortcutsOverlay dlg(this);
        dlg.exec();
        return;
    }
    if (m_hotkeys && m_hotkeys->handleKey(event))
        return;
    QMainWindow::keyPressEvent(event);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (m_engine->telemetrySnapshot().streaming || m_engine->telemetrySnapshot().recording) {
        const auto r = QMessageBox::question(this, QStringLiteral("Quit"),
                                             QStringLiteral("Stream/recording is active. Stop and quit?"));
        if (r != QMessageBox::Yes) {
            event->ignore();
            return;
        }
        m_engine->stopStreaming();
        m_engine->stopRecording();
    }
    m_engine->settings()->sync();
    event->accept();
}

} // namespace railshot
