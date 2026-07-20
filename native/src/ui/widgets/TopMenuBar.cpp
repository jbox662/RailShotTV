#include "ui/widgets/TopMenuBar.h"
#include "core/EngineController.h"
#include "core/SettingsStore.h"
#include "ui/Theme.h"
#include <QSignalBlocker>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QMenu>
#include <QFrame>
#include <QMessageBox>
#include <QStyle>

namespace railshot {

TopMenuBar::TopMenuBar(EngineController* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setObjectName(QStringLiteral("topMenuBar"));
    setFixedHeight(40);
    // MUST scope to #topMenuBar — an unscoped sheet flattens every child QPushButton.
    setStyleSheet(QStringLiteral(
        "QWidget#topMenuBar {"
        "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #22262E, stop:1 #12151A);"
        "  border-bottom: 2px solid #4A4D55;"
        "}"
        "QWidget#topMenuBar QPushButton#menuChromeBtn {"
        "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #3A404C, stop:1 #1A1E26);"
        "  border: 2px solid #6A7080;"
        "  border-radius: 4px;"
        "  color: #F0F2F8;"
        "  font-size: 11px;"
        "  font-weight: 700;"
        "  padding: 5px 12px;"
        "  min-height: 24px;"
        "}"
        "QWidget#topMenuBar QPushButton#menuChromeBtn:hover {"
        "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #4A5568, stop:1 #2A3040);"
        "  border: 2px solid #4F9EFF;"
        "  color: #FFFFFF;"
        "}"
        "QWidget#topMenuBar QPushButton#menuChromeBtn:pressed {"
        "  background: #12151A;"
        "  border-color: #3A3D45;"
        "  padding-top: 6px;"
        "  padding-bottom: 4px;"
        "}"
        "QWidget#topMenuBar QPushButton#menuChromeBtn:checked {"
        "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #1A3A28, stop:1 #122018);"
        "  border: 2px solid #22C55E;"
        "  color: #4ADE80;"
        "}"
        "QWidget#topMenuBar QLabel#mono {"
        "  font-family:'JetBrains Mono','Consolas',monospace;"
        "  font-size:10px;"
        "  color:#A0A8B8;"
        "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #1A1D22, stop:1 #0A0C0F);"
        "  border: 1px solid #4A4D55;"
        "  border-radius: 3px;"
        "  padding: 4px 10px;"
        "}"));
    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(10, 4, 10, 4);
    row->setSpacing(5);

    auto* brand = new QLabel(this);
    brand->setObjectName(QStringLiteral("brandWordmark"));
    brand->setText(QStringLiteral(
        "<span style='font-family:\"Bebas Neue\",\"Arial Narrow\",sans-serif; font-size:18px; letter-spacing:0.06em;'>"
        "<span style='color:#F8F8FF;font-weight:400;'>RAILSHOT</span>"
        "<span style='color:#FF5A2C;font-weight:400;'> TV</span></span>"));
    brand->setStyleSheet(QStringLiteral("background:transparent; border:none;"));
    row->addWidget(brand);
    row->addSpacing(12);

    auto addChrome = [&](const QString& text) {
        auto* b = new QPushButton(text, this);
        b->setObjectName(QStringLiteral("menuChromeBtn"));
        b->setCursor(Qt::PointingHandCursor);
        b->setFocusPolicy(Qt::NoFocus);
        row->addWidget(b);
        return b;
    };

    auto* preset = addChrome(QStringLiteral("Preset"));
    auto* presetMenu = new QMenu(preset);
    presetMenu->addAction(QStringLiteral("Save New Preset…"), this, [this] {
        QMessageBox::information(this, QStringLiteral("Preset"),
                                 QStringLiteral("Use Save / Save As to store the project."));
    });
    presetMenu->addAction(QStringLiteral("Load Last Preset"), this, [this] { emit openProject(); });
    presetMenu->addSeparator();
    presetMenu->addAction(QStringLiteral("Manage Presets…"), this, [this] { emit openSettings(); });
    preset->setMenu(presetMenu);

    connect(addChrome(QStringLiteral("New")), &QPushButton::clicked, this, &TopMenuBar::newProject);
    connect(addChrome(QStringLiteral("Open")), &QPushButton::clicked, this, &TopMenuBar::openProject);
    connect(addChrome(QStringLiteral("Save")), &QPushButton::clicked, this, &TopMenuBar::saveProject);
    connect(addChrome(QStringLiteral("Save As")), &QPushButton::clicked, this, &TopMenuBar::saveProject);
    connect(addChrome(QStringLiteral("Last")), &QPushButton::clicked, this, &TopMenuBar::openProject);

    row->addStretch();

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
    div1->setFixedSize(2, 22);
    div1->setStyleSheet(QStringLiteral("background:#5A5E68; border:none;"));
    row->addWidget(div1);
    row->addSpacing(6);

    m_status = new QLabel(QStringLiteral("1080p29.97   EX FPS: 30   CPU: 3%"), this);
    m_status->setObjectName(QStringLiteral("mono"));
    row->addWidget(m_status);

    auto* div2 = new QFrame(this);
    div2->setFixedSize(2, 22);
    div2->setStyleSheet(QStringLiteral("background:#5A5E68; border:none;"));
    row->addWidget(div2);
    row->addSpacing(6);

    m_pauseInputs = addChrome(QStringLiteral("Pause Inputs"));
    m_pauseInputs->setCheckable(true);
    connect(m_pauseInputs, &QPushButton::toggled, this, [this](bool on) {
        m_inputsPaused = on;
        m_pauseInputs->setText(on ? QStringLiteral("▐▐ Paused") : QStringLiteral("Pause Inputs"));
        m_pauseInputs->setStyleSheet(on
            ? QStringLiteral(
                  "QPushButton { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #4A3018,stop:1 #281E14);"
                  "border:2px solid #F97316; border-radius:4px; color:#FDBA74; font-size:11px; font-weight:800; padding:5px 12px; }")
            : QString());
        if (!on) {
            m_pauseInputs->setObjectName(QStringLiteral("menuChromeBtn"));
            m_pauseInputs->style()->unpolish(m_pauseInputs);
            m_pauseInputs->style()->polish(m_pauseInputs);
        }
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
    m_help->setFixedSize(28, 28);
    m_help->setCheckable(true);
    m_help->setCursor(Qt::PointingHandCursor);
    m_help->setToolTip(QStringLiteral("Keyboard Shortcuts (Shift+?)"));
    m_help->setStyleSheet(QStringLiteral(
        "QPushButton { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #3A404C,stop:1 #1A1E26);"
        "border:2px solid #6A7080; border-radius:4px; color:#C8CAD0; font-size:14px; font-weight:900; }"
        "QPushButton:checked { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #3A6AFF,stop:1 #2A50CC);"
        "border:2px solid #8AB4FF; color:#fff; }"
        "QPushButton:hover { border-color:#4F9EFF; color:#fff; }"));
    connect(m_help, &QPushButton::clicked, this, &TopMenuBar::toggleShortcuts);
    row->addWidget(m_help);

    connect(m_engine, &EngineController::telemetryUpdated, this, [this](const TelemetrySnapshot& s) {
        const int cpu = int(s.cpuPercent);
        m_status->setText(QStringLiteral("1080p29.97   EX FPS: %1   CPU: %2%")
                              .arg(int(s.fpsRender > 0 ? s.fpsRender : 30))
                              .arg(s.streaming ? qMax(cpu, 12) : qMax(cpu, 3)));
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
                  "border:2px solid #22C55E;border-radius:4px;color:#4ADE80;font-size:11px;font-weight:800;padding:5px 12px;}")
            : QString());
        if (!on) {
            m_basic->setObjectName(QStringLiteral("menuChromeBtn"));
            m_basic->style()->unpolish(m_basic);
            m_basic->style()->polish(m_basic);
        }
    }
}

} // namespace railshot
