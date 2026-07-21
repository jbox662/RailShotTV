#include "ui/widgets/TopMenuBar.h"
#include "core/EngineController.h"
#include "core/SettingsStore.h"
#include "ui/Theme.h"
#include <QSignalBlocker>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QSizePolicy>
#include <QFrame>
#include <QMessageBox>
#include <QStyle>

namespace railshot {

TopMenuBar::TopMenuBar(EngineController* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setObjectName(QStringLiteral("topMenuBar"));
    setFixedHeight(32);
    // MUST scope to #topMenuBar — an unscoped sheet flattens every child QPushButton.
    setStyleSheet(QStringLiteral(
        "QWidget#topMenuBar {"
        "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #1E2228, stop:1 #12151A);"
        "  border-bottom: 1px solid #3A3D45;"
        "}"
        "QWidget#topMenuBar QMenuBar {"
        "  background: transparent;"
        "  border: none;"
        "  spacing: 2px;"
        "  color: #E0E2E8;"
        "  font-size: 10px;"
        "  font-weight: 600;"
        "}"
        "QWidget#topMenuBar QMenuBar::item {"
        "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #32363F, stop:1 #1A1E26);"
        "  border: 1px solid #5A5E68;"
        "  border-radius: 3px;"
        "  color: #E0E2E8;"
        "  padding: 2px 10px;"
        "  margin: 0 1px;"
        "}"
        "QWidget#topMenuBar QMenuBar::item:selected {"
        "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #3A404C, stop:1 #242830);"
        "  border: 1px solid #4F9EFF;"
        "  color: #FFFFFF;"
        "}"
        "QWidget#topMenuBar QMenuBar::item:pressed {"
        "  background: #12151A;"
        "  border-color: #3A3D45;"
        "}"
        "QWidget#topMenuBar QPushButton#menuChromeBtn {"
        "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #32363F, stop:1 #1A1E26);"
        "  border: 1px solid #5A5E68;"
        "  border-radius: 3px;"
        "  color: #E0E2E8;"
        "  font-size: 10px;"
        "  font-weight: 600;"
        "  padding: 2px 8px;"
        "  min-height: 20px;"
        "  max-height: 22px;"
        "}"
        "QWidget#topMenuBar QPushButton#menuChromeBtn:hover {"
        "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #3A404C, stop:1 #242830);"
        "  border: 1px solid #4F9EFF;"
        "  color: #FFFFFF;"
        "}"
        "QWidget#topMenuBar QPushButton#menuChromeBtn:pressed {"
        "  background: #12151A;"
        "  border-color: #3A3D45;"
        "}"
        "QWidget#topMenuBar QPushButton#menuChromeBtn:checked {"
        "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #1A3A28, stop:1 #122018);"
        "  border: 1px solid #22C55E;"
        "  color: #4ADE80;"
        "}"
        "QWidget#topMenuBar QLabel#mono {"
        "  font-family:'JetBrains Mono','Consolas',monospace;"
        "  font-size:9px;"
        "  color:#8892A4;"
        "  background: #0A0C0F;"
        "  border: 1px solid #3A3D45;"
        "  border-radius: 2px;"
        "  padding: 2px 8px;"
        "}"));
    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(8, 3, 8, 3);
    row->setSpacing(3);

    auto* brandRow = new QWidget(this);
    brandRow->setStyleSheet(QStringLiteral("background:transparent;border:none;"));
    auto* brandLay = new QHBoxLayout(brandRow);
    brandLay->setContentsMargins(0, 0, 0, 0);
    brandLay->setSpacing(6);
    auto* logo = new QLabel(brandRow);
    logo->setFixedSize(22, 22);
    logo->setScaledContents(false);
    logo->setAlignment(Qt::AlignCenter);
    logo->setStyleSheet(QStringLiteral("background:transparent;border:none;"));
    const QPixmap logoPm = theme::appLogoPixmap(22);
    if (!logoPm.isNull())
        logo->setPixmap(logoPm);
    brandLay->addWidget(logo);
    auto* brand = new QLabel(brandRow);
    brand->setObjectName(QStringLiteral("brandWordmark"));
    brand->setText(QStringLiteral(
        "<span style='font-family:\"Bebas Neue\",\"Arial Narrow\",sans-serif; font-size:15px; letter-spacing:0.05em;'>"
        "<span style='color:#F8F8FF;font-weight:400;'>RAILSHOT</span>"
        "<span style='color:#FF5A2C;font-weight:400;'> TV</span></span>"));
    brand->setStyleSheet(QStringLiteral("background:transparent; border:none;"));
    brandLay->addWidget(brand);
    row->addWidget(brandRow);
    row->addSpacing(8);

    auto* menuBar = new QMenuBar(this);
    menuBar->setNativeMenuBar(false);
    menuBar->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

    auto* fileMenu = menuBar->addMenu(QStringLiteral("File"));
    fileMenu->addAction(QStringLiteral("New"), this, &TopMenuBar::newProject);
    fileMenu->addAction(QStringLiteral("Open…"), this, &TopMenuBar::openProject);
    fileMenu->addAction(QStringLiteral("Save"), this, &TopMenuBar::saveProject);
    fileMenu->addAction(QStringLiteral("Save As…"), this, &TopMenuBar::saveProjectAs);
    fileMenu->addAction(QStringLiteral("Open Last"), this, &TopMenuBar::openLastProject);
    fileMenu->addSeparator();
    fileMenu->addAction(QStringLiteral("Save New Preset…"), this, [this] { emit saveProjectAs(); });
    fileMenu->addAction(QStringLiteral("Load Last Preset"), this, [this] { emit openLastProject(); });
    fileMenu->addSeparator();
    fileMenu->addAction(QStringLiteral("Profiles…"), this, &TopMenuBar::openProfiles);
    fileMenu->addAction(QStringLiteral("Scene Collections…"), this, &TopMenuBar::openCollections);

    auto* editMenu = menuBar->addMenu(QStringLiteral("Edit"));
    editMenu->addAction(QStringLiteral("Advanced Audio Properties…"), this, &TopMenuBar::openAdvAudio);
    editMenu->addSeparator();
    editMenu->addAction(QStringLiteral("Remux Recordings…"), this, &TopMenuBar::openRemux);
    editMenu->addAction(QStringLiteral("Missing Files…"), this, &TopMenuBar::openMissingFiles);

    auto* viewMenu = menuBar->addMenu(QStringLiteral("View"));
    viewMenu->addAction(QStringLiteral("Preview Projector (Windowed)"), this, [this] {
        emit openProjector(false, false);
    });
    viewMenu->addAction(QStringLiteral("Preview Projector (Fullscreen)"), this, [this] {
        emit openProjector(false, true);
    });
    viewMenu->addSeparator();
    viewMenu->addAction(QStringLiteral("Program Projector (Windowed)"), this, [this] {
        emit openProjector(true, false);
    });
    viewMenu->addAction(QStringLiteral("Program Projector (Fullscreen)"), this, [this] {
        emit openProjector(true, true);
    });
    viewMenu->addSeparator();
    viewMenu->addAction(QStringLiteral("Multiview (Windowed)"), this, [this] {
        emit openMultiview(false);
    });
    viewMenu->addAction(QStringLiteral("Multiview (Fullscreen)"), this, [this] {
        emit openMultiview(true);
    });
    viewMenu->addSeparator();
    viewMenu->addAction(QStringLiteral("Screenshot (Preview)"), this, &TopMenuBar::screenshotPreview);
    viewMenu->addAction(QStringLiteral("Screenshot (Program)"), this, &TopMenuBar::screenshotProgram);
    viewMenu->addSeparator();
    viewMenu->addAction(QStringLiteral("Virtual Camera…"), this, &TopMenuBar::openVCamConfig);
    viewMenu->addAction(QStringLiteral("Current Log…"), this, &TopMenuBar::openLogViewer);

    m_docksMenu = menuBar->addMenu(QStringLiteral("Docks"));
    connect(m_docksMenu, &QMenu::aboutToShow, this, [this] {
        emit docksMenuAboutToShow(m_docksMenu);
    });

    row->addWidget(menuBar);
    row->addStretch();

    auto addChrome = [&](const QString& text) {
        auto* b = new QPushButton(text, this);
        b->setObjectName(QStringLiteral("menuChromeBtn"));
        b->setCursor(Qt::PointingHandCursor);
        b->setFocusPolicy(Qt::NoFocus);
        row->addWidget(b);
        return b;
    };

    m_fullscreen = addChrome(QStringLiteral("Fullscreen"));
    m_fullscreen->setCheckable(true);
    connect(m_fullscreen, &QPushButton::toggled, this, [this](bool on) {
        m_fullscreenOn = on;
        m_fullscreen->setText(on ? QStringLiteral("Exit Fullscreen") : QStringLiteral("Fullscreen"));
        if (auto* w = window()) {
            if (on) w->showFullScreen();
            else w->showNormal();
        }
    });

    auto* div1 = new QFrame(this);
    div1->setFixedSize(1, 16);
    div1->setStyleSheet(QStringLiteral("background:#5A5E68; border:none;"));
    row->addWidget(div1);
    row->addSpacing(4);

    m_status = new QLabel(QStringLiteral("1080p29.97   EX FPS: 30   CPU: 3%"), this);
    m_status->setObjectName(QStringLiteral("mono"));
    row->addWidget(m_status);

    auto* div2 = new QFrame(this);
    div2->setFixedSize(1, 16);
    div2->setStyleSheet(QStringLiteral("background:#5A5E68; border:none;"));
    row->addWidget(div2);
    row->addSpacing(4);

    m_pauseInputs = addChrome(QStringLiteral("Pause Inputs"));
    m_pauseInputs->setCheckable(true);
    connect(m_pauseInputs, &QPushButton::toggled, this, [this](bool on) {
        m_inputsPaused = on;
        m_pauseInputs->setText(on ? QStringLiteral("▐▐ Paused") : QStringLiteral("Pause Inputs"));
        m_pauseInputs->setStyleSheet(on
            ? QStringLiteral(
                  "QPushButton { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #4A3018,stop:1 #281E14);"
                  "border:1px solid #F97316; border-radius:3px; color:#FDBA74; font-size:10px; font-weight:700; padding:2px 8px; max-height:22px; }")
            : QString());
        if (!on) {
            m_pauseInputs->setObjectName(QStringLiteral("menuChromeBtn"));
            m_pauseInputs->style()->unpolish(m_pauseInputs);
            m_pauseInputs->style()->polish(m_pauseInputs);
        }
        if (m_engine)
            m_engine->setInputsPaused(on);
    });

    m_basic = addChrome(QStringLiteral("Basic"));
    m_basic->setCheckable(true);
    connect(m_basic, &QPushButton::toggled, this, [this](bool on) {
        setBasicMode(on);
        emit basicModeChanged(on);
        if (m_engine) {
            auto ui = m_engine->settings()->uiState();
            ui.insert(QStringLiteral("basicMode"), on);
            m_engine->settings()->setUiState(ui);
        }
    });
    connect(addChrome(QStringLiteral("Settings")), &QPushButton::clicked, this, &TopMenuBar::openSettings);

    m_help = new QPushButton(QStringLiteral("?"), this);
    m_help->setObjectName(QStringLiteral("menuChromeBtn"));
    m_help->setFixedSize(22, 22);
    m_help->setCheckable(true);
    m_help->setCursor(Qt::PointingHandCursor);
    m_help->setToolTip(QStringLiteral("Keyboard Shortcuts (Shift+?)"));
    m_help->setStyleSheet(QStringLiteral(
        "QPushButton { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #32363F,stop:1 #1A1E26);"
        "border:1px solid #5A5E68; border-radius:3px; color:#C8CAD0; font-size:12px; font-weight:800; }"
        "QPushButton:checked { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #3A6AFF,stop:1 #2A50CC);"
        "border:1px solid #8AB4FF; color:#fff; }"
        "QPushButton:hover { border-color:#4F9EFF; color:#fff; }"));
    connect(m_help, &QPushButton::clicked, this, &TopMenuBar::toggleShortcuts);
    row->addWidget(m_help);

    connect(m_engine, &EngineController::telemetryUpdated, this, [this](const TelemetrySnapshot& s) {
        const int cpu = int(s.cpuPercent);
        OutputProfile profile;
        if (m_engine) {
            profile = m_engine->projectSnapshot().output;
            if (profile.width <= 0)
                profile = m_engine->settings()->outputProfile();
        }
        const int w = profile.width > 0 ? profile.width : 1920;
        const int h = profile.height > 0 ? profile.height : 1080;
        const double fps = profile.fps > 0 ? profile.fps : 59.94;
        m_status->setText(QStringLiteral("%1p%2   EX FPS: %3   CPU: %4%")
                              .arg(h)
                              .arg(fps, 0, 'f', fps == int(fps) ? 0 : 2)
                              .arg(int(s.fpsRender > 0 ? s.fpsRender : fps))
                              .arg(s.streaming ? qMax(cpu, 12) : qMax(cpu, 3)));
        Q_UNUSED(w);
    });

    if (m_engine) {
        const bool basic = m_engine->settings()->uiState().value(QStringLiteral("basicMode")).toBool(false);
        if (basic)
            setBasicMode(true);
    }
}

void TopMenuBar::setBasicMode(bool on)
{
    m_basicMode = on;
    if (m_basic) {
        QSignalBlocker block(m_basic);
        m_basic->setChecked(on);
        m_basic->setText(on ? QStringLiteral("Advanced") : QStringLiteral("Basic"));
        m_basic->setStyleSheet(on
            ? QStringLiteral(
                  "QPushButton{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #1A3A28,stop:1 #122018);"
                  "border:1px solid #22C55E;border-radius:3px;color:#4ADE80;font-size:10px;font-weight:700;padding:2px 8px;max-height:22px;}")
            : QString());
        if (!on) {
            m_basic->setObjectName(QStringLiteral("menuChromeBtn"));
            m_basic->style()->unpolish(m_basic);
            m_basic->style()->polish(m_basic);
        }
    }
}

} // namespace railshot
