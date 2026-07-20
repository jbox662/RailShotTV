#include "OBSDisplay.h"
#include "OBSCore.h"

#include <obs.h>
#include <graphics/graphics.h>

#include <QShowEvent>
#include <QResizeEvent>
#include <QWindow>
#include <QDebug>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>

// MIME type constant — must match OverlayBrowser::kMimeType
static constexpr const char *kOverlayMimeType =
    "application/x-railshot-overlay-template";

// ── Default draw callback: renders the program output ────────────────────────
static void defaultDrawCallback(void * /*param*/, uint32_t cx, uint32_t cy)
{
    // Render the main compositor output texture into the display
    obs_render_main_texture();
    Q_UNUSED(cx); Q_UNUSED(cy);
}

// ── OBSDisplay ────────────────────────────────────────────────────────────────
OBSDisplay::OBSDisplay(QWidget *parent)
    : QWidget(parent)
{
    // Tell Qt this widget has a native window handle (required for HWND access)
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_PaintOnScreen);   // suppress Qt painting over the D3D surface
    setAttribute(Qt::WA_NoSystemBackground);
    setMinimumSize(160, 90);
}

OBSDisplay::~OBSDisplay()
{
    destroyDisplay();
}

void OBSDisplay::setDrawCallback(DrawCallback cb, void *param)
{
    m_drawCallback = cb;
    m_drawParam    = param;
}

void OBSDisplay::setBackgroundColor(uint8_t r, uint8_t g, uint8_t b)
{
    m_bgR = r; m_bgG = g; m_bgB = b;
    if (m_display) {
        obs_display_set_background_color(m_display,
            (static_cast<uint32_t>(r) << 16) |
            (static_cast<uint32_t>(g) << 8)  |
            static_cast<uint32_t>(b));
    }
}

// ── Qt event overrides ────────────────────────────────────────────────────────
void OBSDisplay::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (!m_display && OBSCore::instance().isInitialized()) {
        createDisplay();
    }
}

void OBSDisplay::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    // Keep the display alive when hidden — destroying and recreating is expensive
}

void OBSDisplay::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_display) {
        // Notify libobs of the new pixel dimensions
        obs_display_resize(m_display,
            static_cast<uint32_t>(width()  * devicePixelRatio()),
            static_cast<uint32_t>(height() * devicePixelRatio()));
    }
}

// ── Display lifecycle ─────────────────────────────────────────────────────────
void OBSDisplay::createDisplay()
{
    if (m_display) return;
    if (!OBSCore::instance().isInitialized()) {
        qWarning() << "[OBSDisplay] Cannot create display: OBSCore not initialized";
        return;
    }

    // Get the native Win32 HWND for this widget
    HWND hwnd = reinterpret_cast<HWND>(winId());
    if (!hwnd) {
        qCritical() << "[OBSDisplay] winId() returned null HWND";
        return;
    }

    // Build the display initializer
    gs_init_data info = {};
    info.cx           = static_cast<uint32_t>(width()  * devicePixelRatio());
    info.cy           = static_cast<uint32_t>(height() * devicePixelRatio());
    info.format       = GS_BGRA;
    info.zsformat     = GS_ZS_NONE;
    info.window.hwnd  = hwnd;

    m_display = obs_display_create(&info, 0x00161B2E);  // background = #161B2E
    if (!m_display) {
        qCritical() << "[OBSDisplay] obs_display_create() failed";
        return;
    }

    // Register the draw callback
    DrawCallback cb = m_drawCallback ? m_drawCallback : defaultDrawCallback;
    obs_display_add_draw_callback(m_display, [](void *param, uint32_t cx, uint32_t cy) {
        auto *self = static_cast<OBSDisplay *>(param);
        if (self->m_drawCallback) {
            self->m_drawCallback(self->m_drawParam, cx, cy);
        } else {
            obs_render_main_texture();
            Q_UNUSED(cx); Q_UNUSED(cy);
        }
    }, this);

    // Set background color
    obs_display_set_background_color(m_display,
        (static_cast<uint32_t>(m_bgR) << 16) |
        (static_cast<uint32_t>(m_bgG) << 8)  |
        static_cast<uint32_t>(m_bgB));

    qInfo() << "[OBSDisplay] Display created" << width() << "x" << height();
}

void OBSDisplay::destroyDisplay()
{
    if (!m_display) return;
    obs_display_destroy(m_display);
    m_display = nullptr;
    qInfo() << "[OBSDisplay] Display destroyed";
}

// ── Overlay drag-and-drop ─────────────────────────────────────────────────────
void OBSDisplay::setAcceptOverlayDrops(bool accept)
{
    m_acceptDrops = accept;
    setAcceptDrops(accept);
}

void OBSDisplay::dragEnterEvent(QDragEnterEvent *event)
{
    if (m_acceptDrops &&
        event->mimeData()->hasFormat(kOverlayMimeType)) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void OBSDisplay::dragMoveEvent(QDragMoveEvent *event)
{
    if (m_acceptDrops &&
        event->mimeData()->hasFormat(kOverlayMimeType)) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void OBSDisplay::dropEvent(QDropEvent *event)
{
    if (!m_acceptDrops) { event->ignore(); return; }

    const QByteArray data =
        event->mimeData()->data(kOverlayMimeType);
    if (data.isEmpty()) { event->ignore(); return; }

    // The MIME payload is just the template ID as UTF-8 text.
    // The actual OverlayTemplate is resolved by the SceneEditorPage slot
    // that is connected to overlayDropped(); we emit the ID here.
    const QString templateId = QString::fromUtf8(data);

    // Convert drop position to normalised [0..1] coordinates
    const QPointF pos = event->position();
    const QPointF normPos(
        qBound(0.0, pos.x() / width(),  1.0),
        qBound(0.0, pos.y() / height(), 1.0));

    event->acceptProposedAction();
    emit overlayDropped(templateId, normPos);
}
