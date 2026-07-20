#pragma once
// RailShotTV — OverlayBrowser.h
// Chromatic Command design: collapsible QDockWidget panel with overlay template
// library. Supports drag-to-canvas (QDrag/QMimeData) and double-click-to-add.
// Wires into OBSSourceManager::addSource() to create real libobs sources.

#include <QDockWidget>
#include <QWidget>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QButtonGroup>
#include <QPushButton>
#include <QScrollArea>
#include <QLabel>
#include <QPixmap>
#include <QPainter>
#include <QMimeData>
#include <QDrag>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFlowLayout>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QColor>
#include <QMap>

namespace RailShot {

// ── Overlay template descriptor ───────────────────────────────────────────────
struct OverlayTemplate {
    QString id;           // Unique identifier, e.g. "sb-lower"
    QString category;     // "scoreboard" | "lowerthird" | "ticker" | "alert" | "branding"
    QString name;         // Display name
    QString obsSourceType;// libobs source type id, e.g. "text_gdiplus" / "browser_source"
    QColor  accentColor;  // Category accent colour
    QString description;  // Short tooltip description

    // Default obs_data_t settings as JSON string (applied when source is created)
    QString defaultSettingsJson;
};

// ── Thumbnail delegate item ───────────────────────────────────────────────────
class OverlayThumbnailItem : public QListWidgetItem {
public:
    explicit OverlayThumbnailItem(const OverlayTemplate &tmpl, QListWidget *parent = nullptr);

    const OverlayTemplate &overlayTemplate() const { return m_template; }

    // Renders a 160×90 preview pixmap using QPainter
    static QPixmap renderThumbnail(const OverlayTemplate &tmpl, int w = 160, int h = 90);

private:
    OverlayTemplate m_template;
};

// ── Custom QListWidget with drag support ─────────────────────────────────────
class OverlayListWidget : public QListWidget {
    Q_OBJECT
public:
    explicit OverlayListWidget(QWidget *parent = nullptr);

signals:
    void templateDoubleClicked(const OverlayTemplate &tmpl);
    void templateDragStarted(const OverlayTemplate &tmpl);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    QPoint m_dragStartPos;
    void startDrag(const OverlayTemplate &tmpl);
};

// ── OverlayBrowser panel ─────────────────────────────────────────────────────
class OverlayBrowser : public QDockWidget {
    Q_OBJECT

public:
    explicit OverlayBrowser(QWidget *parent = nullptr);
    ~OverlayBrowser() override = default;

    // MIME type used for drag-and-drop between this panel and OBSDisplay
    static constexpr const char *kMimeType = "application/x-railshot-overlay-template";

    // Returns all registered templates
    const QVector<OverlayTemplate> &templates() const { return m_templates; }

    // Returns template by id, or nullptr
    const OverlayTemplate *findTemplate(const QString &id) const;

signals:
    // Emitted when user double-clicks a template (add at default position)
    void addTemplateRequested(const OverlayTemplate &tmpl);

    // Emitted when user drops a template onto the OBSDisplay canvas
    // position is normalised [0..1, 0..1] relative to the canvas rect
    void dropTemplateRequested(const OverlayTemplate &tmpl, QPointF normalisedPos);

private slots:
    void onSearchChanged(const QString &text);
    void onCategoryToggled(int categoryIndex, bool checked);
    void onTemplateDoubleClicked(const OverlayTemplate &tmpl);
    void refreshList();

private:
    void buildTemplateLibrary();
    void buildUi();
    void applyTheme();
    QWidget *buildCategoryBar();

    // ── Data ──────────────────────────────────────────────────────────────────
    QVector<OverlayTemplate> m_templates;
    QString                  m_searchQuery;
    QString                  m_activeCategory; // "" = all

    // ── Widgets ───────────────────────────────────────────────────────────────
    QLineEdit         *m_searchEdit   = nullptr;
    OverlayListWidget *m_listWidget   = nullptr;
    QButtonGroup      *m_catGroup     = nullptr;
    QWidget           *m_categoryBar  = nullptr;

    // ── Colour constants (Chromatic Command palette) ──────────────────────────
    static const QColor kBgDeep;
    static const QColor kBgPanel;
    static const QColor kBgElevated;
    static const QColor kBgInput;
    static const QColor kBorder;
    static const QColor kTextPrimary;
    static const QColor kTextMuted;
    static const QColor kAccentOrange;
    static const QColor kAccentBlue;
    static const QColor kAccentCyan;
    static const QColor kAccentAmber;
    static const QColor kAccentViolet;
};

} // namespace RailShot
