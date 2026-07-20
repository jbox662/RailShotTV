#pragma once
#include "ui/widgets/OverlayLibraryWidget.h"
#include <QWidget>
#include <QKeyEvent>

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

    EngineController* m_engine = nullptr;
    PreviewWidget* m_canvas = nullptr;
};

} // namespace railshot
