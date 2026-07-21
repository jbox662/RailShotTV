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
#include <utility>

namespace railshot {

ScoreboardControlsWidget::ScoreboardControlsWidget(EngineController* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setObjectName(QStringLiteral("scoreboardControls"));
    setMinimumWidth(200);
    setStyleSheet(QStringLiteral(
        "QWidget#scoreboardControls {"
        "  background:#0D0F12;"
        "  border:none;"
        "}"
        "QWidget#scoreboardControls QLabel#sec {"
        "  color:#22C55E; font-size:9px; font-weight:900; letter-spacing:1px;"
        "  background:transparent; border:none;"
        "}"
        "QWidget#scoreboardControls QLineEdit, QWidget#scoreboardControls QSpinBox {"
        "  background:#12151A; border:1px solid #3A3D45; color:#E0E2E8;"
        "  border-radius:3px; padding:3px 6px; font-size:11px;"
        "}"
        "QWidget#scoreboardControls QLineEdit:focus, QWidget#scoreboardControls QSpinBox:focus {"
        "  border-color:#22C55E;"
        "}"
        "QWidget#scoreboardControls QCheckBox {"
        "  color:#A0A8B8; font-size:10px; font-weight:700; spacing:4px;"
        "  background:transparent;"
        "}"
        "QWidget#scoreboardControls QCheckBox::indicator {"
        "  width:14px; height:14px; border-radius:7px;"
        "  border:1px solid #3A3D45; background:#1A1D22;"
        "}"
        "QWidget#scoreboardControls QCheckBox::indicator:checked {"
        "  background:#22C55E; border-color:#86EFAC;"
        "}"));

    auto* model = engine->scoreboard();
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(8, 8, 8, 8);
    lay->setSpacing(6);

    // Compact quick controls: overlay toggle + settings
    auto* topRow = new QHBoxLayout();
    topRow->setSpacing(6);

    auto* overlayBtn = new QPushButton(QStringLiteral("Overlay"), this);
    overlayBtn->setCheckable(true);
    overlayBtn->setChecked(true);
    overlayBtn->setCursor(Qt::PointingHandCursor);
    overlayBtn->setFixedHeight(24);
    overlayBtn->setToolTip(QStringLiteral("Show / hide scoreboard overlay on Program"));
    overlayBtn->setStyleSheet(QStringLiteral(
        "QPushButton{background:#1A1D22;border:1px solid #3A3D45;color:#8892A4;"
        "font-weight:800;font-size:10px;border-radius:3px;padding:2px 10px;}"
        "QPushButton:checked{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #4ADE80,stop:1 #16A34A);"
        "border:1px solid #86EFAC;color:#04140A;}"));
    topRow->addWidget(overlayBtn);

    auto* settingsBtn = new QPushButton(QStringLiteral("Settings"), this);
    settingsBtn->setCursor(Qt::PointingHandCursor);
    settingsBtn->setFixedHeight(24);
    settingsBtn->setToolTip(QStringLiteral("Sport, layout, theme…"));
    settingsBtn->setStyleSheet(QStringLiteral(
        "QPushButton{background:#1A1D22;border:1px solid #3A3D45;color:#C8CAD0;"
        "font-weight:700;font-size:10px;border-radius:3px;padding:2px 10px;}"
        "QPushButton:hover{border-color:#22C55E;color:#FFFFFF;}"));
    topRow->addWidget(settingsBtn);
    topRow->addStretch(1);
    lay->addLayout(topRow);

    auto* teamsLab = new QLabel(QStringLiteral("TEAMS"), this);
    teamsLab->setObjectName(QStringLiteral("sec"));
    lay->addWidget(teamsLab);

    auto* aName = new QLineEdit(model->state().playerA, this);
    auto* bName = new QLineEdit(model->state().playerB, this);
    aName->setPlaceholderText(QStringLiteral("Player A"));
    bName->setPlaceholderText(QStringLiteral("Player B"));
    lay->addWidget(aName);
    lay->addWidget(bName);

    auto* scoreRead = new QLabel(this);
    scoreRead->setAlignment(Qt::AlignCenter);
    scoreRead->setStyleSheet(QStringLiteral(
        "font-family:'Bebas Neue'; font-size:22px; color:#F0F0F0; background:#12151A;"
        "border:1px solid #3A3D45; border-radius:3px; padding:4px;"));
    lay->addWidget(scoreRead);

    auto* scoreRow = new QHBoxLayout();
    scoreRow->setSpacing(4);
    auto makeScoreBtn = [&](const QString& t, const QString& color) {
        auto* b = new QPushButton(t, this);
        b->setFixedHeight(28);
        b->setCursor(Qt::PointingHandCursor);
        b->setStyleSheet(QStringLiteral(
            "QPushButton{background:#1A1D22;border:1px solid %1;color:%1;"
            "font-weight:900;font-size:14px;border-radius:3px;}").arg(color));
        return b;
    };
    auto* aMinus = makeScoreBtn(QStringLiteral("A−"), QStringLiteral("#FF5A2C"));
    auto* aPlus = makeScoreBtn(QStringLiteral("A+"), QStringLiteral("#FF5A2C"));
    auto* bMinus = makeScoreBtn(QStringLiteral("B−"), QStringLiteral("#4F9EFF"));
    auto* bPlus = makeScoreBtn(QStringLiteral("B+"), QStringLiteral("#4F9EFF"));
    scoreRow->addWidget(aMinus, 1);
    scoreRow->addWidget(aPlus, 1);
    scoreRow->addWidget(bMinus, 1);
    scoreRow->addWidget(bPlus, 1);
    lay->addLayout(scoreRow);

    auto* matchLab = new QLabel(QStringLiteral("MATCH"), this);
    matchLab->setObjectName(QStringLiteral("sec"));
    lay->addWidget(matchLab);

    auto* raceRow = new QHBoxLayout();
    auto* raceLab = new QLabel(QStringLiteral("Race to"), this);
    raceLab->setStyleSheet(QStringLiteral("color:#8892A4; font-size:10px; background:transparent; border:none;"));
    auto* raceTo = new QSpinBox(this);
    raceTo->setRange(1, 25);
    raceTo->setValue(model->state().raceTo);
    raceRow->addWidget(raceLab);
    raceRow->addWidget(raceTo, 1);
    lay->addLayout(raceRow);

    auto* clockRow = new QHBoxLayout();
    auto* clockLbl = new QLabel(QStringLiteral("00:00"), this);
    clockLbl->setStyleSheet(QStringLiteral(
        "font-family:'Bebas Neue'; font-size:18px; color:#22C55E; background:transparent; border:none;"));
    auto* clockRun = new QCheckBox(QStringLiteral("Clock"), this);
    clockRun->setChecked(model->state().clockRunning);
    auto* clockReset = new QPushButton(QStringLiteral("Reset"), this);
    clockReset->setFixedHeight(22);
    clockReset->setCursor(Qt::PointingHandCursor);
    clockReset->setStyleSheet(QStringLiteral(
        "QPushButton{background:#1A1D22;border:1px solid #3A3D45;color:#A0A8B8;font-size:9px;font-weight:700;}"));
    clockRow->addWidget(clockLbl);
    clockRow->addStretch();
    clockRow->addWidget(clockRun);
    clockRow->addWidget(clockReset);
    lay->addLayout(clockRow);

    auto* actions = new QHBoxLayout();
    actions->setSpacing(4);
    auto* reset = new QPushButton(QStringLiteral("Reset"), this);
    auto* swap = new QPushButton(QStringLiteral("Swap"), this);
    auto* push = new QPushButton(QStringLiteral("Push"), this);
    for (auto* b : {reset, swap}) {
        b->setCursor(Qt::PointingHandCursor);
        b->setStyleSheet(QStringLiteral(
            "QPushButton{background:#1A1D22;border:1px solid #3A3D45;color:#C8CAD0;"
            "font-size:10px;font-weight:800;padding:5px;border-radius:3px;}"));
    }
    push->setCursor(Qt::PointingHandCursor);
    push->setStyleSheet(QStringLiteral(
        "QPushButton{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #4ADE80,stop:1 #16A34A);"
        "border:1px solid #86EFAC;color:#04140A;font-size:10px;font-weight:900;padding:5px;border-radius:3px;}"));
    actions->addWidget(reset, 1);
    actions->addWidget(swap, 1);
    actions->addWidget(push, 1);
    lay->addLayout(actions);
    lay->addStretch(1);

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
        scoreRead->setText(QStringLiteral("%1  %2 — %3  %4")
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
