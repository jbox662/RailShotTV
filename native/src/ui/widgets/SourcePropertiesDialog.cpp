#include "ui/widgets/SourcePropertiesDialog.h"
#include "ui/widgets/SourcePropertiesWidget.h"
#include "core/EngineController.h"
#include "core/Types.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QJsonObject>

namespace railshot {

SourcePropertiesDialog::SourcePropertiesDialog(EngineController* engine, const QString& sourceId,
                                               QWidget* parent)
    : QDialog(parent), m_engine(engine), m_sourceId(sourceId)
{
    setObjectName(QStringLiteral("sourcePropertiesDialog"));
    setModal(true);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    resize(560, 680);
    setMinimumSize(480, 520);

    if (m_engine)
        m_engine->setSelectedSourceId(sourceId);

    setStyleSheet(QStringLiteral(
        "QDialog#sourcePropertiesDialog {"
        "  background:#12151A;"
        "  border:2px solid #4F9EFF;"
        "}"
        "QDialogButtonBox QPushButton {"
        "  min-width:88px; min-height:28px; padding:4px 14px;"
        "  background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #3A6AFF,stop:1 #1A3AFF);"
        "  border:1px solid #6B9AFF; border-radius:3px; color:#FFFFFF;"
        "  font-family:'DM Sans'; font-size:11px; font-weight:800;"
        "}"
        "QDialogButtonBox QPushButton:hover { border-color:#8AB4FF; }"
        "QDialogButtonBox QPushButton#defaultsBtn {"
        "  background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #2A2D35,stop:1 #1A1D22);"
        "  border:1px solid #3A3D45; color:#C8CAD0;"
        "}"
        "QDialogButtonBox QPushButton#defaultsBtn:hover { border-color:#4F9EFF; color:#fff; }"));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* accent = new QFrame(this);
    accent->setFixedHeight(3);
    accent->setStyleSheet(QStringLiteral(
        "background:qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        "stop:0 #4F9EFF, stop:0.5 #A855F7, stop:1 #FF5A2C); border:none;"));
    root->addWidget(accent);

    m_props = new SourcePropertiesWidget(m_engine, this);
    m_props->setDialogMode(true);
    root->addWidget(m_props, 1);

    auto* foot = new QWidget(this);
    foot->setStyleSheet(QStringLiteral(
        "background:#0F1114; border-top:1px solid #3A3D45;"));
    auto* footLay = new QHBoxLayout(foot);
    footLay->setContentsMargins(12, 10, 12, 10);

    auto* defaults = new QPushButton(QStringLiteral("Defaults"), foot);
    defaults->setObjectName(QStringLiteral("defaultsBtn"));
    defaults->setCursor(Qt::PointingHandCursor);
    connect(defaults, &QPushButton::clicked, this, &SourcePropertiesDialog::onDefaults);

    auto* box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, foot);
    box->button(QDialogButtonBox::Ok)->setText(QStringLiteral("OK"));
    box->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("Cancel"));
    box->button(QDialogButtonBox::Ok)->setDefault(true);
    box->button(QDialogButtonBox::Ok)->setCursor(Qt::PointingHandCursor);
    box->button(QDialogButtonBox::Cancel)->setCursor(Qt::PointingHandCursor);
    // Cancel keeps dialog chrome; style cancel quieter
    box->button(QDialogButtonBox::Cancel)->setStyleSheet(QStringLiteral(
        "QPushButton{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #2A2D35,stop:1 #1A1D22);"
        "border:1px solid #3A3D45;color:#C8CAD0;}"));

    connect(box, &QDialogButtonBox::accepted, this, &SourcePropertiesDialog::onOk);
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_props, &SourcePropertiesWidget::closeRequested, this, &QDialog::accept);

    footLay->addWidget(defaults);
    footLay->addStretch();
    footLay->addWidget(box);
    root->addWidget(foot);

    refreshTitle();
}

void SourcePropertiesDialog::refreshTitle()
{
    QString name = QStringLiteral("Source");
    if (m_engine) {
        if (const auto src = m_engine->selectedSource())
            name = src->name;
    }
    setWindowTitle(QStringLiteral("Properties for '%1'").arg(name));
}

void SourcePropertiesDialog::onOk()
{
    if (m_props)
        m_props->applyAndClose();
    else
        accept();
}

void SourcePropertiesDialog::onDefaults()
{
    if (!m_engine) return;
    Transform t;
    // Full-frame default for display-like sources; otherwise centered half
    const auto src = m_engine->selectedSource();
    if (src && src->type == SourceType::Display)
        t = {0, 0, 1, 1};
    else
        t = {0.1, 0.1, 0.5, 0.5};
    m_engine->updateSourceTransform(m_sourceId, t);
    QJsonObject settings;
    settings.insert(QStringLiteral("brightness"), 0);
    settings.insert(QStringLiteral("contrast"), 0);
    settings.insert(QStringLiteral("saturation"), 0);
    settings.insert(QStringLiteral("chromaKey"), false);
    settings.insert(QStringLiteral("chromaSimilarity"), 40);
    settings.insert(QStringLiteral("blur"), 0);
    settings.insert(QStringLiteral("audioVolume"), 100);
    settings.insert(QStringLiteral("audioMute"), false);
    // Preserve type-specific keys (device path, URL, etc.)
    if (src) {
        auto keep = src->settings;
        for (auto it = settings.begin(); it != settings.end(); ++it)
            keep.insert(it.key(), it.value());
        m_engine->updateSourceSettings(m_sourceId, keep);
    } else {
        m_engine->updateSourceSettings(m_sourceId, settings);
    }
    if (m_props)
        m_props->reload();
}

} // namespace railshot
