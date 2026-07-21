#include "ui/widgets/TransitionPanel.h"
#include "core/EngineController.h"
#include "core/SceneGraph.h"
#include "core/Types.h"
#include "ui/Theme.h"
#include "ui/Motion.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSlider>
#include <QLabel>
#include <QGridLayout>
#include <QStyle>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QCursor>
#include <QSignalBlocker>
#include <QFontMetrics>

namespace railshot {

namespace {

struct TransEffect {
    const char* name;
    TransitionType type;
    int wipeDir; // -1 = leave engine wipe direction unchanged
};

/// Wirecast-style catalog — each type has a dedicated dual-texture shader path.
constexpr TransEffect kEffects[] = {
    {"3D Plane", TransitionType::Plane3D, -1},
    {"Bands", TransitionType::Bands, -1},
    {"Clock Wipe", TransitionType::ClockWipe, -1},
    {"Cross Blur", TransitionType::CrossBlur, -1},
    {"Cross Dissolve", TransitionType::CrossDissolve, -1},
    {"Crosshair", TransitionType::Crosshair, -1},
    {"Radial Wipe", TransitionType::RadialWipe, -1},
    {"Swap", TransitionType::Swap, -1},
    {"Flip Over", TransitionType::FlipOver, -1},
    {"Grid Wipe", TransitionType::GridWipe, -1},
    {"Curtain Drop Wipe", TransitionType::CurtainDrop, -1},
    {"Fade to Black", TransitionType::FTB, -1},
    {"Fade to White", TransitionType::FadeToWhite, -1},
    {"Circle Wipe", TransitionType::CircleWipe, -1},
    {"Vacuum", TransitionType::Vacuum, -1},
    {"Wave Wipe", TransitionType::WaveWipe, -1},
    {"Push", TransitionType::Push, 0},
    {"Windshield Wipe", TransitionType::WindshieldWipe, -1},
    {"Fly Over", TransitionType::FlyOver, -1},
    {"RGB Channels", TransitionType::RgbChannels, -1},
};

constexpr int kEffectCount = int(sizeof(kEffects) / sizeof(kEffects[0]));

const TransEffect* findEffect(const QString& name)
{
    for (int i = 0; i < kEffectCount; ++i) {
        if (name == QLatin1String(kEffects[i].name))
            return &kEffects[i];
    }
    return &kEffects[4]; // Cross Dissolve
}

QString wirecastMenuStyle()
{
    return QStringLiteral(
        "QMenu {"
        "  background:#1A1C20; color:#F0F0F0;"
        "  border:1px solid #3A3D45; padding:4px 0;"
        "  font-family:'DM Sans','Segoe UI',sans-serif; font-size:12px;"
        "}"
        "QMenu::item {"
        "  padding:6px 28px 6px 28px; background:transparent;"
        "}"
        "QMenu::item:selected {"
        "  background:#2E3238;"
        "}"
        "QMenu::indicator {"
        "  width:14px; height:14px; margin-left:8px;"
        "}"
        "QMenu::separator {"
        "  height:1px; background:#3A3D45; margin:4px 10px;"
        "}");
}

} // namespace

TransitionPanel::TransitionPanel(EngineController* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setObjectName(QStringLiteral("transitionPanel"));
    setFixedWidth(158);
    setStyleSheet(QStringLiteral(
        "QWidget#transitionPanel {"
        "  background:#141619;"
        "  border-left:1px solid #2A2D35;"
        "  border-right:1px solid #2A2D35;"
        "}"
        "QWidget#transitionPanel QLabel#sec {"
        "  color:#8892A4; font-family:'DM Sans'; font-size:8px; font-weight:800;"
        "  letter-spacing:1px; background:transparent; border:none;"
        "}"
        "QWidget#transitionPanel QLabel#hint {"
        "  color:#6B7280; font-size:9px; background:transparent; border:none;"
        "}"
        // Wirecast-like chrome dropdowns
        "QWidget#transitionPanel QPushButton#wcDrop {"
        "  background:#2A2C30; border:1px solid #5A5E66; border-radius:4px;"
        "  color:#E8E8EC; font-size:11px; font-weight:600; text-align:left;"
        "  padding:6px 22px 6px 10px; min-height:28px;"
        "}"
        "QWidget#transitionPanel QPushButton#wcDrop:hover {"
        "  border-color:#8A8E96; color:#FFFFFF;"
        "}"
        "QWidget#transitionPanel QPushButton#wcDrop[wirecastActive=\"true\"] {"
        "  background:#C9A227; border:1px solid #A6851A; color:#1A1608;"
        "  font-weight:700;"
        "}"
        "QWidget#transitionPanel QPushButton#wcDrop[wirecastActive=\"true\"]:hover {"
        "  background:#D4AF37; border-color:#C9A227; color:#1A1608;"
        "}"
        "QWidget#transitionPanel QPushButton#scenePadBtn {"
        "  background:#12151A; border:1px solid #3A3D45; border-radius:3px;"
        "  color:#E5E7EB; font-family:'JetBrains Mono','Consolas',monospace;"
        "  font-size:12px; font-weight:800; min-height:28px; min-width:28px;"
        "  padding:0;"
        "}"
        "QWidget#transitionPanel QPushButton#scenePadBtn:hover {"
        "  border-color:#4F9EFF;"
        "}"
        "QWidget#transitionPanel QPushButton#scenePadBtn:disabled {"
        "  color:#3A3D45; border-color:#1F2329;"
        "}"
        "QWidget#transitionPanel QPushButton#scenePadBtn[role=\"preview\"] {"
        "  background:#0A2818; border:2px solid #22C55E; color:#4ADE80;"
        "}"
        "QWidget#transitionPanel QPushButton#scenePadBtn[role=\"program\"] {"
        "  background:#2A1408; border:2px solid #FF5A2C; color:#FF8A6A;"
        "}"
        "QWidget#transitionPanel QSlider#speedSlider::groove:horizontal {"
        "  background:#1A1D22; height:6px; border:1px solid #3A3D45; border-radius:3px;"
        "}"
        "QWidget#transitionPanel QSlider#speedSlider::handle:horizontal {"
        "  background:#C9A227; width:12px; height:14px; margin:-5px 0;"
        "  border:1px solid #E8C84A; border-radius:3px;"
        "}"));

    auto* col = new QVBoxLayout(this);
    col->setContentsMargins(8, 10, 8, 8);
    col->setSpacing(6);

    auto* takeLab = new QLabel(QStringLiteral("TAKE"), this);
    takeLab->setObjectName(QStringLiteral("sec"));
    col->addWidget(takeLab);

    // Wirecast row (stacked in our vertical strip): mode → effect → go
    m_modeBtn = new QPushButton(this);
    m_modeBtn->setObjectName(QStringLiteral("wcDrop"));
    m_modeBtn->setCursor(Qt::PointingHandCursor);
    m_modeBtn->setToolTip(QStringLiteral("Cut = instant · Smooth = selected effect"));
    connect(m_modeBtn, &QPushButton::clicked, this, &TransitionPanel::showModeMenu);
    col->addWidget(m_modeBtn);

    m_effectBtn = new QPushButton(this);
    m_effectBtn->setObjectName(QStringLiteral("wcDrop"));
    m_effectBtn->setCursor(Qt::PointingHandCursor);
    m_effectBtn->setToolTip(QStringLiteral("Transition effect (used when mode is Smooth)"));
    connect(m_effectBtn, &QPushButton::clicked, this, &TransitionPanel::showEffectMenu);
    col->addWidget(m_effectBtn);

    m_go = new QPushButton(QStringLiteral("▶  ○"), this);
    m_go->setObjectName(QStringLiteral("goButton"));
    m_go->setMinimumHeight(34);
    m_go->setCursor(Qt::PointingHandCursor);
    m_go->setToolTip(QStringLiteral("Take Preview → Program (Space)"));
    connect(m_go, &QPushButton::clicked, this, [this] {
        if (m_engine->projectSnapshot().previewSceneId.isEmpty())
            return;
        applyEffectToEngine();
        m_engine->go(takeType());
    });
    col->addWidget(m_go);

    updateModeButton();
    updateEffectButton();
    applyEffectToEngine();

    auto* padLab = new QLabel(QStringLiteral("PREVIEW SCENES"), this);
    padLab->setObjectName(QStringLiteral("sec"));
    col->addWidget(padLab);
    auto* padHint = new QLabel(QStringLiteral("Tap 1–8 → load Preview"), this);
    padHint->setObjectName(QStringLiteral("hint"));
    padHint->setAlignment(Qt::AlignCenter);
    col->addWidget(padHint);

    auto* grid = new QGridLayout();
    grid->setSpacing(4);
    grid->setContentsMargins(0, 2, 0, 2);
    for (int i = 0; i < 8; ++i) {
        auto* b = new QPushButton(QString::number(i + 1), this);
        b->setObjectName(QStringLiteral("scenePadBtn"));
        b->setCursor(Qt::PointingHandCursor);
        b->setFixedSize(30, 28);
        connect(b, &QPushButton::clicked, this, [this, i] {
            auto p = m_engine->projectSnapshot();
            if (i < p.scenes.size())
                m_engine->setPreviewScene(p.scenes[i].id);
        });
        grid->addWidget(b, i / 4, i % 4, Qt::AlignCenter);
        m_scenePad.push_back(b);
    }
    col->addLayout(grid);

    auto* speedLab = new QLabel(QStringLiteral("DURATION"), this);
    speedLab->setObjectName(QStringLiteral("sec"));
    col->addWidget(speedLab);

    auto* speedRow = new QHBoxLayout();
    m_speedValue = new QLabel(QStringLiteral("500 ms"), this);
    m_speedValue->setObjectName(QStringLiteral("hint"));
    m_speedValue->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_speedValue->setMinimumWidth(52);
    speedRow->addStretch(1);
    speedRow->addWidget(m_speedValue);
    col->addLayout(speedRow);

    m_speed = new QSlider(Qt::Horizontal, this);
    m_speed->setObjectName(QStringLiteral("speedSlider"));
    m_speed->setRange(0, 2000);
    m_speed->setSingleStep(50);
    m_speed->setPageStep(100);
    m_speed->setValue(m_engine->projectSnapshot().transitionMs > 0
                          ? m_engine->projectSnapshot().transitionMs
                          : 500);
    m_speed->setToolTip(QStringLiteral("Transition duration (ms)"));
    connect(m_speed, &QSlider::valueChanged, this, [this](int v) {
        m_speedValue->setText(QStringLiteral("%1 ms").arg(v));
        applyEffectToEngine();
    });
    m_speedValue->setText(QStringLiteral("%1 ms").arg(m_speed->value()));
    col->addWidget(m_speed);
    col->addStretch();

    connect(m_engine->sceneGraph(), &SceneGraph::projectChanged, this, [this] {
        refreshScenePad();
        updateGoArmed();
        syncSpeedFromProject();
    });
    connect(m_engine, &EngineController::telemetryUpdated, this, [this](const TelemetrySnapshot&) {
        refreshScenePad();
    });
    refreshScenePad();
    updateGoArmed();
    motion::installPressScale(m_go);
}

TransitionType TransitionPanel::takeType() const
{
    if (m_mode == QLatin1String("Cut"))
        return TransitionType::Cut;
    // Smooth with no exotic pick still dissolves
    if (m_mode == QLatin1String("Smooth") && m_effect.isEmpty())
        return TransitionType::CrossDissolve;
    return findEffect(m_effect)->type;
}

void TransitionPanel::applyEffectToEngine()
{
    const auto* fx = findEffect(m_effect);
    if (fx->wipeDir >= 0)
        m_engine->setWipeDirection(fx->wipeDir);
    const int ms = m_speed ? m_speed->value() : m_engine->projectSnapshot().transitionMs;
    // Keep engine configured for Smooth takes; Cut is applied at go() time.
    m_engine->setTransition(fx->type, ms);
}

void TransitionPanel::updateModeButton()
{
    if (!m_modeBtn) return;
    m_modeBtn->setText(QStringLiteral("%1   ▾").arg(m_mode));
    m_modeBtn->setProperty("wirecastActive", m_mode != QLatin1String("Cut"));
    m_modeBtn->style()->unpolish(m_modeBtn);
    m_modeBtn->style()->polish(m_modeBtn);
}

void TransitionPanel::updateEffectButton()
{
    if (!m_effectBtn) return;
    // Elide long names to fit strip width
    const QFontMetrics fm(m_effectBtn->font());
    const QString shown = fm.elidedText(m_effect, Qt::ElideRight, 110);
    m_effectBtn->setText(QStringLiteral("%1   ▾").arg(shown));
    m_effectBtn->setProperty("wirecastActive", true); // amber like Wirecast selected effect
    m_effectBtn->style()->unpolish(m_effectBtn);
    m_effectBtn->style()->polish(m_effectBtn);
}

void TransitionPanel::showModeMenu()
{
    QMenu menu(this);
    menu.setStyleSheet(wirecastMenuStyle());
    auto* group = new QActionGroup(&menu);
    group->setExclusive(true);
    auto addMode = [&](const QString& name) {
        auto* a = menu.addAction(name);
        a->setCheckable(true);
        a->setChecked(m_mode == name);
        group->addAction(a);
        connect(a, &QAction::triggered, this, [this, name] {
            m_mode = name;
            updateModeButton();
            applyEffectToEngine();
        });
    };
    addMode(QStringLiteral("Cut"));
    addMode(QStringLiteral("Smooth"));
    menu.exec(m_modeBtn->mapToGlobal(QPoint(0, m_modeBtn->height())));
}

void TransitionPanel::showEffectMenu()
{
    QMenu menu(this);
    menu.setStyleSheet(wirecastMenuStyle());
    auto* group = new QActionGroup(&menu);
    group->setExclusive(true);

    // Mirror Wirecast: Cut + Smooth at top, then full effect list
    auto addItem = [&](const QString& name, bool isEffect) {
        auto* a = menu.addAction(name);
        a->setCheckable(true);
        if (isEffect)
            a->setChecked(m_effect == name);
        else
            a->setChecked(false);
        group->addAction(a);
        connect(a, &QAction::triggered, this, [this, name, isEffect] {
            if (name == QLatin1String("Cut")) {
                m_mode = QStringLiteral("Cut");
                updateModeButton();
                return;
            }
            if (name == QLatin1String("Smooth")) {
                m_mode = QStringLiteral("Smooth");
                updateModeButton();
                applyEffectToEngine();
                return;
            }
            if (isEffect) {
                m_effect = name;
                m_mode = QStringLiteral("Smooth");
                updateModeButton();
                updateEffectButton();
                applyEffectToEngine();
            }
        });
    };

    addItem(QStringLiteral("Cut"), false);
    addItem(QStringLiteral("Smooth"), false);
    menu.addSeparator();
    for (int i = 0; i < kEffectCount; ++i)
        addItem(QString::fromUtf8(kEffects[i].name), true);
    menu.exec(m_effectBtn->mapToGlobal(QPoint(0, m_effectBtn->height())));
}

void TransitionPanel::syncSpeedFromProject()
{
    if (!m_speed || m_speed->isSliderDown()) return;
    const int ms = m_engine->projectSnapshot().transitionMs;
    QSignalBlocker b(m_speed);
    m_speed->setValue(ms);
    if (m_speedValue)
        m_speedValue->setText(QStringLiteral("%1 ms").arg(ms));
}

void TransitionPanel::refreshScenePad()
{
    const auto p = m_engine->projectSnapshot();
    for (int i = 0; i < m_scenePad.size(); ++i) {
        auto* b = m_scenePad[i];
        if (i >= p.scenes.size()) {
            b->setEnabled(false);
            b->setToolTip(QStringLiteral("No scene in slot %1").arg(i + 1));
            b->setProperty("role", QVariant());
            b->style()->unpolish(b);
            b->style()->polish(b);
            continue;
        }
        b->setEnabled(true);
        const auto& sc = p.scenes[i];
        QString tip = sc.name;
        if (sc.id == p.previewSceneId) {
            b->setProperty("role", QStringLiteral("preview"));
            tip += QStringLiteral("  ·  Preview");
        } else if (sc.id == p.programSceneId) {
            b->setProperty("role", QStringLiteral("program"));
            tip += QStringLiteral("  ·  Program");
        } else {
            b->setProperty("role", QVariant());
        }
        b->setToolTip(tip);
        b->style()->unpolish(b);
        b->style()->polish(b);
        b->update();
    }
}

void TransitionPanel::updateGoArmed()
{
    const bool armed = !m_engine->projectSnapshot().previewSceneId.isEmpty();
    m_go->setEnabled(armed);
    // Wirecast-like dark chrome take button (not giant green slab)
    m_go->setStyleSheet(armed
                            ? QStringLiteral(
                                  "QPushButton#goButton {"
                                  "  background:#2A2C30; border:1px solid #5A5E66; border-radius:4px;"
                                  "  color:#F0F0F0; font-weight:700; font-size:14px; padding:8px;"
                                  "}"
                                  "QPushButton#goButton:hover {"
                                  "  background:#34363C; border-color:#8A8E96; color:#FFFFFF;"
                                  "}"
                                  "QPushButton#goButton:pressed {"
                                  "  background:#1E2024;"
                                  "}")
                            : QStringLiteral(
                                  "QPushButton#goButton {"
                                  "  background:#1A1C20; border:1px solid #2A2D35; border-radius:4px;"
                                  "  color:#505860; font-weight:700; font-size:14px; padding:8px;"
                                  "}"));
    m_go->style()->unpolish(m_go);
    m_go->style()->polish(m_go);
}

} // namespace railshot
