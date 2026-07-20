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

namespace railshot {

TransitionPanel::TransitionPanel(EngineController* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setFixedWidth(124);
    setStyleSheet(QStringLiteral(
        "background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #1A1D22, stop:1 #0D0F12);"
        "border-left: 2px solid #3A6AFF88;"
        "border-right: 2px solid #3A6AFF88;"));
    auto* col = new QVBoxLayout(this);
    col->setContentsMargins(6, 8, 6, 8);
    col->setSpacing(3);

    m_go = new QPushButton(QStringLiteral("GO"), this);
    m_go->setObjectName(QStringLiteral("goButton"));
    m_go->setMinimumHeight(36);
    m_go->setCursor(Qt::PointingHandCursor);
    connect(m_go, &QPushButton::clicked, this, [this] {
        if (!m_engine->projectSnapshot().previewSceneId.isEmpty())
            m_engine->go(transitionTypeFromString(m_active));
    });
    col->addWidget(m_go);

    m_activeLabel = new QLabel(QStringLiteral("CUT"), this);
    m_activeLabel->setAlignment(Qt::AlignCenter);
    m_activeLabel->setStyleSheet(QStringLiteral(
        "color:#7AB8FF; font-family:'DM Sans'; font-size:9px; font-weight:800; letter-spacing:1.5px;"));
    col->addWidget(m_activeLabel);

    const QStringList types = {QStringLiteral("Cut"), QStringLiteral("Fade"), QStringLiteral("Merge"),
                               QStringLiteral("Wipe"), QStringLiteral("CubeZoom"), QStringLiteral("FTB")};
    auto* group = new QButtonGroup(this);
    group->setExclusive(true);
    for (const auto& t : types) {
        auto* row = new QHBoxLayout();
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(0);
        auto* b = new QPushButton(t, this);
        b->setObjectName(QStringLiteral("transTypeBtn"));
        b->setCheckable(true);
        b->setChecked(t == QLatin1String("Cut"));
        b->setCursor(Qt::PointingHandCursor);
        if (t == QLatin1String("CubeZoom"))
            b->setToolTip(QStringLiteral("Uses a crossfade for now (true 3D cube deferred)"));
        group->addButton(b);
        m_typeButtons.push_back(b);
        connect(b, &QPushButton::clicked, this, [this, t] {
            m_active = t;
            m_activeLabel->setText(t.toUpper());
            m_engine->setTransition(transitionTypeFromString(t),
                                    m_engine->projectSnapshot().transitionMs);
            restyleTypes();
        });
        auto* opt = new QPushButton(QStringLiteral("▾"), this);
        opt->setObjectName(QStringLiteral("transOptBtn"));
        opt->setFixedWidth(18);
        opt->setCursor(Qt::PointingHandCursor);
        opt->setToolTip(QStringLiteral("%1 options").arg(t));
        connect(opt, &QPushButton::clicked, this, [this, t] {
            bool ok = false;
            const int ms = QInputDialog::getInt(this, QStringLiteral("%1 Duration").arg(t),
                                                QStringLiteral("Duration (ms)"),
                                                m_engine->projectSnapshot().transitionMs,
                                                0, 5000, 50, &ok);
            if (!ok) return;
            m_active = t;
            m_activeLabel->setText(t.toUpper());
            m_engine->setTransition(transitionTypeFromString(t), ms);
            restyleTypes();
        });
        row->addWidget(b, 1);
        row->addWidget(opt);
        col->addLayout(row);
    }
    restyleTypes();

    auto* grid = new QGridLayout();
    grid->setSpacing(2);
    grid->setContentsMargins(0, 6, 0, 2);
    for (int i = 0; i < 8; ++i) {
        auto* b = new QPushButton(QString::number(i + 1), this);
        b->setObjectName(QStringLiteral("scenePadBtn"));
        b->setCursor(Qt::PointingHandCursor);
        connect(b, &QPushButton::clicked, this, [this, i] {
            auto p = m_engine->projectSnapshot();
            if (i < p.scenes.size()) {
                m_engine->setPreviewScene(p.scenes[i].id);
            }
        });
        grid->addWidget(b, i / 4, i % 4);
        m_scenePad.push_back(b);
    }
    col->addLayout(grid);

    auto* speedLabel = new QLabel(QStringLiteral("Speed"), this);
    speedLabel->setAlignment(Qt::AlignCenter);
    speedLabel->setStyleSheet(QStringLiteral("color:#606878; font-size:9px; font-family:'DM Sans';"));
    col->addWidget(speedLabel);
    auto* speed = new QSlider(Qt::Horizontal, this);
    speed->setObjectName(QStringLiteral("speedSlider"));
    speed->setRange(100, 2000);
    speed->setValue(500);
    connect(speed, &QSlider::valueChanged, this, [this](int v) {
        m_engine->setTransition(transitionTypeFromString(m_active), v);
    });
    col->addWidget(speed);
    col->addStretch();

    connect(m_engine->sceneGraph(), &SceneGraph::projectChanged, this, [this] { refreshScenePad(); updateGoArmed(); });
    connect(m_engine, &EngineController::telemetryUpdated, this, [this](const TelemetrySnapshot&) { refreshScenePad(); });
    refreshScenePad();
    updateGoArmed();
    motion::installPressScale(m_go);
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
            b->setProperty("role", QVariant());
            b->style()->unpolish(b);
            b->style()->polish(b);
            continue;
        }
        b->setEnabled(true);
        const auto& sc = p.scenes[i];
        if (sc.id == p.previewSceneId)
            b->setProperty("role", QStringLiteral("preview"));
        else if (sc.id == p.programSceneId)
            b->setProperty("role", QStringLiteral("program"));
        else
            b->setProperty("role", QVariant());
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
                                  "}")
                            : QString());
    m_go->style()->unpolish(m_go);
    m_go->style()->polish(m_go);
}

} // namespace railshot
