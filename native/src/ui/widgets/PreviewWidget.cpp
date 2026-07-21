#include "ui/widgets/PreviewWidget.h"
#include "core/EngineController.h"
#include "core/SceneGraph.h"
#include "compositor/D3D11Compositor.h"
#include "ui/DropFiles.h"
#include "ui/widgets/SourcePropertiesDialog.h"
#include "ui/widgets/FiltersDialog.h"
#include "ui/widgets/TransformDialog.h"
#include "ui/widgets/ProjectorWindow.h"
#include "ui/widgets/InteractDialog.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QHBoxLayout>
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QStackedLayout>
#include <QScrollArea>
#include <QPushButton>
#include <QFrame>
#include <QEvent>
#include <QMenu>
#include <QCursor>
#include <QSignalBlocker>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <cmath>
#include <algorithm>

namespace railshot {

namespace {

enum class Handle {
    None,
    Move,
    ResizeTL,
    ResizeTC,
    ResizeTR,
    ResizeCL,
    ResizeCR,
    ResizeBL,
    ResizeBC,
    ResizeBR,
    Rotate
};

constexpr double kMinSize = 0.02;
constexpr double kSnapDist = 0.012; // ~1.2% of canvas
constexpr qreal kHandlePad = 8.0;
constexpr qreal kRotateOffset = 28.0;

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

/** Widget pixel → normalized canvas (0..1). Outside letterbox returns nullopt-ish via ok=false. */
bool widgetToNorm(const QPointF& widgetPt, const QSize& widgetSz, QPointF* outNorm)
{
    const QRect lb = letterboxRect(widgetSz);
    if (lb.width() <= 0 || lb.height() <= 0 || !outNorm) return false;
    if (!lb.contains(widgetPt.toPoint()) && !lb.adjusted(-2, -2, 2, 2).contains(widgetPt.toPoint()))
        return false;
    outNorm->setX((widgetPt.x() - lb.x()) / qreal(lb.width()));
    outNorm->setY((widgetPt.y() - lb.y()) / qreal(lb.height()));
    return true;
}

QPointF normDeltaFromWidgetDelta(const QPointF& widgetDelta, const QSize& widgetSz)
{
    const QRect lb = letterboxRect(widgetSz);
    if (lb.width() <= 0 || lb.height() <= 0) return {};
    return QPointF(widgetDelta.x() / qreal(lb.width()), widgetDelta.y() / qreal(lb.height()));
}

QRectF transformToNormRect(const Transform& t)
{
    // Support negative w/h (flip): normalize to positive AABB for hit-test chrome.
    QRectF r(t.x, t.y, t.w, t.h);
    return r.normalized();
}

Transform snapTransform(Transform t, bool snapX, bool snapY, double dist)
{
    auto snapEdge = [dist](double& v, double size, bool enable) {
        if (!enable) return;
        if (std::abs(v) < dist) v = 0.0;
        if (std::abs(v + size - 1.0) < dist) v = 1.0 - size;
        if (std::abs(v + size * 0.5 - 0.5) < dist) v = 0.5 - size * 0.5;
    };
    snapEdge(t.x, t.w, snapX);
    snapEdge(t.y, t.h, snapY);
    return t;
}

void lockAspect(Transform& t, const Transform& start, Handle h, bool lock)
{
    if (!lock) return;
    const double aspect = (std::abs(start.h) > 1e-9) ? (std::abs(start.w) / std::abs(start.h)) : 1.0;
    const double signW = start.w < 0 ? -1.0 : 1.0;
    const double signH = start.h < 0 ? -1.0 : 1.0;

    switch (h) {
    case Handle::ResizeBR:
    case Handle::ResizeTR:
    case Handle::ResizeBL:
    case Handle::ResizeTL: {
        // Prefer width as driver; recompute height from aspect
        const double aw = std::abs(t.w);
        const double ah = aw / aspect;
        const double cx = start.x + start.w * 0.5;
        const double cy = start.y + start.h * 0.5;
        // Keep the opposite corner fixed based on handle
        if (h == Handle::ResizeBR) {
            t.w = signW * aw;
            t.h = signH * ah;
            t.x = start.x;
            t.y = start.y;
        } else if (h == Handle::ResizeTR) {
            t.w = signW * aw;
            t.h = signH * ah;
            t.x = start.x;
            t.y = start.y + start.h - t.h;
        } else if (h == Handle::ResizeBL) {
            t.w = signW * aw;
            t.h = signH * ah;
            t.x = start.x + start.w - t.w;
            t.y = start.y;
        } else { // TL
            t.w = signW * aw;
            t.h = signH * ah;
            t.x = start.x + start.w - t.w;
            t.y = start.y + start.h - t.h;
        }
        Q_UNUSED(cx);
        Q_UNUSED(cy);
        break;
    }
    case Handle::ResizeCR:
    case Handle::ResizeCL: {
        const double aw = std::abs(t.w);
        const double ah = aw / aspect;
        const double cy = start.y + start.h * 0.5;
        t.h = signH * ah;
        t.y = cy - t.h * 0.5;
        break;
    }
    case Handle::ResizeTC:
    case Handle::ResizeBC: {
        const double ah = std::abs(t.h);
        const double aw = ah * aspect;
        const double cx = start.x + start.w * 0.5;
        t.w = signW * aw;
        t.x = cx - t.w * 0.5;
        break;
    }
    default:
        break;
    }
}

Handle hitHandleNorm(const QRectF& r, const QPointF& p, qreal padNormX, qreal padNormY)
{
    const QPointF tl = r.topLeft();
    const QPointF tr = r.topRight();
    const QPointF bl = r.bottomLeft();
    const QPointF br = r.bottomRight();
    const QPointF tc((tl.x() + tr.x()) * 0.5, tl.y());
    const QPointF bc((bl.x() + br.x()) * 0.5, bl.y());
    const QPointF cl(tl.x(), (tl.y() + bl.y()) * 0.5);
    const QPointF cr(tr.x(), (tr.y() + br.y()) * 0.5);

    auto nearPt = [&](const QPointF& c) {
        return std::abs(p.x() - c.x()) <= padNormX && std::abs(p.y() - c.y()) <= padNormY;
    };

    if (nearPt(tl)) return Handle::ResizeTL;
    if (nearPt(tr)) return Handle::ResizeTR;
    if (nearPt(bl)) return Handle::ResizeBL;
    if (nearPt(br)) return Handle::ResizeBR;
    if (nearPt(tc)) return Handle::ResizeTC;
    if (nearPt(bc)) return Handle::ResizeBC;
    if (nearPt(cl)) return Handle::ResizeCL;
    if (nearPt(cr)) return Handle::ResizeCR;
    if (r.contains(p)) return Handle::Move;
    return Handle::None;
}

Handle hitHandleOnCanvas(const Transform& t, const QPointF& normPt, const QSize& widgetSz)
{
    const QRect lb = letterboxRect(widgetSz);
    if (lb.width() <= 0 || lb.height() <= 0) return Handle::None;

    const QRectF r = transformToNormRect(t);
    const qreal padX = kHandlePad / qreal(lb.width());
    const qreal padY = kHandlePad / qreal(lb.height());

    const QPointF tc((r.left() + r.right()) * 0.5, r.top());
    const qreal rotOffY = kRotateOffset / qreal(lb.height());
    const QPointF rotPt(tc.x(), tc.y() - rotOffY);
    if (std::abs(normPt.x() - rotPt.x()) <= padX * 1.4 && std::abs(normPt.y() - rotPt.y()) <= padY * 1.4)
        return Handle::Rotate;

    return hitHandleNorm(r, normPt, padX, padY);
}

Qt::CursorShape cursorForHandle(Handle h)
{
    switch (h) {
    case Handle::ResizeTL:
    case Handle::ResizeBR:
        return Qt::SizeFDiagCursor;
    case Handle::ResizeTR:
    case Handle::ResizeBL:
        return Qt::SizeBDiagCursor;
    case Handle::ResizeTC:
    case Handle::ResizeBC:
        return Qt::SizeVerCursor;
    case Handle::ResizeCL:
    case Handle::ResizeCR:
        return Qt::SizeHorCursor;
    case Handle::Rotate:
        return Qt::OpenHandCursor;
    case Handle::Move:
        return Qt::SizeAllCursor;
    default:
        return Qt::ArrowCursor;
    }
}

double angleDegFromCenter(const QPointF& center, const QPointF& p)
{
    return std::atan2(p.y() - center.y(), p.x() - center.x()) * 180.0 / 3.14159265358979323846;
}

double snapAngle(double deg, bool shift, bool ctrl)
{
    if (ctrl && !shift) return deg;
    const double step = shift ? 15.0 : 45.0;
    const double soft = 5.0;
    // Soft snap to multiples
    const double nearest = std::round(deg / step) * step;
    if (std::abs(deg - nearest) <= soft || shift)
        return nearest;
    // Also soft-snap to original multiples of 45 when not shift
    return deg;
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
    CanvasOverlay(EngineController* engine, PreviewWidget* owner, PreviewSurface* surface, bool program)
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
        if (engine) {
            connect(engine, &EngineController::selectedSourceChanged, this, [this](const QString&) {
                refreshChrome();
            });
            connect(engine->sceneGraph(), &SceneGraph::projectChanged, this, [this] {
                refreshChrome();
            });
        }
    }

