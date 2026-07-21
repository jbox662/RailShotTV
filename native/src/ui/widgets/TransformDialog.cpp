#include "ui/widgets/TransformDialog.h"
#include "core/EngineController.h"
#include "core/SceneGraph.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QGroupBox>
#include <QLabel>

namespace railshot {

Transform TransformDialog::s_clipboard{};
bool TransformDialog::s_hasClipboard = false;

TransformDialog::TransformDialog(EngineController* engine, const QString& sourceId, QWidget* parent)
    : QDialog(parent), m_engine(engine), m_sourceId(sourceId)
{
    setWindowTitle(QStringLiteral("Edit Transform"));
    setMinimumWidth(380);
    setStyleSheet(QStringLiteral(
        "QDialog{background:#0F1114;}"
        "QLabel{color:#C8CCD4; font-family:'DM Sans';}"
        "QGroupBox{color:#8892A4; font-weight:800; border:1px solid #2A2D35; margin-top:8px;}"
        "QDoubleSpinBox{background:#0A0C0F; border:1px solid #3A3D45; color:#E0E2E8; padding:2px 4px;}"
        "QPushButton{background:#1A1E26; border:1px solid #5A5E68; border-radius:3px;"
        "  color:#E0E2E8; font-weight:700; padding:4px 10px;}"
        "QPushButton:hover{border-color:#4F9EFF;}"));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);

    m_lockHint = new QLabel(this);
    m_lockHint->setWordWrap(true);
    m_lockHint->setStyleSheet(QStringLiteral("color:#FDBA74; font-weight:700;"));
    m_lockHint->hide();
    root->addWidget(m_lockHint);

    auto* posBox = new QGroupBox(QStringLiteral("Position / Size"), this);
    auto* posForm = new QFormLayout(posBox);
    m_x = addSpin(-2.0, 2.0, 0.001, 4);
    m_y = addSpin(-2.0, 2.0, 0.001, 4);
    m_w = addSpin(-2.0, 2.0, 0.001, 4);
    m_h = addSpin(-2.0, 2.0, 0.001, 4);
    m_rot = addSpin(-360.0, 360.0, 0.1, 1);
    m_opacity = addSpin(0.0, 1.0, 0.01, 2);
    posForm->addRow(QStringLiteral("Position X"), m_x);
    posForm->addRow(QStringLiteral("Position Y"), m_y);
    posForm->addRow(QStringLiteral("Size W"), m_w);
    posForm->addRow(QStringLiteral("Size H"), m_h);
    posForm->addRow(QStringLiteral("Rotation"), m_rot);
    posForm->addRow(QStringLiteral("Opacity"), m_opacity);
    root->addWidget(posBox);

    auto* cropBox = new QGroupBox(QStringLiteral("Crop"), this);
    auto* cropForm = new QFormLayout(cropBox);
    m_cropL = addSpin(0.0, 1.0, 0.001, 4);
    m_cropR = addSpin(0.0, 1.0, 0.001, 4);
    m_cropT = addSpin(0.0, 1.0, 0.001, 4);
    m_cropB = addSpin(0.0, 1.0, 0.001, 4);
    cropForm->addRow(QStringLiteral("Left"), m_cropL);
    cropForm->addRow(QStringLiteral("Right"), m_cropR);
    cropForm->addRow(QStringLiteral("Top"), m_cropT);
    cropForm->addRow(QStringLiteral("Bottom"), m_cropB);
    root->addWidget(cropBox);

    auto* clipRow = new QHBoxLayout();
    auto* copyBtn = new QPushButton(QStringLiteral("Copy"), this);
    m_pasteBtn = new QPushButton(QStringLiteral("Paste"), this);
    m_resetBtn = new QPushButton(QStringLiteral("Reset"), this);
    m_pasteBtn->setEnabled(hasClipboard());
    connect(copyBtn, &QPushButton::clicked, this, [this] {
        Transform t;
        t.x = m_x->value(); t.y = m_y->value();
        t.w = m_w->value(); t.h = m_h->value();
        t.rotation = m_rot->value(); t.opacity = m_opacity->value();
        t.cropLeft = m_cropL->value(); t.cropRight = m_cropR->value();
        t.cropTop = m_cropT->value(); t.cropBottom = m_cropB->value();
        copyTransform(t);
        if (m_pasteBtn) m_pasteBtn->setEnabled(true);
    });
    connect(m_pasteBtn, &QPushButton::clicked, this, [this] {
        if (!hasClipboard()) return;
        const Transform t = clipboardTransform();
        m_loading = true;
        m_x->setValue(t.x); m_y->setValue(t.y);
        m_w->setValue(t.w); m_h->setValue(t.h);
        m_rot->setValue(t.rotation); m_opacity->setValue(t.opacity);
        m_cropL->setValue(t.cropLeft); m_cropR->setValue(t.cropRight);
        m_cropT->setValue(t.cropTop); m_cropB->setValue(t.cropBottom);
        m_loading = false;
        applyToSource();
    });
    connect(m_resetBtn, &QPushButton::clicked, this, [this] {
        resetTransform(m_engine, m_sourceId);
        loadFromSource();
    });
    clipRow->addWidget(copyBtn);
    clipRow->addWidget(m_pasteBtn);
    clipRow->addWidget(m_resetBtn);
    clipRow->addStretch();
    root->addLayout(clipRow);

