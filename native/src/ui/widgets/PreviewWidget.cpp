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
#include <QEvent>
#include <cmath>
#include <QFontMetrics>

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

QRect letterboxRect(const QSize& sz)
{
    if (sz.width() <= 0 || sz.height() <= 0) return QRect();
    const qreal target = 16.0 / 9.0;
    const qreal cur = qreal(sz.width()) / qreal(sz.height());
    if (cur > target) {
        const int w = int(sz.height() * target);
        return QRect((sz.width() - w) / 2, 0, w, sz.height());
    }
    const int h = int(sz.width() / target);
    return QRect(0, (sz.height() - h) / 2, sz.width(), h);
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

/** Thin accent frame around the D3D HWND (no corner L-brackets). */
class StageChrome : public QWidget {
public:
    StageChrome(bool program, QWidget* parent = nullptr)
        : QWidget(parent), m_program(program)
    {
        setAttribute(Qt::WA_StyledBackground, true);
        setStyleSheet(QStringLiteral("background:#060608;"));
    }
    void setLive(bool on)
    {
        if (m_live == on) return;
        m_live = on;
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, false);
        const QColor accent = m_program ? QColor(QStringLiteral("#FF5A2C"))
                                        : QColor(QStringLiteral("#22C55E"));
        const int a = m_program ? (m_live ? 255 : 180) : 220;
        QColor border = accent;
        border.setAlpha(a);
        p.setPen(QPen(border, 2));
        p.setBrush(Qt::NoBrush);
        p.drawRect(rect().adjusted(1, 1, -2, -2));

        if (m_program && m_live) {
            QFont f = p.font();
            f.setFamily(QStringLiteral("DM Sans"));
            f.setBold(true);
            f.setPointSize(8);
            p.setFont(f);
            const QRect air(10, 4, 54, 16);
            p.setBrush(QColor(QStringLiteral("#FF5A2C")));
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(air, 2, 2);
            p.setPen(Qt::white);
            p.drawText(air, Qt::AlignCenter, QStringLiteral("ON AIR"));
        }
    }

private:
    bool m_program = false;
    bool m_live = false;
};
} // namespace

class PreviewWidget::CanvasOverlay : public QObject {
public:
    CanvasOverlay(EngineController* engine, PreviewWidget* owner, QWidget* surface, bool program)
        : QObject(owner), m_engine(engine), m_owner(owner), m_surface(surface), m_program(program)
    {
        if (m_surface) {
            m_surface->setMouseTracking(true);
            m_surface->installEventFilter(this);
        }
        if (program && engine) {
            connect(engine, &EngineController::telemetryUpdated, this, [this](const TelemetrySnapshot& s) {
                m_live = s.streaming;
            });
            m_live = engine->telemetrySnapshot().streaming;
        }
    }

    void setInteractive(bool on) { m_interactive = on; }

    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (!m_interactive || !m_engine || watched != m_surface)
            return QObject::eventFilter(watched, event);

        switch (event->type()) {
        case QEvent::MouseButtonPress:
            return handlePress(static_cast<QMouseEvent*>(event));
        case QEvent::MouseMove:
            handleMove(static_cast<QMouseEvent*>(event));
            return m_dragging;
        case QEvent::MouseButtonRelease:
            if (m_dragging) {
                m_dragging = false;
                m_handle = Handle::None;
                return true;
            }
            break;
        default:
            break;
        }
        return QObject::eventFilter(watched, event);
    }

