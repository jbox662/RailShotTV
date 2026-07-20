// SceneEditorPage.cpp
// Chromatic Command design: dark navy surfaces, vivid accent colours.
// Integrates OverlayBrowser panel with OBSSourceManager signal wiring.
// Source tree selection drives the SourcePropertiesPanel dynamically.

#include "SceneEditorPage.h"
#include "widgets/OverlayBrowser.h"
#include "widgets/SourcePropertiesPanel.h"
#include "obs/OBSDisplay.h"
#include "obs/OBSSourceManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QPushButton>
#include <QListWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QSlider>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QPainter>
#include <QPaintEvent>
#include <QDebug>

using namespace RailShot;

// ── Helpers ────────────────────────────────────────────────────────────────────
static QLabel *uiLbl(const QString &t, const QString &c,
                     int sz = 11, bool bold = false, QWidget *p = nullptr)
{
    QLabel *l = new QLabel(t, p);
    l->setStyleSheet(QString("font-family:'DM Sans','Segoe UI',sans-serif;"
                             "font-size:%1px;color:%2;font-weight:%3;")
                         .arg(sz).arg(c).arg(bold ? "600" : "400"));
    return l;
}
static QLabel *bebasLbl(const QString &t, const QString &c,
                        int sz = 18, QWidget *p = nullptr)
{
    QLabel *l = new QLabel(t, p);
    l->setStyleSheet(QString("font-family:'Bebas Neue','Impact',sans-serif;"
                             "font-size:%1px;color:%2;letter-spacing:1px;").arg(sz).arg(c));
    return l;
}
static QFrame *hSep(QWidget *p = nullptr)
{
    QFrame *f = new QFrame(p);
    f->setFrameShape(QFrame::HLine);
    f->setFixedHeight(1);
    f->setStyleSheet("background:#2A3350;border:none;");
    return f;
}
static QPushButton *toolBtn(const QString &text, QWidget *p = nullptr)
{
    QPushButton *btn = new QPushButton(text, p);
    btn->setObjectName("ToolBtn");
    btn->setFixedHeight(26);
    btn->setMinimumWidth(32);
    return btn;
}

// ── SceneEditorPage ────────────────────────────────────────────────────────────
SceneEditorPage::SceneEditorPage(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("SceneEditorPage");
    setupUi();
}

void SceneEditorPage::setupUi()
{
    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── Top bar ────────────────────────────────────────────────────────────────
    QWidget *topBar = new QWidget(this);
    topBar->setObjectName("TopBar");
    topBar->setFixedHeight(46);
    buildTopBar(topBar);
    root->addWidget(topBar);

    // ── Tool bar ───────────────────────────────────────────────────────────────
    QWidget *toolBar = new QWidget(this);
    toolBar->setObjectName("ToolBar");
    toolBar->setFixedHeight(36);
    buildToolBar(toolBar);
    root->addWidget(toolBar);

    // ── Main body ──────────────────────────────────────────────────────────────
    QWidget *body = new QWidget(this);
    QHBoxLayout *bodyLayout = new QHBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);
    root->addWidget(body, 1);

    // Left panel (scenes + sources)
    QWidget *leftPanel = new QWidget(body);
    leftPanel->setFixedWidth(180);
    buildLeftPanel(leftPanel);
    bodyLayout->addWidget(leftPanel);

    // Canvas
    QWidget *canvas = new QWidget(body);
    buildCanvas(canvas);
    bodyLayout->addWidget(canvas, 1);

    // Overlay browser panel (collapsible, 240px)
    m_overlayPanel = new QWidget(body);
    m_overlayPanel->setFixedWidth(240);
    buildOverlayBrowserPanel(m_overlayPanel);
    bodyLayout->addWidget(m_overlayPanel);

    // Properties panel — SourcePropertiesPanel (320px)
    m_propertiesPanel = new SourcePropertiesPanel(body);
    m_propertiesPanel->setFixedWidth(320);
    bodyLayout->addWidget(m_propertiesPanel);

    // ── Transitions rail ───────────────────────────────────────────────────────
    QWidget *transRail = new QWidget(this);
    transRail->setObjectName("TransitionsRail");
    transRail->setFixedHeight(44);
    buildTransitionsRail(transRail);
    root->addWidget(transRail);
}

