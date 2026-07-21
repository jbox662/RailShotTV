#include "ui/widgets/ScoreboardControlsWidget.h"
#include "ui/widgets/ScoreboardSettingsDialog.h"
#include "core/EngineController.h"
#include "core/SceneGraph.h"
#include "core/Types.h"
#include "scoreboard/ScoreboardModel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QTimer>
#include <QSignalBlocker>
#include <QScrollArea>
#include <QFrame>
#include <QSizePolicy>
#include <QButtonGroup>
#include <QVector>
#include <QAbstractSpinBox>
#include <QAbstractSpinBox>
#include <QEvent>
#include <utility>

namespace railshot {

namespace {

QString poolGameLabel(const QString& sport)
{
    if (sport == QLatin1String("9ball"))
        return QStringLiteral("9-Ball");
    if (sport == QLatin1String("10ball"))
        return QStringLiteral("10-Ball");
    if (sport == QLatin1String("straight"))
        return QStringLiteral("Straight Pool");
    if (sport == QLatin1String("onepocket"))
        return QStringLiteral("One-Pocket");
    return QStringLiteral("8-Ball");
}

QString poolGameSport(const QString& label)
{
    if (label == QLatin1String("9-Ball"))
        return QStringLiteral("9ball");
    if (label == QLatin1String("10-Ball"))
        return QStringLiteral("10ball");
    if (label == QLatin1String("Straight Pool"))
        return QStringLiteral("straight");
    if (label == QLatin1String("One-Pocket"))
        return QStringLiteral("onepocket");
    return QStringLiteral("8ball");
}

} // namespace

ScoreboardControlsWidget::ScoreboardControlsWidget(EngineController* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setObjectName(QStringLiteral("scoreboardControls"));
    setMinimumWidth(280);
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
        "QWidget#scoreboardControls QLabel#fieldLab {"
        "  color:#8892A4; font-size:9px; font-weight:700;"
        "  background:transparent; border:none;"
        "}"
        "QWidget#scoreboardControls QLineEdit,"
        "QWidget#scoreboardControls QSpinBox,"
        "QWidget#scoreboardControls QComboBox {"
        "  background:#12151A; border:1px solid #3A3D45; color:#E0E2E8;"
        "  border-radius:3px; padding:3px 8px; font-size:10px; font-weight:600;"
        "  min-height:24px; max-height:26px;"
        "}"
        "QWidget#scoreboardControls QLineEdit:focus,"
        "QWidget#scoreboardControls QSpinBox:focus,"
        "QWidget#scoreboardControls QComboBox:focus {"
        "  border-color:#22C55E;"
        "}"
        "QWidget#scoreboardControls QSpinBox {"
        "  padding-right:2px;"
        "}"
        "QWidget#scoreboardControls QSpinBox::up-button,"
        "QWidget#scoreboardControls QSpinBox::down-button {"
        "  subcontrol-origin: border;"
        "  width:18px;"
        "  background:#1A1D22;"
        "  border:none;"
        "  border-left:1px solid #3A3D45;"
        "}"
        "QWidget#scoreboardControls QSpinBox::up-button {"
        "  subcontrol-position: top right;"
        "  border-top-right-radius:2px;"
        "}"
        "QWidget#scoreboardControls QSpinBox::down-button {"
        "  subcontrol-position: bottom right;"
        "  border-bottom-right-radius:2px;"
        "}"
        "QWidget#scoreboardControls QSpinBox::up-button:hover,"
        "QWidget#scoreboardControls QSpinBox::down-button:hover {"
        "  background:#243028;"
        "}"
        "QWidget#scoreboardControls QSpinBox::up-arrow {"
        "  width:0; height:0;"
        "  border-left:4px solid transparent;"
        "  border-right:4px solid transparent;"
        "  border-bottom:5px solid #A0A8B8;"
        "}"
        "QWidget#scoreboardControls QSpinBox::down-arrow {"
        "  width:0; height:0;"
        "  border-left:4px solid transparent;"
        "  border-right:4px solid transparent;"
        "  border-top:5px solid #A0A8B8;"
        "}"
        "QWidget#scoreboardControls QComboBox {"
        "  padding:3px 4px 3px 6px;"
        "  min-width:0;"
        "}"
        "QWidget#scoreboardControls QComboBox::drop-down {"
        "  subcontrol-origin: padding;"
        "  subcontrol-position: top right;"
        "  width:16px;"
        "  border:none;"
        "  border-left:1px solid #3A3D45;"
        "  background:#1A1D22;"
        "}"
        "QWidget#scoreboardControls QComboBox::down-arrow {"
        "  width:0; height:0;"
        "  border-left:3px solid transparent;"
        "  border-right:3px solid transparent;"
        "  border-top:4px solid #A0A8B8;"
        "  margin-right:4px;"
        "}"
        "QWidget#scoreboardControls QComboBox QAbstractItemView {"
        "  background:#12151A; color:#E0E2E8;"
        "  border:1px solid #3A3D45;"
        "  selection-background-color:#1A3A28;"
        "  outline:0;"
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
    auto* settingsBtn = new QPushButton(QStringLiteral("Settings"), host);
    settingsBtn->setCursor(Qt::PointingHandCursor);
    settingsBtn->setFixedHeight(22);
    settingsBtn->setToolTip(QStringLiteral("Sport boards, presets, colors…"));
    settingsBtn->setStyleSheet(QStringLiteral(
        "QPushButton{background:#1A1D22;border:1px solid #3A3D45;color:#C8CAD0;"
        "font-weight:700;font-size:9px;border-radius:3px;padding:1px 8px;}"
        "QPushButton:hover{border-color:#22C55E;color:#FFFFFF;}"));
    topRow->addWidget(overlayBtn);
    topRow->addWidget(settingsBtn);
    topRow->addStretch(1);
    lay->addLayout(topRow);

    auto* sportTag = new QLabel(host);
    sportTag->setObjectName(QStringLiteral("sec"));
    lay->addWidget(sportTag);

    auto* names = new QHBoxLayout();
    names->setSpacing(4);
    auto* aName = new QLineEdit(model->state().playerA, host);
    auto* bName = new QLineEdit(model->state().playerB, host);
    aName->setPlaceholderText(QStringLiteral("Player A"));
    bName->setPlaceholderText(QStringLiteral("Player B"));
    names->addWidget(aName, 1);
    names->addWidget(bName, 1);
    lay->addLayout(names);

    auto* scoreBand = new QHBoxLayout();
    scoreBand->setSpacing(3);
    scoreBand->setContentsMargins(0, 0, 0, 0);
    auto makeScoreBtn = [&](const QString& t, const QString& color) {
        auto* b = new QPushButton(t, host);
        b->setFixedSize(28, 26);
        b->setCursor(Qt::PointingHandCursor);
        b->setStyleSheet(QStringLiteral(
            "QPushButton{background:#1A1D22;border:1px solid %1;color:%1;"
            "font-weight:900;font-size:11px;border-radius:3px;padding:0;}").arg(color));
        return b;
    };
    auto* aMinus = makeScoreBtn(QStringLiteral("−"), QStringLiteral("#FF5A2C"));
    auto* aPlus = makeScoreBtn(QStringLiteral("+"), QStringLiteral("#FF5A2C"));
    auto* bMinus = makeScoreBtn(QStringLiteral("−"), QStringLiteral("#4F9EFF"));
    auto* bPlus = makeScoreBtn(QStringLiteral("+"), QStringLiteral("#4F9EFF"));
    aMinus->setToolTip(QStringLiteral("Player A −1"));
    aPlus->setToolTip(QStringLiteral("Player A +1"));
    bMinus->setToolTip(QStringLiteral("Player B −1"));
    bPlus->setToolTip(QStringLiteral("Player B +1"));

    auto* scoreRead = new QLabel(host);
    scoreRead->setAlignment(Qt::AlignCenter);
    scoreRead->setFixedHeight(26);
    scoreRead->setMinimumWidth(64);
    scoreRead->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    scoreRead->setStyleSheet(QStringLiteral(
        "font-family:'Bebas Neue','Arial Narrow',sans-serif; font-size:18px; color:#F0F0F0;"
        "background:#12151A; border:1px solid #3A3D45; border-radius:3px; padding:1px 10px;"));

    // Tight centered cluster — score does not stretch across the dock
    auto* scoreCluster = new QWidget(host);
    scoreCluster->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    auto* scoreClusterLay = new QHBoxLayout(scoreCluster);
    scoreClusterLay->setContentsMargins(0, 0, 0, 0);
    scoreClusterLay->setSpacing(3);
    scoreClusterLay->addWidget(aMinus);
    scoreClusterLay->addWidget(aPlus);
    scoreClusterLay->addWidget(scoreRead);
    scoreClusterLay->addWidget(bMinus);
    scoreClusterLay->addWidget(bPlus);
    scoreBand->addStretch(1);
    scoreBand->addWidget(scoreCluster);
    scoreBand->addStretch(1);
    lay->addLayout(scoreBand);

    // ── Pool extras ──
    auto* poolBox = new QWidget(host);
    auto* poolLay = new QVBoxLayout(poolBox);
    poolLay->setContentsMargins(0, 0, 0, 0);
    poolLay->setSpacing(4);

    auto makeFieldLab = [](const QString& text, QWidget* parent) {
        auto* lab = new QLabel(text, parent);
        lab->setObjectName(QStringLiteral("fieldLab"));
        lab->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        return lab;
    };

    // Race + Game as one tight cluster (no stretch gap between them)
    auto* poolFields = new QHBoxLayout();
    poolFields->setContentsMargins(0, 0, 0, 0);
    poolFields->setSpacing(0);

    auto* raceGame = new QWidget(poolBox);
    raceGame->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    auto* raceGameLay = new QHBoxLayout(raceGame);
    raceGameLay->setContentsMargins(0, 0, 0, 0);
    raceGameLay->setSpacing(4);

    auto* raceLab = makeFieldLab(QStringLiteral("Race"), raceGame);
    auto* raceTo = new QSpinBox(raceGame);
    raceTo->setRange(1, 25);
    raceTo->setValue(model->state().raceTo);
    raceTo->setAlignment(Qt::AlignCenter);
    raceTo->setFixedWidth(56);
    raceTo->setButtonSymbols(QAbstractSpinBox::UpDownArrows);
    raceTo->setToolTip(QStringLiteral("Race to"));
    raceGameLay->addWidget(raceLab);
    raceGameLay->addWidget(raceTo);

    auto* gameLab = makeFieldLab(QStringLiteral("Game"), raceGame);
    auto* gameCombo = new QComboBox(raceGame);
    gameCombo->addItems({QStringLiteral("8-Ball"), QStringLiteral("9-Ball"), QStringLiteral("10-Ball"),
                         QStringLiteral("Straight Pool"), QStringLiteral("One-Pocket")});
    gameCombo->setCurrentText(poolGameLabel(model->state().sport));
    gameCombo->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    {
        // Fit longest label + padding + chevron — not the dock width.
        const QFontMetrics fm(gameCombo->font());
        int maxTw = 0;
        for (int i = 0; i < gameCombo->count(); ++i)
            maxTw = qMax(maxTw, fm.horizontalAdvance(gameCombo->itemText(i)));
        gameCombo->setFixedWidth(maxTw + 28);
    }
    raceGameLay->addWidget(gameLab);
    raceGameLay->addWidget(gameCombo);

    poolFields->addWidget(raceGame);
    poolFields->addStretch(1);
    poolLay->addLayout(poolFields);

    auto* turnRow = new QHBoxLayout();
    turnRow->setSpacing(6);
    auto* turnLab = makeFieldLab(QStringLiteral("At table"), poolBox);
    auto* turnA = new QPushButton(QStringLiteral("A"), poolBox);
    auto* turnB = new QPushButton(QStringLiteral("B"), poolBox);
    for (auto* b : {turnA, turnB}) {
        b->setCheckable(true);
        b->setFixedSize(28, 26);
        b->setCursor(Qt::PointingHandCursor);
        b->setStyleSheet(QStringLiteral(
            "QPushButton{background:#1A1D22;border:1px solid #3A3D45;color:#A0A8B8;font-weight:800;font-size:10px;border-radius:3px;}"
            "QPushButton:checked{border-color:#22C55E;color:#86EFAC;background:#0F1A14;}"));
    }
    auto* turnGroup = new QButtonGroup(poolBox);
    turnGroup->setExclusive(true);
    turnGroup->addButton(turnA, 1);
    turnGroup->addButton(turnB, 2);
    turnRow->addWidget(turnLab);
    turnRow->addWidget(turnA);
    turnRow->addWidget(turnB);
    turnRow->addStretch(1);
    poolLay->addLayout(turnRow);

    // Ball rack — only shown when the active board draws a rack strip
    auto* rackBox = new QWidget(poolBox);
    auto* rackBoxLay = new QVBoxLayout(rackBox);
    rackBoxLay->setContentsMargins(0, 0, 0, 0);
    rackBoxLay->setSpacing(4);
    auto* rackHead = new QHBoxLayout();
    rackHead->setContentsMargins(0, 0, 0, 0);
    rackHead->setSpacing(6);
    auto* rackLab = new QLabel(QStringLiteral("Balls on table"), rackBox);
    rackLab->setObjectName(QStringLiteral("sec"));
    rackHead->addWidget(rackLab);
    rackHead->addStretch(1);
    auto* rackReset = new QPushButton(QStringLiteral("All on"), rackBox);
    auto* rackClear = new QPushButton(QStringLiteral("All off"), rackBox);
    for (auto* b : {rackReset, rackClear}) {
        b->setCursor(Qt::PointingHandCursor);
        b->setFixedHeight(20);
        b->setStyleSheet(QStringLiteral(
            "QPushButton{background:#1A1D22;border:1px solid #3A3D45;color:#A0A8B8;"
            "font-size:8px;font-weight:700;padding:1px 6px;border-radius:3px;}"
            "QPushButton:hover{border-color:#4F9EFF;color:#E0E2E8;}"));
    }
    rackReset->setToolTip(QStringLiteral("Put all balls back on the table"));
    rackClear->setToolTip(QStringLiteral("Mark all balls pocketed"));
    rackHead->addWidget(rackReset);
    rackHead->addWidget(rackClear);
    rackBoxLay->addLayout(rackHead);
    auto* rackHint = new QLabel(QStringLiteral("Click a ball to pocket / put back"), rackBox);
    rackHint->setStyleSheet(QStringLiteral("color:#6A7384; font-size:8px; background:transparent; border:none;"));
    rackBoxLay->addWidget(rackHint);

    static const QColor kBallCol[] = {
        {},
        QColor(242, 196, 28), QColor(30, 96, 210), QColor(210, 40, 45), QColor(110, 40, 160),
        QColor(230, 120, 20), QColor(30, 140, 55), QColor(130, 30, 40), QColor(18, 18, 20),
        QColor(242, 196, 28), QColor(30, 96, 210), QColor(210, 40, 45), QColor(110, 40, 160),
        QColor(230, 120, 20), QColor(30, 140, 55), QColor(130, 30, 40),
    };

    // Tight centered rack — do NOT use a stretching QGridLayout (that spaced balls across the dock).
    auto* rackCluster = new QWidget(rackBox);
    rackCluster->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    auto* rackClusterLay = new QVBoxLayout(rackCluster);
    rackClusterLay->setContentsMargins(0, 0, 0, 0);
    rackClusterLay->setSpacing(3);
    auto* ballRow1 = new QHBoxLayout();
    ballRow1->setContentsMargins(0, 0, 0, 0);
    ballRow1->setSpacing(3);
    auto* ballRow2 = new QHBoxLayout();
    ballRow2->setContentsMargins(0, 0, 0, 0);
    ballRow2->setSpacing(3);
    QVector<QPushButton*> ballBtns;
    ballBtns.reserve(15);
    const int mask0 = model->state().pocketedMask;
    for (int n = 1; n <= 15; ++n) {
        auto* btn = new QPushButton(QString::number(n), rackCluster);
        btn->setCheckable(true);
        btn->setChecked((mask0 & (1 << (n - 1))) == 0); // checked = still on table
        btn->setFixedSize(22, 22);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setProperty("ballNumber", n);
        const QColor c = kBallCol[n];
        const bool dark = c.lightness() < 80;
        const QString fg = dark ? QStringLiteral("#FFFFFF") : QStringLiteral("#111111");
        btn->setStyleSheet(QStringLiteral(
            "QPushButton{"
            "  background:%1; border:1px solid #2A2E36; border-radius:11px;"
            "  color:%2; font-size:8px; font-weight:900; padding:0;}"
            "QPushButton:!checked{"
            "  background:#2A2E36; color:#5A6070; border-color:#1A1D22;}"
            "QPushButton:hover{border-color:#86EFAC;}")
                               .arg(c.name(), fg));
        ballBtns.push_back(btn);
        (n <= 8 ? ballRow1 : ballRow2)->addWidget(btn);
    }
    rackClusterLay->addLayout(ballRow1);
    rackClusterLay->addLayout(ballRow2);
    auto* rackCenter = new QHBoxLayout();
    rackCenter->setContentsMargins(0, 0, 0, 0);
    rackCenter->addStretch(1);
    rackCenter->addWidget(rackCluster);
    rackCenter->addStretch(1);
    rackBoxLay->addLayout(rackCenter);
    poolLay->addWidget(rackBox);
    lay->addWidget(poolBox);

    // ── Baseball extras ──
    auto* bbBox = new QWidget(host);
    auto* bbLay = new QVBoxLayout(bbBox);
    bbLay->setContentsMargins(0, 0, 0, 0);
    bbLay->setSpacing(3);
    auto* countRow = new QHBoxLayout();
    auto* ballsSp = new QSpinBox(bbBox);
    ballsSp->setRange(0, 3);
    ballsSp->setPrefix(QStringLiteral("B "));
    ballsSp->setValue(model->state().balls);
    auto* strikesSp = new QSpinBox(bbBox);
    strikesSp->setRange(0, 2);
    strikesSp->setPrefix(QStringLiteral("S "));
    strikesSp->setValue(model->state().strikes);
    auto* outsSp = new QSpinBox(bbBox);
    outsSp->setRange(0, 2);
    outsSp->setPrefix(QStringLiteral("O "));
    outsSp->setValue(model->state().outs);
    countRow->addWidget(ballsSp);
    countRow->addWidget(strikesSp);
    countRow->addWidget(outsSp);
    bbLay->addLayout(countRow);
    auto* innRow = new QHBoxLayout();
    auto* innSp = new QSpinBox(bbBox);
    innSp->setRange(1, 20);
    innSp->setPrefix(QStringLiteral("Inn "));
    innSp->setValue(model->state().inning);
    auto* topBtn = new QPushButton(QStringLiteral("TOP"), bbBox);
    auto* botBtn = new QPushButton(QStringLiteral("BOT"), bbBox);
    for (auto* b : {topBtn, botBtn}) {
        b->setCheckable(true);
        b->setFixedHeight(22);
        b->setCursor(Qt::PointingHandCursor);
        b->setStyleSheet(QStringLiteral(
            "QPushButton{background:#1A1D22;border:1px solid #3A3D45;color:#A0A8B8;font-weight:800;font-size:9px;}"
            "QPushButton:checked{border-color:#F97316;color:#FDBA74;background:#1A1208;}"));
    }
    auto* halfGroup = new QButtonGroup(bbBox);
    halfGroup->setExclusive(true);
    halfGroup->addButton(topBtn);
    halfGroup->addButton(botBtn);
    innRow->addWidget(innSp);
    innRow->addWidget(topBtn);
    innRow->addWidget(botBtn);
    bbLay->addLayout(innRow);
    auto* baseRow = new QHBoxLayout();
    auto* b1 = new QCheckBox(QStringLiteral("1B"), bbBox);
    auto* b2 = new QCheckBox(QStringLiteral("2B"), bbBox);
    auto* b3 = new QCheckBox(QStringLiteral("3B"), bbBox);
    b1->setChecked(model->state().onFirst);
    b2->setChecked(model->state().onSecond);
    b3->setChecked(model->state().onThird);
    baseRow->addWidget(b1);
    baseRow->addWidget(b2);
    baseRow->addWidget(b3);
    baseRow->addStretch(1);
    bbLay->addLayout(baseRow);
    lay->addWidget(bbBox);

    // ── Basketball period ──
    auto* hoopBox = new QWidget(host);
    auto* hoopLay = new QHBoxLayout(hoopBox);
    hoopLay->setContentsMargins(0, 0, 0, 0);
    hoopLay->setSpacing(8);
    auto* periodLab = new QLabel(QStringLiteral("Quarter"), hoopBox);
    periodLab->setObjectName(QStringLiteral("fieldLab"));
    auto* periodSp = new QSpinBox(hoopBox);
    periodSp->setRange(1, 10);
    periodSp->setValue(model->state().period);
    periodSp->setAlignment(Qt::AlignCenter);
    periodSp->setMinimumWidth(72);
    hoopLay->addWidget(periodLab);
    hoopLay->addWidget(periodSp, 1);
    lay->addWidget(hoopBox);

    // ── Tennis sets (reuse balls/strikes) ──
    auto* tennisBox = new QWidget(host);
    auto* tennisLay = new QHBoxLayout(tennisBox);
    tennisLay->setContentsMargins(0, 0, 0, 0);
    auto* setASp = new QSpinBox(tennisBox);
    setASp->setRange(0, 5);
    setASp->setPrefix(QStringLiteral("Sets A "));
    setASp->setValue(model->state().balls);
    auto* setBSp = new QSpinBox(tennisBox);
    setBSp->setRange(0, 5);
    setBSp->setPrefix(QStringLiteral("B "));
    setBSp->setValue(model->state().strikes);
    tennisLay->addWidget(setASp);
    tennisLay->addWidget(setBSp);
    lay->addWidget(tennisBox);

    // Clock row (shared)
    auto* matchRow = new QHBoxLayout();
    matchRow->setSpacing(4);
    auto* clockLbl = new QLabel(QStringLiteral("00:00"), host);
    clockLbl->setStyleSheet(QStringLiteral(
        "font-family:'Bebas Neue','Arial Narrow',sans-serif; font-size:14px; color:#22C55E;"
        "background:transparent; border:none;"));
    auto* clockRun = new QCheckBox(QStringLiteral("Clock"), host);
    clockRun->setChecked(model->state().clockRunning);
    auto* clockReset = new QPushButton(QStringLiteral("0:00"), host);
    clockReset->setFixedHeight(20);
    clockReset->setCursor(Qt::PointingHandCursor);
    clockReset->setStyleSheet(QStringLiteral(
        "QPushButton{background:#1A1D22;border:1px solid #3A3D45;color:#A0A8B8;"
        "font-size:8px;font-weight:700;padding:1px 5px;}"));
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
    connect(gameCombo, &QComboBox::currentTextChanged, this, [model](const QString& label) {
        auto st = model->state();
        const QString next = poolGameSport(label);
        if (st.sport == next)
            return;
        st.sport = next;
        // Drop pocketed bits for balls that aren't in this game
        const int keep = (1 << poolObjectBallCount(next)) - 1;
        st.pocketedMask &= keep;
        model->setState(st);
    });
    connect(turnGroup, &QButtonGroup::idClicked, this, [model](int id) {
        auto st = model->state();
        st.activeSide = id;
        model->setState(st);
    });
    for (auto* btn : ballBtns) {
        connect(btn, &QPushButton::clicked, this, [model, btn] {
            const int n = btn->property("ballNumber").toInt();
            if (n < 1 || n > 15)
                return;
            auto st = model->state();
            if (n > poolObjectBallCount(st.sport))
                return;
            const int bit = 1 << (n - 1);
            // checked = on table → clear pocketed bit; unchecked = pocketed → set bit
            if (btn->isChecked())
                st.pocketedMask &= ~bit;
            else
                st.pocketedMask |= bit;
            model->setState(st);
        });
    }
    connect(rackReset, &QPushButton::clicked, this, [model] {
        auto st = model->state();
        st.pocketedMask = 0;
        model->setState(st);
    });
    connect(rackClear, &QPushButton::clicked, this, [model] {
        auto st = model->state();
        st.pocketedMask = (1 << poolObjectBallCount(st.sport)) - 1;
        model->setState(st);
    });
    connect(ballsSp, QOverload<int>::of(&QSpinBox::valueChanged), this, [model](int v) {
        auto st = model->state();
        st.balls = v;
        model->setState(st);
    });
    connect(strikesSp, QOverload<int>::of(&QSpinBox::valueChanged), this, [model](int v) {
        auto st = model->state();
        st.strikes = v;
        model->setState(st);
    });
    connect(outsSp, QOverload<int>::of(&QSpinBox::valueChanged), this, [model](int v) {
        auto st = model->state();
        st.outs = v;
        model->setState(st);
    });
    connect(innSp, QOverload<int>::of(&QSpinBox::valueChanged), this, [model](int v) {
        auto st = model->state();
        st.inning = v;
        model->setState(st);
    });
    connect(topBtn, &QPushButton::clicked, this, [model] {
        auto st = model->state();
        st.topHalf = true;
        model->setState(st);
    });
    connect(botBtn, &QPushButton::clicked, this, [model] {
        auto st = model->state();
        st.topHalf = false;
        model->setState(st);
    });
    connect(b1, &QCheckBox::toggled, this, [model](bool on) {
        auto st = model->state();
        st.onFirst = on;
        model->setState(st);
    });
    connect(b2, &QCheckBox::toggled, this, [model](bool on) {
        auto st = model->state();
        st.onSecond = on;
        model->setState(st);
    });
    connect(b3, &QCheckBox::toggled, this, [model](bool on) {
        auto st = model->state();
        st.onThird = on;
        model->setState(st);
    });
    connect(periodSp, QOverload<int>::of(&QSpinBox::valueChanged), this, [model](int v) {
        auto st = model->state();
        st.period = v;
        model->setState(st);
    });
    connect(setASp, QOverload<int>::of(&QSpinBox::valueChanged), this, [model](int v) {
        auto st = model->state();
        st.balls = v;
        model->setState(st);
    });
    connect(setBSp, QOverload<int>::of(&QSpinBox::valueChanged), this, [model](int v) {
        auto st = model->state();
        st.strikes = v;
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
        std::swap(s.colorA, s.colorB);
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
        const int m = st.clockSeconds / 60;
        const int sec = st.clockSeconds % 60;
        clockLbl->setText(QStringLiteral("%1:%2")
                              .arg(m, 2, 10, QLatin1Char('0'))
                              .arg(sec, 2, 10, QLatin1Char('0')));
        if (aName->text() != st.playerA) aName->setText(st.playerA);
        if (bName->text() != st.playerB) bName->setText(st.playerB);
        if (raceTo->value() != st.raceTo) {
            QSignalBlocker b(raceTo);
            raceTo->setValue(st.raceTo);
        }
        if (clockRun->isChecked() != st.clockRunning) {
            QSignalBlocker b(clockRun);
            clockRun->setChecked(st.clockRunning);
        }

        const bool pool = isPoolSport(st.sport);
        const bool showRack = scoreboardShowsBallRack(st);
        const bool baseball = st.sport == QLatin1String("baseball");
        const bool basketball = st.sport == QLatin1String("basketball");
        const bool tennis = st.sport == QLatin1String("tennis");
        poolBox->setVisible(pool);
        rackBox->setVisible(showRack);
        bbBox->setVisible(baseball);
        hoopBox->setVisible(basketball);
        tennisBox->setVisible(tennis);

        QString tag = QStringLiteral("GENERIC");
        if (pool)
            tag = QStringLiteral("POOL · %1").arg(poolGameLabel(st.sport).toUpper());
        else if (baseball) tag = QStringLiteral("BASEBALL");
        else if (basketball) tag = QStringLiteral("BASKETBALL");
        else if (st.sport == QLatin1String("soccer")) tag = QStringLiteral("SOCCER");
        else if (tennis) tag = QStringLiteral("TENNIS");
        sportTag->setText(tag);

        if (pool) {
            const QString wantGame = poolGameLabel(st.sport);
            if (gameCombo->currentText() != wantGame) {
                QSignalBlocker block(gameCombo);
                gameCombo->setCurrentText(wantGame);
            }
            QSignalBlocker ba(turnA);
            QSignalBlocker bb(turnB);
            turnA->setChecked(st.activeSide == 1);
            turnB->setChecked(st.activeSide == 2);
            if (showRack) {
                const int ballCount = poolObjectBallCount(st.sport);
                for (auto* btn : ballBtns) {
                    const int n = btn->property("ballNumber").toInt();
                    const bool inGame = n >= 1 && n <= ballCount;
                    btn->setVisible(inGame);
                    if (!inGame)
                        continue;
                    const bool onTable = (st.pocketedMask & (1 << (n - 1))) == 0;
                    if (btn->isChecked() != onTable) {
                        QSignalBlocker block(btn);
                        btn->setChecked(onTable);
                    }
                }
                rackLab->setText(QStringLiteral("Balls on table (%1)").arg(ballCount));
            }
        }
        if (baseball) {
            QSignalBlocker b1b(ballsSp);
            QSignalBlocker b2b(strikesSp);
            QSignalBlocker b3b(outsSp);
            QSignalBlocker b4b(innSp);
            ballsSp->setValue(st.balls);
            strikesSp->setValue(st.strikes);
            outsSp->setValue(st.outs);
            innSp->setValue(st.inning);
            topBtn->setChecked(st.topHalf);
            botBtn->setChecked(!st.topHalf);
            QSignalBlocker c1(b1);
            QSignalBlocker c2(b2);
            QSignalBlocker c3(b3);
            b1->setChecked(st.onFirst);
            b2->setChecked(st.onSecond);
            b3->setChecked(st.onThird);
        }
        if (basketball && periodSp->value() != st.period) {
            QSignalBlocker b(periodSp);
            periodSp->setValue(st.period);
        }
        if (tennis) {
            QSignalBlocker ba(setASp);
            QSignalBlocker bb(setBSp);
            setASp->setValue(st.balls);
            setBSp->setValue(st.strikes);
        }

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

    // Block mouse-wheel value changes on spin/combo (hover-scroll was changing race/game/etc.)
    auto blockWheelOn = [this](QWidget* w) {
        if (!w)
            return;
        w->setFocusPolicy(Qt::StrongFocus); // click to edit; wheel alone must not tweak values
        w->installEventFilter(this);
        for (auto* child : w->findChildren<QWidget*>())
            child->installEventFilter(this);
    };
    for (auto* spin : findChildren<QAbstractSpinBox*>())
        blockWheelOn(spin);
    for (auto* combo : findChildren<QComboBox*>())
        blockWheelOn(combo);
}

bool ScoreboardControlsWidget::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::Wheel) {
        for (QObject* o = watched; o; o = o->parent()) {
            if (qobject_cast<QAbstractSpinBox*>(o) || qobject_cast<QComboBox*>(o))
                return true; // swallow — do not change value
        }
    }
    return QWidget::eventFilter(watched, event);
}

} // namespace railshot