    void setInteractive(bool on) { m_interactive = on; }

    void refreshChrome()
    {
        if (!m_surface || m_program) {
            if (m_surface) m_surface->setEditChrome({});
            return;
        }
        PreviewEditChrome chrome;
        if (!m_engine) {
            m_surface->setEditChrome(chrome);
            return;
        }
        const auto src = m_engine->selectedSource();
        if (!src || !src->visible) {
            m_surface->setEditChrome(chrome);
            return;
        }
        chrome.visible = true;
        chrome.cropping = m_dragging && m_cropping;
        chrome.rect = transformToNormRect(src->transform);
        chrome.rotationDeg = src->transform.rotation;
        chrome.cropLeft = src->transform.cropLeft;
        chrome.cropRight = src->transform.cropRight;
        chrome.cropTop = src->transform.cropTop;
        chrome.cropBottom = src->transform.cropBottom;
        chrome.color = QColor(QStringLiteral("#22C55E"));
        m_surface->setEditChrome(chrome);
    }

    void nudgeSelected(double dx, double dy)
    {
        if (!m_engine || !m_interactive || m_owner->editLocked()) return;
        auto src = m_engine->selectedSource();
        if (!src || src->locked) return;
        Transform t = src->transform;
        t.x += dx;
        t.y += dy;
        m_engine->updateSourceTransform(src->id, t);
        refreshChrome();
    }

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
            if (event->type() == QEvent::MouseButtonRelease) {
                auto* me = static_cast<QMouseEvent*>(event);
                if (me->button() == Qt::RightButton)
                    return handleContextMenu(me);
                if (m_dragging && me->button() == Qt::LeftButton) {
                    m_dragging = false;
                    m_cropping = false;
                    m_handle = Handle::None;
                    if (m_surface)
                        m_surface->unsetCursor();
                    refreshChrome();
                    return true;
                }
            }
            break;
        case QEvent::KeyPress:
            return handleKey(static_cast<QKeyEvent*>(event));
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

