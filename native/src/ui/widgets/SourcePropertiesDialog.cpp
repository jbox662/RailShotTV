#include "ui/widgets/SourcePropertiesDialog.h"
#include "ui/widgets/SourcePropertiesWidget.h"
#include "compositor/D3D11Compositor.h"
#include "capture/CaptureManager.h"
#include "capture/IVideoSource.h"
#include "core/EngineController.h"
#include "core/Types.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QJsonObject>
#include <QTimer>
#include <QPainter>
#include <QPaintEvent>

namespace railshot {

SourcePropsPreviewWidget::SourcePropsPreviewWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(200);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAttribute(Qt::WA_OpaquePaintEvent);
}

void SourcePropsPreviewWidget::setFrame(const QImage& img)
{
    m_frame = img;
    if (!m_frame.isNull())
        m_placeholder.clear();
    update();
}

void SourcePropsPreviewWidget::setPlaceholder(const QString& msg)
{
    m_placeholder = msg;
    m_frame = {};
    update();
}

void SourcePropsPreviewWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.fillRect(rect(), QColor(8, 10, 13));
    if (!m_frame.isNull()) {
        const QSize box = size();
        QImage scaled = m_frame.scaled(box, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        const int x = (box.width() - scaled.width()) / 2;
        const int y = (box.height() - scaled.height()) / 2;
        p.drawImage(x, y, scaled);
        return;
    }
    p.setPen(QColor(64, 69, 80));
    QFont iconFont = p.font();
    iconFont.setPointSize(22);
    p.setFont(iconFont);
    p.drawText(rect().adjusted(0, -18, 0, 0), Qt::AlignCenter, QStringLiteral("▣"));
    QFont msgFont = p.font();
    msgFont.setFamily(QStringLiteral("DM Sans"));
    msgFont.setPointSize(10);
    msgFont.setBold(true);
    p.setFont(msgFont);
    p.setPen(QColor(58, 69, 96));
    p.drawText(rect().adjusted(0, 28, 0, 0), Qt::AlignCenter,
               m_placeholder.isEmpty() ? QStringLiteral("No Signal") : m_placeholder);
}

SourcePropertiesDialog::SourcePropertiesDialog(EngineController* engine, const QString& sourceId,
                                               QWidget* parent)
    : QDialog(parent), m_engine(engine), m_sourceId(sourceId)
{
    setObjectName(QStringLiteral("sourcePropertiesDialog"));
    setModal(true);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    resize(560, 760);
    setMinimumSize(480, 560);

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

    // OBS BasicProperties: live source preview sits above the property form.
    m_previewHost = new QWidget(this);
    m_previewHost->setObjectName(QStringLiteral("sourcePropsPreviewHost"));
    m_previewHost->setStyleSheet(QStringLiteral(
        "QWidget#sourcePropsPreviewHost {"
        "  background:#0A0C10; border-bottom:1px solid #2A2D35;"
        "}"));
    auto* previewLay = new QVBoxLayout(m_previewHost);
    previewLay->setContentsMargins(10, 8, 10, 10);
    previewLay->setSpacing(6);

    m_previewCaption = new QLabel(QStringLiteral("PREVIEW"), m_previewHost);
    m_previewCaption->setStyleSheet(QStringLiteral(
        "color:#4F9EFF; font-size:9px; font-weight:900; letter-spacing:1.5px;"
        "font-family:'DM Sans'; background:transparent; border:none;"));
    previewLay->addWidget(m_previewCaption);

    m_preview = new SourcePropsPreviewWidget(m_previewHost);
    m_preview->setMinimumHeight(220);
    previewLay->addWidget(m_preview, 1);
    root->addWidget(m_previewHost, 0);

    const bool showPreview = sourceHasVideoPreview();
    m_previewHost->setVisible(showPreview);
    if (!showPreview && m_preview)
        m_preview->setPlaceholder(QStringLiteral("Audio source — no video preview"));

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
    // Pin/attach on first tick so dialog chrome appears before Browser helper start().

    m_previewTimer = new QTimer(this);
    connect(m_previewTimer, &QTimer::timeout, this, &SourcePropertiesDialog::tickPreview);
    if (showPreview) {
        m_preview->setPlaceholder(QStringLiteral("Starting preview…"));
        m_previewTimer->start(50); // ~20 fps — readback path
        QTimer::singleShot(0, this, &SourcePropertiesDialog::tickPreview);
    }
}

