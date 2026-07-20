#include "ui/widgets/ScoreboardControlsWidget.h"
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
#include <QButtonGroup>
#include <QAbstractButton>
#include <QTimer>
#include <QScrollArea>
#include <QFrame>
#include <utility>

namespace railshot {

namespace {
QString mapSportUiToModel(const QString& ui)
{
    if (ui == QLatin1String("Pool")) return QStringLiteral("8ball");
    if (ui == QLatin1String("Basketball")) return QStringLiteral("basketball");
    if (ui == QLatin1String("Soccer")) return QStringLiteral("soccer");
    if (ui == QLatin1String("Tennis")) return QStringLiteral("tennis");
    if (ui == QLatin1String("Custom")) return QStringLiteral("custom");
    return QStringLiteral("generic");
}
QString mapThemeUiToModel(const QString& ui)
{
    if (ui == QLatin1String("Light")) return QStringLiteral("classic");
    if (ui == QLatin1String("Team")) return QStringLiteral("broadcast");
    if (ui == QLatin1String("Neon")) return QStringLiteral("neon");
    return QStringLiteral("railshot");
}
QString mapLayoutUiToModel(const QString& ui)
{
    if (ui == QLatin1String("Center")) return QStringLiteral("standard");
    if (ui == QLatin1String("Corner")) return QStringLiteral("compact");
    if (ui == QLatin1String("Full")) return QStringLiteral("wide");
    return QStringLiteral("standard");
}
} // namespace

ScoreboardControlsWidget::ScoreboardControlsWidget(EngineController* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setObjectName(QStringLiteral("scoreboardControls"));
    setMinimumWidth(220);
    setStyleSheet(QStringLiteral(
        "QWidget#scoreboardControls {"
        "  background:#0A0C0F;"
        "  border-top:3px solid #22C55E;"
        "  border-left:2px solid #22C55E;"
        "}"
        "QWidget#scoreboardControls QLabel#sec {"
        "  color:#22C55E; font-size:9px; font-weight:900; letter-spacing:1px;"
        "  background:transparent; border:none;"
        "}"
        "QWidget#scoreboardControls QLineEdit, QWidget#scoreboardControls QSpinBox,"
        "QWidget#scoreboardControls QComboBox {"
        "  background:#12151A; border:1px solid #3A3D45; color:#E0E2E8;"
        "  border-radius:3px; padding:3px 6px; font-size:11px;"
        "}"
        "QWidget#scoreboardControls QLineEdit:focus, QWidget#scoreboardControls QSpinBox:focus,"
        "QWidget#scoreboardControls QComboBox:focus { border-color:#22C55E; }"));

    auto* model = engine->scoreboard();
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setStyleSheet(QStringLiteral("background:transparent; border:none;"));
    auto* host = new QWidget(scroll);
    host->setStyleSheet(QStringLiteral("background:transparent;"));
    auto* lay = new QVBoxLayout(host);
    lay->setContentsMargins(8, 8, 8, 8);
    lay->setSpacing(6);

    auto* overlayBtn = new QPushButton(QStringLiteral("OVERLAY ON"), host);
    overlayBtn->setCheckable(true);
    overlayBtn->setChecked(true);
    overlayBtn->setCursor(Qt::PointingHandCursor);
    overlayBtn->setStyleSheet(QStringLiteral(
        "QPushButton{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #4ADE80,stop:1 #16A34A);"
        "border:1px solid #86EFAC;color:#04140A;font-weight:900;font-size:10px;border-radius:3px;padding:6px;}"
        "QPushButton:!checked{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #2A2D35,stop:1 #1A1D22);"
        "border:1px solid #3A3D45;color:#8892A4;}"));
    lay->addWidget(overlayBtn);

    auto* sportLab = new QLabel(QStringLiteral("SPORT"), host);
    sportLab->setObjectName(QStringLiteral("sec"));
    lay->addWidget(sportLab);

    auto* sportGrid = new QGridLayout();
    sportGrid->setSpacing(3);
    const QStringList sports = {QStringLiteral("Generic"), QStringLiteral("Pool"),
                                QStringLiteral("Basketball"), QStringLiteral("Soccer"),
                                QStringLiteral("Tennis"), QStringLiteral("Custom")};
    auto* sportGroup = new QButtonGroup(host);
    sportGroup->setExclusive(true);
    int si = 0;
    for (const auto& sp : sports) {
        auto* b = new QPushButton(sp, host);
        b->setCheckable(true);
        b->setCursor(Qt::PointingHandCursor);
        b->setStyleSheet(QStringLiteral(
            "QPushButton{background:#1A1D22;border:1px solid #3A3D45;color:#A0A8B8;"
            "font-size:9px;font-weight:700;padding:4px 2px;border-radius:2px;}"
            "QPushButton:checked{border:2px solid #F59E0B;color:#FBBF24;background:#2A2010;}"));
        if (sp == QLatin1String("Pool"))
            b->setChecked(true);
        sportGroup->addButton(b);
        sportGrid->addWidget(b, si / 3, si % 3);
        ++si;
    }
    lay->addLayout(sportGrid);

    auto* teamsLab = new QLabel(QStringLiteral("TEAMS"), host);
    teamsLab->setObjectName(QStringLiteral("sec"));
    lay->addWidget(teamsLab);

    auto* aName = new QLineEdit(model->state().playerA, host);
    auto* bName = new QLineEdit(model->state().playerB, host);
    aName->setPlaceholderText(QStringLiteral("Player A"));
    bName->setPlaceholderText(QStringLiteral("Player B"));
    lay->addWidget(aName);
    lay->addWidget(bName);

    auto* scoreRead = new QLabel(host);
    scoreRead->setAlignment(Qt::AlignCenter);
    scoreRead->setStyleSheet(QStringLiteral(
        "font-family:'Bebas Neue'; font-size:22px; color:#F0F0F0; background:#12151A;"
        "border:1px solid #3A3D45; border-radius:3px; padding:4px;"));
    lay->addWidget(scoreRead);

    auto* scoreRow = new QHBoxLayout();
    scoreRow->setSpacing(4);
    auto makeScoreBtn = [&](const QString& t, const QString& color) {
        auto* b = new QPushButton(t, host);
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

    auto* raceRow = new QHBoxLayout();
    auto* raceLab = new QLabel(QStringLiteral("Race to"), host);
    raceLab->setStyleSheet(QStringLiteral("color:#8892A4; font-size:10px; background:transparent; border:none;"));
    auto* raceTo = new QSpinBox(host);
    raceTo->setRange(1, 25);
    raceTo->setValue(model->state().raceTo);
    raceRow->addWidget(raceLab);
    raceRow->addWidget(raceTo, 1);
    lay->addLayout(raceRow);

    auto* layoutBox = new QComboBox(host);
    layoutBox->addItems({QStringLiteral("Lower Third"), QStringLiteral("Center"),
                         QStringLiteral("Corner"), QStringLiteral("Full")});
    auto* themeBox = new QComboBox(host);
    themeBox->addItems({QStringLiteral("Dark"), QStringLiteral("Light"), QStringLiteral("Team"),
                        QStringLiteral("Neon")});
    lay->addWidget(layoutBox);
    lay->addWidget(themeBox);

    auto* clockRow = new QHBoxLayout();
    auto* clockLbl = new QLabel(QStringLiteral("00:00"), host);
    clockLbl->setStyleSheet(QStringLiteral(
        "font-family:'Bebas Neue'; font-size:18px; color:#22C55E; background:transparent; border:none;"));
    auto* clockRun = new QCheckBox(QStringLiteral("Clock"), host);
    clockRun->setChecked(model->state().clockRunning);
    auto* clockReset = new QPushButton(QStringLiteral("Reset"), host);
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
    auto* reset = new QPushButton(QStringLiteral("Reset"), host);
    auto* swap = new QPushButton(QStringLiteral("Swap"), host);
    auto* push = new QPushButton(QStringLiteral("Push"), host);
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

    scroll->setWidget(host);
    outer->addWidget(scroll, 1);

    auto applyMeta = [=] {
        auto st = model->state();
        QString sportUi = QStringLiteral("Pool");
        if (auto* checked = sportGroup->checkedButton())
            sportUi = checked->text();
        st.sport = mapSportUiToModel(sportUi);
        st.theme = mapThemeUiToModel(themeBox->currentText());
        st.layout = mapLayoutUiToModel(layoutBox->currentText());
        st.raceTo = raceTo->value();
        model->setState(st);
    };
    connect(sportGroup, &QButtonGroup::buttonClicked, this, [applyMeta](QAbstractButton*) { applyMeta(); });
    connect(themeBox, &QComboBox::currentTextChanged, this, [applyMeta](const QString&) { applyMeta(); });
    connect(layoutBox, &QComboBox::currentTextChanged, this, [applyMeta](const QString&) { applyMeta(); });
    connect(raceTo, QOverload<int>::of(&QSpinBox::valueChanged), this, [applyMeta](int) { applyMeta(); });

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
        overlayBtn->setText(on ? QStringLiteral("OVERLAY ON") : QStringLiteral("OVERLAY OFF"));
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
    };
    connect(model, &ScoreboardModel::changed, this, refresh);
    refresh(model->state());

    auto* clock = new QTimer(this);
    connect(clock, &QTimer::timeout, model, &ScoreboardModel::tickClock);
    clock->start(1000);
}

} // namespace railshot
