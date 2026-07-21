#include "ui/widgets/TransitionPanel.h"
#include "core/EngineController.h"
#include "core/SceneGraph.h"
#include "ui/Theme.h"
#include "ui/Motion.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSlider>
#include <QLabel>
#include <QGridLayout>
#include <QButtonGroup>
#include <QStyle>
#include <QInputDialog>
#include <QMenu>
#include <QAction>
#include <QCursor>
#include <QSignalBlocker>

namespace railshot {

TransitionPanel::TransitionPanel(EngineController* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setObjectName(QStringLiteral("transitionPanel"));
    setFixedWidth(148);
    // MUST scope to #transitionPanel — unscoped rules clipped scene-pad digits and opt buttons.
    setStyleSheet(QStringLiteral(
        "QWidget#transitionPanel {"
        "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #1A1D22, stop:1 #0D0F12);"
        "  border-left: 2px solid #3A6AFF88;"
        "  border-right: 2px solid #3A6AFF88;"
        "}"
        "QWidget#transitionPanel QLabel#sec {"
        "  color:#7AB8FF; font-family:'DM Sans'; font-size:9px; font-weight:800;"
        "  letter-spacing:1px; background:transparent; border:none;"
        "}"
        "QWidget#transitionPanel QLabel#hint {"
        "  color:#6B7280; font-size:9px; background:transparent; border:none;"
        "}"
        "QWidget#transitionPanel QPushButton#transTypeBtn {"
        "  background:#12151A; border:1px solid #3A3D45; border-radius:3px;"
        "  color:#D0D2D8; font-size:11px; font-weight:700; text-align:left;"
        "  padding:5px 8px; min-height:24px;"
        "}"
        "QWidget#transitionPanel QPushButton#transTypeBtn:hover {"
        "  border-color:#4F9EFF; color:#FFFFFF;"
        "}"
        "QWidget#transitionPanel QPushButton#transTypeBtn:checked {"
        "  background:#1A2A3A; border:1px solid #4F9EFF; color:#7AB8FF;"
        "}"
        "QWidget#transitionPanel QPushButton#transOptBtn {"
        "  background:#1A1D22; border:1px solid #3A3D45; border-radius:3px;"
        "  color:#A0A8B8; font-size:10px; font-weight:800;"
        "  min-width:28px; max-width:28px; min-height:24px;"
        "}"
        "QWidget#transitionPanel QPushButton#transOptBtn:hover {"
        "  border-color:#4F9EFF; color:#FFFFFF;"
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
        "  background:#C4B5FD; width:12px; height:14px; margin:-5px 0;"
        "  border:1px solid #A855F7; border-radius:3px;"
        "}"));

    auto* col = new QVBoxLayout(this);
    col->setContentsMargins(8, 8, 8, 8);
    col->setSpacing(6);

    m_go = new QPushButton(QStringLiteral("GO"), this);
    m_go->setObjectName(QStringLiteral("goButton"));
    m_go->setMinimumHeight(36);
    m_go->setCursor(Qt::PointingHandCursor);
    m_go->setToolTip(QStringLiteral("Take Preview → Program (Space)"));
    connect(m_go, &QPushButton::clicked, this, [this] {
        if (!m_engine->projectSnapshot().previewSceneId.isEmpty())
            m_engine->go(transitionTypeFromString(m_active));
    });
    col->addWidget(m_go);

    m_activeLabel = new QLabel(QStringLiteral("CUT"), this);
    m_activeLabel->setObjectName(QStringLiteral("sec"));
    m_activeLabel->setAlignment(Qt::AlignCenter);
    col->addWidget(m_activeLabel);

    auto* typeLab = new QLabel(QStringLiteral("TRANSITION"), this);
    typeLab->setObjectName(QStringLiteral("sec"));
    col->addWidget(typeLab);

    const QStringList types = {QStringLiteral("Cut"), QStringLiteral("Fade"), QStringLiteral("Merge"),
                               QStringLiteral("Wipe"), QStringLiteral("CubeZoom"), QStringLiteral("FTB")};
    auto* group = new QButtonGroup(this);
    group->setExclusive(true);
    for (const auto& t : types) {
        auto* row = new QHBoxLayout();
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(4);

        auto* b = new QPushButton(t, this);
        b->setObjectName(QStringLiteral("transTypeBtn"));
        b->setCheckable(true);
        b->setChecked(t == QLatin1String("Cut"));
        b->setCursor(Qt::PointingHandCursor);
        if (t == QLatin1String("CubeZoom"))
            b->setToolTip(QStringLiteral("Placeholder — uses Fade until true 3D cube ships"));
        else if (t == QLatin1String("FTB"))
            b->setToolTip(QStringLiteral("Fade to Black"));
        else if (t == QLatin1String("Merge"))
            b->setToolTip(QStringLiteral("Quadratic dissolve (different curve than Fade)"));
        group->addButton(b);
        m_typeButtons.push_back(b);
        connect(b, &QPushButton::clicked, this, [this, t] {
            m_active = t;
            m_activeLabel->setText(t.toUpper());
            m_engine->setTransition(transitionTypeFromString(t),
                                    m_engine->projectSnapshot().transitionMs);
            restyleTypes();
        });

        // Explicit "…" label — was a clipped ▾ that looked like a blank side strip.
        auto* opt = new QPushButton(QStringLiteral("…"), this);
        opt->setObjectName(QStringLiteral("transOptBtn"));
        opt->setCursor(Qt::PointingHandCursor);
        opt->setToolTip(QStringLiteral("%1 options (duration%2)")
                            .arg(t, t == QLatin1String("Wipe") ? QStringLiteral(", wipe direction")
                                                               : QString()));
        connect(opt, &QPushButton::clicked, this, [this, t] { showTypeOptions(t); });

        row->addWidget(b, 1);
        row->addWidget(opt);
        col->addLayout(row);
    }
    restyleTypes();

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
        m_engine->setTransition(transitionTypeFromString(m_active), v);
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

void TransitionPanel::showTypeOptions(const QString& t)
{
    QMenu menu(this);
    menu.addAction(QStringLiteral("Duration…"), this, [this, t] {
        bool ok = false;
        const int ms = QInputDialog::getInt(this, QStringLiteral("%1 Duration").arg(t),
                                            QStringLiteral("Duration (ms)"),
                                            m_engine->projectSnapshot().transitionMs,
                                            0, 5000, 50, &ok);
        if (!ok) return;
        m_active = t;
        m_activeLabel->setText(t.toUpper());
        m_engine->setTransition(transitionTypeFromString(t), ms);
        if (m_speed) {
            QSignalBlocker b(m_speed);
            m_speed->setValue(ms);
        }
        if (m_speedValue)
            m_speedValue->setText(QStringLiteral("%1 ms").arg(ms));
        restyleTypes();
    });
    if (t == QLatin1String("Wipe")) {
        menu.addSeparator();
        auto* dir = menu.addMenu(QStringLiteral("Wipe Direction"));
        const int cur = m_engine->wipeDirection();
        auto addDir = [&](const QString& label, int d) {
            auto* a = dir->addAction(label);
            a->setCheckable(true);
            a->setChecked(cur == d);
            connect(a, &QAction::triggered, this, [this, t, d] {
                m_engine->setWipeDirection(d);
                m_active = t;
                m_activeLabel->setText(t.toUpper());
                m_engine->setTransition(TransitionType::Wipe,
                                        m_engine->projectSnapshot().transitionMs);
                restyleTypes();
            });
        };
        addDir(QStringLiteral("Right →"), 0);
        addDir(QStringLiteral("← Left"), 1);
        addDir(QStringLiteral("Down ↓"), 2);
        addDir(QStringLiteral("Up ↑"), 3);
    } else if (t == QLatin1String("Merge")) {
        menu.addSeparator();
        menu.addAction(QStringLiteral("Merge uses a quadratic dissolve (≠ Fade)"))->setEnabled(false);
    } else if (t == QLatin1String("CubeZoom")) {
        menu.addSeparator();
        menu.addAction(QStringLiteral("True 3D cube deferred — uses Fade for now"))->setEnabled(false);
    } else if (t == QLatin1String("FTB")) {
        menu.addSeparator();
        menu.addAction(QStringLiteral("Fade to Black: out → swap → in"))->setEnabled(false);
    }
    menu.exec(QCursor::pos());
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

void TransitionPanel::restyleTypes()
{
    for (auto* b : m_typeButtons)
        b->setChecked(b->text() == m_active);
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
    m_go->setStyleSheet(armed
                            ? QStringLiteral(
                                  "QPushButton#goButton {"
                                  "  background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #4ADE80,stop:1 #16A34A);"
                                  "  border:2px solid #86EFAC; border-radius:4px; color:#04140A;"
                                  "  font-weight:900; font-size:14px; letter-spacing:2px; padding:8px;"
                                  "}"
                                  "QPushButton#goButton:hover {"
                                  "  background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #86EFAC,stop:1 #22C55E);"
                                  "  border:2px solid #BBF7D0;"
                                  "}"
                                  "QPushButton#goButton:disabled { background:#1A1D22; color:#505860; border-color:#2A2D35; }")
                            : QStringLiteral(
                                  "QPushButton#goButton {"
                                  "  background:#1A1D22; border:1px solid #3A3D45; border-radius:4px;"
                                  "  color:#505860; font-weight:900; font-size:14px; letter-spacing:2px; padding:8px;"
                                  "}"));
    m_go->style()->unpolish(m_go);
    m_go->style()->polish(m_go);
}

} // namespace railshot
