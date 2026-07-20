#include "ui/widgets/PreviewWidget.h"
#include "core/EngineController.h"
#include "core/SceneGraph.h"
#include "compositor/D3D11Compositor.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QHBoxLayout>
#include <QPainter>
#include <QMouseEvent>
#include <QStackedLayout>
#include <cmath>

namespace railshot {

namespace {
enum class Handle {
    None,
    Move,
    ResizeTL,
    ResizeTR,
    ResizeBL,
    ResizeBR
};

QRectF normToWidget(const Transform& t, const QSize& sz)
{
    return QRectF(t.x * sz.width(), t.y * sz.height(), t.w * sz.width(), t.h * sz.height());
}

Transform widgetDeltaToNorm(const QPointF& delta, const QSize& sz)
{
    Transform d;
    d.x = sz.width() > 0 ? delta.x() / sz.width() : 0;
    d.y = sz.height() > 0 ? delta.y() / sz.height() : 0;
    return d;
}

Handle hitHandle(const QRectF& r, const QPointF& p, qreal pad = 8.0)
{
    const QRectF tl(r.topLeft() - QPointF(pad, pad), QSizeF(pad * 2, pad * 2));
    const QRectF tr(r.topRight() - QPointF(pad, pad), QSizeF(pad * 2, pad * 2));
    const QRectF bl(r.bottomLeft() - QPointF(pad, pad), QSizeF(pad * 2, pad * 2));
    const QRectF br(r.bottomRight() - QPointF(pad, pad), QSizeF(pad * 2, pad * 2));
    if (tl.contains(p)) return Handle::ResizeTL;
    if (tr.contains(p)) return Handle::ResizeTR;
    if (bl.contains(p)) return Handle::ResizeBL;
    if (br.contains(p)) return Handle::ResizeBR;
    if (r.contains(p)) return Handle::Move;
    return Handle::None;
}
} // namespace

class PreviewWidget::CanvasOverlay : public QWidget {
public:
    CanvasOverlay(EngineController* engine, PreviewWidget* owner)
        : QWidget(owner), m_engine(engine), m_owner(owner)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents, false);
        setMouseTracking(true);
        setCursor(Qt::ArrowCursor);
    }

    void setInteractive(bool on) { m_interactive = on; update(); }

protected:
    void paintEvent(QPaintEvent*) override
    {
        if (!m_interactive || !m_engine) return;
        const auto sel = m_engine->selectedSource();
        if (!sel || !sel->visible) return;
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const QRectF r = normToWidget(sel->transform, size());
        p.setPen(QPen(QColor(QStringLiteral("#4F9EFF")), 2.0));
        p.setBrush(Qt::NoBrush);
        p.drawRect(r);
        const qreal hs = 7.0;
        p.setBrush(QColor(QStringLiteral("#4F9EFF")));
        for (const QPointF& c : {r.topLeft(), r.topRight(), r.bottomLeft(), r.bottomRight()})
            p.drawRect(QRectF(c.x() - hs / 2, c.y() - hs / 2, hs, hs));
        p.setPen(QColor(QStringLiteral("#F8F8FF")));
        p.drawText(r.adjusted(4, 4, -4, -4), Qt::AlignTop | Qt::AlignLeft, sel->name);
    }

    void mousePressEvent(QMouseEvent* e) override
    {
        if (!m_interactive || !m_engine || e->button() != Qt::LeftButton) {
            QWidget::mousePressEvent(e);
            return;
        }
        const auto p = m_engine->projectSnapshot();
        const auto* sc = p.findScene(p.previewSceneId.isEmpty() ? p.activeSceneId : p.previewSceneId);
        if (!sc) return;

        Handle hit = Handle::None;
        QString hitId;
        for (int i = sc->sources.size() - 1; i >= 0; --i) {
            const auto& src = sc->sources[i];
            if (!src.visible) continue;
            const QRectF r = normToWidget(src.transform, size());
            hit = hitHandle(r, e->position());
            if (hit != Handle::None) {
                hitId = src.id;
                break;
            }
        }
        if (hitId.isEmpty()) {
            m_engine->setSelectedSourceId({});
            emit m_owner->sourceSelected({});
            update();
            return;
        }
        m_engine->setSelectedSourceId(hitId);
        emit m_owner->sourceSelected(hitId);
        const auto src = m_engine->selectedSource();
        if (!src || src->locked) {
            update();
            return;
        }
        m_dragging = true;
        m_handle = hit;
        m_dragStart = e->position();
        m_startTransform = src->transform;
        update();
    }

    void mouseMoveEvent(QMouseEvent* e) override
    {
        if (!m_dragging || !m_engine) return;
        auto src = m_engine->selectedSource();
        if (!src || src->locked) return;

        const QPointF delta = e->position() - m_dragStart;
        const auto d = widgetDeltaToNorm(delta, size());
        Transform t = m_startTransform;
        constexpr double kMin = 0.02;

        switch (m_handle) {
        case Handle::Move:
            t.x = m_startTransform.x + d.x;
            t.y = m_startTransform.y + d.y;
            break;
        case Handle::ResizeBR:
            t.w = qMax(kMin, m_startTransform.w + d.x);
            t.h = qMax(kMin, m_startTransform.h + d.y);
            break;
        case Handle::ResizeTR:
            t.y = m_startTransform.y + d.y;
            t.w = qMax(kMin, m_startTransform.w + d.x);
            t.h = qMax(kMin, m_startTransform.h - d.y);
            if (t.h < kMin) { t.y = m_startTransform.y + m_startTransform.h - kMin; t.h = kMin; }
            break;
        case Handle::ResizeBL:
            t.x = m_startTransform.x + d.x;
            t.w = qMax(kMin, m_startTransform.w - d.x);
            t.h = qMax(kMin, m_startTransform.h + d.y);
            if (t.w < kMin) { t.x = m_startTransform.x + m_startTransform.w - kMin; t.w = kMin; }
            break;
        case Handle::ResizeTL:
            t.x = m_startTransform.x + d.x;
            t.y = m_startTransform.y + d.y;
            t.w = qMax(kMin, m_startTransform.w - d.x);
            t.h = qMax(kMin, m_startTransform.h - d.y);
            if (t.w < kMin) { t.x = m_startTransform.x + m_startTransform.w - kMin; t.w = kMin; }
            if (t.h < kMin) { t.y = m_startTransform.y + m_startTransform.h - kMin; t.h = kMin; }
            break;
        default:
            break;
        }
        m_engine->updateSourceTransform(src->id, t);
        update();
    }

    void mouseReleaseEvent(QMouseEvent* e) override
    {
        Q_UNUSED(e);
        m_dragging = false;
        m_handle = Handle::None;
    }