    const SceneItem* previewScene() const
    {
        if (!m_engine) return nullptr;
        const auto p = m_engine->projectSnapshot();
        return p.findScene(p.previewSceneId.isEmpty() ? p.activeSceneId : p.previewSceneId);
    }

    bool handleKey(QKeyEvent* e)
    {
        if (!m_interactive || m_owner->editLocked()) return false;
        const bool shift = e->modifiers() & Qt::ShiftModifier;
        const double step = shift ? (10.0 / 1920.0) : (1.0 / 1920.0);
        switch (e->key()) {
        case Qt::Key_Left:
            nudgeSelected(-step, 0);
            return true;
        case Qt::Key_Right:
            nudgeSelected(step, 0);
            return true;
        case Qt::Key_Up:
            nudgeSelected(0, -step);
            return true;
        case Qt::Key_Down:
            nudgeSelected(0, step);
            return true;
        default:
            break;
        }
        return false;
    }

    bool handleContextMenu(QMouseEvent* e)
    {
        QPointF norm;
        if (!widgetToNorm(e->position(), surfaceSize(), &norm)) {
            // Still allow menu on empty / letterbox
        }

        // Select under cursor if any
        if (const auto* sc = previewScene()) {
            for (int i = sc->sources.size() - 1; i >= 0; --i) {
                const auto& src = sc->sources[i];
                if (!src.visible) continue;
                if (transformToNormRect(src.transform).contains(norm)) {
                    m_engine->setSelectedSourceId(src.id);
                    emit m_owner->sourceSelected(src.id);
                    break;
                }
            }
        }

        auto src = m_engine->selectedSource();
        QMenu menu(m_owner);
        menu.setStyleSheet(QStringLiteral(
            "QMenu { background:#1A1E26; color:#E8ECF4; border:1px solid #2A3140; }"
            "QMenu::item:selected { background:#2A3140; }"));

        QAction* props = menu.addAction(QStringLiteral("Properties"));
        props->setEnabled(src.has_value());
        QAction* filters = menu.addAction(QStringLiteral("Filters"));
        filters->setEnabled(src.has_value());
        menu.addSeparator();

        auto* transform = menu.addMenu(QStringLiteral("Transform"));
        QAction* editXform = transform->addAction(QStringLiteral("Edit Transform…"));
        QAction* copyXform = transform->addAction(QStringLiteral("Copy Transform"));
        QAction* pasteXform = transform->addAction(QStringLiteral("Paste Transform"));
        pasteXform->setEnabled(TransformDialog::hasClipboard());
        transform->addSeparator();
        QAction* reset = transform->addAction(QStringLiteral("Reset Transform"));
        QAction* rotCw = transform->addAction(QStringLiteral("Rotate 90° Clockwise"));
        QAction* rotCcw = transform->addAction(QStringLiteral("Rotate 90° Counterclockwise"));
        QAction* rot180 = transform->addAction(QStringLiteral("Rotate 180°"));
        transform->addSeparator();
        QAction* flipH = transform->addAction(QStringLiteral("Flip Horizontal"));
        QAction* flipV = transform->addAction(QStringLiteral("Flip Vertical"));
        transform->addSeparator();
        QAction* fit = transform->addAction(QStringLiteral("Fit to Screen"));
        QAction* stretch = transform->addAction(QStringLiteral("Stretch to Screen"));
        QAction* center = transform->addAction(QStringLiteral("Center to Screen"));
        transform->setEnabled(src.has_value() && !src->locked && !m_owner->editLocked());

        menu.addSeparator();
        QAction* interact = menu.addAction(QStringLiteral("Interact…"));
        interact->setEnabled(src.has_value());
        menu.addSeparator();
        auto* scaleMenu = menu.addMenu(QStringLiteral("Preview Scaling"));
        QAction* fitWin = scaleMenu->addAction(QStringLiteral("Fit to Window"));
        QAction* zoom100 = scaleMenu->addAction(QStringLiteral("Canvas (100%)"));
        QAction* zoomIn = scaleMenu->addAction(QStringLiteral("Zoom In"));
        QAction* zoomOut = scaleMenu->addAction(QStringLiteral("Zoom Out"));
        QAction* zoomReset = scaleMenu->addAction(QStringLiteral("Reset Zoom"));
        menu.addSeparator();
        auto* projMenu = menu.addMenu(QStringLiteral("Projector"));
        QAction* projWin = projMenu->addAction(QStringLiteral("Windowed Projector"));
        QAction* projFs = projMenu->addAction(QStringLiteral("Fullscreen Projector"));

        QAction* chosen = menu.exec(e->globalPosition().toPoint());
        if (!chosen) return true;

        if (chosen == interact && src) {
            emit m_owner->interactRequested(src->id);
            return true;
        }
        if (chosen == fitWin) {
            m_owner->setDisplayMode(PreviewDisplayMode::FitWindow);
            return true;
        }
        if (chosen == zoom100) {
            m_owner->setDisplayMode(PreviewDisplayMode::FixedScale);
            m_owner->setScaleAmount(1.0f);
            return true;
        }
        if (chosen == zoomIn) {
            m_owner->zoomIn();
            return true;
        }
        if (chosen == zoomOut) {
            m_owner->zoomOut();
            return true;
        }
        if (chosen == zoomReset) {
            m_owner->resetScale();
            return true;
        }
        if (chosen == projWin) {
            m_owner->openProjectorWindowed();
            return true;
        }
        if (chosen == projFs) {
            m_owner->openProjectorFullscreen();
            return true;
        }

        if (!src) return true;

        if (chosen == props) {
            emit m_owner->configureSourceRequested(src->id);
            return true;
        }
        if (chosen == filters) {
            FiltersDialog dlg(m_engine, src->id, m_owner);
            dlg.exec();
            return true;
        }
        if (chosen == editXform) {
            TransformDialog dlg(m_engine, src->id, m_owner);
            dlg.exec();
            refreshChrome();
            return true;
        }
        if (chosen == copyXform) {
            TransformDialog::copyTransform(src->transform);
            return true;
        }
        if (chosen == pasteXform) {
            TransformDialog::pasteOnto(m_engine, src->id);
            refreshChrome();
            return true;
        }

        Transform t = src->transform;
        if (chosen == reset) {
            t = Transform{};
            t.w = 1.0;
            t.h = 1.0;
        } else if (chosen == rotCw) {
            t.rotation = std::fmod(t.rotation + 90.0, 360.0);
        } else if (chosen == rotCcw) {
            t.rotation = std::fmod(t.rotation - 90.0 + 360.0, 360.0);
        } else if (chosen == rot180) {
            t.rotation = std::fmod(t.rotation + 180.0, 360.0);
        } else if (chosen == flipH) {
            t.x = t.x + t.w;
            t.w = -t.w;
        } else if (chosen == flipV) {
            t.y = t.y + t.h;
            t.h = -t.h;
        } else if (chosen == fit) {
            const double aw = std::abs(t.w);
            const double ah = std::abs(t.h);
            const double aspect = ah > 1e-9 ? aw / ah : 1.0;
            double nw = 1.0;
            double nh = 1.0;
            if (aspect >= 1.0) {
                nw = 1.0;
                nh = 1.0 / aspect;
            } else {
                nh = 1.0;
                nw = aspect;
            }
            const double sw = t.w < 0 ? -1.0 : 1.0;
            const double sh = t.h < 0 ? -1.0 : 1.0;
            t.w = sw * nw;
            t.h = sh * nh;
            t.x = 0.5 - t.w * 0.5;
            t.y = 0.5 - t.h * 0.5;
        } else if (chosen == stretch) {
            const double sw = t.w < 0 ? -1.0 : 1.0;
            const double sh = t.h < 0 ? -1.0 : 1.0;
            t.w = sw;
            t.h = sh;
            t.x = 0.5 - t.w * 0.5;
            t.y = 0.5 - t.h * 0.5;
        } else if (chosen == center) {
            t.x = 0.5 - t.w * 0.5;
            t.y = 0.5 - t.h * 0.5;
        } else {
            return true;
        }
        m_engine->updateSourceTransform(src->id, t);
        refreshChrome();
        return true;
    }

