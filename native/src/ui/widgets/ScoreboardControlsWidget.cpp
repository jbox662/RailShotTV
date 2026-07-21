#include "ui/widgets/ScoreboardControlsWidget.h"
#include "ui/widgets/ScoreboardSettingsDialog.h"
#include "core/EngineController.h"
#include "core/SceneGraph.h"
#include "core/Types.h"
#include "scoreboard/ScoreboardModel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QTimer>
#include <QSignalBlocker>
#include <QScrollArea>
#include <QFrame>
#include <QSizePolicy>
#include <utility>

namespace railshot {

ScoreboardControlsWidget::ScoreboardControlsWidget(EngineController* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setObjectName(QStringLiteral("scoreboardControls"));
    setMinimumWidth(180);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    setStyleSheet(QStringLiteral(
        "QWidget#scoreboardControls {"
        "  background:#0D0F12;"
        "  border:none;"
        "}"
        "QWidget#scoreboardControls QLabel#sec {"
        "  color:#22C55E; font-size:8px; font-weight:900; letter-spacing:1px;"
        "  background:transparent; border:none;"
        "}"
        "QWidget#scoreboardControls QLineEdit, QWidget#scoreboardControls QSpinBox {"
        "  background:#12151A; border:1px solid #3A3D45; color:#E0E2E8;"
        "  border-radius:3px; padding:2px 5px; font-size:10px; min-height:20px; max-height:22px;"
        "}"
        "QWidget#scoreboardControls QLineEdit:focus, QWidget#scoreboardControls QSpinBox:focus {"
        "  border-color:#22C55E;"
        "}"
        "QWidget#scoreboardControls QCheckBox {"
        "  color:#A0A8B8; font-size:9px; font-weight:700; spacing:3px;"
        "  background:transparent;"
        "}"
        "QWidget#scoreboardControls QCheckBox::indicator {"
        "  width:12px; height:12px; border-radius:6px;"
        "  border:1px solid #3A3D45; background:#1A1D22;"
        "}"
        "QWidget#scoreboardControls QCheckBox::indicator:checked {"
        "  background:#22C55E; border-color:#86EFAC;"
        "}"));

    auto* model = engine->scoreboard();

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    // Scroll so a short bottom dock never clips score / match controls.
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setStyleSheet(QStringLiteral(
        "QScrollArea{background:transparent;border:none;}"
        "QScrollBar:vertical{width:7px;background:#0A0C0F;}"
        "QScrollBar::handle:vertical{background:#3A3D45;border-radius:3px;min-height:20px;}"));

    auto* host = new QWidget(scroll);
    host->setObjectName(QStringLiteral("scoreboardHost"));
    host->setStyleSheet(QStringLiteral("QWidget#scoreboardHost{background:transparent;}"));
    auto* lay = new QVBoxLayout(host);
    lay->setContentsMargins(6, 4, 6, 6);
    lay->setSpacing(4);

    auto* topRow = new QHBoxLayout();
    topRow->setSpacing(4);

    auto* overlayBtn = new QPushButton(QStringLiteral("Overlay"), host);
    overlayBtn->setCheckable(true);
    overlayBtn->setChecked(true);
    overlayBtn->setCursor(Qt::PointingHandCursor);
    overlayBtn->setFixedHeight(22);
    overlayBtn->setToolTip(QStringLiteral("Show / hide scoreboard overlay on Program"));
    overlayBtn->setStyleSheet(QStringLiteral(
        "QPushButton{background:#1A1D22;border:1px solid #3A3D45;color:#8892A4;"
        "font-weight:800;font-size:9px;border-radius:3px;padding:1px 8px;}"
        "QPushButton:checked{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #4ADE80,stop:1 #16A34A);"
        "border:1px solid #86EFAC;color:#04140A;}"));
    topRow->addWidget(overlayBtn);

    auto* settingsBtn = new QPushButton(QStringLiteral("Settings"), host);
    settingsBtn->setCursor(Qt::PointingHandCursor);
    settingsBtn->setFixedHeight(22);
    settingsBtn->setToolTip(QStringLiteral("Presets, colors, layout…"));
    settingsBtn->setStyleSheet(QStringLiteral(
        "QPushButton{background:#1A1D22;border:1px solid #3A3D45;color:#C8CAD0;"
        "font-weight:700;font-size:9px;border-radius:3px;padding:1px 8px;}"
        "QPushButton:hover{border-color:#22C55E;color:#FFFFFF;}"));
    topRow->addWidget(settingsBtn);
    topRow->addStretch(1);
    lay->addLayout(topRow);

    // Names side-by-side to save vertical space
    auto* names = new QHBoxLayout();
    names->setSpacing(4);
    auto* aName = new QLineEdit(model->state().playerA, host);
    auto* bName = new QLineEdit(model->state().playerB, host);
    aName->setPlaceholderText(QStringLiteral("Player A"));
    bName->setPlaceholderText(QStringLiteral("Player B"));
    names->addWidget(aName, 1);
    names->addWidget(bName, 1);
    lay->addLayout(names);

    // Score readout + ± on one compact band
    auto* scoreBand = new QHBoxLayout();
    scoreBand->setSpacing(3);
    auto makeScoreBtn = [&](const QString& t, const QString& color) {
        auto* b = new QPushButton(t, host);
        b->setFixedSize(32, 26);
        b->setCursor(Qt::PointingHandCursor);
        b->setStyleSheet(QStringLiteral(
            "QPushButton{background:#1A1D22;border:1px solid %1;color:%1;"
            "font-weight:900;font-size:11px;border-radius:3px;padding:0;}").arg(color));
        return b;
    };
    auto* aMinus = makeScoreBtn(QStringLiteral("A−"), QStringLiteral("#FF5A2C"));
    auto* aPlus = makeScoreBtn(QStringLiteral("A+"), QStringLiteral("#FF5A2C"));
    auto* bMinus = makeScoreBtn(QStringLiteral("B−"), QStringLiteral("#4F9EFF"));
    auto* bPlus = makeScoreBtn(QStringLiteral("B+"), QStringLiteral("#4F9EFF"));

    auto* scoreRead = new QLabel(host);
    scoreRead->setAlignment(Qt::AlignCenter);
    scoreRead->setMinimumWidth(72);
    scoreRead->setStyleSheet(QStringLiteral(
        "font-family:'Bebas Neue','Arial Narrow',sans-serif; font-size:20px; color:#F0F0F0;"
        "background:#12151A; border:1px solid #3A3D45; border-radius:3px; padding:2px 6px;"));
    scoreBand->addWidget(aMinus);
    scoreBand->addWidget(aPlus);
    scoreBand->addWidget(scoreRead, 1);
    scoreBand->addWidget(bMinus);
    scoreBand->addWidget(bPlus);
    lay->addLayout(scoreBand);

    // Match: race-to + clock on one row
    auto* matchRow = new QHBoxLayout();
    matchRow->setSpacing(4);
    auto* raceLab = new QLabel(QStringLiteral("Race"), host);
    raceLab->setStyleSheet(QStringLiteral("color:#8892A4; font-size:9px; background:transparent; border:none;"));
    auto* raceTo = new QSpinBox(host);
    raceTo->setRange(1, 25);
    raceTo->setValue(model->state().raceTo);
    raceTo->setFixedWidth(44);
    auto* clockLbl = new QLabel(QStringLiteral("00:00"), host);
    clockLbl->setStyleSheet(QStringLiteral(
        "font-family:'Bebas Neue','Arial Narrow',sans-serif; font-size:14px; color:#22C55E;"
        "background:transparent; border:none;"));
    auto* clockRun = new QCheckBox(QStringLiteral("Clock"), host);
    clockRun->setChecked(model->state().clockRunning);
    auto* clockReset = new QPushButton(QStringLiteral("0:00"), host);
    clockReset->setFixedHeight(20);
    clockReset->setCursor(Qt::PointingHandCursor);
    clockReset->setToolTip(QStringLiteral("Reset clock"));
    clockReset->setStyleSheet(QStringLiteral(
        "QPushButton{background:#1A1D22;border:1px solid #3A3D45;color:#A0A8B8;"
        "font-size:8px;font-weight:700;padding:1px 5px;}"));
    matchRow->addWidget(raceLab);
    matchRow->addWidget(raceTo);
    matchRow->addStretch(1);
    matchRow->addWidget(clockLbl);
    matchRow->addWidget(clockRun);
    matchRow->addWidget(clockReset);
    lay->addLayout(matchRow);

    auto* actions = new QHBoxLayout();
    actions->setSpacing(3);
    auto* reset = new QPushButton(QStringLiteral("Reset"), host);
    auto* swap = new QPushButton(QStringLiteral("Swap"), host);
    auto* push = new QPushButton(QStringLiteral("Push"), host);
    for (auto* b : {reset, swap}) {
        b->setCursor(Qt::PointingHandCursor);
        b->setFixedHeight(22);
        b->setStyleSheet(QStringLiteral(
            "QPushButton{background:#1A1D22;border:1px solid #3A3D45;color:#C8CAD0;"
            "font-size:9px;font-weight:800;padding:2px 4px;border-radius:3px;}"));
    }
    push->setCursor(Qt::PointingHandCursor);
    push->setFixedHeight(22);
    push->setStyleSheet(QStringLiteral(
        "QPushButton{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #4ADE80,stop:1 #16A34A);"
        "border:1px solid #86EFAC;color:#04140A;font-size:9px;font-weight:900;padding:2px 4px;border-radius:3px;}"));
    actions->addWidget(reset, 1);
    actions->addWidget(swap, 1);
    actions->addWidget(push, 1);
    lay->addLayout(actions);
    lay->addStretch(1);

    scroll->setWidget(host);
    outer->addWidget(scroll, 1);

    connect(settingsBtn, &QPushButton::clicked, this, [this, engine] {
        ScoreboardSettingsDialog dlg(engine, this);
        dlg.exec();
    });

    connect(raceTo, QOverload<int>::of(&QSpinBox::valueChanged), this, [model](int v) {
        auto st = model->state();
        st.raceTo = v;
        model->setState(st);
    });

    connect(aPlus, &QPushButton::clicked, model, [model] { model->incrementA(1); });
    connect(aMinus, &QPushButton::clicked, model, [model] { model->incrementA(-1); });
    connect(bPlus, &QPushButton::clicked, model, [model] { model->incrementB(1); });
    connect(bMinus, &QPushButton::clicked, model, [model] { model->incrementB(-1); });
    connect(reset, &QPushButton::clicked, model, &ScoreboardModel::resetScores);
    connect(swap, &QPushButton::clicked, this, [model] {
        auto s = model->state();
        std::swap(s.playerA, s.playerB);
        std::swap(s.scoreA, s.scoreB);
        model->setState(s);
    });
    connect(push, &QPushButton::clicked, engine, &EngineController::pushScoreboardToProgram);
    connect(aName, &QLineEdit::textChanged, this, [model](const QString& t) {
        auto s = model->state();
        s.playerA = t;
        model->setState(s);
    });
    connect(bName, &QLineEdit::textChanged, this, [model](const QString& t) {
        auto s = model->state();
        s.playerB = t;
        model->setState(s);
    });
    connect(clockRun, &QCheckBox::toggled, this, [model](bool on) {
        if (on) model->startClock();
        else model->stopClock();
    });
    connect(clockReset, &QPushButton::clicked, this, [model] {
        auto s = model->state();
        s.clockSeconds = 0;
        model->setState(s);
    });
    connect(overlayBtn, &QPushButton::toggled, this, [=](bool on) {
        engine->sceneGraph()->mutate([&](Project& p) {
            for (auto& scene : p.scenes) {
                for (auto& src : scene.sources) {
                    if (src.type == SourceType::Scoreboard)
                        src.visible = on;
                }
            }
        });
        engine->pushScoreboardToProgram();
    });

    auto refresh = [=](const ScoreboardState& st) {
        scoreRead->setText(QStringLiteral("%1 — %2").arg(st.scoreA).arg(st.scoreB));
        scoreRead->setToolTip(QStringLiteral("%1  %2 — %3  %4")
                                  .arg(st.playerA)
                                  .arg(st.scoreA)
                                  .arg(st.scoreB)
                                  .arg(st.playerB));
        const int m = st.clockSeconds / 60;
        const int sec = st.clockSeconds % 60;
        clockLbl->setText(QStringLiteral("%1:%2")
                              .arg(m, 2, 10, QLatin1Char('0'))
                              .arg(sec, 2, 10, QLatin1Char('0')));
        if (aName->text() != st.playerA)
            aName->setText(st.playerA);
        if (bName->text() != st.playerB)
            bName->setText(st.playerB);
        if (raceTo->value() != st.raceTo)
            raceTo->setValue(st.raceTo);
        if (clockRun->isChecked() != st.clockRunning)
            clockRun->setChecked(st.clockRunning);

        bool anyVisible = false;
        bool anyScoreboard = false;
        const auto proj = engine->projectSnapshot();
        for (const auto& scene : proj.scenes) {
            for (const auto& src : scene.sources) {
                if (src.type != SourceType::Scoreboard) continue;
                anyScoreboard = true;
                if (src.visible) anyVisible = true;
            }
        }
        if (anyScoreboard && overlayBtn->isChecked() != anyVisible) {
            QSignalBlocker b(overlayBtn);
            overlayBtn->setChecked(anyVisible);
        }
    };
    connect(model, &ScoreboardModel::changed, this, refresh);
    refresh(model->state());

    auto* clock = new QTimer(this);
    connect(clock, &QTimer::timeout, model, &ScoreboardModel::tickClock);
    clock->start(1000);
}

} // namespace railshot
