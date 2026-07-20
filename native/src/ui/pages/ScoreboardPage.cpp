#include "ui/pages/ScoreboardPage.h"
#include "ui/Theme.h"
#include "core/EngineController.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QTimer>
#include <QCheckBox>
#include <QButtonGroup>
#include <QAbstractButton>
#include <QFrame>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <utility>

namespace railshot {

namespace {
QString mapSportUiToModel(const QString& ui)
{
    if (ui == QLatin1String("Pool / Billiards")) return QStringLiteral("8ball");
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
    if (ui == QLatin1String("Minimal")) return QStringLiteral("railshot");
    return QStringLiteral("railshot"); // Dark
}
QString mapLayoutUiToModel(const QString& ui)
{
    if (ui == QLatin1String("Center Banner")) return QStringLiteral("standard");
    if (ui == QLatin1String("Corner Compact")) return QStringLiteral("compact");
    if (ui == QLatin1String("Full Width")) return QStringLiteral("wide");
    return QStringLiteral("standard"); // Lower Third
}
} // namespace

ScoreboardPage::ScoreboardPage(EngineController* engine, QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("scoreboardPage"));
    auto* model = engine->scoreboard();
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* header = theme::makePageHeader(QStringLiteral("Scoreboard"), theme::PanelAccent::Emerald, this);
    auto* headerLay = qobject_cast<QHBoxLayout*>(header->layout());
    auto* overlayBtn = new QPushButton(QStringLiteral("OVERLAY ON"), header);
    overlayBtn->setCheckable(true);
    overlayBtn->setChecked(true);
    overlayBtn->setObjectName(QStringLiteral("chromeBtnSuccess"));
    auto* savePreset = new QPushButton(QStringLiteral("SAVE PRESET"), header);
    savePreset->setObjectName(QStringLiteral("chromeBtnPrimary"));
    auto* loadPreset = new QPushButton(QStringLiteral("LOAD PRESET"), header);
    loadPreset->setObjectName(QStringLiteral("chromeBtnViolet"));
    if (headerLay) {
        headerLay->addWidget(overlayBtn);
        headerLay->addWidget(savePreset);
        headerLay->addWidget(loadPreset);
    }
    root->addWidget(header);

    auto* body = new QHBoxLayout();
    body->setContentsMargins(0, 0, 0, 0);
    body->setSpacing(0);

    // Left controls
    auto* left = new QFrame(this);
    left->setObjectName(QStringLiteral("scoreboardLeft"));
    left->setFixedWidth(280);
    auto* leftLay = new QVBoxLayout(left);
    leftLay->setContentsMargins(12, 12, 12, 12);

    auto* sportLab = new QLabel(QStringLiteral("SPORT"), left);
    sportLab->setStyleSheet(QStringLiteral("color:#22C55E; font-weight:800; font-size:10px; letter-spacing:1.5px;"));
    leftLay->addWidget(sportLab);
    auto* sportRow = new QHBoxLayout();
    const QStringList sports = {QStringLiteral("Generic"), QStringLiteral("Pool / Billiards"),
                                QStringLiteral("Basketball"), QStringLiteral("Soccer"),
                                QStringLiteral("Tennis"), QStringLiteral("Custom")};
    auto* sportGroup = new QButtonGroup(left);
    sportGroup->setExclusive(true);
    QPushButton* activeSportBtn = nullptr;
    for (const auto& sp : sports) {
        auto* b = new QPushButton(sp, left);
        b->setCheckable(true);
        b->setStyleSheet(QStringLiteral(
            "QPushButton{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #2A2D35,stop:1 #1A1D22);"
            "border:1px solid #3A3D45;color:#A0A8B8;font-size:9px;padding:5px;font-weight:700;}"
            "QPushButton:checked{border:2px solid #F59E0B;color:#FBBF24;"
            "background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #2A2010,stop:1 #1A1508);}"));
        if (sp == QLatin1String("Pool / Billiards")) {
            b->setChecked(true);
            activeSportBtn = b;
        }
        sportGroup->addButton(b);
        sportRow->addWidget(b);
        if (sportRow->count() == 3) {
            leftLay->addLayout(sportRow);
            sportRow = new QHBoxLayout();
        }
    }
    if (sportRow->count()) leftLay->addLayout(sportRow);
    Q_UNUSED(activeSportBtn);

