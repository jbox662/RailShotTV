#include "ui/widgets/InteractDialog.h"
#include "compositor/PreviewSurface.h"
#include "compositor/D3D11Compositor.h"
#include "core/EngineController.h"
#include "core/Types.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QMouseEvent>
#include <QDesktopServices>
#include <QUrl>
#include <cmath>
#include <algorithm>

namespace railshot {

namespace {
QRectF transformToNormRect(const Transform& t)
{
    const double x0 = std::min(t.x, t.x + t.w);
    const double y0 = std::min(t.y, t.y + t.h);
    const double x1 = std::max(t.x, t.x + t.w);
    const double y1 = std::max(t.y, t.y + t.h);
    return QRectF(QPointF(x0, y0), QPointF(x1, y1));
}
} // namespace

InteractDialog::InteractDialog(EngineController* engine, const QString& sourceId, QWidget* parent)
    : QDialog(parent), m_engine(engine), m_sourceId(sourceId)
{
    setWindowTitle(QStringLiteral("Interact"));
    setMinimumSize(640, 400);
    resize(900, 560);
    setSizeGripEnabled(true);
    setStyleSheet(QStringLiteral(
        "QDialog{background:#0F1114;}"
        "QLabel{color:#C8CCD4; font-family:'DM Sans';}"
        "QPushButton{background:#1A1E26; border:1px solid #5A5E68; border-radius:3px;"
        "  color:#E0E2E8; font-weight:700; padding:6px 12px;}"
        "QPushButton:hover{border-color:#4F9EFF;}"));

    QString name = sourceId;
    SourceType type = SourceType::Unknown;
    if (engine) {
        if (const auto* s = engine->projectSnapshot().findSourceAnywhere(sourceId)) {
            name = s->name;
            type = s->type;
        }
    }
    setWindowTitle(QStringLiteral("Interact — %1").arg(name));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(10, 10, 10, 10);
    root->setSpacing(8);

    m_hint = new QLabel(this);
    if (type == SourceType::Browser) {
        m_hint->setText(QStringLiteral(
            "Browser interact: move over the live feed to see source-relative coordinates. "
            "Use Open URL for full page interaction (browser runs in an isolated helper)."));
    } else {
        m_hint->setText(QStringLiteral(
            "Interact preview: pointer position is mapped into the selected source. "
            "Transform editing stays locked while this window is focused on interaction."));
    }
    m_hint->setWordWrap(true);
    root->addWidget(m_hint);

    m_surface = new PreviewSurface(this);
    m_surface->setMinimumHeight(280);
    m_surface->setMouseTracking(true);
    m_surface->installEventFilter(this);
    if (engine && engine->graphicsDevice())
        m_surface->setDevice(engine->graphicsDevice());
    m_surface->setEmptyMessage(QStringLiteral("NO PREVIEW"));
    root->addWidget(m_surface, 1);

    m_status = new QLabel(QStringLiteral("Move mouse over the preview"), this);
    m_status->setStyleSheet(QStringLiteral(
        "font-family:'JetBrains Mono'; font-size:11px; color:#7AB8FF;"
        "background:#0A0C0F; border:1px solid #2A2D35; border-radius:3px; padding:6px 8px;"));
    root->addWidget(m_status);

    auto* row = new QHBoxLayout();
    if (type == SourceType::Browser) {
        auto* openBtn = new QPushButton(QStringLiteral("Open URL"), this);
        connect(openBtn, &QPushButton::clicked, this, &InteractDialog::openUrl);
        row->addWidget(openBtn);
    }
    row->addStretch();
    auto* closeBtn = new QPushButton(QStringLiteral("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    row->addWidget(closeBtn);
    root->addLayout(row);

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &InteractDialog::tick);
    m_timer->start(16);
}

void InteractDialog::tick()
{
    if (!m_engine || !m_engine->compositor() || !m_surface)
        return;
    if (m_engine->graphicsDevice())
        m_surface->setDevice(m_engine->graphicsDevice());
    if (auto* tex = m_engine->compositor()->previewTexture())
        m_surface->presentTexture(tex);
}

void InteractDialog::openUrl()
{
    if (!m_engine) return;
    const auto* s = m_engine->projectSnapshot().findSourceAnywhere(m_sourceId);
    if (!s) return;
    const QString url = s->settings.value(QStringLiteral("url")).toString(
        QStringLiteral("https://railshottv.local"));
    QDesktopServices::openUrl(QUrl(url));
}

void InteractDialog::updateHint(const QPointF& widgetPos)
{
    if (!m_engine || !m_surface) return;
    const QSize sz = m_surface->size();
    if (sz.width() <= 0 || sz.height() <= 0) return;
    const double nx = widgetPos.x() / sz.width();
    const double ny = widgetPos.y() / sz.height();

    const auto* src = m_engine->projectSnapshot().findSourceAnywhere(m_sourceId);
    if (!src) {
        m_status->setText(QStringLiteral("Source missing"));
        return;
    }
    const QRectF r = transformToNormRect(src->transform);
    const bool inside = r.contains(QPointF(nx, ny));
    double localX = 0, localY = 0;
    if (r.width() > 1e-9 && r.height() > 1e-9) {
        localX = (nx - r.x()) / r.width();
        localY = (ny - r.y()) / r.height();
    }
    const int canvasW = m_engine->compositor() ? m_engine->compositor()->width() : 1920;
    const int canvasH = m_engine->compositor() ? m_engine->compositor()->height() : 1080;
    const int px = int(std::lround(localX * canvasW * std::abs(src->transform.w)));
    const int py = int(std::lround(localY * canvasH * std::abs(src->transform.h)));

    m_status->setText(QStringLiteral("canvas (%1, %2)  ·  source-local (%3, %4)%5")
                          .arg(nx * canvasW, 0, 'f', 0)
                          .arg(ny * canvasH, 0, 'f', 0)
                          .arg(std::clamp(px, 0, canvasW))
                          .arg(std::clamp(py, 0, canvasH))
                          .arg(inside ? QStringLiteral("  ·  INSIDE") : QStringLiteral("  ·  outside")));
}

bool InteractDialog::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_surface) {
        if (event->type() == QEvent::MouseMove || event->type() == QEvent::MouseButtonPress) {
            auto* me = static_cast<QMouseEvent*>(event);
            updateHint(me->position());
        }
    }
    return QDialog::eventFilter(watched, event);
}

} // namespace railshot
