#include "ui/pages/ScoreboardPage.h"
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
#include <utility>

namespace railshot {

ScoreboardPage::ScoreboardPage(EngineController* engine, QWidget* parent)
    : QWidget(parent)
{
    auto* model = engine->scoreboard();
    auto* root = new QVBoxLayout(this);
    auto* title = new QLabel(QStringLiteral("SCOREBOARD"), this);
    title->setStyleSheet(QStringLiteral("font-weight:800; letter-spacing:2px; color:#22C55E;"));
    root->addWidget(title);

    auto* hint = new QLabel(QStringLiteral("Edits push live into Scoreboard sources. Hotkeys: Q/A · E/D · R reset · S swap."), this);
    hint->setStyleSheet(QStringLiteral("color:#606878;"));
    root->addWidget(hint);

    auto* preview = new QLabel(this);
    preview->setMinimumHeight(180);
    preview->setAlignment(Qt::AlignCenter);
    preview->setStyleSheet(QStringLiteral("background:#0A0B0F; border:1px solid #2A2D35; font-size:24px; font-weight:800;"));
    root->addWidget(preview);

    auto* form = new QFormLayout();
    auto* sport = new QComboBox(this);
    sport->addItems({QStringLiteral("8ball"), QStringLiteral("9ball"), QStringLiteral("10ball"), QStringLiteral("straight")});
    auto* theme = new QComboBox(this);
    theme->addItems({QStringLiteral("railshot"), QStringLiteral("classic"), QStringLiteral("neon"), QStringLiteral("broadcast")});
    auto* layout = new QComboBox(this);
    layout->addItems({QStringLiteral("standard"), QStringLiteral("compact"), QStringLiteral("wide")});
    auto* raceTo = new QSpinBox(this);
    raceTo->setRange(1, 25);
    raceTo->setValue(model->state().raceTo);
    sport->setCurrentText(model->state().sport);
    theme->setCurrentText(model->state().theme);
    layout->setCurrentText(model->state().layout);
    form->addRow(QStringLiteral("Sport"), sport);
    form->addRow(QStringLiteral("Theme"), theme);
    form->addRow(QStringLiteral("Layout"), layout);
    form->addRow(QStringLiteral("Race to"), raceTo);
    root->addLayout(form);

    auto* names = new QHBoxLayout();
    auto* aName = new QLineEdit(model->state().playerA, this);
    auto* bName = new QLineEdit(model->state().playerB, this);
    names->addWidget(aName);
    names->addWidget(bName);
    root->addLayout(names);

    auto* scores = new QHBoxLayout();
    auto* aMinus = new QPushButton(QStringLiteral("−"), this);
    auto* aPlus = new QPushButton(QStringLiteral("+"), this);
    auto* bMinus = new QPushButton(QStringLiteral("−"), this);
    auto* bPlus = new QPushButton(QStringLiteral("+"), this);
    auto* reset = new QPushButton(QStringLiteral("Reset"), this);
    auto* swap = new QPushButton(QStringLiteral("Swap"), this);
    auto* push = new QPushButton(QStringLiteral("Push to Program"), this);
    scores->addWidget(aMinus); scores->addWidget(aPlus);
    scores->addStretch();
    scores->addWidget(reset);
    scores->addWidget(swap);
    scores->addWidget(push);
    scores->addStretch();
    scores->addWidget(bMinus); scores->addWidget(bPlus);
    root->addLayout(scores);

    auto* clockRow = new QHBoxLayout();
    auto* clockLbl = new QLabel(this);
    auto* clockRun = new QCheckBox(QStringLiteral("Match clock"), this);
    clockRun->setChecked(model->state().clockRunning);
    auto* clockReset = new QPushButton(QStringLiteral("Reset clock"), this);
    clockRow->addWidget(clockRun);
    clockRow->addWidget(clockLbl, 1);
    clockRow->addWidget(clockReset);
    root->addLayout(clockRow);
    root->addStretch();

    auto applyMeta = [model, sport, theme, layout, raceTo] {
        auto s = model->state();
        s.sport = sport->currentText();
        s.theme = theme->currentText();
        s.layout = layout->currentText();
        s.raceTo = raceTo->value();
        model->setState(s);
    };
    connect(sport, &QComboBox::currentTextChanged, this, applyMeta);
    connect(theme, &QComboBox::currentTextChanged, this, applyMeta);
    connect(layout, &QComboBox::currentTextChanged, this, applyMeta);
    connect(raceTo, QOverload<int>::of(&QSpinBox::valueChanged), this, applyMeta);

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

    auto refresh = [preview, clockLbl, raceTo](const ScoreboardState& s) {
        preview->setText(QStringLiteral("%1  %2  —  %3  %4\nrace to %5 · %6 · %7")
                             .arg(s.playerA).arg(s.scoreA).arg(s.scoreB).arg(s.playerB)
                             .arg(s.raceTo).arg(s.theme).arg(s.sport));
        const int m = s.clockSeconds / 60;
        const int sec = s.clockSeconds % 60;
        clockLbl->setText(QStringLiteral("%1:%2").arg(m, 2, 10, QLatin1Char('0')).arg(sec, 2, 10, QLatin1Char('0')));
        Q_UNUSED(raceTo);
    };
    connect(model, &ScoreboardModel::changed, this, refresh);
    refresh(model->state());

    auto* clock = new QTimer(this);
    connect(clock, &QTimer::timeout, model, &ScoreboardModel::tickClock);
    clock->start(1000);
}

} // namespace railshot