    bool handlePress(QMouseEvent* e)
    {
        if (e->button() == Qt::RightButton)
            return handleContextMenu(e);
        if (e->button() != Qt::LeftButton)
            return false;

        if (m_surface)
            m_surface->setFocus(Qt::MouseFocusReason);

        QPointF norm;
        if (!widgetToNorm(e->position(), surfaceSize(), &norm)) {
            m_engine->setSelectedSourceId({});
            emit m_owner->sourceSelected({});
            refreshChrome();
            return true;
        }

        const auto* sc = previewScene();
        if (!sc) return false;

        Handle hit = Handle::None;
        QString hitId;

        // Prefer handles on currently selected source first
        if (auto cur = m_engine->selectedSource()) {
            if (cur->visible) {
                hit = hitHandleOnCanvas(cur->transform, norm, surfaceSize());
                if (hit != Handle::None)
                    hitId = cur->id;
            }
        }

        if (hitId.isEmpty()) {
            for (int i = sc->sources.size() - 1; i >= 0; --i) {
                const auto& src = sc->sources[i];
                if (!src.visible) continue;
                hit = hitHandleOnCanvas(src.transform, norm, surfaceSize());
                if (hit != Handle::None) {
                    hitId = src.id;
                    break;
                }
            }
        }

        if (hitId.isEmpty()) {
            m_engine->setSelectedSourceId({});
            emit m_owner->sourceSelected({});
            refreshChrome();
            return true;
        }

        m_engine->setSelectedSourceId(hitId);
        emit m_owner->sourceSelected(hitId);
        const auto src = m_engine->selectedSource();
        refreshChrome();
        if (!src || src->locked || m_owner->editLocked())
            return true;

        m_dragging = true;
        m_handle = hit;
        m_cropping = (e->modifiers() & Qt::AltModifier) && hit != Handle::Move && hit != Handle::Rotate
                     && hit != Handle::None;
        m_dragStart = e->position();
        m_dragStartNorm = norm;
        m_startTransform = src->transform;
        m_startAngle = src->transform.rotation;
        const QRectF rr = transformToNormRect(src->transform);
        m_startCenter = rr.center();
        if (m_surface)
            m_surface->setCursor(m_cropping ? Qt::CrossCursor
                                            : (hit == Handle::Rotate ? Qt::ClosedHandCursor
                                                                     : cursorForHandle(hit)));
        return true;
    }