// ── Top bar ────────────────────────────────────────────────────────────────────
void SceneEditorPage::buildTopBar(QWidget *bar)
{
    bar->setStyleSheet("background:#1A2035;border-bottom:1px solid #2A3350;");
    QHBoxLayout *l = new QHBoxLayout(bar);
    l->setContentsMargins(16, 0, 16, 0);
    l->setSpacing(12);
    QLabel *rail = bebasLbl("RAILSHOT", "#F8F8FF", 18, bar);
    QLabel *tv   = bebasLbl(" TV", "#FF5A2C", 18, bar);
    l->addWidget(rail); l->addWidget(tv);
    QFrame *sep = new QFrame(bar);
    sep->setFrameShape(QFrame::VLine);
    sep->setFixedWidth(1);
    sep->setStyleSheet("background:#2A3350;border:none;");
    l->addWidget(sep);
    l->addWidget(uiLbl("SCENE EDITOR", "#8892A4", 11, true, bar));
    l->addStretch();

    m_overlayToggle = new QPushButton("⊞ Overlays", bar);
    m_overlayToggle->setObjectName("ToolBtn");
    m_overlayToggle->setFixedHeight(26);
    m_overlayToggle->setCheckable(true);
    m_overlayToggle->setChecked(true);
    m_overlayToggle->setToolTip("Toggle Overlay Library panel");
    connect(m_overlayToggle, &QPushButton::toggled,
            this, &SceneEditorPage::toggleOverlayBrowser);
    l->addWidget(m_overlayToggle);

    l->addWidget(uiLbl("● SIG OK", "#22C55E", 11, false, bar));
    l->addWidget(uiLbl("Default Profile", "#50506A", 11, false, bar));
}

// ── Tool bar ───────────────────────────────────────────────────────────────────
void SceneEditorPage::buildToolBar(QWidget *bar)
{
    bar->setStyleSheet("background:#1A2035;border-bottom:1px solid #2A3350;");
    QHBoxLayout *l = new QHBoxLayout(bar);
    l->setContentsMargins(8, 0, 8, 0);
    l->setSpacing(4);
    for (const QString &t : {"Select","Crop","Move","Rotate"})
        l->addWidget(toolBtn(t, bar));
    QFrame *vsep = new QFrame(bar);
    vsep->setFrameShape(QFrame::VLine);
    vsep->setFixedWidth(1);
    vsep->setStyleSheet("background:#2A3350;border:none;");
    l->addWidget(vsep);
    for (const QString &t : {"Fit","1:1","Grid","Lock"})
        l->addWidget(toolBtn(t, bar));
    l->addStretch();
    l->addWidget(uiLbl("Canvas: 1920×1080", "#50506A", 10, false, bar));
}

// ── Left panel: Scenes + Sources ───────────────────────────────────────────────
void SceneEditorPage::buildLeftPanel(QWidget *panel)
{
    panel->setStyleSheet("background:#1A2035;border-right:1px solid #2A3350;");
    QVBoxLayout *vl = new QVBoxLayout(panel);
    vl->setContentsMargins(0, 0, 0, 0);
    vl->setSpacing(0);

    // Scenes section
    QWidget *sceneHeader = new QWidget(panel);
    sceneHeader->setFixedHeight(28);
    sceneHeader->setStyleSheet("background:#1E2640;border-bottom:1px solid #2A3350;");
    QHBoxLayout *sh = new QHBoxLayout(sceneHeader);
    sh->setContentsMargins(8, 0, 4, 0);
    sh->addWidget(uiLbl("SCENES", "#A0A0B8", 10, true, sceneHeader));
    sh->addStretch();
    QPushButton *addScene = new QPushButton("+", sceneHeader);
    addScene->setObjectName("ToolBtn");
    addScene->setFixedSize(20, 20);
    sh->addWidget(addScene);
    vl->addWidget(sceneHeader);

    m_sceneList = new QListWidget(panel);
    m_sceneList->setObjectName("SceneList");
    m_sceneList->setFixedHeight(120);
    for (const QString &s : {"Main View","Wide Angle","Replay","Interview","Scoreboard","End Screen"})
        m_sceneList->addItem(s);
    m_sceneList->setCurrentRow(0);
    vl->addWidget(m_sceneList);

    vl->addWidget(hSep(panel));

    // Sources section
    QWidget *srcHeader = new QWidget(panel);
    srcHeader->setFixedHeight(28);
    srcHeader->setStyleSheet("background:#1E2640;border-bottom:1px solid #2A3350;");
    QHBoxLayout *srch = new QHBoxLayout(srcHeader);
    srch->setContentsMargins(8, 0, 4, 0);
    srch->addWidget(uiLbl("SOURCES", "#A0A0B8", 10, true, srcHeader));
    srch->addStretch();
    QPushButton *addSrc = new QPushButton("+", srcHeader);
    addSrc->setObjectName("ToolBtn");
    addSrc->setFixedSize(20, 20);
    srch->addWidget(addSrc);
    vl->addWidget(srcHeader);

    m_sourceTree = new QTreeWidget(panel);
    m_sourceTree->setObjectName("SourceTree");
    m_sourceTree->setHeaderHidden(true);
    m_sourceTree->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Seed with example sources (itemId stored in Qt::UserRole+1)
    struct SeedSrc { QString name; QString typeId; };
    for (const SeedSrc &s : {
            SeedSrc{"Game Capture",  "game_capture"},
            SeedSrc{"Webcam",        "dshow_input"},
            SeedSrc{"Lower Third",   "image_source"},
            SeedSrc{"Score Overlay", "browser_source"},
            SeedSrc{"Desktop Audio", "wasapi_output_capture"}}) {
        appendSourceToTree(s.name, s.typeId, -1);
    }

    // ── KEY WIRING: selection → properties panel ──────────────────────────────
    connect(m_sourceTree, &QTreeWidget::currentItemChanged,
            this, &SceneEditorPage::onSourceSelectionChanged);

    vl->addWidget(m_sourceTree, 1);
}