    auto* closeRow = new QHBoxLayout();
    closeRow->addStretch();
    auto* closeBtn = new QPushButton(QStringLiteral("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    closeRow->addWidget(closeBtn);
    root->addLayout(closeRow);

    for (auto* s : {m_x, m_y, m_w, m_h, m_rot, m_opacity, m_cropL, m_cropR, m_cropT, m_cropB})
        connect(s, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &TransformDialog::applyToSource);

    loadFromSource();
}

QDoubleSpinBox* TransformDialog::addSpin(double min, double max, double step, int decimals)
{
    auto* s = new QDoubleSpinBox(this);
    s->setRange(min, max);
    s->setSingleStep(step);
    s->setDecimals(decimals);
    return s;
}

void TransformDialog::loadFromSource()
{
    m_loading = true;
    const auto* src = m_engine->projectSnapshot().findSourceAnywhere(m_sourceId);
    if (!src) { m_loading = false; return; }
    setWindowTitle(QStringLiteral("Edit Transform — %1").arg(src->name));
    const auto& t = src->transform;
    m_x->setValue(t.x); m_y->setValue(t.y);
    m_w->setValue(t.w); m_h->setValue(t.h);
    m_rot->setValue(t.rotation); m_opacity->setValue(t.opacity);
    m_cropL->setValue(t.cropLeft); m_cropR->setValue(t.cropRight);
    m_cropT->setValue(t.cropTop); m_cropB->setValue(t.cropBottom);
    setLockedUi(src->locked);
    m_loading = false;
}

void TransformDialog::setLockedUi(bool locked)
{
    if (m_lockHint) {
        m_lockHint->setVisible(locked);
        m_lockHint->setText(locked
            ? QStringLiteral("Source is locked — unlock it in Sources or the context toolbar to edit transform.")
            : QString());
    }
    for (auto* s : {m_x, m_y, m_w, m_h, m_rot, m_opacity, m_cropL, m_cropR, m_cropT, m_cropB}) {
        if (s) s->setEnabled(!locked);
    }
    if (m_pasteBtn) m_pasteBtn->setEnabled(!locked && hasClipboard());
    if (m_resetBtn) m_resetBtn->setEnabled(!locked);
}

void TransformDialog::applyToSource()
{
    if (m_loading) return;
    m_engine->sceneGraph()->mutate([this](Project& p) {
        if (auto* s = p.findSourceAnywhere(m_sourceId)) {
            if (s->locked) return;
            s->transform.x = m_x->value(); s->transform.y = m_y->value();
            s->transform.w = m_w->value(); s->transform.h = m_h->value();
            s->transform.rotation = m_rot->value();
            s->transform.opacity = m_opacity->value();
            s->transform.cropLeft = m_cropL->value();
            s->transform.cropRight = m_cropR->value();
            s->transform.cropTop = m_cropT->value();
            s->transform.cropBottom = m_cropB->value();
        }
    });
}

void TransformDialog::copyTransform(const Transform& t)
{
    s_clipboard = t;
    s_hasClipboard = true;
}

bool TransformDialog::hasClipboard() { return s_hasClipboard; }
Transform TransformDialog::clipboardTransform() { return s_clipboard; }

void TransformDialog::pasteOnto(EngineController* engine, const QString& sourceId)
{
    if (!s_hasClipboard || !engine) return;
    engine->sceneGraph()->mutate([&](Project& p) {
        if (auto* s = p.findSourceAnywhere(sourceId)) {
            if (s->locked) return;
            s->transform = s_clipboard;
        }
    });
}

void TransformDialog::resetTransform(EngineController* engine, const QString& sourceId)
{
    if (!engine) return;
    engine->sceneGraph()->mutate([&](Project& p) {
        if (auto* s = p.findSourceAnywhere(sourceId)) {
            if (s->locked) return;
            s->transform = Transform{};
            s->transform.w = 1.0;
            s->transform.h = 1.0;
        }
    });
}

} // namespace railshot