    void handleMove(QMouseEvent* e)
    {
        if (!m_dragging) {
            // Hover cursor
            QPointF norm;
            if (widgetToNorm(e->position(), surfaceSize(), &norm)) {
                Handle h = Handle::None;
                if (auto src = m_engine->selectedSource(); src && src->visible && !src->locked)
                    h = hitHandleOnCanvas(src->transform, norm, surfaceSize());
                if (h == Handle::None) {
                    if (const auto* sc = previewScene()) {
                        for (int i = sc->sources.size() - 1; i >= 0; --i) {
                            if (!sc->sources[i].visible) continue;
                            if (transformToNormRect(sc->sources[i].transform).contains(norm)) {
                                h = Handle::Move;
                                break;
                            }
                        }
                    }
                }
                if (m_surface)
                    m_surface->setCursor(cursorForHandle(h));
            } else if (m_surface) {
                m_surface->unsetCursor();
            }
            return;
        }

        auto src = m_engine->selectedSource();
        if (!src || src->locked) return;

        const QPointF d = normDeltaFromWidgetDelta(e->position() - m_dragStart, surfaceSize());
        QPointF norm;
        widgetToNorm(e->position(), surfaceSize(), &norm);

        const bool shift = e->modifiers() & Qt::ShiftModifier;
        const bool ctrl = e->modifiers() & Qt::ControlModifier;
        const bool alt = e->modifiers() & Qt::AltModifier;
        m_cropping = alt && m_handle != Handle::Move && m_handle != Handle::Rotate;

        Transform t = m_startTransform;

        if (m_handle == Handle::Rotate) {
            const double a0 = angleDegFromCenter(m_startCenter, m_dragStartNorm);
            const double a1 = angleDegFromCenter(m_startCenter, norm);
            double deg = m_startAngle + (a1 - a0);
            // Soft-snap relative to absolute angle
            deg = snapAngle(deg, shift, ctrl);
            // Also soft-snap delta to 0
            if (!ctrl && std::abs((a1 - a0)) < 5.0 && !shift)
                deg = m_startAngle;
            t.rotation = deg;
            m_engine->updateSourceTransform(src->id, t);
            refreshChrome();
            return;
        }

        if (m_cropping) {
            applyCrop(t, d);
            m_engine->updateSourceTransform(src->id, t);
            refreshChrome();
            return;
        }

        switch (m_handle) {
        case Handle::Move:
            t.x = m_startTransform.x + d.x();
            t.y = m_startTransform.y + d.y();
            if (!ctrl)
                t = snapTransform(t, true, true, kSnapDist);
            break;
        case Handle::ResizeBR:
            t.w = m_startTransform.w + d.x();
            t.h = m_startTransform.h + d.y();
            clampMin(t, m_startTransform, Handle::ResizeBR);
            lockAspect(t, m_startTransform, Handle::ResizeBR, !shift);
            break;
        case Handle::ResizeTR:
            t.y = m_startTransform.y + d.y();
            t.w = m_startTransform.w + d.x();
            t.h = m_startTransform.h - d.y();
            clampMin(t, m_startTransform, Handle::ResizeTR);
            lockAspect(t, m_startTransform, Handle::ResizeTR, !shift);
            break;
        case Handle::ResizeBL:
            t.x = m_startTransform.x + d.x();
            t.w = m_startTransform.w - d.x();
            t.h = m_startTransform.h + d.y();
            clampMin(t, m_startTransform, Handle::ResizeBL);
            lockAspect(t, m_startTransform, Handle::ResizeBL, !shift);
            break;
        case Handle::ResizeTL:
            t.x = m_startTransform.x + d.x();
            t.y = m_startTransform.y + d.y();
            t.w = m_startTransform.w - d.x();
            t.h = m_startTransform.h - d.y();
            clampMin(t, m_startTransform, Handle::ResizeTL);
            lockAspect(t, m_startTransform, Handle::ResizeTL, !shift);
            break;
        case Handle::ResizeCR:
            t.w = m_startTransform.w + d.x();
            clampMin(t, m_startTransform, Handle::ResizeCR);
            lockAspect(t, m_startTransform, Handle::ResizeCR, !shift);
            break;
        case Handle::ResizeCL:
            t.x = m_startTransform.x + d.x();
            t.w = m_startTransform.w - d.x();
            clampMin(t, m_startTransform, Handle::ResizeCL);
            lockAspect(t, m_startTransform, Handle::ResizeCL, !shift);
            break;
        case Handle::ResizeBC:
            t.h = m_startTransform.h + d.y();
            clampMin(t, m_startTransform, Handle::ResizeBC);
            lockAspect(t, m_startTransform, Handle::ResizeBC, !shift);
            break;
        case Handle::ResizeTC:
            t.y = m_startTransform.y + d.y();
            t.h = m_startTransform.h - d.y();
            clampMin(t, m_startTransform, Handle::ResizeTC);
            lockAspect(t, m_startTransform, Handle::ResizeTC, !shift);
            break;
        default:
            break;
        }

        if (!ctrl && m_handle != Handle::Move) {
            // Snap edges of resized box
            Transform snapped = t;
            const QRectF r = transformToNormRect(snapped);
            if (std::abs(r.left()) < kSnapDist) {
                const double dxs = -r.left();
                snapped.x += dxs;
                if (m_handle == Handle::ResizeCL || m_handle == Handle::ResizeTL || m_handle == Handle::ResizeBL)
                    snapped.w -= dxs;
            }
            if (std::abs(r.right() - 1.0) < kSnapDist) {
                const double dxs = 1.0 - r.right();
                if (m_handle == Handle::ResizeCR || m_handle == Handle::ResizeTR || m_handle == Handle::ResizeBR)
                    snapped.w += dxs;
            }
            if (std::abs(r.top()) < kSnapDist) {
                const double dys = -r.top();
                snapped.y += dys;
                if (m_handle == Handle::ResizeTC || m_handle == Handle::ResizeTL || m_handle == Handle::ResizeTR)
                    snapped.h -= dys;
            }
            if (std::abs(r.bottom() - 1.0) < kSnapDist) {
                const double dys = 1.0 - r.bottom();
                if (m_handle == Handle::ResizeBC || m_handle == Handle::ResizeBL || m_handle == Handle::ResizeBR)
                    snapped.h += dys;
            }
            t = snapped;
        }

        m_engine->updateSourceTransform(src->id, t);
        refreshChrome();
    }