    auto* teamsLab = new QLabel(QStringLiteral("TEAMS"), left);
    teamsLab->setStyleSheet(QStringLiteral("color:#22C55E; font-weight:800; font-size:10px; letter-spacing:1.5px; padding-top:8px;"));
    leftLay->addWidget(teamsLab);
    auto* aName = new QLineEdit(model->state().playerA.isEmpty() ? QStringLiteral("Team Alpha") : model->state().playerA, left);
    auto* bName = new QLineEdit(model->state().playerB.isEmpty() ? QStringLiteral("Team Beta") : model->state().playerB, left);
    leftLay->addWidget(aName);
    leftLay->addWidget(bName);

    const QStringList swatches = {
        QStringLiteral("#FF5A2C"), QStringLiteral("#4F9EFF"), QStringLiteral("#A855F7"),
        QStringLiteral("#22C55E"), QStringLiteral("#22D3EE"), QStringLiteral("#F59E0B"),
        QStringLiteral("#EF4444"), QStringLiteral("#EC4899"), QStringLiteral("#FFFFFF"),
        QStringLiteral("#94A3B8")};
    QString* colorA = new QString(QStringLiteral("#FF5A2C"));
    QString* colorB = new QString(QStringLiteral("#4F9EFF"));
    auto* swA = new QHBoxLayout();
    auto* swB = new QHBoxLayout();
    for (const auto& c : swatches) {
        auto* ba = new QPushButton(left);
        ba->setFixedSize(20, 20);
        ba->setStyleSheet(QStringLiteral("background:%1; border:1px solid #3A3D45; border-radius:3px;").arg(c));
        connect(ba, &QPushButton::clicked, this, [colorA, c] { *colorA = c; });
        swA->addWidget(ba);
        auto* bb = new QPushButton(left);
        bb->setFixedSize(20, 20);
        bb->setStyleSheet(QStringLiteral("background:%1; border:1px solid #3A3D45; border-radius:3px;").arg(c));
        connect(bb, &QPushButton::clicked, this, [colorB, c] { *colorB = c; });
        swB->addWidget(bb);
    }
    leftLay->addWidget(new QLabel(QStringLiteral("Team A color"), left));
    leftLay->addLayout(swA);
    leftLay->addWidget(new QLabel(QStringLiteral("Team B color"), left));
    leftLay->addLayout(swB);

    auto* raceTo = new QSpinBox(left);
    raceTo->setRange(1, 25);
    raceTo->setValue(model->state().raceTo);
    auto* raceForm = new QFormLayout();
    raceForm->addRow(QStringLiteral("Race / Period target"), raceTo);
    leftLay->addLayout(raceForm);

    auto* scoreRow = new QHBoxLayout();
    auto makeScoreBtn = [&](const QString& t, const QString& color) {
        auto* b = new QPushButton(t, left);
        b->setFixedSize(36, 36);
        b->setStyleSheet(QStringLiteral(
            "QPushButton{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #2A2D35,stop:1 #1A1D22);"
            "border:1px solid %1;color:%1;font-weight:900;font-size:16px;border-radius:3px;}").arg(color));
        return b;
    };
    auto* aMinus = makeScoreBtn(QStringLiteral("−"), QStringLiteral("#FF5A2C"));
    auto* aPlus = makeScoreBtn(QStringLiteral("+"), QStringLiteral("#FF5A2C"));
    auto* bMinus = makeScoreBtn(QStringLiteral("−"), QStringLiteral("#4F9EFF"));
    auto* bPlus = makeScoreBtn(QStringLiteral("+"), QStringLiteral("#4F9EFF"));
    scoreRow->addWidget(aMinus);
    scoreRow->addWidget(aPlus);
    scoreRow->addStretch();
    scoreRow->addWidget(bMinus);
    scoreRow->addWidget(bPlus);
    leftLay->addLayout(scoreRow);