private:
    QSize surfaceSize() const
    {
        return m_surface ? m_surface->size() : QSize();
    }

    bool handlePress(QMouseEvent* e)
    {
        if (e->button() != Qt::LeftButton)
            return false;
        const auto p = m_engine->projectSnapshot();
        const auto* sc = p.findScene(p.previewSceneId.isEmpty() ? p.activeSceneId : p.previewSceneId);
        if (!sc) return false;

        Handle hit = Handle::None;
        QString hitId;
        const QSize sz = surfaceSize();
        for (int i = sc->sources.size() - 1; i >= 0; --i) {
            const auto& src = sc->sources[i];
            if (!src.visible) continue;
            const QRectF r = normToWidget(src.transform, sz);
            hit = hitHandle(r, e->position());
            if (hit != Handle::None) {
                hitId = src.id;
                break;
            }
        }
        if (hitId.isEmpty()) {
            m_engine->setSelectedSourceId({});
            emit m_owner->sourceSelected({});
            return true;
        }
        m_engine->setSelectedSourceId(hitId);
        emit m_owner->sourceSelected(hitId);
        const auto src = m_engine->selectedSource();
        if (!src || src->locked)
            return true;
        m_dragging = true;
        m_handle = hit;
        m_dragStart = e->position();
        m_startTransform = src->transform;
        return true;
    }

    void handleMove(QMouseEvent* e)
    {
        if (!m_dragging) return;
        auto src = m_engine->selectedSource();
        if (!src || src->locked) return;

        const QPointF delta = e->position() - m_dragStart;
        const auto d = widgetDeltaToNorm(delta, surfaceSize());
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
    }

    EngineController* m_engine = nullptr;
    PreviewWidget* m_owner = nullptr;
    QWidget* m_surface = nullptr;
    bool m_program = false;
    bool m_interactive = false;
    bool m_live = false;
    bool m_dragging = false;
    Handle m_handle = Handle::None;
    QPointF m_dragStart;
    Transform m_startTransform;
};