    static void clampMin(Transform& t, const Transform& start, Handle h)
    {
        const double minW = kMinSize;
        const double minH = kMinSize;
        if (std::abs(t.w) < minW) {
            const double sign = start.w < 0 ? -1.0 : 1.0;
            if (h == Handle::ResizeCL || h == Handle::ResizeTL || h == Handle::ResizeBL)
                t.x = start.x + start.w - sign * minW;
            t.w = sign * minW;
        }
        if (std::abs(t.h) < minH) {
            const double sign = start.h < 0 ? -1.0 : 1.0;
            if (h == Handle::ResizeTC || h == Handle::ResizeTL || h == Handle::ResizeTR)
                t.y = start.y + start.h - sign * minH;
            t.h = sign * minH;
        }
    }

    void applyCrop(Transform& t, const QPointF& d)
    {
        // Map handle drag into crop fractions; shrink box accordingly for edge crops.
        auto clampCrop = [](double v) { return std::clamp(v, 0.0, 0.49); };

        switch (m_handle) {
        case Handle::ResizeCR:
        case Handle::ResizeBR:
        case Handle::ResizeTR:
            t.cropRight = clampCrop(m_startTransform.cropRight + d.x());
            t.w = m_startTransform.w - (t.cropRight - m_startTransform.cropRight);
            break;
        case Handle::ResizeCL:
        case Handle::ResizeBL:
        case Handle::ResizeTL:
            t.cropLeft = clampCrop(m_startTransform.cropLeft - d.x());
            {
                const double dc = t.cropLeft - m_startTransform.cropLeft;
                t.x = m_startTransform.x + dc;
                t.w = m_startTransform.w - dc;
            }
            break;
        default:
            break;
        }

        switch (m_handle) {
        case Handle::ResizeBC:
        case Handle::ResizeBR:
        case Handle::ResizeBL:
            t.cropBottom = clampCrop(m_startTransform.cropBottom + d.y());
            t.h = m_startTransform.h - (t.cropBottom - m_startTransform.cropBottom);
            break;
        case Handle::ResizeTC:
        case Handle::ResizeTR:
        case Handle::ResizeTL:
            t.cropTop = clampCrop(m_startTransform.cropTop - d.y());
            {
                const double dc = t.cropTop - m_startTransform.cropTop;
                t.y = m_startTransform.y + dc;
                t.h = m_startTransform.h - dc;
            }
            break;
        default:
            break;
        }
        clampMin(t, m_startTransform, m_handle);
    }

