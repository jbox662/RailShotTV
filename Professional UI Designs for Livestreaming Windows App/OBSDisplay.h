#pragma once
/**
 * OBSDisplay — Qt widget that hosts an obs_display_t
 *
 * This widget creates a native Win32 child window and passes its HWND
 * to obs_display_create(), which renders the libobs compositor output
 * directly into that window using Direct3D 11.
 *
 * Usage:
 *   OBSDisplay *preview = new OBSDisplay(this);
 *   preview->setDrawCallback([](void*, uint32_t cx, uint32_t cy) {
 *       obs_render_main_texture();
 *   });
 *   layout->addWidget(preview);
 *
 * The display is automatically destroyed when the widget is destroyed.
 *
 * Thread safety:
 *   All obs_display_* calls must happen on the main thread.
 *   The draw callback is called from the libobs render thread.
 */

#include <QWidget>
#include <functional>
#include <QPointF>

// Forward-declare libobs types
struct obs_display;

// OverlayBrowser.h is NOT included here to avoid circular deps.
// The drop signal carries only the template ID string; SceneEditorPage
// resolves the full OverlayTemplate via OverlayBrowser::findTemplate().

class OBSDisplay : public QWidget
{
    Q_OBJECT

public:
    using DrawCallback = std::function<void(void *param, uint32_t cx, uint32_t cy)>;

    explicit OBSDisplay(QWidget *parent = nullptr);
    ~OBSDisplay() override;

    /**
     * Set the draw callback that libobs calls every frame.
     * Typically: obs_render_main_texture() for the program output,
     * or obs_render_main_texture_src_color_only() for the scene editor.
     *
     * Must be called before the widget is shown, or after destroying
     * and recreating the display.
     */
    void setDrawCallback(DrawCallback cb, void *param = nullptr);

    /**
     * Set the background color shown when no source is active.
     * @param r,g,b  0–255
     */
    void setBackgroundColor(uint8_t r, uint8_t g, uint8_t b);

    /**
     * Return true if the obs_display_t was created successfully.
     */
    bool isValid() const { return m_display != nullptr; }

    /**
     * Enable or disable accepting overlay template drops from OverlayBrowser.
     * When enabled the widget accepts drops of
     * "application/x-railshot-overlay-template" MIME data and emits
     * overlayDropped() with the decoded template and normalised drop position.
     */
    void setAcceptOverlayDrops(bool accept);

signals:
    /**
     * Emitted when a valid overlay template is dropped onto the canvas.
     * @param templateId  the OverlayTemplate::id string from the MIME payload
     * @param normPos     drop position normalised to [0..1, 0..1] relative to
     *                    the widget's content rect
     */
    void overlayDropped(const QString &templateId, QPointF normPos);

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    QPaintEngine *paintEngine() const override { return nullptr; }  // native rendering
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent  *event) override;
    void dropEvent(QDropEvent          *event) override;

private:
    void createDisplay();
    void destroyDisplay();

    obs_display  *m_display      = nullptr;
    DrawCallback  m_drawCallback = nullptr;
    void         *m_drawParam    = nullptr;
    uint8_t       m_bgR = 22, m_bgG = 27, m_bgB = 46;  // #161B2E
    bool          m_acceptDrops  = false;
};
