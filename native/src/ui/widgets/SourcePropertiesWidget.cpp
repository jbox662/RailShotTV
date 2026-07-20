#include "ui/widgets/SourcePropertiesWidget.h"
#include "core/EngineController.h"
#include "core/SceneGraph.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QScrollArea>

namespace railshot {

SourcePropertiesWidget::SourcePropertiesWidget(EngineController* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setFixedWidth(220);
    setStyleSheet(QStringLiteral("background:#0F1114; border-left:1px solid #1A1D24;"));
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    auto* title = new QLabel(QStringLiteral("SOURCE"), this);
    title->setStyleSheet(QStringLiteral("color:#3B82F6; font-weight:800; letter-spacing:1px;"));
    root->addWidget(title);

    m_empty = new QLabel(QStringLiteral("Select a source on Preview"), this);
    m_empty->setWordWrap(true);
    m_empty->setStyleSheet(QStringLiteral("color:#606878;"));
    root->addWidget(m_empty);

    m_formHost = new QWidget(this);
    auto* form = new QFormLayout(m_formHost);
    form->setContentsMargins(0, 0, 0, 0);
    m_name = new QLineEdit(m_formHost);
    m_visible = new QCheckBox(QStringLiteral("Visible"), m_formHost);
    m_locked = new QCheckBox(QStringLiteral("Locked"), m_formHost);
    m_opacity = new QDoubleSpinBox(m_formHost);
    m_opacity->setRange(0.0, 1.0);
    m_opacity->setSingleStep(0.05);
    m_opacity->setDecimals(2);
    auto makeCrop = [this] {
        auto* s = new QDoubleSpinBox(m_formHost);
        s->setRange(0.0, 0.49);
        s->setSingleStep(0.01);
        s->setDecimals(3);
        return s;
    };
    m_cropL = makeCrop();
    m_cropR = makeCrop();
    m_cropT = makeCrop();
    m_cropB = makeCrop();
    form->addRow(QStringLiteral("Name"), m_name);
    form->addRow(m_visible);
    form->addRow(m_locked);
    form->addRow(QStringLiteral("Opacity"), m_opacity);
    form->addRow(QStringLiteral("Crop L"), m_cropL);
    form->addRow(QStringLiteral("Crop R"), m_cropR);
    form->addRow(QStringLiteral("Crop T"), m_cropT);
    form->addRow(QStringLiteral("Crop B"), m_cropB);

    auto* zRow = new QHBoxLayout();
    auto* zUp = new QPushButton(QStringLiteral("Z ↑"), m_formHost);
    auto* zDown = new QPushButton(QStringLiteral("Z ↓"), m_formHost);
    zRow->addWidget(zUp);
    zRow->addWidget(zDown);
    form->addRow(zRow);

    root->addWidget(m_formHost);
    root->addStretch();

    connect(m_name, &QLineEdit::editingFinished, this, [this] {
        if (m_block || !m_engine) return;
        m_engine->setSourceName(m_engine->selectedSourceId(), m_name->text());
    });
    connect(m_visible, &QCheckBox::toggled, this, [this](bool on) {
        if (m_block || !m_engine) return;
        m_engine->setSourceVisible(m_engine->selectedSourceId(), on);
    });
    connect(m_locked, &QCheckBox::toggled, this, [this](bool on) {
        if (m_block || !m_engine) return;
        m_engine->setSourceLocked(m_engine->selectedSourceId(), on);
    });
    auto apply = [this](double) { applyTransformFromUi(); };
    connect(m_opacity, qOverload<double>(&QDoubleSpinBox::valueChanged), this, apply);
    connect(m_cropL, qOverload<double>(&QDoubleSpinBox::valueChanged), this, apply);
    connect(m_cropR, qOverload<double>(&QDoubleSpinBox::valueChanged), this, apply);
    connect(m_cropT, qOverload<double>(&QDoubleSpinBox::valueChanged), this, apply);
    connect(m_cropB, qOverload<double>(&QDoubleSpinBox::valueChanged), this, apply);
    connect(zUp, &QPushButton::clicked, this, [this] {
        if (m_engine) m_engine->moveSourceZOrder(m_engine->selectedSourceId(), 1);
    });
    connect(zDown, &QPushButton::clicked, this, [this] {
        if (m_engine) m_engine->moveSourceZOrder(m_engine->selectedSourceId(), -1);
    });

    connect(engine, &EngineController::selectedSourceChanged, this, [this](const QString&) { rebuild(); });
    connect(engine->sceneGraph(), &SceneGraph::projectChanged, this, [this] { rebuild(); });
    rebuild();
}

void SourcePropertiesWidget::applyTransformFromUi()
{
    if (m_block || !m_engine) return;
    auto src = m_engine->selectedSource();
    if (!src) return;
    Transform t = src->transform;
    t.opacity = m_opacity->value();
    t.cropLeft = m_cropL->value();
    t.cropRight = m_cropR->value();
    t.cropTop = m_cropT->value();
    t.cropBottom = m_cropB->value();
    m_engine->updateSourceTransform(src->id, t);
}

void SourcePropertiesWidget::rebuild()
{
    m_block = true;
    const auto src = m_engine ? m_engine->selectedSource() : std::nullopt;
    const bool has = src.has_value();
    m_empty->setVisible(!has);
    m_formHost->setVisible(has);
    if (has) {
        m_name->setText(src->name);
        m_visible->setChecked(src->visible);
        m_locked->setChecked(src->locked);
        m_opacity->setValue(src->transform.opacity);
        m_cropL->setValue(src->transform.cropLeft);
        m_cropR->setValue(src->transform.cropRight);
        m_cropT->setValue(src->transform.cropTop);
        m_cropB->setValue(src->transform.cropBottom);
    }
    m_block = false;
}

} // namespace railshot
