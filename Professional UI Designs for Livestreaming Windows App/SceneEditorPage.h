#pragma once
// SceneEditorPage — Scene Editor with integrated OverlayBrowser panel.
//
// Layout (left→right):
//
//  ┌──────────────────────────────────────────────────────────────────────────┐
//  │ TopBar: RAILSHOT TV | SCENE EDITOR | sig | ⊞ Overlays            46px   │
//  ├──────────────────────────────────────────────────────────────────────────┤
//  │ ToolBar: Select | Crop | Move | Rotate | Fit | 1:1 | Grid | Lock 36px   │
//  ├──────────────┬──────────────────────────┬──────────────┬─────────────────┤
//  │ SCENES LIST  │  CANVAS (OBSDisplay)     │  OVERLAY     │  PROPERTIES     │
//  │ (QListWidget)│                          │  BROWSER     │  (SourceProps   │
//  │  180px wide  │                          │  (collapsible│   Panel)        │
//  │              │                          │   240px)     │  320px wide     │
//  │ SOURCES LIST │                          │              │                 │
//  │ (QTreeWidget)│                          │              │                 │
//  ├──────────────┴──────────────────────────┴──────────────┴─────────────────┤
//  │ TRANSITIONS rail: Cut | Fade | Slide | Wipe | Stinger | Duration  44px  │
//  └──────────────────────────────────────────────────────────────────────────┘

#include <QWidget>
#include <QLabel>
#include <QListWidget>
#include <QTreeWidget>
#include <QPushButton>

// Forward declarations
namespace RailShot { class OverlayBrowser; struct OverlayTemplate; }
class OBSDisplay;
class SourcePropertiesPanel;

class SceneEditorPage : public QWidget
{
    Q_OBJECT

public:
    explicit SceneEditorPage(QWidget *parent = nullptr);

private slots:
    // ── OverlayBrowser signal handlers ──────────────────────────────────────
    void onAddTemplateRequested(const RailShot::OverlayTemplate &tmpl);
    void onDropTemplateRequested(const RailShot::OverlayTemplate &tmpl, QPointF normPos);
    void onCanvasOverlayDropped(const QString &templateId, QPointF normPos);
    void toggleOverlayBrowser();

    // ── Source selection ─────────────────────────────────────────────────────
    /** Called when the user selects a different item in the sources tree. */
    void onSourceSelectionChanged(QTreeWidgetItem *current,
                                  QTreeWidgetItem *previous);

private:
    void setupUi();
    void buildTopBar(QWidget *parent);
    void buildToolBar(QWidget *parent);
    void buildLeftPanel(QWidget *parent);
    void buildCanvas(QWidget *parent);
    void buildOverlayBrowserPanel(QWidget *parent);
    void buildTransitionsRail(QWidget *parent);

    void appendSourceToTree(const QString &name, const QString &typeId, int itemId = -1);

    QListWidget               *m_sceneList        = nullptr;
    QTreeWidget               *m_sourceTree       = nullptr;
    OBSDisplay                *m_obsDisplay       = nullptr;
    RailShot::OverlayBrowser  *m_overlayBrowser   = nullptr;
    QWidget                   *m_overlayPanel     = nullptr;
    QPushButton               *m_overlayToggle    = nullptr;
    SourcePropertiesPanel     *m_propertiesPanel  = nullptr;
};