    auto* reset = new QPushButton(QStringLiteral("Reset Scores"), left);
    reset->setObjectName(QStringLiteral("chromeBtn"));
    auto* swap = new QPushButton(QStringLiteral("Swap Sides"), left);
    swap->setObjectName(QStringLiteral("chromeBtn"));
    auto* push = new QPushButton(QStringLiteral("Push to Program"), left);
    push->setObjectName(QStringLiteral("chromeBtnSuccess"));
    leftLay->addWidget(reset);
    leftLay->addWidget(swap);
    leftLay->addWidget(push);
    leftLay->addStretch();
    body->addWidget(left);

    // Center preview
    auto* center = new QWidget(this);
    auto* centerLay = new QVBoxLayout(center);
    centerLay->setContentsMargins(16, 16, 16, 8);
    auto* preview = new QLabel(center);
    preview->setMinimumHeight(280);
    preview->setAlignment(Qt::AlignCenter);
    preview->setStyleSheet(QStringLiteral(
        "background:#0A0B0F; border:2px solid #3A3D45; font-size:28px; font-weight:800;"
        "font-family:'Bebas Neue'; letter-spacing:1px;"));
    centerLay->addWidget(preview, 1);

    auto* timerBar = new QFrame(center);
    timerBar->setObjectName(QStringLiteral("chromeElevated"));
    timerBar->setFixedHeight(52);
    timerBar->setStyleSheet(QStringLiteral(
        "QFrame{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #1A1D22,stop:1 #0F1114);"
        "border:1px solid #4A4D55; border-radius:4px;}"));
    auto* timerLay = new QHBoxLayout(timerBar);
    auto* clockLbl = new QLabel(QStringLiteral("00:00"), timerBar);
    clockLbl->setStyleSheet(QStringLiteral(
        "font-family:'Bebas Neue'; font-size:28px; color:#22C55E; background:transparent;"));
    auto* clockRun = new QCheckBox(QStringLiteral("Match clock"), timerBar);
    clockRun->setChecked(model->state().clockRunning);
    auto* clockReset = new QPushButton(QStringLiteral("Reset"), timerBar);
    clockReset->setObjectName(QStringLiteral("chromeBtn"));
    timerLay->addWidget(clockLbl);
    timerLay->addStretch();
    timerLay->addWidget(clockRun);
    timerLay->addWidget(clockReset);
    centerLay->addWidget(timerBar);

    auto* hiddenOverlay = new QLabel(QStringLiteral("OVERLAY HIDDEN"), preview);
    hiddenOverlay->setAlignment(Qt::AlignCenter);
    hiddenOverlay->setGeometry(0, 0, 400, 280);
    hiddenOverlay->setStyleSheet(QStringLiteral(
        "background: repeating-linear-gradient(45deg,#0A0B0F,#0A0B0F 8px,#15181E 8px,#15181E 16px);"
        "color:#606878; font-weight:800; letter-spacing:2px;"));
    hiddenOverlay->hide();
    hiddenOverlay->setParent(preview);
    body->addWidget(center, 1);

    // Right style
    auto* right = new QFrame(this);
    right->setObjectName(QStringLiteral("scoreboardRight"));
    right->setFixedWidth(240);
    auto* rightLay = new QVBoxLayout(right);
    rightLay->setContentsMargins(12, 12, 12, 12);
    auto* layLab = new QLabel(QStringLiteral("LAYOUT"), right);
    layLab->setStyleSheet(QStringLiteral("color:#4F9EFF; font-weight:800; font-size:10px; letter-spacing:1.5px;"));
    rightLay->addWidget(layLab);
    auto* layoutBox = new QComboBox(right);
    layoutBox->addItems({QStringLiteral("Lower Third"), QStringLiteral("Center Banner"),
                         QStringLiteral("Corner Compact"), QStringLiteral("Full Width")});
    rightLay->addWidget(layoutBox);

