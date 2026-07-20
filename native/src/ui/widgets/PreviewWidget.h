#pragma once
#include "compositor/PreviewSurface.h"

namespace railshot {
class EngineController;

/// Preview or Program monitor. Preview supports scene-editor hit testing.
class PreviewWidget : public QWidget {
    Q_OBJECT
public:
    PreviewWidget(EngineController* engine, bool program, QWidget* parent = nullptr);
    void tick();

signals:
    void sourceSelected(const QString& sourceId);

private:
    class CanvasOverlay;

    EngineController* m_engine = nullptr;
    bool m_program = false;
    PreviewSurface* m_surface = nullptr;
    CanvasOverlay* m_overlay = nullptr;
};

} // namespace railshot