PreviewWidget::PreviewWidget(EngineController* engine, bool program, QWidget* parent)
    : QWidget(parent), m_engine(engine), m_program(program)
{
    const QString accent = program ? QStringLiteral("#FF5A2C") : QStringLiteral("#22C55E");

    setStyleSheet(QStringLiteral(
        "background: #0D0F12;"
        "border-right: 2px solid %1;")
                      .arg(program ? QStringLiteral("#FF5A2C55") : QStringLiteral("#22C55E55")));

    auto* col = new QVBoxLayout(this);
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(0);

    auto* header = new QWidget(this);
    header->setFixedHeight(28);
    header->setStyleSheet(QStringLiteral(
        "background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #1E2228, stop:1 #12151A);"
        "border-bottom: 2px solid %1;")
                              .arg(accent));
    auto* h = new QHBoxLayout(header);
    h->setContentsMargins(10, 0, 10, 0);
    h->setSpacing(8);
    auto* title = new QLabel(program ? QStringLiteral("PROGRAM") : QStringLiteral("PREVIEW"), header);
    title->setObjectName(program ? QStringLiteral("programLabel") : QStringLiteral("previewLabel"));
    title->setStyleSheet(program
                             ? QStringLiteral(
                                   "color:#FFFFFF; font-weight:900; font-size:11px; letter-spacing:1.5px;"
                                   "background:#FF5A2C; border:1px solid #FF8C42; border-radius:2px; padding:3px 10px;")
                             : QStringLiteral(
                                   "color:#04140A; font-weight:900; font-size:11px; letter-spacing:1.5px;"
                                   "background:#22C55E; border:1px solid #4ADE80; border-radius:2px; padding:3px 10px;"));
    h->addWidget(title);
    h->addStretch();

    auto* sceneName = new QLabel(QStringLiteral("No Scene"), header);
    sceneName->setObjectName(QStringLiteral("mono"));
    sceneName->setStyleSheet(QStringLiteral(
        "font-family:'JetBrains Mono'; font-size:10px; color:#A0A8B8; background:transparent;"));
    h->addWidget(sceneName);

    StageChrome* stage = nullptr;

    if (program) {
        auto* live = new QLabel(QStringLiteral("● LIVE"), header);
        live->setObjectName(QStringLiteral("liveBadge"));
        live->setStyleSheet(QStringLiteral(
            "font-family:'JetBrains Mono'; font-size:10px; color:#FF5A2C; font-weight:800;"
            "background:transparent;"));
        live->setVisible(false);
        h->addWidget(live);
        auto* tc = new QLabel(QStringLiteral("00:00:00"), header);
        tc->setObjectName(QStringLiteral("liveTimecode"));
        tc->setStyleSheet(QStringLiteral(
            "font-family:'JetBrains Mono'; font-size:10px; color:#606878; background:transparent;"));
        h->addWidget(tc);
        if (engine) {
            connect(engine, &EngineController::telemetryUpdated, this,
                    [live, tc, title](const TelemetrySnapshot& s) {
                        live->setVisible(s.streaming);
                        // Keep PROGRAM chip solid orange; only LIVE badge toggles
                        title->setStyleSheet(QStringLiteral(
                            "color:#FFFFFF; font-weight:800; font-size:10px; letter-spacing:1px;"
                            "background:#FF5A2C; border-radius:2px; padding:2px 8px;"));
                        if (!s.streaming) {
                            tc->setText(QStringLiteral("00:00:00"));
                            return;
                        }
                        const int secs = int(s.streamUptimeSec);
                        tc->setText(QStringLiteral("%1:%2:%3")
                                        .arg(secs / 3600, 2, 10, QChar('0'))
                                        .arg((secs % 3600) / 60, 2, 10, QChar('0'))
                                        .arg(secs % 60, 2, 10, QChar('0')));
                    });
        }
    }

    auto* clearBtn = new QLabel(QStringLiteral("CLEAR"), header);
    clearBtn->setCursor(Qt::PointingHandCursor);
    clearBtn->setStyleSheet(QStringLiteral(
        "padding:1px 7px; border:1px solid %1; border-radius:2px; color:%2;"
        "font-size:9px; font-weight:700; letter-spacing:1px; background:transparent;")
                                .arg(program ? QStringLiteral("#FF5A2C60") : QStringLiteral("#22C55E60"),
                                     program ? QStringLiteral("#FF8A6A") : QStringLiteral("#6EE7A0")));
    clearBtn->installEventFilter(this);
    clearBtn->setProperty("clearRole", program ? QStringLiteral("program") : QStringLiteral("preview"));
    h->addWidget(clearBtn);
    col->addWidget(header);

    // Stage chrome: brackets in margins; label chip lives in header only
    stage = new StageChrome(program, this);
    auto* stageLay = new QVBoxLayout(stage);
    stageLay->setContentsMargins(3, m_program ? 20 : 3, 3, 3);
    stageLay->setSpacing(0);

    auto* stackHost = new QWidget(stage);
    stackHost->setStyleSheet(QStringLiteral("background:#000000;"));
    auto* stack = new QStackedLayout(stackHost);
    stack->setStackingMode(QStackedLayout::StackAll);
    stack->setContentsMargins(0, 0, 0, 0);

    m_surface = new PreviewSurface(stackHost);
    if (engine && engine->graphicsDevice())
        m_surface->setDevice(engine->graphicsDevice());
    // Don't paint a second PROGRAM/PREVIEW chip on the surface — header chip is enough
    m_surface->setLabel({}, QColor(accent));
    m_surface->setEmptyMessage(program ? QStringLiteral("NO OUTPUT") : QStringLiteral("NO PREVIEW"));
    stack->addWidget(m_surface);

    // Hit-testing lives as an event filter on the D3D surface. Never stack a Qt
    // sibling over the HWND — that covers composed frames (Preview-only blank bug).
    m_overlay = new CanvasOverlay(engine, this, m_surface, program);
    m_overlay->setInteractive(!program);
    stageLay->addWidget(stackHost, 1);
    col->addWidget(stage, 1);

    if (engine && program) {
        connect(engine, &EngineController::telemetryUpdated, stage, [stage](const TelemetrySnapshot& s) {
            stage->setLive(s.streaming);
        });
        stage->setLive(engine->telemetrySnapshot().streaming);
    }

    if (engine) {
        connect(engine->sceneGraph(), &SceneGraph::projectChanged, this, [this, sceneName, clearBtn] {
            const auto p = m_engine->projectSnapshot();
            const QString id = m_program ? p.programSceneId : p.previewSceneId;
            const auto* sc = p.findScene(id);
            sceneName->setText(sc ? sc->name : QStringLiteral("No Scene"));
            clearBtn->setVisible(!id.isEmpty());
        });
        const auto p = engine->projectSnapshot();
        const QString id = program ? p.programSceneId : p.previewSceneId;
        const auto* sc = p.findScene(id);
        sceneName->setText(sc ? sc->name : QStringLiteral("No Scene"));
        clearBtn->setVisible(!id.isEmpty());
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
}

bool PreviewWidget::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonRelease && m_engine) {
        if (auto* lab = qobject_cast<QLabel*>(watched)) {
            const auto role = lab->property("clearRole").toString();
            if (role == QLatin1String("preview"))
                m_engine->setPreviewScene({});
            else if (role == QLatin1String("program"))
                m_engine->sceneGraph()->setProgramSceneId({});
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

} // namespace railshot