    auto* themeLab = new QLabel(QStringLiteral("THEME"), right);
    themeLab->setStyleSheet(QStringLiteral("color:#A855F7; font-weight:800; font-size:10px; letter-spacing:1.5px; padding-top:12px;"));
    rightLay->addWidget(themeLab);
    auto* themeBox = new QComboBox(right);
    themeBox->addItems({QStringLiteral("Dark"), QStringLiteral("Light"), QStringLiteral("Team"),
                        QStringLiteral("Neon"), QStringLiteral("Minimal")});
    rightLay->addWidget(themeBox);
    auto* themeHint = new QLabel(QStringLiteral("Accent follows theme preset."), right);
    themeHint->setStyleSheet(QStringLiteral("color:#606878; font-size:10px;"));
    themeHint->setWordWrap(true);
    rightLay->addWidget(themeHint);
    rightLay->addStretch();
    body->addWidget(right);
    root->addLayout(body, 1);

    auto applyMeta = [=] {
        auto st = model->state();
        QString sportUi = QStringLiteral("Pool / Billiards");
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
        auto s = model->state(); s.playerA = t; model->setState(s);
    });
    connect(bName, &QLineEdit::textChanged, this, [model](const QString& t) {
        auto s = model->state(); s.playerB = t; model->setState(s);
    });
    connect(clockRun, &QCheckBox::toggled, this, [model](bool on) {
        if (on) model->startClock(); else model->stopClock();
    });
    connect(clockReset, &QPushButton::clicked, this, [model] {
        auto s = model->state();
        s.clockSeconds = 0;
        model->setState(s);
    });
    connect(overlayBtn, &QPushButton::toggled, this, [=](bool on) {
        overlayBtn->setText(on ? QStringLiteral("OVERLAY ON") : QStringLiteral("OVERLAY OFF"));
        hiddenOverlay->setVisible(!on);
        if (!on) {
            hiddenOverlay->setGeometry(preview->rect());
            hiddenOverlay->raise();
        }
    });

    connect(savePreset, &QPushButton::clicked, this, [=] {
        QSettings s(QStringLiteral("RailShotTV"), QStringLiteral("RailShotTV"));
        s.setValue(QStringLiteral("scoreboard/preset"), QJsonDocument(model->state().toJson()).toJson());
        s.setValue(QStringLiteral("scoreboard/colorA"), *colorA);
        s.setValue(QStringLiteral("scoreboard/colorB"), *colorB);
    });
    connect(loadPreset, &QPushButton::clicked, this, [=] {
        QSettings s(QStringLiteral("RailShotTV"), QStringLiteral("RailShotTV"));
        const auto raw = s.value(QStringLiteral("scoreboard/preset")).toByteArray();
        if (raw.isEmpty()) return;
        model->setState(ScoreboardState::fromJson(QJsonDocument::fromJson(raw).object()));
        *colorA = s.value(QStringLiteral("scoreboard/colorA"), *colorA).toString();
        *colorB = s.value(QStringLiteral("scoreboard/colorB"), *colorB).toString();
    });

    auto refresh = [=](const ScoreboardState& st) {
        preview->setText(QStringLiteral("%1  %2   —   %3  %4\n%5  ·  race %6")
                             .arg(st.playerA).arg(st.scoreA).arg(st.scoreB).arg(st.playerB)
                             .arg(st.sport).arg(st.raceTo));
        preview->setStyleSheet(QStringLiteral(
            "background:#0A0B0F; border:1px solid #2A2D35; font-size:28px; font-weight:800;"
            "font-family:'Bebas Neue'; color:%1;").arg(*colorA));
        const int m = st.clockSeconds / 60;
        const int sec = st.clockSeconds % 60;
        clockLbl->setText(QStringLiteral("%1:%2").arg(m, 2, 10, QLatin1Char('0')).arg(sec, 2, 10, QLatin1Char('0')));
        if (hiddenOverlay->isVisible())
            hiddenOverlay->setGeometry(preview->rect());
    };
    connect(model, &ScoreboardModel::changed, this, refresh);
    refresh(model->state());

    auto* clock = new QTimer(this);
    connect(clock, &QTimer::timeout, model, &ScoreboardModel::tickClock);
    clock->start(1000);
}

} // namespace railshot