private:
    EngineController* m_engine = nullptr;
    PreviewWidget* m_owner = nullptr;
    bool m_interactive = false;
    bool m_dragging = false;
    Handle m_handle = Handle::None;
    QPointF m_dragStart;
    Transform m_startTransform;
};

PreviewWidget::PreviewWidget(EngineController* engine, bool program, QWidget* parent)
    : QWidget(parent), m_engine(engine), m_program(program)
{
    const QString accent = program ? QStringLiteral("#FF5A2C") : QStringLiteral("#22C55E");
    const QString accentDim = program ? QStringLiteral("rgba(255,90,44,0.35)")
                                      : QStringLiteral("rgba(34,197,94,0.30)");

    setStyleSheet(QStringLiteral(
        "background: #0D0F12;"
        "border: 2px solid %1;")
                      .arg(accentDim));

    auto* col = new QVBoxLayout(this);
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(0);

    auto* header = new QWidget(this);
    header->setFixedHeight(26);
    header->setStyleSheet(QStringLiteral(
        "background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 %1, stop:0.12 transparent);"
        "border-bottom: 1px solid #1A1D24;")
                              .arg(program ? QStringLiteral("rgba(255,90,44,0.18)")
                                           : QStringLiteral("rgba(34,197,94,0.18)")));
    auto* h = new QHBoxLayout(header);
    h->setContentsMargins(10, 0, 10, 0);
    auto* title = new QLabel(program ? QStringLiteral("PROGRAM") : QStringLiteral("PREVIEW"), header);
    title->setStyleSheet(QStringLiteral(
        "color:%1; font-weight:800; font-size:11px; letter-spacing:2px; background:transparent;")
                             .arg(accent));
    h->addWidget(title);
    if (!program) {
        auto* hint = new QLabel(QStringLiteral("Drag to position · corners to resize"), header);
        hint->setStyleSheet(QStringLiteral("color:#4A5060; font-size:10px; background:transparent;"));
        h->addWidget(hint);
    } else {
        auto* badge = new QLabel(QStringLiteral("PGM"), header);
        badge->setStyleSheet(QStringLiteral(
            "background:#FF5A2C; color:#fff; font-size:9px; font-weight:800;"
            "padding:2px 6px; border-radius:3px;"));
        h->addWidget(badge);
    }
    h->addStretch();
    col->addWidget(header);

    auto* stackHost = new QWidget(this);
    stackHost->setStyleSheet(QStringLiteral("background:#080A0D;"));
    auto* stack = new QStackedLayout(stackHost);
    stack->setStackingMode(QStackedLayout::StackAll);
    stack->setContentsMargins(0, 0, 0, 0);

    m_surface = new PreviewSurface(stackHost);
    if (engine && engine->graphicsDevice())
        m_surface->setDevice(engine->graphicsDevice());
    m_surface->setLabel(program ? QStringLiteral("PGM") : QStringLiteral("PVW"),
                        QColor(accent));
    m_surface->setEmptyMessage(program ? QStringLiteral("NO OUTPUT") : QStringLiteral("NO PREVIEW"));
    stack->addWidget(m_surface);

    m_overlay = new CanvasOverlay(engine, this);
    m_overlay->setInteractive(!program);
    if (program)
        m_overlay->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    stack->addWidget(m_overlay);

    col->addWidget(stackHost, 1);

    if (engine) {
        connect(engine, &EngineController::selectedSourceChanged, m_overlay, QOverload<>::of(&QWidget::update));
        connect(engine->sceneGraph(), &SceneGraph::projectChanged, m_overlay, QOverload<>::of(&QWidget::update));
    }
}

void PreviewWidget::tick()
{
    if (!m_engine || !m_engine->compositor()) return;
    if (m_engine->graphicsDevice())
        m_surface->setDevice(m_engine->graphicsDevice());
    auto* tex = m_program ? m_engine->compositor()->programTexture()
                          : m_engine->compositor()->previewTexture();
    if (tex)
        m_surface->presentTexture(tex);
    if (m_overlay)
        m_overlay->update();
}

} // namespace railshot
