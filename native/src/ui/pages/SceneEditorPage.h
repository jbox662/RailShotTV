#pragma once
#include "ui/widgets/OverlayLibraryWidget.h"
#include <QWidget>
#include <QKeyEvent>
#include <QFrame>

namespace railshot {
class EngineController;
class PreviewWidget;

class SceneEditorPage : public QWidget {
    Q_OBJECT
public:
    explicit SceneEditorPage(EngineController* engine, QWidget* parent = nullptr);

signals:
    void backToDashboard();

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    void applyOverlayTemplate(const OverlayTemplateInfo& tmpl);
    void flashDrop();

    EngineController* m_engine = nullptr;
    PreviewWidget* m_canvas = nullptr;
    QFrame* m_canvasHost = nullptr;
};

} // namespace railshot
