#include "ui/widgets/TransitionPanel.h"
#include "core/EngineController.h"
#include <QVBoxLayout>
#include <QSlider>
#include <QLabel>
#include <QGridLayout>

namespace railshot {

TransitionPanel::TransitionPanel(EngineController* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setFixedWidth(120);
    setStyleSheet(QStringLiteral(
        "background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #16191E, stop:1 #101318);"
        "border-left: 1px solid #2A2D35;"
        "border-right: 1px solid #2A2D35;"));
    auto* col = new QVBoxLayout(this);
    col->setContentsMargins(6, 10, 6, 8);
    col->setSpacing(5);

    m_go = new QPushButton(QStringLiteral("GO"), this);
    m_go->setObjectName(QStringLiteral("goButton"));
    m_go->setMinimumHeight(44);
    m_go->setCursor(Qt::PointingHandCursor);
    connect(m_go, &QPushButton::clicked, this, [this] { m_engine->go(); });
    col->addWidget(m_go);

    auto* label = new QLabel(QStringLiteral("CUT"), this);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet(QStringLiteral(
        "color:#4F9EFF; font-family:'JetBrains Mono','Consolas',monospace;"
        "font-size:10px; letter-spacing:2px; font-weight:700;"));
    col->addWidget(label);

    const QStringList types = {QStringLiteral("Cut"), QStringLiteral("Fade"), QStringLiteral("Wipe"),
                               QStringLiteral("Merge"), QStringLiteral("CubeZoom"), QStringLiteral("FTB")};
    for (const auto& t : types) {
        auto* b = new QPushButton(t, this);
        if (t == QLatin1String("Cut"))
            b->setObjectName(QStringLiteral("cutButton"));
        b->setCursor(Qt::PointingHandCursor);
        connect(b, &QPushButton::clicked, this, [this, t, label, types] {
            m_active = t;
            label->setText(t.toUpper());
            m_engine->setTransition(transitionTypeFromString(t),
                                    m_engine->projectSnapshot().transitionMs);
            // Restyle siblings: Cut stays blue when selected conceptually via label
            Q_UNUSED(types);
        });
        col->addWidget(b);
    }

    auto* grid = new QGridLayout();
    grid->setSpacing(3);
    for (int i = 0; i < 8; ++i) {
        auto* b = new QPushButton(QString::number(i + 1), this);
        b->setFixedHeight(22);
        b->setStyleSheet(QStringLiteral(
            "QPushButton { font-size:10px; padding:2px; background:#1A1D22; border:1px solid #2A2D35; }"
            "QPushButton:hover { border-color:#4F9EFF; color:#4F9EFF; }"));
        connect(b, &QPushButton::clicked, this, [this, i] {
            auto p = m_engine->projectSnapshot();
            if (i < p.scenes.size())
                m_engine->setPreviewScene(p.scenes[i].id);
        });
        grid->addWidget(b, i / 4, i % 4);
    }
    col->addLayout(grid);

    auto* speedLabel = new QLabel(QStringLiteral("SPEED"), this);
    speedLabel->setAlignment(Qt::AlignCenter);
    speedLabel->setStyleSheet(QStringLiteral("color:#606878; font-size:9px; letter-spacing:1px; font-weight:700;"));
    col->addWidget(speedLabel);
    auto* speed = new QSlider(Qt::Horizontal, this);
    speed->setRange(100, 2000);
    speed->setValue(500);
    connect(speed, &QSlider::valueChanged, this, [this](int v) {
        m_engine->setTransition(transitionTypeFromString(m_active), v);
    });
    col->addWidget(speed);
    col->addStretch();
}

} // namespace railshot