SourcePropertiesDialog::~SourcePropertiesDialog()
{
    unpinCaptureSource();
}

bool SourcePropertiesDialog::sourceHasVideoPreview() const
{
    if (!m_engine) return false;
    const auto* src = m_engine->projectSnapshot().findSourceAnywhere(m_sourceId);
    if (!src) return true;
    return src->type != SourceType::AudioInput && src->type != SourceType::AudioOutput;
}

void SourcePropertiesDialog::pinCaptureSource()
{
    if (!m_engine || !m_engine->capture()) return;
    if (const auto* src = m_engine->projectSnapshot().findSourceAnywhere(m_sourceId)) {
        auto item = *src;
        item.visible = true; // Properties always wants a feed
        m_engine->capture()->pinSource(item);
    }
}

void SourcePropertiesDialog::unpinCaptureSource()
{
    if (m_engine && m_engine->capture())
        m_engine->capture()->unpinSource(m_sourceId);
}

void SourcePropertiesDialog::tickPreview()
{
    if (!m_engine || !m_preview || !m_previewHost || !m_previewHost->isVisible())
        return;

    // Refresh pin from latest settings (URL/size changes while dialog is open).
    pinCaptureSource();

    CaptureManager* cap = m_engine->capture();
    D3D11Compositor* comp = m_engine->compositor();
    if (!cap || !comp) {
        m_preview->setPlaceholder(QStringLiteral("No Signal"));
        return;
    }

    if (!cap->pollSource(m_sourceId)) {
        m_preview->setPlaceholder(QStringLiteral("Waiting for source…"));
        return;
    }

    const auto frame = cap->frameBus().latest(m_sourceId);
    if (!frame || !frame->texture) {
        m_preview->setPlaceholder(QStringLiteral("No Signal"));
        return;
    }

    const QImage img = comp->readbackTexture(frame->texture);
    if (img.isNull()) {
        m_preview->setPlaceholder(QStringLiteral("Preview readback failed"));
        return;
    }
    m_preview->setFrame(img);
}

void SourcePropertiesDialog::refreshTitle()
{
    QString name = QStringLiteral("Source");
    if (m_engine) {
        if (const auto* src = m_engine->projectSnapshot().findSourceAnywhere(m_sourceId))
            name = src->name;
        else if (const auto sel = m_engine->selectedSource())
            name = sel->name;
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
    const auto* src = m_engine->projectSnapshot().findSourceAnywhere(m_sourceId);
    if (src && (src->type == SourceType::Display || src->type == SourceType::Window
                || src->type == SourceType::Game))
        t = {0, 0, 1, 1};
    else
        t = {0.1, 0.1, 0.5, 0.5};
    m_engine->updateSourceTransform(m_sourceId, t);
    if (m_props)
        m_props->resetTypeSpecificDefaults();
    QJsonObject settings;
    if (src)
        settings = src->settings;
    settings.insert(QStringLiteral("brightness"), 0);
    settings.insert(QStringLiteral("contrast"), 0);
    settings.insert(QStringLiteral("saturation"), 0);
    settings.insert(QStringLiteral("chromaKey"), false);
    settings.insert(QStringLiteral("chromaSimilarity"), 40);
    settings.insert(QStringLiteral("blur"), 0);
    if (src && sourceTypeSupportsAudio(src->type)) {
        settings.insert(QStringLiteral("audioVolume"), 100);
        settings.insert(QStringLiteral("audioMute"), false);
        if (src->type == SourceType::Browser)
            settings.insert(QStringLiteral("controlAudioViaObs"), false);
    } else {
        settings.remove(QStringLiteral("audioVolume"));
        settings.remove(QStringLiteral("audioMute"));
        settings.remove(QStringLiteral("controlAudioViaObs"));
    }
    m_engine->updateSourceSettings(m_sourceId, settings);
    if (m_props)
        m_props->reload();
}

} // namespace railshot