// ── Canvas ─────────────────────────────────────────────────────────────────────
void SceneEditorPage::buildCanvas(QWidget *canvas)
{
    canvas->setStyleSheet("background:#161B2E;");
    QVBoxLayout *vl = new QVBoxLayout(canvas);
    vl->setContentsMargins(0, 0, 0, 0);
    vl->setSpacing(0);

    QWidget *header = new QWidget(canvas);
    header->setFixedHeight(28);
    header->setStyleSheet("background:#1A2035;border-bottom:2px solid #4F9EFF;");
    QHBoxLayout *hl = new QHBoxLayout(header);
    hl->setContentsMargins(12, 0, 12, 0);
    hl->addWidget(uiLbl("CANVAS WORKSPACE", "#A0A0B8", 11, true, header));
    hl->addStretch();
    hl->addWidget(uiLbl("1920×1080  ·  16:9", "#50506A", 10, false, header));
    vl->addWidget(header);

    m_obsDisplay = new OBSDisplay(canvas);
    m_obsDisplay->setObjectName("CanvasWorkspace");
    m_obsDisplay->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_obsDisplay->setBackgroundColor(0x16, 0x1B, 0x2E);
    m_obsDisplay->setAcceptOverlayDrops(true);
    connect(m_obsDisplay, &OBSDisplay::overlayDropped,
            this, &SceneEditorPage::onCanvasOverlayDropped);
    vl->addWidget(m_obsDisplay, 1);
}

// ── Overlay Browser panel ──────────────────────────────────────────────────────
void SceneEditorPage::buildOverlayBrowserPanel(QWidget *panel)
{
    panel->setStyleSheet("background:#14192A;border-left:1px solid #2A3350;");
    QVBoxLayout *vl = new QVBoxLayout(panel);
    vl->setContentsMargins(0, 0, 0, 0);
    vl->setSpacing(0);

    QWidget *header = new QWidget(panel);
    header->setFixedHeight(28);
    header->setStyleSheet("background:#1E2640;border-bottom:2px solid #FF5A2C;");
    QHBoxLayout *hl = new QHBoxLayout(header);
    hl->setContentsMargins(12, 0, 8, 0);
    hl->addWidget(uiLbl("OVERLAY LIBRARY", "#A0A0B8", 11, true, header));
    hl->addStretch();
    vl->addWidget(header);

    m_overlayBrowser = new OverlayBrowser(panel);
    QWidget *inner = m_overlayBrowser->widget();
    if (inner) {
        inner->setParent(panel);
        vl->addWidget(inner, 1);
    } else {
        m_overlayBrowser->setParent(panel);
        m_overlayBrowser->setFeatures(QDockWidget::NoDockWidgetFeatures);
        m_overlayBrowser->setTitleBarWidget(new QWidget());
        vl->addWidget(m_overlayBrowser, 1);
    }

    connect(m_overlayBrowser, &OverlayBrowser::addTemplateRequested,
            this, &SceneEditorPage::onAddTemplateRequested);
    connect(m_overlayBrowser, &OverlayBrowser::dropTemplateRequested,
            this, &SceneEditorPage::onDropTemplateRequested);
}

// ── Transitions rail ───────────────────────────────────────────────────────────
void SceneEditorPage::buildTransitionsRail(QWidget *rail)
{
    rail->setStyleSheet("background:#1E2640;border-top:1px solid #2A3350;");
    QHBoxLayout *l = new QHBoxLayout(rail);
    l->setContentsMargins(12, 0, 12, 0);
    l->setSpacing(6);
    l->addWidget(uiLbl("TRANSITION:", "#8892A4", 10, true, rail));
    for (const QString &t : {"Cut","Fade","Slide","Wipe","Stinger"}) {
        QPushButton *btn = new QPushButton(t, rail);
        btn->setObjectName(t == "Fade" ? "TransBtnActive" : "TransBtn");
        btn->setFixedHeight(28);
        l->addWidget(btn);
    }
    l->addWidget(uiLbl("|", "#2A3350", 14, false, rail));
    l->addWidget(uiLbl("Duration:", "#8892A4", 10, false, rail));
    QSpinBox *dur = new QSpinBox(rail);
    dur->setObjectName("PropSpinBox");
    dur->setRange(100, 5000);
    dur->setValue(300);
    dur->setSuffix(" ms");
    dur->setFixedWidth(90);
    l->addWidget(dur);
    l->addStretch();
    QPushButton *transBtn = new QPushButton("▶  TRANSITION", rail);
    transBtn->setObjectName("TransitionTriggerBtn");
    transBtn->setFixedHeight(28);
    l->addWidget(transBtn);
}

