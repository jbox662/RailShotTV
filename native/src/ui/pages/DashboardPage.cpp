#include "ui/pages/DashboardPage.h"
#include "ui/widgets/PreviewWidget.h"
#include "ui/widgets/TransitionPanel.h"
#include "ui/widgets/SceneListWidget.h"
#include "ui/widgets/InputTilesWidget.h"
#include "ui/widgets/AudioMixerWidget.h"
#include "ui/widgets/BottomToolbar.h"
#include "ui/widgets/GoLiveDialog.h"
#include "ui/widgets/SourcePropertiesWidget.h"
#include "ui/widgets/AddSourceDialog.h"
#include "ui/widgets/MultiCorderPanel.h"
#include "ui/widgets/PlayListPanel.h"
#include "ui/Theme.h"
#include "core/EngineController.h"
#include "core/Types.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QMenu>
#include <QMessageBox>
#include <QFile>
#include <QDir>
#include <QUrl>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QResizeEvent>
#include <QEvent>
#include <QFrame>

namespace railshot {

namespace {
bool addBrowserPreset(EngineController* engine, const QString& name, const QString& resourcePath,
                      double x = 0.05, double y = 0.72, double w = 0.55, double h = 0.22)
{
    const QString qrc = QStringLiteral(":/overlays/%1").arg(resourcePath);
    QFile f(qrc);
    if (!f.open(QIODevice::ReadOnly)) return false;
    const QString destDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                            + QStringLiteral("/overlays");
    QDir().mkpath(destDir);
    const QString dest = destDir + QLatin1Char('/') + resourcePath;
    {
        QFile out(dest);
        if (out.open(QIODevice::WriteOnly | QIODevice::Truncate))
            out.write(f.readAll());
    }
    QJsonObject settings;
    settings.insert(QStringLiteral("url"), QUrl::fromLocalFile(dest).toString());
    settings.insert(QStringLiteral("width"), 1280);
    settings.insert(QStringLiteral("height"), 720);
    settings.insert(QStringLiteral("fps"), 30);
    const QString id = engine->addSource(SourceType::Browser, name);
    if (id.isEmpty()) return false;
    engine->updateSourceSettings(id, settings);
    Transform t;
    t.x = x; t.y = y; t.w = w; t.h = h;
    engine->updateSourceTransform(id, t);
    engine->setSelectedSourceId(id);
    return true;
}

QString addTypedOverlay(EngineController* engine, SourceType type, const QString& name,
                        double x, double y, double w, double h)
{
    const QString id = engine->addSource(type, name);
    if (id.isEmpty()) return {};
    Transform t;
    t.x = x; t.y = y; t.w = w; t.h = h;
    engine->updateSourceTransform(id, t);
    engine->setSelectedSourceId(id);
    return id;
}

void populateOverlayMenu(QMenu* menu, EngineController* engine, DashboardPage* page)
{
    auto* openEd = menu->addAction(QStringLiteral("Open Scene Editor"));
    QObject::connect(openEd, &QAction::triggered, page, &DashboardPage::openSceneEditorRequested);
    menu->addSeparator();
    menu->addAction(QStringLiteral("Billiards Scoreboard"), page, [engine] {
        addTypedOverlay(engine, SourceType::Scoreboard, QStringLiteral("Billiards Scoreboard"), 0.05, 0.78, 0.9, 0.18);
        engine->pushScoreboardToProgram();
    });
    menu->addAction(QStringLiteral("Basketball Board"), page, [engine] {
        addTypedOverlay(engine, SourceType::Scoreboard, QStringLiteral("Basketball Board"), 0.15, 0.05, 0.7, 0.12);
    });
    menu->addAction(QStringLiteral("Player Lower Third"), page, [engine] {
        if (!addBrowserPreset(engine, QStringLiteral("Player Lower Third"), QStringLiteral("player-intro.html"),
                              0.05, 0.78, 0.55, 0.18))
            addTypedOverlay(engine, SourceType::LowerThird, QStringLiteral("Player Lower Third"), 0.05, 0.78, 0.55, 0.18);
    });
    menu->addAction(QStringLiteral("Team Lower Third"), page, [engine] {
        addTypedOverlay(engine, SourceType::LowerThird, QStringLiteral("Team Lower Third"), 0.05, 0.78, 0.55, 0.18);
    });
    menu->addAction(QStringLiteral("Score Ticker"), page, [engine] {
        if (!addBrowserPreset(engine, QStringLiteral("Score Ticker"), QStringLiteral("match-point.html"),
                              0.0, 0.92, 1.0, 0.08))
            addTypedOverlay(engine, SourceType::Browser, QStringLiteral("Score Ticker"), 0.0, 0.92, 1.0, 0.08);
    });
    menu->addAction(QStringLiteral("Sub Alert"), page, [engine] {
        addTypedOverlay(engine, SourceType::Alert, QStringLiteral("Sub Alert"), 0.3, 0.3, 0.4, 0.3);
    });
    menu->addAction(QStringLiteral("Logo Overlay"), page, [engine] {
        addTypedOverlay(engine, SourceType::Image, QStringLiteral("Logo Overlay"), 0.85, 0.05, 0.12, 0.12);
    });
    menu->addAction(QStringLiteral("Camera Frame"), page, [engine] {
        if (!addBrowserPreset(engine, QStringLiteral("Camera Frame"), QStringLiteral("break-and-run.html"),
                              0.0, 0.0, 1.0, 1.0))
            addTypedOverlay(engine, SourceType::Browser, QStringLiteral("Camera Frame"), 0.0, 0.0, 1.0, 1.0);
    });
    menu->addSeparator();
    menu->addAction(QStringLiteral("Break and Run"), page, [engine] {
        addBrowserPreset(engine, QStringLiteral("Break and Run"), QStringLiteral("break-and-run.html"),
                         0.2, 0.25, 0.6, 0.4);
    });
}
} // namespace

DashboardPage::DashboardPage(EngineController* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* monitors = new QHBoxLayout();
    monitors->setContentsMargins(0, 0, 0, 0);
    monitors->setSpacing(0);
    m_preview = new PreviewWidget(engine, false, this);
    m_program = new PreviewWidget(engine, true, this);
    auto* transitions = new TransitionPanel(engine, this);
    monitors->addWidget(m_preview, 1);
    monitors->addWidget(transitions);
    monitors->addWidget(m_program, 1);
    root->addLayout(monitors, 1);

    auto* inputsRow = new QHBoxLayout();
    inputsRow->setContentsMargins(0, 0, 0, 0);
    inputsRow->setSpacing(0);

    auto* scenesCol = new QWidget(this);
    scenesCol->setFixedWidth(180);
    scenesCol->setStyleSheet(QStringLiteral("background:#0A0C0F; border-right:1px solid #2A2D35;"));
    auto* scenesLay = new QVBoxLayout(scenesCol);
    scenesLay->setContentsMargins(0, 0, 0, 0);
    scenesLay->setSpacing(0);
    auto* scenesHeader = new QWidget(scenesCol);
    scenesHeader->setFixedHeight(28);
    scenesHeader->setStyleSheet(
        QStringLiteral(
            "background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 rgba(79,158,255,0.14), stop:0.65 transparent);"
            "border-bottom:1px solid #1A1D24; border-left:2px solid #4F9EFF;"));
    auto* scenesHeaderLay = new QHBoxLayout(scenesHeader);
    scenesHeaderLay->setContentsMargins(10, 0, 8, 0);
    auto* scenesTitle = new QLabel(QStringLiteral("Scenes"), scenesHeader);
    scenesTitle->setObjectName(QStringLiteral("panelTitleBlue"));
    scenesTitle->setStyleSheet(QStringLiteral(
        "color:#4F9EFF; font-weight:800; font-size:10px; letter-spacing:1.5px; background:transparent;"
        "text-transform:uppercase;"));
    auto* addScene = new QPushButton(QStringLiteral("+"), scenesHeader);
    addScene->setObjectName(QStringLiteral("panelAddButton"));
    addScene->setFixedSize(24, 20);
    addScene->setStyleSheet(QStringLiteral(
        "QPushButton{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #1A3AFF,stop:1 #1230CC);"
        "border:1px solid #3A6AFF;border-radius:3px;color:white;font-weight:800;}"));
    connect(addScene, &QPushButton::clicked, this, [this] {
        m_engine->sceneGraph()->mutate([](Project& p) { p.addScene(); });
    });
    scenesHeaderLay->addWidget(scenesTitle);
    scenesHeaderLay->addStretch();
    scenesHeaderLay->addWidget(addScene);
    scenesLay->addWidget(scenesHeader);

    auto* scenes = new SceneListWidget(engine, scenesCol);
    scenesLay->addWidget(scenes, 1);
    inputsRow->addWidget(scenesCol);

    m_tiles = new InputTilesWidget(engine, this);
    inputsRow->addWidget(m_tiles, 1);

    m_mixer = new AudioMixerWidget(engine, this);
    inputsRow->addWidget(m_mixer);
    root->addLayout(inputsRow);

    m_toolbar = new BottomToolbar(engine, this);
    m_toolbar->setMixerOpen(true);
    root->addWidget(m_toolbar);

    m_drawerBackdrop = new QFrame(this);
    m_drawerBackdrop->setObjectName(QStringLiteral("drawerBackdrop"));
    m_drawerBackdrop->hide();
    m_drawerBackdrop->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    connect(m_drawerBackdrop, &QFrame::destroyed, this, [] {});

    m_props = new SourcePropertiesWidget(engine, this);
    m_props->hide();
    connect(m_props, &SourcePropertiesWidget::closeRequested, this, [this] { closeDrawer(); });

    m_multi = new MultiCorderPanel(engine, this);
    m_multi->hide();
    connect(m_multi, &MultiCorderPanel::closeRequested, this, [this] { setMultiCorderOpen(false); });

    m_playlist = new PlayListPanel(engine, this);
    m_playlist->hide();
    connect(m_playlist, &PlayListPanel::closeRequested, this, [this] { setPlayListOpen(false); });

    // Click backdrop to close — use event filter via mouse
    m_drawerBackdrop->installEventFilter(this);

    connect(m_preview, &PreviewWidget::sourceSelected, this, [this](const QString& id) {
        m_engine->setSelectedSourceId(id);
    });

    connect(m_tiles, &InputTilesWidget::configureSourceRequested, this, [this](const QString&) {
        openDrawer();
    });

    connect(m_toolbar, &BottomToolbar::addInputRequested, this, [this] {
        AddSourceDialog dlg(m_engine, this);
        if (dlg.exec() != QDialog::Accepted) return;
        const auto r = dlg.result();
        if (!r.accepted) return;
        const QString id = m_engine->addSource(r.type, r.name);
        if (!id.isEmpty() && !r.settings.isEmpty())
            m_engine->updateSourceSettings(id, r.settings);
        m_engine->setSelectedSourceId(id);
        openDrawer();
    });

    connect(m_toolbar, &BottomToolbar::goLiveRequested, this, [this] {
        GoLiveDialog dlg(m_engine, this);
        dlg.exec();
    });

    // Mixer stays visible; toolbar button reflects open state only
    connect(m_toolbar, &BottomToolbar::mixerToggleRequested, this, [this] {
        m_toolbar->setMixerOpen(true);
    });

    connect(m_toolbar, &BottomToolbar::multiCorderRequested, this, [this] {
        const bool open = !m_multi->isVisible();
        setMultiCorderOpen(open);
        if (open) setPlayListOpen(false);
    });
    connect(m_toolbar, &BottomToolbar::playListRequested, this, [this] {
        const bool open = !m_playlist->isVisible();
        setPlayListOpen(open);
        if (open) setMultiCorderOpen(false);
    });
    connect(m_toolbar, &BottomToolbar::overlayMenuRequested, this, [this](const QPoint& globalPos) {
        QMenu menu(this);
        populateOverlayMenu(&menu, m_engine, this);
        menu.exec(globalPos);
    });

    auto* tick = new QTimer(this);
    connect(tick, &QTimer::timeout, this, [this] {
        m_preview->tick();
        m_program->tick();
    });
    tick->start(16);

    setFocusPolicy(Qt::StrongFocus);
}

void DashboardPage::setBasicMode(bool on)
{
    if (m_toolbar)
        m_toolbar->setBasicMode(on);
    if (on) {
        setMultiCorderOpen(false);
        setPlayListOpen(false);
    }
}

void DashboardPage::setMixerOpen(bool open)
{
    // Keep mixer permanently visible in the inputs strip.
    Q_UNUSED(open);
    m_mixerOpen = true;
    if (m_toolbar)
        m_toolbar->setMixerOpen(true);
    if (m_mixer) {
        m_mixer->setMinimumWidth(280);
        m_mixer->setMaximumWidth(360);
    }
}

void DashboardPage::openDrawer()
{
    setMultiCorderOpen(false);
    setPlayListOpen(false);
    layoutDrawer();
    m_drawerBackdrop->show();
    m_drawerBackdrop->raise();
    m_props->show();
    m_props->raise();
}

void DashboardPage::closeDrawer()
{
    m_props->hide();
    if (!m_multi->isVisible() && !m_playlist->isVisible())
        m_drawerBackdrop->hide();
}

void DashboardPage::layoutDrawer()
{
    if (!m_drawerBackdrop || !m_props) return;
    m_drawerBackdrop->setGeometry(0, 0, width(), height());
    m_props->setGeometry(width() - 460, 0, 460, height());
}

void DashboardPage::setMultiCorderOpen(bool open)
{
    if (!m_multi) return;
    if (open) {
        closeDrawer();
        layoutSidePanels();
        m_drawerBackdrop->show();
        m_drawerBackdrop->raise();
        m_multi->show();
        m_multi->raise();
        m_multi->refresh();
    } else {
        m_multi->hide();
        if (!m_playlist->isVisible() && !m_props->isVisible())
            m_drawerBackdrop->hide();
    }
}

void DashboardPage::setPlayListOpen(bool open)
{
    if (!m_playlist) return;
    if (open) {
        closeDrawer();
        layoutSidePanels();
        m_drawerBackdrop->show();
        m_drawerBackdrop->raise();
        m_playlist->show();
        m_playlist->raise();
        m_playlist->refresh();
    } else {
        m_playlist->hide();
        if (!m_multi->isVisible() && !m_props->isVisible())
            m_drawerBackdrop->hide();
    }
}

void DashboardPage::layoutSidePanels()
{
    if (m_drawerBackdrop)
        m_drawerBackdrop->setGeometry(0, 0, width(), height());
    if (m_multi)
        m_multi->setGeometry(width() - 320, 36, 320, height() - 36);
    if (m_playlist)
        m_playlist->setGeometry(width() - 300, 36, 300, height() - 36);
}

void DashboardPage::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    if (m_props && m_props->isVisible())
        layoutDrawer();
    if ((m_multi && m_multi->isVisible()) || (m_playlist && m_playlist->isVisible()))
        layoutSidePanels();
}

bool DashboardPage::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_drawerBackdrop && event->type() == QEvent::MouseButtonPress) {
        closeDrawer();
        setMultiCorderOpen(false);
        setPlayListOpen(false);
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

} // namespace railshot
