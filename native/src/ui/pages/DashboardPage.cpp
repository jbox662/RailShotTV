#include "ui/pages/DashboardPage.h"
#include "ui/widgets/PreviewWidget.h"
#include "ui/widgets/TransitionPanel.h"
#include "ui/widgets/SceneListWidget.h"
#include "ui/widgets/InputTilesWidget.h"
#include "ui/widgets/AudioMixerWidget.h"
#include "ui/widgets/ScoreboardControlsWidget.h"
#include "ui/widgets/BottomToolbar.h"
#include "ui/widgets/GoLiveDialog.h"
#include "ui/widgets/SourcePropertiesDialog.h"
#include "ui/widgets/AddSourceDialog.h"
#include "ui/widgets/MultiCorderPanel.h"
#include "ui/widgets/PlayListPanel.h"
#include "ui/widgets/SourceContextToolbar.h"
#include "ui/widgets/FiltersDialog.h"
#include "ui/widgets/TransformDialog.h"
#include "ui/widgets/InteractDialog.h"
#include "ui/widgets/ProjectorWindow.h"
#include "ui/widgets/ObsStatsDock.h"
#include "ui/widgets/ExtraBrowserPanel.h"
#include "ui/Theme.h"
#include "core/EngineController.h"
#include "core/SceneGraph.h"
#include "core/SettingsStore.h"
#include "core/Types.h"
#include "capture/OverlaySource.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QMenu>
#include <QMessageBox>
#include <QFile>
#include <QFileDialog>
#include <QDir>
#include <QUrl>
#include <QJsonObject>
#include <QJsonArray>
#include <QInputDialog>
#include <QLineEdit>
#include <QStandardPaths>
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QResizeEvent>
#include <QHideEvent>
#include <QEvent>
#include <QMainWindow>
#include <QDockWidget>
#include <QFrame>
#include <QByteArray>
#include <QMouseEvent>
#include <QCursor>
#include <QSignalBlocker>
#include <QSizePolicy>

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
        const double w = 0.85;
        const auto proj = engine->projectSnapshot();
        const double h = scoreboardNormHeight(w, proj.output.width, proj.output.height);
        addTypedOverlay(engine, SourceType::Scoreboard, QStringLiteral("Billiards Scoreboard"),
                        0.075, 1.0 - h - 0.02, w, h);
        engine->pushScoreboardToProgram();
    });
    menu->addAction(QStringLiteral("Basketball Board"), page, [engine] {
        const double w = 0.7;
        const auto proj = engine->projectSnapshot();
        const double h = scoreboardNormHeight(w, proj.output.width, proj.output.height);
        addTypedOverlay(engine, SourceType::Scoreboard, QStringLiteral("Basketball Board"),
                        (1.0 - w) / 2.0, 0.04, w, h);
        engine->pushScoreboardToProgram();
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
    menu->addAction(QStringLiteral("Logo Overlay"), page, [engine, page] {
        const auto path = QFileDialog::getOpenFileName(
            page, QStringLiteral("Logo Image"), {},
            QStringLiteral("Images (*.png *.jpg *.jpeg *.bmp *.webp)"));
        if (path.isEmpty()) return;
        const QString id = engine->addSource(SourceType::Image, QStringLiteral("Logo Overlay"));
        engine->updateSourceSettings(id, QJsonObject{{QStringLiteral("path"), path}});
        Transform t;
        t.x = 0.85; t.y = 0.05; t.w = 0.12; t.h = 0.12;
        engine->updateSourceTransform(id, t);
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

constexpr const char* kDockHostStyle = R"(
QMainWindow#dashboardDockHost {
  background: #05070A;
  border-top: 2px solid #2A2D35;
}
/* Visible gutter between docks — keeps panels from reading as one slab */
QMainWindow#dashboardDockHost QMainWindow::separator {
  background: #05070A;
  width: 6px;
  height: 6px;
}
QMainWindow#dashboardDockHost QMainWindow::separator:hover {
  background: #3A6AFF;
}
QMainWindow#dashboardDockHost QSplitter::handle:horizontal {
  background: #05070A;
  width: 6px;
  margin: 0px;
}
QMainWindow#dashboardDockHost QSplitter::handle:horizontal:hover {
  background: #3A6AFF;
}
QMainWindow#dashboardDockHost QSplitter::handle:vertical {
  background: #05070A;
  height: 6px;
}
QMainWindow#dashboardDockHost QDockWidget {
  border: 1px solid #4A5060;
  background: #0D0F12;
  titlebar-close-icon: none;
  titlebar-normal-icon: none;
}
QMainWindow#dashboardDockHost QDockWidget > QWidget {
  background: #0D0F12;
  border: none;
}
QMainWindow#dashboardDockHost QFrame#dockContentFrame {
  background: #0D0F12;
  border: none;
  margin: 0px;
}
)";