// ── Helpers ────────────────────────────────────────────────────────────────────
void SceneEditorPage::appendSourceToTree(const QString &name,
                                          const QString &typeId,
                                          int itemId)
{
    QTreeWidgetItem *item = new QTreeWidgetItem(m_sourceTree);
    item->setText(0, name);
    item->setData(0, Qt::UserRole,     typeId);
    item->setData(0, Qt::UserRole + 1, itemId);   // scene-item ID (-1 if unknown)
    m_sourceTree->scrollToItem(item);
    m_sourceTree->setCurrentItem(item);
}

// ── Source selection → Properties panel ───────────────────────────────────────
void SceneEditorPage::onSourceSelectionChanged(QTreeWidgetItem *current,
                                                QTreeWidgetItem * /*previous*/)
{
    if (!m_propertiesPanel) return;

    if (!current) {
        m_propertiesPanel->clearSource();
        return;
    }

    const int itemId = current->data(0, Qt::UserRole + 1).toInt();

    if (itemId >= 0) {
        // Real libobs scene-item: load live settings
        m_propertiesPanel->loadSource(itemId);
    } else {
        // Seeded placeholder item (not yet backed by a real OBS source):
        // show the panel in "offline" mode with the name and type only.
        // We call clearSource() to show the placeholder text, then manually
        // update the name/type labels so the user sees something useful.
        m_propertiesPanel->clearSource();
        qInfo() << "[SceneEditorPage] Selected placeholder source:"
                << current->text(0)
                << "type:" << current->data(0, Qt::UserRole).toString();
    }
}

// ── OverlayBrowser signal handlers ────────────────────────────────────────────
void SceneEditorPage::onAddTemplateRequested(const OverlayTemplate &tmpl)
{
    qInfo() << "[SceneEditorPage] Adding overlay template (centre):"
            << tmpl.name << "type:" << tmpl.obsSourceType;

    const bool ok = OBSSourceManager::instance().addSourceAt(
        tmpl.obsSourceType, tmpl.name, tmpl.defaultSettingsJson,
        QPointF(0.5, 0.5));

    if (ok) {
        // Retrieve the newly added item's ID (last item in the scene)
        const auto sources = OBSSourceManager::instance().sourcesInActiveScene();
        const int newId = sources.isEmpty() ? -1 : sources.first().id;
        appendSourceToTree(tmpl.name, tmpl.obsSourceType, newId);
        qInfo() << "[SceneEditorPage] Overlay added, itemId:" << newId;
    } else {
        qWarning() << "[SceneEditorPage] addSourceAt failed for template:" << tmpl.id;
    }
}

void SceneEditorPage::onDropTemplateRequested(const OverlayTemplate &tmpl,
                                               QPointF normPos)
{
    const bool ok = OBSSourceManager::instance().addSourceAt(
        tmpl.obsSourceType, tmpl.name, tmpl.defaultSettingsJson, normPos);

    if (ok) {
        const auto sources = OBSSourceManager::instance().sourcesInActiveScene();
        const int newId = sources.isEmpty() ? -1 : sources.first().id;
        appendSourceToTree(tmpl.name, tmpl.obsSourceType, newId);
    }
}

void SceneEditorPage::onCanvasOverlayDropped(const QString &templateId,
                                              QPointF normPos)
{
    if (!m_overlayBrowser) return;
    const OverlayTemplate *tmpl = m_overlayBrowser->findTemplate(templateId);
    if (!tmpl) {
        qWarning() << "[SceneEditorPage] Unknown template id:" << templateId;
        return;
    }

    const bool ok = OBSSourceManager::instance().addSourceAt(
        tmpl->obsSourceType, tmpl->name, tmpl->defaultSettingsJson, normPos);

    if (ok) {
        const auto sources = OBSSourceManager::instance().sourcesInActiveScene();
        const int newId = sources.isEmpty() ? -1 : sources.first().id;
        appendSourceToTree(tmpl->name, tmpl->obsSourceType, newId);
    }
}

void SceneEditorPage::toggleOverlayBrowser()
{
    if (!m_overlayPanel) return;
    const bool visible = m_overlayToggle ? m_overlayToggle->isChecked() : true;
    m_overlayPanel->setVisible(visible);
}

