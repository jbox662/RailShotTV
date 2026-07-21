#pragma once
#include "compositor/PreviewSurface.h"
#include <QKeyEvent>
#include <QFocusEvent>
#include <QWheelEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>

class QEvent;
class QLabel;
class QPushButton;
class QScrollArea;
class QWidget;

namespace railshot {
class EngineController;

enum class PreviewDisplayMode {
    FitWindow = 0,  // stretch canvas to widget (default)
    FixedScale = 1, // pixel zoom with scroll
};

/// Preview or Program monitor. Preview supports scene-editor hit testing.
class PreviewWidget : public QWidget {
    Q_OBJECT
public:
    PreviewWidget(EngineController* engine, bool program, QWidget* parent = nullptr);
    void tick();

    PreviewDisplayMode displayMode() const { return m_displayMode; }
    void setDisplayMode(PreviewDisplayMode mode);
    float scaleAmount() const { return m_scaleAmount; }
    void setScaleAmount(float amount);
    void zoomIn();
    void zoomOut();
    void resetScale();

    bool editLocked() const { return m_editLocked; }
    void setEditLocked(bool locked);

    bool isProgram() const { return m_program; }
    PreviewSurface* surface() const { return m_surface; }

signals:
    void sourceSelected(const QString& sourceId);
    void configureSourceRequested(const QString& sourceId);
    void interactRequested(const QString& sourceId);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    class CanvasOverlay;

    void applyDisplayLayout();
    void updateScaleLabel();
    void openProjectorWindowed();
    void openProjectorFullscreen();
    int canvasWidth() const;
    int canvasHeight() const;

    EngineController* m_engine = nullptr;
    bool m_program = false;
    PreviewSurface* m_surface = nullptr;
    CanvasOverlay* m_overlay = nullptr;
    QScrollArea* m_scroll = nullptr;
    QWidget* m_stackHost = nullptr;
    QLabel* m_scaleLabel = nullptr;
    QPushButton* m_lockBtn = nullptr;
    PreviewDisplayMode m_displayMode = PreviewDisplayMode::FitWindow;
    float m_scaleAmount = 1.0f;
    bool m_editLocked = false;
};

} // namespace railshot