class DockTitleBar : public QWidget {
public:
    DockTitleBar(QDockWidget* dock, const QString& title, const QString& accent, QWidget* parent = nullptr)
        : QWidget(parent), m_dock(dock)
    {
        // Title row + separate rule widget (CSS border-bottom was bleeding into content).
        setFixedHeight(30);
        setCursor(Qt::SizeAllCursor);
        setObjectName(QStringLiteral("dockTitleBar"));
        setAttribute(Qt::WA_StyledBackground, true);
        setAutoFillBackground(true);
        setStyleSheet(QStringLiteral(
            "QWidget#dockTitleBar {"
            "  border: none;"
            "  border-left: 3px solid %1;"
            "  background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
            "    stop:0 %2, stop:0.5 #14181E);"
            "}")
                          .arg(accent, tintForAccent(accent)));

        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(0, 0, 0, 0);
        root->setSpacing(0);

        auto* bar = new QWidget(this);
        bar->setFixedHeight(28);
        bar->setStyleSheet(QStringLiteral("background:transparent; border:none;"));
        auto* lay = new QHBoxLayout(bar);
        lay->setContentsMargins(10, 0, 4, 0);
        lay->setSpacing(2);

        auto* lab = new QLabel(title.toUpper(), bar);
        lab->setStyleSheet(QStringLiteral(
            "color:#F0F0F0; font-family:'DM Sans','Segoe UI'; font-size:11px; font-weight:800;"
            "letter-spacing:1.2px; background:transparent; border:none;"));
        lay->addWidget(lab, 1);

        const QString btnStyle = QStringLiteral(
            "QPushButton{background:transparent;border:1px solid transparent;color:%1;"
            "font-size:11px;font-weight:700;padding:0px;min-width:20px;max-width:20px;border-radius:2px;}"
            "QPushButton:hover{color:#F8F8FF;background:rgba(255,255,255,0.08);border-color:#3A3D45;}")
                                     .arg(accent);

        auto* floatBtn = new QPushButton(QStringLiteral("□"), bar);
        floatBtn->setFixedSize(20, 20);
        floatBtn->setCursor(Qt::PointingHandCursor);
        floatBtn->setToolTip(QStringLiteral("Float / dock"));
        floatBtn->setStyleSheet(btnStyle);
        connect(floatBtn, &QPushButton::clicked, this, [this] {
            if (m_dock) m_dock->setFloating(!m_dock->isFloating());
        });

        auto* hideBtn = new QPushButton(QStringLiteral("×"), bar);
        hideBtn->setFixedSize(20, 20);
        hideBtn->setCursor(Qt::PointingHandCursor);
        hideBtn->setToolTip(QStringLiteral("Hide panel"));
        hideBtn->setStyleSheet(btnStyle);
        connect(hideBtn, &QPushButton::clicked, this, [this] {
            if (m_dock) m_dock->hide();
        });

        lay->addWidget(floatBtn);
        lay->addWidget(hideBtn);
        root->addWidget(bar);

        auto* rule = new QFrame(this);
        rule->setObjectName(QStringLiteral("dockTitleRule"));
        rule->setFixedHeight(2);
        rule->setFrameShape(QFrame::NoFrame);
        rule->setStyleSheet(QStringLiteral(
            "QFrame#dockTitleRule{background:#4A5060; border:none; max-height:2px; min-height:2px;}"));
        root->addWidget(rule);
    }

protected:
    void mouseDoubleClickEvent(QMouseEvent* e) override
    {
        if (m_dock && e->button() == Qt::LeftButton)
            m_dock->setFloating(!m_dock->isFloating());
        QWidget::mouseDoubleClickEvent(e);
    }

private:
    static QString tintForAccent(const QString& accent)
    {
        if (accent.contains(QLatin1String("FF5A"))) return QStringLiteral("rgba(255,90,44,0.32)");
        if (accent.contains(QLatin1String("A855"))) return QStringLiteral("rgba(168,85,247,0.32)");
        if (accent.contains(QLatin1String("22C5"))) return QStringLiteral("rgba(34,197,94,0.32)");
        return QStringLiteral("rgba(79,158,255,0.32)");
    }
    QDockWidget* m_dock = nullptr;
};
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

    m_previewColumn = new QWidget(this);
    auto* previewColLay = new QVBoxLayout(m_previewColumn);
    previewColLay->setContentsMargins(0, 0, 0, 0);
    previewColLay->setSpacing(0);
    m_preview = new PreviewWidget(engine, false, m_previewColumn);
    previewColLay->addWidget(m_preview, 1);

    m_program = new PreviewWidget(engine, true, this);
    m_transitions = new TransitionPanel(engine, this);
    monitors->addWidget(m_previewColumn, 1);
    monitors->addWidget(m_transitions, 0);
    monitors->addWidget(m_program, 1);
    root->addLayout(monitors, 5);

    // Context toolbar under both monitors so Preview/Program canvases stay equal height.
    m_contextBar = new SourceContextToolbar(engine, this);
    root->addWidget(m_contextBar);

    // Nested QMainWindow hosts OBS-style docks (must be Qt::Widget, not a top-level window).
    m_dockHost = new QMainWindow(this);
    m_dockHost->setObjectName(QStringLiteral("dashboardDockHost"));
    m_dockHost->setWindowFlags(Qt::Widget);
    m_dockHost->setDockNestingEnabled(true);
    m_dockHost->setDockOptions(QMainWindow::AnimatedDocks
                               | QMainWindow::AllowNestedDocks
                               | QMainWindow::AllowTabbedDocks
                               | QMainWindow::GroupedDragging);
    m_dockHost->setStyleSheet(QString::fromLatin1(kDockHostStyle));
    m_dockHost->setMinimumHeight(200);
    m_dockHost->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_dockHost, &QWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        QMenu menu(this);
        menu.addAction(QStringLiteral("Reset desk layout"), this, &DashboardPage::resetDockLayout);
        if (m_scenesDock)
            menu.addAction(m_scenesDock->toggleViewAction());
        if (m_sourcesDock)
            menu.addAction(m_sourcesDock->toggleViewAction());
        if (m_mixerDock)
            menu.addAction(m_mixerDock->toggleViewAction());
        if (m_scoreboardDock)
            menu.addAction(m_scoreboardDock->toggleViewAction());
        menu.exec(m_dockHost->mapToGlobal(pos));
    });

    auto* centralStub = new QWidget(m_dockHost);
    centralStub->setFixedHeight(0);
    centralStub->setMaximumHeight(0);
    m_dockHost->setCentralWidget(centralStub);

    // Scenes content — Chromatic Command blue panel
    auto* scenesCol = new QWidget;
    scenesCol->setObjectName(QStringLiteral("chromePanel"));
    scenesCol->setMinimumWidth(150);
    scenesCol->setStyleSheet(QStringLiteral(
        "QWidget#chromePanel{background:#0D0F12; border:none;}"));
    auto* scenesLay = new QVBoxLayout(scenesCol);
    scenesLay->setContentsMargins(0, 0, 0, 0);
    scenesLay->setSpacing(0);
    auto* scenesTools = new QWidget(scenesCol);
    scenesTools->setObjectName(QStringLiteral("scenesTools"));
    scenesTools->setFixedHeight(36);
    scenesTools->setAttribute(Qt::WA_StyledBackground, true);
    scenesTools->setStyleSheet(QStringLiteral(
        "QWidget#scenesTools{"
        "  background:#0D0F12;"
        "  border:none;"
        "  border-bottom:1px solid #2A2D35;"
        "}"));
    auto* scenesToolsLay = new QHBoxLayout(scenesTools);
    scenesToolsLay->setContentsMargins(8, 6, 8, 6);
    scenesToolsLay->setSpacing(4);
    scenesToolsLay->setAlignment(Qt::AlignVCenter);

    const auto sceneToolStyle = QStringLiteral(
        "QPushButton{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #32363F,stop:1 #1A1E26);"
        "border:1px solid #5A5E68;color:#E0E2E8;font-family:'DM Sans';font-size:11px;"
        "font-weight:800;border-radius:3px;padding:0;margin:0;}"
        "QPushButton:hover{border-color:#4F9EFF;color:#FFFFFF;}"
        "QPushButton:disabled{color:#505860;border-color:#2A2D35;}");

    auto makeSceneTool = [&](const QString& text, const QString& tip, const QString& style) {
        auto* b = new QPushButton(text, scenesTools);
        b->setToolTip(tip);
        b->setCursor(Qt::PointingHandCursor);
        b->setFixedSize(26, 22);
        b->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        b->setStyleSheet(style);
        return b;
    };

    auto* addScene = makeSceneTool(
        QStringLiteral("+"), QStringLiteral("Add scene"),
        QStringLiteral(
            "QPushButton{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #3A6AFF,stop:1 #1A3AFF);"
            "border:1px solid #6B9AFF;color:#FFFFFF;font-family:'DM Sans';font-size:13px;"
            "font-weight:800;border-radius:3px;padding:0;margin:0;}"
            "QPushButton:hover{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #4A7AFF,stop:1 #2A4AFF);}"));
    connect(addScene, &QPushButton::clicked, this, [this] {
        m_engine->sceneGraph()->mutate([](Project& p) { p.addScene(); });
    });

    auto* remScene = makeSceneTool(QStringLiteral("\u2212"), QStringLiteral("Remove scene"), sceneToolStyle);
    connect(remScene, &QPushButton::clicked, this, [this] {
        if (!m_sceneList || !m_sceneList->currentItem()) return;
        const QString id = m_sceneList->currentItem()->data(Qt::UserRole).toString();
        if (id.isEmpty()) return;
        m_engine->sceneGraph()->mutate([&](Project& p) { p.removeScene(id); });
    });

    auto* dupScene = makeSceneTool(QStringLiteral("⧉"), QStringLiteral("Duplicate scene"), sceneToolStyle);
    connect(dupScene, &QPushButton::clicked, this, [this] {
        if (!m_sceneList || !m_sceneList->currentItem()) return;
        const QString id = m_sceneList->currentItem()->data(Qt::UserRole).toString();
        if (id.isEmpty()) return;
        m_engine->sceneGraph()->mutate([&](Project& p) { p.duplicateScene(id); });
    });

    auto* upScene = makeSceneTool(QStringLiteral("↑"), QStringLiteral("Move scene up"), sceneToolStyle);
    connect(upScene, &QPushButton::clicked, this, [this] {
        if (!m_sceneList || !m_sceneList->currentItem()) return;
        const int row = m_sceneList->currentRow();
        if (row <= 0) return;
        m_engine->sceneGraph()->mutate([&](Project& p) { p.reorderScenes(row, row - 1); });
    });

    auto* downScene = makeSceneTool(QStringLiteral("↓"), QStringLiteral("Move scene down"), sceneToolStyle);
    connect(downScene, &QPushButton::clicked, this, [this] {
        if (!m_sceneList || !m_sceneList->currentItem()) return;
        const int row = m_sceneList->currentRow();
        if (row < 0 || row >= m_sceneList->count() - 1) return;
        m_engine->sceneGraph()->mutate([&](Project& p) { p.reorderScenes(row, row + 1); });
    });

    scenesToolsLay->addWidget(addScene, 0, Qt::AlignVCenter);
    scenesToolsLay->addWidget(remScene, 0, Qt::AlignVCenter);
    scenesToolsLay->addWidget(dupScene, 0, Qt::AlignVCenter);
    scenesToolsLay->addWidget(upScene, 0, Qt::AlignVCenter);
    scenesToolsLay->addWidget(downScene, 0, Qt::AlignVCenter);
    scenesToolsLay->addStretch();
    scenesLay->addWidget(scenesTools);
    m_sceneList = new SceneListWidget(engine, scenesCol);
    scenesLay->addWidget(m_sceneList, 1);

    m_tiles = new InputTilesWidget(engine, nullptr);
    m_tiles->setMinimumWidth(220);
    m_mixer = new AudioMixerWidget(engine, nullptr);
    // OBS: dock can grow; content enforces tick-label minimum via minimumSizeHint()
    m_mixer->setMinimumWidth(m_mixer->minimumSizeHint().width());
    m_scoreboardControls = new ScoreboardControlsWidget(engine, nullptr);
    m_scoreboardControls->setMinimumWidth(200);

    m_scenesDock = makeDock(QStringLiteral("Scenes"), QStringLiteral("scenesDock"),
                            scenesCol, QStringLiteral("#4F9EFF"));
    m_sourcesDock = makeDock(QStringLiteral("Sources"), QStringLiteral("sourcesDock"),
                             m_tiles, QStringLiteral("#FF5A2C"));
    m_mixerDock = makeDock(QStringLiteral("Audio Mixer"), QStringLiteral("mixerDock"),
                           m_mixer, QStringLiteral("#A855F7"));
    m_scoreboardDock = makeDock(QStringLiteral("Scoreboard"), QStringLiteral("scoreboardDock"),
                                m_scoreboardControls, QStringLiteral("#22C55E"));
    m_statsWidget = new ObsStatsDock(engine, nullptr);
    m_statsDock = makeDock(QStringLiteral("Stats"), QStringLiteral("statsDock"),
                           m_statsWidget, QStringLiteral("#38BDF8"));

    applyDefaultDockLayout();
    m_defaultDockState = m_dockHost->saveState();
    restoreExtraBrowserPanels();
    restoreDockState();
    m_dockStateReady = true;

    root->addWidget(m_dockHost, 2);

    m_toolbar = new BottomToolbar(engine, this);
    root->addWidget(m_toolbar);

    m_drawerBackdrop = new QFrame(this);
    m_drawerBackdrop->setObjectName(QStringLiteral("drawerBackdrop"));
    m_drawerBackdrop->hide();
    m_drawerBackdrop->setAttribute(Qt::WA_TransparentForMouseEvents, false);

    // Input settings open as an OBS-style floating dialog (not a fullscreen/side drawer).
    m_props = nullptr;

    m_multi = new MultiCorderPanel(engine, this);
    m_multi->hide();
    connect(m_multi, &MultiCorderPanel::closeRequested, this, [this] { setMultiCorderOpen(false); });

    m_playlist = new PlayListPanel(engine, this);
    m_playlist->hide();
    connect(m_playlist, &PlayListPanel::closeRequested, this, [this] { setPlayListOpen(false); });

    m_drawerBackdrop->installEventFilter(this);

    connect(m_preview, &PreviewWidget::sourceSelected, this, [this](const QString& id) {
        m_engine->setSelectedSourceId(id);
    });
    connect(m_program, &PreviewWidget::sourceSelected, this, [this](const QString& id) {
        m_engine->setSelectedSourceId(id);
    });

    connect(m_contextBar, &SourceContextToolbar::propertiesRequested, this, [this](const QString& id) {
        if (id.isEmpty()) return;
        SourcePropertiesDialog dlg(m_engine, id, this);
        dlg.exec();
    });
    connect(m_contextBar, &SourceContextToolbar::filtersRequested, this, [this](const QString& id) {
        if (id.isEmpty()) return;
        FiltersDialog dlg(m_engine, id, this);
        dlg.exec();
    });
    connect(m_contextBar, &SourceContextToolbar::transformRequested, this, [this](const QString& id) {
        if (id.isEmpty()) return;
        TransformDialog dlg(m_engine, id, this);
        dlg.exec();
    });
    connect(m_contextBar, &SourceContextToolbar::interactRequested, this, [this](const QString& id) {
        if (id.isEmpty()) return;
        InteractDialog dlg(m_engine, id, this);
        dlg.exec();
    });
    auto openInteract = [this](const QString& id) {
        if (id.isEmpty()) return;
        InteractDialog dlg(m_engine, id, this);
        dlg.exec();
    };
    connect(m_preview, &PreviewWidget::interactRequested, this, openInteract);
    connect(m_program, &PreviewWidget::interactRequested, this, openInteract);

    connect(m_toolbar, &BottomToolbar::studioModeToggled, this, &DashboardPage::setStudioMode);
    connect(m_engine, &EngineController::studioModeChanged, this, &DashboardPage::setStudioMode);
    // Default Studio Mode is ON — Preview editable, Program view-only.
    m_engine->setStudioMode(true);
    setStudioMode(true);

    connect(m_tiles, &InputTilesWidget::configureSourceRequested, this, [this](const QString& id) {
        if (id.isEmpty()) return;
        SourcePropertiesDialog dlg(m_engine, id, this);
        dlg.exec();
    });

    connect(m_tiles, &InputTilesWidget::addSourceTypeRequested, this, [this](SourceType type) {
        if (type == SourceType::Unknown) return;
        AddSourceDialog dlg(m_engine, type, this);
        if (dlg.exec() != QDialog::Accepted) return;
        const auto r = dlg.result();
        if (!r.accepted) return;
        const QString id = m_engine->addSource(r.type, r.name, r.settings);
        if (r.type == SourceType::Scoreboard)
            m_engine->pushScoreboardToProgram();
        m_engine->setSelectedSourceId(id);
        if (r.openProperties && !id.isEmpty()) {
            SourcePropertiesDialog props(m_engine, id, this);
            props.exec();
        }
    });

    connect(m_toolbar, &BottomToolbar::addInputRequested, this, [this] {
        const SourceType type = showAddSourceTypeMenu(this, QCursor::pos());
        if (type == SourceType::Unknown) return;
        AddSourceDialog dlg(m_engine, type, this);
        if (dlg.exec() != QDialog::Accepted) return;
        const auto r = dlg.result();
        if (!r.accepted) return;
        const QString id = m_engine->addSource(r.type, r.name, r.settings);
        if (r.type == SourceType::Scoreboard)
            m_engine->pushScoreboardToProgram();
        m_engine->setSelectedSourceId(id);
        if (r.openProperties && !id.isEmpty()) {
            SourcePropertiesDialog props(m_engine, id, this);
            props.exec();
        }
    });

    connect(m_toolbar, &BottomToolbar::goLiveRequested, this, [this] {
        GoLiveDialog dlg(m_engine, this);
        dlg.exec();
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
        menu.addSeparator();
        menu.addAction(QStringLiteral("Reset desk layout"), this, &DashboardPage::resetDockLayout);
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

DashboardPage::~DashboardPage()
{
    saveDockState();
}

QDockWidget* DashboardPage::makeDock(const QString& title, const QString& objectName,
                                     QWidget* content, const QString& accent)
{
    auto* dock = new QDockWidget(title, m_dockHost);
    dock->setObjectName(objectName);
    // Inset under title so tool rows never paint into the header rule.
    auto* frame = new QFrame(dock);
    frame->setObjectName(QStringLiteral("dockContentFrame"));
    auto* frameLay = new QVBoxLayout(frame);
    frameLay->setContentsMargins(0, 6, 0, 0);
    frameLay->setSpacing(0);
    content->setParent(frame);
    frameLay->addWidget(content);
    dock->setWidget(frame);
    dock->setAllowedAreas(Qt::AllDockWidgetAreas);
    dock->setFeatures(QDockWidget::DockWidgetMovable
                      | QDockWidget::DockWidgetFloatable
                      | QDockWidget::DockWidgetClosable);
    dock->setMinimumWidth(140);
    dock->setMinimumHeight(140);
    dock->setTitleBarWidget(new DockTitleBar(dock, title, accent));
    dock->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(dock, &QDockWidget::customContextMenuRequested, this, [this, dock](const QPoint& pos) {
        QMenu menu(dock);
        menu.addAction(QStringLiteral("Reset desk layout"), this, &DashboardPage::resetDockLayout);
        menu.addAction(dock->toggleViewAction());
        menu.exec(dock->mapToGlobal(pos));
    });
    connect(dock, &QDockWidget::dockLocationChanged, this, [this](Qt::DockWidgetArea) {
        scheduleSaveDockState();
    });
    connect(dock, &QDockWidget::topLevelChanged, this, [this](bool) {
        scheduleSaveDockState();
    });
    connect(dock, &QDockWidget::visibilityChanged, this, [this](bool) {
        scheduleSaveDockState();
    });
    return dock;
}

void DashboardPage::applyDefaultDockLayout()
{
    if (!m_dockHost || !m_scenesDock || !m_sourcesDock || !m_mixerDock || !m_scoreboardDock)
        return;

    m_dockHost->removeDockWidget(m_scenesDock);
    m_dockHost->removeDockWidget(m_sourcesDock);
    m_dockHost->removeDockWidget(m_mixerDock);
    m_dockHost->removeDockWidget(m_scoreboardDock);
    if (m_statsDock)
        m_dockHost->removeDockWidget(m_statsDock);

    m_dockHost->addDockWidget(Qt::BottomDockWidgetArea, m_scenesDock);
    m_dockHost->addDockWidget(Qt::BottomDockWidgetArea, m_sourcesDock);
    m_dockHost->addDockWidget(Qt::BottomDockWidgetArea, m_mixerDock);
    m_dockHost->addDockWidget(Qt::BottomDockWidgetArea, m_scoreboardDock);
    m_dockHost->splitDockWidget(m_scenesDock, m_sourcesDock, Qt::Horizontal);
    m_dockHost->splitDockWidget(m_sourcesDock, m_mixerDock, Qt::Horizontal);
    m_dockHost->splitDockWidget(m_mixerDock, m_scoreboardDock, Qt::Horizontal);

    m_scenesDock->show();
    m_sourcesDock->show();
    m_mixerDock->show();
    m_scoreboardDock->show();

    if (m_statsDock) {
        m_dockHost->addDockWidget(Qt::RightDockWidgetArea, m_statsDock);
        m_statsDock->hide();
    }

    // Scenes | Sources | Mixer | Scoreboard
    m_dockHost->resizeDocks({m_scenesDock, m_sourcesDock, m_mixerDock, m_scoreboardDock},
                            {200, 280, 420, 220}, Qt::Horizontal);
}

void DashboardPage::scheduleSaveDockState()
{
    if (!m_dockStateReady) return;
    QTimer::singleShot(250, this, [this] { saveDockState(); });
}

void DashboardPage::saveDockState()
{
    if (!m_dockHost || !m_engine || !m_engine->settings()) return;
    auto ui = m_engine->settings()->uiState();
    ui.insert(QStringLiteral("dashboardDockStateV3"),
              QString::fromLatin1(m_dockHost->saveState().toBase64()));
    ui.remove(QStringLiteral("dashboardDockState"));
    ui.remove(QStringLiteral("dashboardDockStateV2"));
    m_engine->settings()->setUiState(ui);
    m_engine->settings()->sync();
}

void DashboardPage::restoreDockState()
{
    if (!m_dockHost || !m_engine || !m_engine->settings()) return;
    const auto ui = m_engine->settings()->uiState();
    const QString b64 = ui.value(QStringLiteral("dashboardDockStateV3")).toString();
    if (b64.isEmpty()) return;
    const QByteArray state = QByteArray::fromBase64(b64.toLatin1());
    if (!state.isEmpty())
        m_dockHost->restoreState(state);
}

void DashboardPage::resetDockLayout()
{
    m_dockStateReady = false;
    applyDefaultDockLayout();
    m_defaultDockState = m_dockHost ? m_dockHost->saveState() : QByteArray();
    if (m_engine && m_engine->settings()) {
        auto ui = m_engine->settings()->uiState();
        ui.remove(QStringLiteral("dashboardDockState"));
        ui.remove(QStringLiteral("dashboardDockStateV2"));
        ui.remove(QStringLiteral("dashboardDockStateV3"));
        m_engine->settings()->setUiState(ui);
        m_engine->settings()->sync();
    }
    m_dockStateReady = true;
}

void DashboardPage::populateDocksMenu(QMenu* menu)
{
    if (!menu) return;
    menu->clear();
    auto addToggle = [menu](QDockWidget* dock) {
        if (!dock) return;
        QAction* act = dock->toggleViewAction();
        act->setText(dock->windowTitle());
        act->setCheckable(true);
        act->setChecked(dock->isVisible());
        menu->addAction(act);
    };
    addToggle(m_scenesDock);
    addToggle(m_sourcesDock);
    addToggle(m_mixerDock);
    addToggle(m_scoreboardDock);
    addToggle(m_statsDock);
    if (!m_browserDocks.isEmpty()) {
        menu->addSeparator();
        for (auto* d : m_browserDocks)
            addToggle(d);
    }
    menu->addSeparator();
    menu->addAction(QStringLiteral("Add Browser Panel…"), this, [this] { addExtraBrowserPanel(); });
    menu->addAction(QStringLiteral("Reset Desk Layout"), this, &DashboardPage::resetDockLayout);

    // Tools formerly on the bottom Controls strip (OBS puts these under Docks/Tools).
    menu->addSeparator();
    menu->addAction(QStringLiteral("MultiCorder"), this, [this] {
        const bool open = !m_multi->isVisible();
        setMultiCorderOpen(open);
        if (open) setPlayListOpen(false);
    });
    menu->addAction(QStringLiteral("PlayList"), this, [this] {
        const bool open = !m_playlist->isVisible();
        setPlayListOpen(open);
        if (open) setMultiCorderOpen(false);
    });
    menu->addAction(QStringLiteral("Overlays…"), this, [this] {
        QMenu overlay(this);
        populateOverlayMenu(&overlay, m_engine, this);
        overlay.addSeparator();
        overlay.addAction(QStringLiteral("Reset desk layout"), this, &DashboardPage::resetDockLayout);
        overlay.exec(QCursor::pos());
    });
    auto* externalAct = menu->addAction(QStringLiteral("External Output"));
    externalAct->setCheckable(true);
    externalAct->setToolTip(QStringLiteral("Send Program to External (Virtual Camera path)"));
    connect(externalAct, &QAction::toggled, this, [this, externalAct](bool on) {
        QString err;
        if (!m_engine->setExternalOutputEnabled(on, &err)) {
            QMessageBox::warning(this, QStringLiteral("External"), err);
            QSignalBlocker b(externalAct);
            externalAct->setChecked(false);
        }
    });
}

QDockWidget* DashboardPage::createExtraBrowserDock(const QString& panelId, const QString& title,
                                                   const QString& url)
{
    auto* panel = new ExtraBrowserPanel(m_engine, panelId, title, url, nullptr);
    auto* dock = makeDock(title, QStringLiteral("browserDock_") + panelId, panel,
                          QStringLiteral("#F59E0B"));
    connect(panel, &ExtraBrowserPanel::urlChanged, this, [this](const QString&, const QString&) {
        persistExtraBrowserPanels();
    });
    connect(panel, &ExtraBrowserPanel::removeRequested, this, [this, panelId] {
        for (int i = 0; i < m_browserDocks.size(); ++i) {
            auto* d = m_browserDocks[i];
            if (!d) continue;
            if (d->objectName() == QStringLiteral("browserDock_") + panelId) {
                m_dockHost->removeDockWidget(d);
                m_browserDocks.removeAt(i);
                d->deleteLater();
                persistExtraBrowserPanels();
                scheduleSaveDockState();
                break;
            }
        }
    });
    m_dockHost->addDockWidget(Qt::RightDockWidgetArea, dock);
    dock->show();
    m_browserDocks.push_back(dock);
    return dock;
}

void DashboardPage::addExtraBrowserPanel(const QString& title, const QString& url)
{
    bool ok = false;
    QString name = title;
    if (name.isEmpty()) {
        name = QInputDialog::getText(this, QStringLiteral("Browser Panel"),
                                     QStringLiteral("Panel title:"), QLineEdit::Normal,
                                     QStringLiteral("Browser"), &ok);
        if (!ok || name.trimmed().isEmpty()) return;
        name = name.trimmed();
    }
    QString u = url;
    if (u.isEmpty()) {
        u = QInputDialog::getText(this, QStringLiteral("Browser Panel"),
                                  QStringLiteral("URL:"), QLineEdit::Normal,
                                  QStringLiteral("https://"), &ok);
        if (!ok) return;
        u = u.trimmed();
    }
    const QString id = newId(QStringLiteral("browser"));
    createExtraBrowserDock(id, name, u);
    persistExtraBrowserPanels();
    scheduleSaveDockState();
}

void DashboardPage::persistExtraBrowserPanels()
{
    if (!m_engine || !m_engine->settings()) return;
    QJsonArray arr;
    for (auto* dock : m_browserDocks) {
        if (!dock) continue;
        auto* panel = qobject_cast<ExtraBrowserPanel*>(dock->widget());
        if (!panel) continue;
        QJsonObject o;
        o.insert(QStringLiteral("id"), panel->panelId());
        o.insert(QStringLiteral("title"), dock->windowTitle());
        o.insert(QStringLiteral("url"), panel->currentUrl());
        arr.append(o);
    }
    auto ui = m_engine->settings()->uiState();
    ui.insert(QStringLiteral("extraBrowserPanels"), arr);
    m_engine->settings()->setUiState(ui);
    m_engine->settings()->sync();
}

void DashboardPage::restoreExtraBrowserPanels()
{
    if (!m_engine || !m_engine->settings() || !m_dockHost) return;
    const auto ui = m_engine->settings()->uiState();
    const QJsonArray arr = ui.value(QStringLiteral("extraBrowserPanels")).toArray();
    for (const auto& v : arr) {
        const QJsonObject o = v.toObject();
        const QString id = o.value(QStringLiteral("id")).toString();
        if (id.isEmpty()) continue;
        createExtraBrowserDock(id,
                               o.value(QStringLiteral("title")).toString(QStringLiteral("Browser")),
                               o.value(QStringLiteral("url")).toString());
    }
}

void DashboardPage::hideEvent(QHideEvent* event)
{
    saveDockState();
    QWidget::hideEvent(event);
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

void DashboardPage::setStudioMode(bool enabled)
{
    if (m_previewColumn)
        m_previewColumn->setVisible(enabled);
    if (m_transitions)
        m_transitions->setVisible(enabled);
    if (m_contextBar)
        m_contextBar->setVisible(true); // always available for source edits

    // OBS: Studio Mode → Preview editable, Program view-only.
    // Without Studio Mode → single Program monitor is the edit canvas.
    if (m_preview)
        m_preview->setInteractive(enabled);
    if (m_program)
        m_program->setInteractive(!enabled);
}

void DashboardPage::openDrawer()
{
    // Legacy side-drawer path removed — input settings use SourcePropertiesDialog.
}

void DashboardPage::closeDrawer()
{
    if (!m_multi->isVisible() && !m_playlist->isVisible())
        m_drawerBackdrop->hide();
}

void DashboardPage::layoutDrawer()
{
    if (!m_drawerBackdrop) return;
    m_drawerBackdrop->setGeometry(0, 0, width(), height());
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
        if (!m_playlist->isVisible())
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
        if (!m_multi->isVisible())
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
