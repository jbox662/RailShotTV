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

namespace railshot {

TopMenuBar::TopMenuBar(EngineController* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setFixedHeight(36);
    setStyleSheet(QStringLiteral(
        "background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #1A1D22, stop:1 #141619);"
        "border-bottom: 1px solid #2A2D35;"));
    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(8, 0, 8, 0);
    row->setSpacing(2);

    auto* brand = new QLabel(this);
    brand->setObjectName(QStringLiteral("brandWordmark"));
    brand->setText(QStringLiteral(
        "<span style='font-family:\"Bebas Neue\",\"Arial Narrow\",sans-serif; font-size:16px; letter-spacing:1px;'>"
        "<span style='color:#F0F0F0;'>RAILSHOT</span>"
        "<span style='color:#FF5A2C;'>TV</span></span>"));
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
    div1->setFixedSize(1, 20);
    div1->setStyleSheet(QStringLiteral("background:#3A3D45;"));
    row->addWidget(div1);
    row->addSpacing(6);

    m_status = new QLabel(QStringLiteral("1080p29.97   EX FPS: 30   CPU: 3%"), this);
    m_status->setObjectName(QStringLiteral("mono"));
    m_status->setStyleSheet(QStringLiteral(
        "font-family:'JetBrains Mono','Consolas',monospace; font-size:10px; color:#606878;"));
    row->addWidget(m_status);

    auto* div2 = new QFrame(this);
    div2->setFixedSize(1, 20);
    div2->setStyleSheet(QStringLiteral("background:#3A3D45;"));
    row->addWidget(div2);
    row->addSpacing(6);

    m_pauseInputs = addChrome(QStringLiteral("Pause Inputs"));
    m_pauseInputs->setCheckable(true);
    connect(m_pauseInputs, &QPushButton::toggled, this, [this](bool on) {
        m_inputsPaused = on;
        m_pauseInputs->setText(on ? QStringLiteral("▐▐ Paused") : QStringLiteral("Pause Inputs"));
        m_pauseInputs->setStyleSheet(on
            ? QStringLiteral(
                  "QPushButton { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #3A2A1A,stop:1 #281E14);"
                  "border:1px solid #F9731660; border-radius:3px; color:#F97316; font-size:11px; padding:3px 10px; }")
            : QString());
        if (!on) m_pauseInputs->setObjectName(QStringLiteral("menuChromeBtn"));
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
    m_help->setFixedSize(26, 26);
    m_help->setCheckable(true);
    m_help->setCursor(Qt::PointingHandCursor);
    m_help->setToolTip(QStringLiteral("Keyboard Shortcuts (Shift+?)"));
    m_help->setStyleSheet(QStringLiteral(
        "QPushButton { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #1E2128,stop:1 #16181E);"
        "border:1px solid #3A3D45; border-radius:3px; color:#808898; font-size:13px; font-weight:700; }"
        "QPushButton:checked { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #3A6AFF,stop:1 #2A50CC);"
        "border:1px solid #5A8AFF; color:#fff; }"));
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
                  "QPushButton{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #1A3A2A,stop:1 #122018);"
                  "border:1px solid #22C55E60;border-radius:3px;color:#22C55E;font-size:11px;padding:3px 10px;}")
            : QString());
        if (!on) m_basic->setObjectName(QStringLiteral("menuChromeBtn"));
    }
}

} // namespace railshot
