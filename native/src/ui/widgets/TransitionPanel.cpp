#include "ui/widgets/TransitionPanel.h"
#include "core/EngineController.h"
#include <QVBoxLayout>
#include <QButtonGroup>
#include <QSlider>
#include <QLabel>
#include <QGridLayout>

namespace railshot {

TransitionPanel::TransitionPanel(EngineController* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setFixedWidth(120);
    setStyleSheet(QStringLiteral("background:#141619; border-left:1px solid #2A2D35; border-right:1px solid #2A2D35;"));
    auto* col = new QVBoxLayout(this);
    col->setContentsMargins(6, 8, 6, 8);
    col->setSpacing(4);

    m_go = new QPushButton(QStringLiteral("GO"), this);
    m_go->setObjectName(QStringLiteral("goButton"));
    m_go->setMinimumHeight(40);
    connect(m_go, &QPushButton::clicked, this, [this] {
        m_engine->go();
    });
    col->addWidget(m_go);

    auto* label = new QLabel(QStringLiteral("CUT"), this);
    label->setAlignment(Qt::AlignCenter);
    label->setObjectName(QStringLiteral("mono"));
    col->addWidget(label);

    const QStringList types = {QStringLiteral("Cut"), QStringLiteral("Fade"), QStringLiteral("Wipe"),
                               QStringLiteral("Merge"), QStringLiteral("CubeZoom"), QStringLiteral("FTB")};
    for (const auto& t : types) {
        auto* b = new QPushButton(t, this);
        connect(b, &QPushButton::clicked, this, [this, t, label] {
            m_active = t;
            label->setText(t.toUpper());
            m_engine->setTransition(transitionTypeFromString(t),
                                    m_engine->projectSnapshot().transitionMs);
        });
        col->addWidget(b);
    }

    auto* grid = new QGridLayout();
    for (int i = 0; i < 8; ++i) {
        auto* b = new QPushButton(QString::number(i + 1), this);
        connect(b, &QPushButton::clicked, this, [this, i] {
            auto p = m_engine->projectSnapshot();
            if (i < p.scenes.size())
                m_engine->setPreviewScene(p.scenes[i].id);
        });
        grid->addWidget(b, i / 4, i % 4);
    }
    col->addLayout(grid);

    auto* speedLabel = new QLabel(QStringLiteral("Speed"), this);
    speedLabel->setAlignment(Qt::AlignCenter);
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