    EngineController* m_engine = nullptr;
    PreviewWidget* m_owner = nullptr;
    PreviewSurface* m_surface = nullptr;
    bool m_program = false;
    bool m_interactive = false;
    bool m_live = false;
    bool m_dragging = false;
    bool m_cropping = false;
    Handle m_handle = Handle::None;
    QPointF m_dragStart;
    QPointF m_dragStartNorm;
    QPointF m_startCenter;
    Transform m_startTransform;
    double m_startAngle = 0.0;
};

PreviewWidget::PreviewWidget(EngineController* engine, bool program, QWidget* parent)
    : QWidget(parent), m_engine(engine), m_program(program)
{
    const QString accent = program ? QStringLiteral("#FF5A2C") : QStringLiteral("#22C55E");

    setFocusPolicy(program ? Qt::NoFocus : Qt::StrongFocus);
    setAcceptDrops(!program);
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

    // Scale / lock / projector chrome (OBS Preview power)
    m_scaleLabel = new QLabel(QStringLiteral("Fit"), header);
    m_scaleLabel->setStyleSheet(QStringLiteral(
        "font-family:'JetBrains Mono'; font-size:9px; color:#A0A8B8;"
        "background:#0A0C0F; border:1px solid #3A3D45; border-radius:2px; padding:2px 6px;"));
    h->addWidget(m_scaleLabel);

    auto* scaleBtn = new QPushButton(QStringLiteral("Scale"), header);
    scaleBtn->setCursor(Qt::PointingHandCursor);
    scaleBtn->setFixedHeight(20);
    scaleBtn->setStyleSheet(QStringLiteral(
        "QPushButton{background:#1A1E26;border:1px solid #5A5E68;border-radius:2px;color:#E0E2E8;"
        "font-size:9px;font-weight:700;padding:1px 8px;}"
        "QPushButton:hover{border-color:#4F9EFF;}"));
    auto* scaleMenu = new QMenu(scaleBtn);
    scaleMenu->addAction(QStringLiteral("Fit to Window"), this, [this] {
        setDisplayMode(PreviewDisplayMode::FitWindow);
    });
    scaleMenu->addAction(QStringLiteral("Canvas (100%)"), this, [this] {
        setDisplayMode(PreviewDisplayMode::FixedScale);
        setScaleAmount(1.0f);
    });
    scaleMenu->addSeparator();
    scaleMenu->addAction(QStringLiteral("Zoom In"), this, &PreviewWidget::zoomIn);
    scaleMenu->addAction(QStringLiteral("Zoom Out"), this, &PreviewWidget::zoomOut);
    scaleMenu->addAction(QStringLiteral("Reset Zoom"), this, &PreviewWidget::resetScale);
    scaleBtn->setMenu(scaleMenu);
    h->addWidget(scaleBtn);

    if (!program) {
        m_lockBtn = new QPushButton(QStringLiteral("Lock"), header);
        m_lockBtn->setCheckable(true);
        m_lockBtn->setCursor(Qt::PointingHandCursor);
        m_lockBtn->setFixedHeight(20);
        m_lockBtn->setToolTip(QStringLiteral("Lock preview editing (move/resize)"));
        m_lockBtn->setStyleSheet(QStringLiteral(
            "QPushButton{background:#1A1E26;border:1px solid #5A5E68;border-radius:2px;color:#E0E2E8;"
            "font-size:9px;font-weight:700;padding:1px 8px;}"
            "QPushButton:checked{background:#3A2010;border-color:#F97316;color:#FDBA74;}"
            "QPushButton:hover{border-color:#4F9EFF;}"));
        connect(m_lockBtn, &QPushButton::toggled, this, &PreviewWidget::setEditLocked);
        h->addWidget(m_lockBtn);
    }

    auto* projBtn = new QPushButton(QStringLiteral("Proj"), header);
    projBtn->setCursor(Qt::PointingHandCursor);
    projBtn->setFixedHeight(20);
    projBtn->setToolTip(QStringLiteral("Open projector"));
    projBtn->setStyleSheet(QStringLiteral(
        "QPushButton{background:#1A1E26;border:1px solid #5A5E68;border-radius:2px;color:#E0E2E8;"
        "font-size:9px;font-weight:700;padding:1px 8px;}"
        "QPushButton:hover{border-color:#4F9EFF;}"));
    auto* projMenu = new QMenu(projBtn);
    projMenu->addAction(QStringLiteral("Windowed Projector"), this, &PreviewWidget::openProjectorWindowed);
    projMenu->addAction(QStringLiteral("Fullscreen Projector"), this, &PreviewWidget::openProjectorFullscreen);
    projBtn->setMenu(projMenu);
    h->addWidget(projBtn);

    col->addWidget(header);

    stage = new StageChrome(program, this);
    auto* stageLay = new QVBoxLayout(stage);
    stageLay->setContentsMargins(3, 3, 3, 3);
    stageLay->setSpacing(0);

    m_scroll = new QScrollArea(stage);
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setAlignment(Qt::AlignCenter);
    m_scroll->setStyleSheet(QStringLiteral("QScrollArea{background:#000;border:none;}"));
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    m_stackHost = new QWidget();
    m_stackHost->setStyleSheet(QStringLiteral("background:#000000;"));
    auto* stack = new QStackedLayout(m_stackHost);
    stack->setStackingMode(QStackedLayout::StackAll);
    stack->setContentsMargins(0, 0, 0, 0);

    m_surface = new PreviewSurface(m_stackHost);
    if (engine && engine->graphicsDevice())
        m_surface->setDevice(engine->graphicsDevice());
    m_surface->setLabel({}, QColor(accent));
    m_surface->setEmptyMessage(program ? QStringLiteral("NO OUTPUT") : QStringLiteral("NO PREVIEW"));
    stack->addWidget(m_surface);

    m_overlay = new CanvasOverlay(engine, this, m_surface, program);
    m_overlay->setInteractive(!program);
    m_scroll->setWidget(m_stackHost);
    stageLay->addWidget(m_scroll, 1);
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

    if (!program) {
        connect(this, &PreviewWidget::configureSourceRequested, this, [this](const QString& id) {
            if (!m_engine || id.isEmpty()) return;
            SourcePropertiesDialog dlg(m_engine, id, this);
            dlg.exec();
        });
    }
}

void PreviewWidget::tick()
{
    if (!m_engine || !m_engine->compositor()) return;
    if (m_engine->graphicsDevice())
        m_surface->setDevice(m_engine->graphicsDevice());
    if (m_overlay)
        m_overlay->refreshChrome();
    auto* tex = m_program ? m_engine->compositor()->programTexture()
                          : m_engine->compositor()->previewTexture();
    if (tex)
        m_surface->presentTexture(tex);
}

void PreviewWidget::keyPressEvent(QKeyEvent* event)
{
    if (!m_program && m_overlay && m_overlay->eventFilter(m_surface, event))
        return;
    QWidget::keyPressEvent(event);
}

void PreviewWidget::focusInEvent(QFocusEvent* event)
{
    QWidget::focusInEvent(event);
    if (!m_program && m_surface)
        m_surface->setFocus(Qt::OtherFocusReason);
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

int PreviewWidget::canvasWidth() const
{
    if (m_engine && m_engine->compositor())
        return std::max(1, m_engine->compositor()->width());
    return 1920;
}

int PreviewWidget::canvasHeight() const
{
    if (m_engine && m_engine->compositor())
        return std::max(1, m_engine->compositor()->height());
    return 1080;
}

void PreviewWidget::updateScaleLabel()
{
    if (!m_scaleLabel) return;
    if (m_displayMode == PreviewDisplayMode::FitWindow)
        m_scaleLabel->setText(QStringLiteral("Fit"));
    else
        m_scaleLabel->setText(QStringLiteral("%1%").arg(int(std::lround(m_scaleAmount * 100.f))));
}

void PreviewWidget::applyDisplayLayout()
{
    if (!m_scroll || !m_stackHost || !m_surface)
        return;
    if (m_displayMode == PreviewDisplayMode::FitWindow) {
        m_scroll->setWidgetResizable(true);
        m_stackHost->setMinimumSize(0, 0);
        m_stackHost->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        m_surface->setMinimumSize(160, 90);
        m_surface->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    } else {
        m_scroll->setWidgetResizable(false);
        const int w = std::max(160, int(std::lround(canvasWidth() * double(m_scaleAmount))));
        const int h = std::max(90, int(std::lround(canvasHeight() * double(m_scaleAmount))));
        m_stackHost->setFixedSize(w, h);
        m_surface->setFixedSize(w, h);
    }
    updateScaleLabel();
}

void PreviewWidget::setDisplayMode(PreviewDisplayMode mode)
{
    m_displayMode = mode;
    if (mode == PreviewDisplayMode::FitWindow)
        m_scaleAmount = 1.0f;
    applyDisplayLayout();
}

void PreviewWidget::setScaleAmount(float amount)
{
    m_scaleAmount = std::clamp(amount, 0.25f, 4.0f);
    m_displayMode = PreviewDisplayMode::FixedScale;
    applyDisplayLayout();
}

void PreviewWidget::zoomIn()
{
    setScaleAmount(m_scaleAmount * 1.25f);
}

void PreviewWidget::zoomOut()
{
    setScaleAmount(m_scaleAmount / 1.25f);
}

void PreviewWidget::resetScale()
{
    setDisplayMode(PreviewDisplayMode::FitWindow);
}

void PreviewWidget::setEditLocked(bool locked)
{
    m_editLocked = locked;
    if (m_lockBtn) {
        QSignalBlocker b(m_lockBtn);
        m_lockBtn->setChecked(locked);
        m_lockBtn->setText(locked ? QStringLiteral("Locked") : QStringLiteral("Lock"));
    }
    if (m_overlay)
        m_overlay->refreshChrome();
}

void PreviewWidget::openProjectorWindowed()
{
    ProjectorWindow::openWindowed(m_engine,
                                  m_program ? ProjectorKind::Program : ProjectorKind::Preview,
                                  window());
}

void PreviewWidget::openProjectorFullscreen()
{
    ProjectorWindow::openFullscreen(m_engine,
                                    m_program ? ProjectorKind::Program : ProjectorKind::Preview,
                                    this);
}

void PreviewWidget::wheelEvent(QWheelEvent* event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->angleDelta().y() > 0)
            zoomIn();
        else if (event->angleDelta().y() < 0)
            zoomOut();
        event->accept();
        return;
    }
    QWidget::wheelEvent(event);
}

void PreviewWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    if (m_displayMode == PreviewDisplayMode::FitWindow)
        applyDisplayLayout();
}

void PreviewWidget::dragEnterEvent(QDragEnterEvent* event)
{
    if (m_program || !event || !event->mimeData()) return;
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void PreviewWidget::dragMoveEvent(QDragMoveEvent* event)
{
    if (m_program || !event || !event->mimeData()) return;
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void PreviewWidget::dropEvent(QDropEvent* event)
{
    if (m_program || !event || !event->mimeData() || !m_engine) return;
    if (!event->mimeData()->hasUrls()) return;
    const int n = importDroppedUrls(m_engine, event->mimeData()->urls());
    if (n > 0)
        event->acceptProposedAction();
}

} // namespace railshot
