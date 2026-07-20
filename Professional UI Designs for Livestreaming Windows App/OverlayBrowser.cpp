// RailShotTV — OverlayBrowser.cpp
// Chromatic Command: dark navy/slate surfaces, vivid accent colours.
// Overlay template library panel for Scene Editor.

#include "OverlayBrowser.h"

#include <QApplication>
#include <QFrame>
#include <QSizePolicy>
#include <QToolTip>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFont>
#include <QPainterPath>
#include <QStyleOption>
#include <QScrollBar>
#include <algorithm>

namespace RailShot {

// ── Colour constants ──────────────────────────────────────────────────────────
const QColor OverlayBrowser::kBgDeep      { 0x06, 0x06, 0x08 };
const QColor OverlayBrowser::kBgPanel     { 0x14, 0x19, 0x28 };
const QColor OverlayBrowser::kBgElevated  { 0x1A, 0x20, 0x35 };
const QColor OverlayBrowser::kBgInput     { 0x1E, 0x26, 0x40 };
const QColor OverlayBrowser::kBorder      { 0x2A, 0x33, 0x50 };
const QColor OverlayBrowser::kTextPrimary { 0xF8, 0xF8, 0xFF };
const QColor OverlayBrowser::kTextMuted   { 0x88, 0x92, 0xA4 };
const QColor OverlayBrowser::kAccentOrange{ 0xFF, 0x5A, 0x2C };
const QColor OverlayBrowser::kAccentBlue  { 0x4F, 0x9E, 0xFF };
const QColor OverlayBrowser::kAccentCyan  { 0x22, 0xD3, 0xEE };
const QColor OverlayBrowser::kAccentAmber { 0xFB, 0xBF, 0x24 };
const QColor OverlayBrowser::kAccentViolet{ 0xA8, 0x55, 0xF7 };

// ─────────────────────────────────────────────────────────────────────────────
// OverlayThumbnailItem
// ─────────────────────────────────────────────────────────────────────────────

OverlayThumbnailItem::OverlayThumbnailItem(const OverlayTemplate &tmpl,
                                           QListWidget *parent)
    : QListWidgetItem(parent)
    , m_template(tmpl)
{
    setText(tmpl.name);
    setToolTip(tmpl.description);
    setIcon(QIcon(renderThumbnail(tmpl)));
    setData(Qt::UserRole, tmpl.id);
}

QPixmap OverlayThumbnailItem::renderThumbnail(const OverlayTemplate &tmpl,
                                               int w, int h)
{
    QPixmap pix(w, h);
    pix.fill(QColor(0x0D, 0x12, 0x20));
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    const QColor &ac = tmpl.accentColor;
    const QString &cat = tmpl.category;

    // ── Draw category-specific preview ────────────────────────────────────────
    if (cat == "scoreboard") {
        // Lower bar with team scores
        QRect bar(0, h * 2 / 3, w, h / 3);
        p.fillRect(bar, QColor(0x1A, 0x20, 0x35, 230));
        p.fillRect(QRect(0, h * 2 / 3, 4, h / 3), ac);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(QRect(8, h * 2 / 3 + 4, w / 2 - 8, 14), Qt::AlignLeft | Qt::AlignVCenter, "TEAM A");
        p.drawText(QRect(w / 2 + 4, h * 2 / 3 + 4, w / 2 - 12, 14), Qt::AlignRight | Qt::AlignVCenter, "TEAM B");
        p.setPen(ac);
        p.setFont(QFont("Arial", 10, QFont::Bold));
        p.drawText(QRect(w / 2 - 20, h * 2 / 3 + 2, 40, h / 3 - 2), Qt::AlignCenter, "3–1");
    } else if (cat == "lowerthird") {
        // Left-anchored lower third bar
        QRect bar(0, h - 28, w * 3 / 4, 28);
        p.fillRect(bar, QColor(0x1A, 0x20, 0x35, 230));
        p.fillRect(QRect(0, h - 28, 4, 28), ac);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(QRect(10, h - 26, bar.width() - 14, 14), Qt::AlignLeft | Qt::AlignVCenter, "PLAYER NAME");
        p.setPen(ac);
        p.setFont(QFont("Arial", 6));
        p.drawText(QRect(10, h - 13, bar.width() - 14, 12), Qt::AlignLeft | Qt::AlignVCenter, "Title / Role");
    } else if (cat == "ticker") {
        // Bottom ticker strip
        QRect strip(0, h - 14, w, 14);
        p.fillRect(strip, QColor(0x1A, 0x20, 0x35, 240));
        p.fillRect(QRect(0, h - 14, 36, 14), ac);
        p.setPen(QColor(0x1A, 0x20, 0x35));
        p.setFont(QFont("Arial", 6, QFont::Bold));
        p.drawText(QRect(2, h - 14, 34, 14), Qt::AlignCenter, "LIVE");
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 6));
        p.drawText(QRect(40, h - 14, w - 44, 14), Qt::AlignLeft | Qt::AlignVCenter,
                   "Latest updates scroll here continuously...");
    } else if (cat == "alert") {
        // Centred alert card
        QRect card(w / 4, h / 4, w / 2, h / 2);
        p.fillRect(card, QColor(0x1A, 0x20, 0x35, 230));
        p.fillRect(QRect(card.x(), card.y(), card.width(), 3), ac);
        p.setPen(ac);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(card.adjusted(4, 6, -4, -card.height() / 2), Qt::AlignCenter, "ALERT");
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 6));
        p.drawText(card.adjusted(4, card.height() / 2, -4, -4), Qt::AlignCenter, "username");
    } else if (cat == "branding") {
        // Top sponsor bar
        QRect bar(0, 0, w, 16);
        p.fillRect(bar, QColor(0x1A, 0x20, 0x35, 220));
        p.setPen(kTextMuted);
        p.setFont(QFont("Arial", 5));
        p.drawText(QRect(4, 0, 50, 16), Qt::AlignLeft | Qt::AlignVCenter, "SPONSORS");
        // Placeholder logo boxes
        for (int i = 0; i < 3; ++i) {
            QRect logo(44 + i * 30, 3, 26, 10);
            p.fillRect(logo, QColor(0x30, 0x3D, 0x5A));
            p.setPen(kTextMuted);
            p.setFont(QFont("Arial", 4));
            p.drawText(logo, Qt::AlignCenter, "LOGO");
        }
        // Watermark dot in corner
        p.setPen(Qt::NoPen);
        p.setBrush(ac);
        p.drawEllipse(QPoint(w - 8, h - 8), 5, 5);
    }

    // Category dot in bottom-left
    p.setPen(Qt::NoPen);
    p.setBrush(ac);
    p.drawEllipse(QPoint(5, h - 5), 3, 3);

    p.end();
    return pix;
}

// ─────────────────────────────────────────────────────────────────────────────
// OverlayListWidget
// ─────────────────────────────────────────────────────────────────────────────

OverlayListWidget::OverlayListWidget(QWidget *parent)
    : QListWidget(parent)
{
    setViewMode(QListView::IconMode);
    setIconSize(QSize(120, 68));
    setGridSize(QSize(136, 100));
    setResizeMode(QListView::Adjust);
    setMovement(QListView::Static);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setDragEnabled(true);
    setAcceptDrops(false);
    setDropIndicatorShown(false);
    setSpacing(4);
    setUniformItemSizes(true);
    setWordWrap(true);
    setTextElideMode(Qt::ElideRight);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
}

void OverlayListWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        m_dragStartPos = event->pos();
    QListWidget::mousePressEvent(event);
}

void OverlayListWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton))
        return;
    if ((event->pos() - m_dragStartPos).manhattanLength() < QApplication::startDragDistance())
        return;

    QListWidgetItem *item = itemAt(m_dragStartPos);
    if (!item) return;

    auto *tmplItem = dynamic_cast<OverlayThumbnailItem *>(item);
    if (!tmplItem) return;

    startDrag(tmplItem->overlayTemplate());
}

void OverlayListWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    QListWidgetItem *item = itemAt(event->pos());
    if (!item) return;
    auto *tmplItem = dynamic_cast<OverlayThumbnailItem *>(item);
    if (tmplItem)
        emit templateDoubleClicked(tmplItem->overlayTemplate());
}

void OverlayListWidget::startDrag(const OverlayTemplate &tmpl)
{
    emit templateDragStarted(tmpl);

    auto *drag = new QDrag(this);
    auto *mime = new QMimeData;

    // Encode template id into MIME payload
    mime->setData(OverlayBrowser::kMimeType, tmpl.id.toUtf8());
    mime->setText(tmpl.id);
    drag->setMimeData(mime);

    // Drag pixmap: thumbnail at 50% opacity
    QPixmap pix = OverlayThumbnailItem::renderThumbnail(tmpl, 120, 68);
    QPixmap dragPix(pix.size());
    dragPix.fill(Qt::transparent);
    QPainter p(&dragPix);
    p.setOpacity(0.75);
    p.drawPixmap(0, 0, pix);
    p.end();
    drag->setPixmap(dragPix);
    drag->setHotSpot(QPoint(dragPix.width() / 2, dragPix.height() / 2));

    drag->exec(Qt::CopyAction, Qt::CopyAction);
}

// ─────────────────────────────────────────────────────────────────────────────
// OverlayBrowser
// ─────────────────────────────────────────────────────────────────────────────

OverlayBrowser::OverlayBrowser(QWidget *parent)
    : QDockWidget(tr("Overlay Library"), parent)
{
    setObjectName("OverlayBrowser");
    setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    setMinimumWidth(220);
    setMaximumWidth(320);

    buildTemplateLibrary();
    buildUi();
    applyTheme();
}

// ── Template library ──────────────────────────────────────────────────────────
void OverlayBrowser::buildTemplateLibrary()
{
    // Helper lambda
    auto add = [&](const QString &id, const QString &cat, const QString &name,
                   const QString &obsType, const QColor &color,
                   const QString &desc, const QString &json = "{}") {
        m_templates.push_back({ id, cat, name, obsType, color, desc, json });
    };

    // ── Scoreboard ────────────────────────────────────────────────────────────
    add("sb-lower",  "scoreboard", "Lower Third Score",
        "browser_source", kAccentOrange,
        "Compact score bar anchored to the lower-left of the frame.",
        R"({"url":"about:blank","width":1920,"height":1080,"css":"body{margin:0}"})");

    add("sb-center", "scoreboard", "Center Banner Score",
        "browser_source", kAccentOrange,
        "Centred score banner with team names and match clock.",
        R"({"url":"about:blank","width":1920,"height":1080,"css":"body{margin:0}"})");

    add("sb-corner", "scoreboard", "Corner Compact",
        "browser_source", kAccentOrange,
        "Minimal corner score bug — always visible, low footprint.",
        R"({"url":"about:blank","width":1920,"height":1080,"css":"body{margin:0}"})");

    add("sb-full",   "scoreboard", "Full Width Score",
        "browser_source", kAccentOrange,
        "Full-width top-bar scoreboard for multi-sport tickers.",
        R"({"url":"about:blank","width":1920,"height":1080,"css":"body{margin:0}"})");

    // ── Lower Thirds ──────────────────────────────────────────────────────────
    add("lt-player", "lowerthird", "Player Name / Title",
        "text_gdiplus", kAccentBlue,
        "Classic lower-third name card with title line.",
        R"({"text":"Player Name","font":{"face":"DM Sans","size":36,"flags":1},"color":4294967295})");

    add("lt-commentator", "lowerthird", "Commentator ID",
        "text_gdiplus", kAccentBlue,
        "Commentator identification card with accent background.",
        R"({"text":"Commentator","font":{"face":"DM Sans","size":32,"flags":1},"color":4294967295})");

    add("lt-sponsor", "lowerthird", "Sponsor Mention",
        "browser_source", kAccentBlue,
        "Animated sponsor mention with logo placeholder.",
        R"({"url":"about:blank","width":1920,"height":1080,"css":"body{margin:0}"})");

    add("lt-intro", "lowerthird", "Event Intro",
        "browser_source", kAccentBlue,
        "Full event introduction card — title, venue, date.",
        R"({"url":"about:blank","width":1920,"height":1080,"css":"body{margin:0}"})");

    // ── Tickers ───────────────────────────────────────────────────────────────
    add("tk-news",   "ticker", "Breaking News Ticker",
        "browser_source", kAccentCyan,
        "Scrolling news ticker with BREAKING tag.",
        R"({"url":"about:blank","width":1920,"height":1080,"css":"body{margin:0}"})");

    add("tk-score",  "ticker", "Score Ticker",
        "browser_source", kAccentCyan,
        "Multi-match score ticker — shows live results across events.",
        R"({"url":"about:blank","width":1920,"height":1080,"css":"body{margin:0}"})");

    add("tk-social", "ticker", "Social Feed Ticker",
        "browser_source", kAccentCyan,
        "Live chat/social messages scrolling at the bottom.",
        R"({"url":"about:blank","width":1920,"height":1080,"css":"body{margin:0}"})");

    // ── Alerts ────────────────────────────────────────────────────────────────
    add("al-follow",   "alert", "Follow Alert",
        "browser_source", kAccentAmber,
        "Animated pop-up when a new viewer follows the channel.",
        R"({"url":"about:blank","width":1920,"height":1080,"css":"body{margin:0}"})");

    add("al-donation", "alert", "Donation Alert",
        "browser_source", kAccentAmber,
        "Donation notification with amount and donor name.",
        R"({"url":"about:blank","width":1920,"height":1080,"css":"body{margin:0}"})");

    add("al-sub",      "alert", "Subscription Alert",
        "browser_source", kAccentAmber,
        "New subscriber pop-up with tier badge.",
        R"({"url":"about:blank","width":1920,"height":1080,"css":"body{margin:0}"})");

    // ── Branding ──────────────────────────────────────────────────────────────
    add("br-sponsor",  "branding", "Sponsor Logo Bar",
        "image_source", kAccentViolet,
        "Top bar with up to 4 sponsor logo placeholders.",
        R"({"file":""})");

    add("br-watermark","branding", "Watermark / Bug",
        "image_source", kAccentViolet,
        "Corner channel watermark / station bug.",
        R"({"file":""})");
}

// ── UI construction ───────────────────────────────────────────────────────────
void OverlayBrowser::buildUi()
{
    auto *container = new QWidget(this);
    container->setObjectName("OverlayBrowserContainer");
    auto *rootLayout = new QVBoxLayout(container);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // ── Search bar ────────────────────────────────────────────────────────────
    auto *searchRow = new QWidget;
    searchRow->setObjectName("SearchRow");
    auto *searchLayout = new QHBoxLayout(searchRow);
    searchLayout->setContentsMargins(8, 6, 8, 6);
    searchLayout->setSpacing(6);

    m_searchEdit = new QLineEdit;
    m_searchEdit->setObjectName("OverlaySearch");
    m_searchEdit->setPlaceholderText("Search overlays…");
    m_searchEdit->setClearButtonEnabled(true);
    searchLayout->addWidget(m_searchEdit);
    rootLayout->addWidget(searchRow);

    // ── Category bar ──────────────────────────────────────────────────────────
    m_categoryBar = buildCategoryBar();
    rootLayout->addWidget(m_categoryBar);

    // ── Separator ─────────────────────────────────────────────────────────────
    auto *sep = new QFrame;
    sep->setFrameShape(QFrame::HLine);
    sep->setObjectName("BrowserSep");
    rootLayout->addWidget(sep);

    // ── Template list ─────────────────────────────────────────────────────────
    m_listWidget = new OverlayListWidget;
    m_listWidget->setObjectName("OverlayList");
    rootLayout->addWidget(m_listWidget, 1);

    // ── Help label ────────────────────────────────────────────────────────────
    auto *helpLabel = new QLabel("Drag to canvas · Double-click to add");
    helpLabel->setObjectName("OverlayHelp");
    helpLabel->setAlignment(Qt::AlignCenter);
    helpLabel->setWordWrap(true);
    rootLayout->addWidget(helpLabel);

    setWidget(container);

    // ── Connections ───────────────────────────────────────────────────────────
    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &OverlayBrowser::onSearchChanged);

    connect(m_listWidget, &OverlayListWidget::templateDoubleClicked,
            this, &OverlayBrowser::onTemplateDoubleClicked);

    // Initial population
    refreshList();
}

QWidget *OverlayBrowser::buildCategoryBar()
{
    struct CatDef { QString id; QString label; QColor color; };
    const QVector<CatDef> cats = {
        { "",            "All",          kTextMuted    },
        { "scoreboard",  "Scoreboard",   kAccentOrange },
        { "lowerthird",  "Lower Thirds", kAccentBlue   },
        { "ticker",      "Tickers",      kAccentCyan   },
        { "alert",       "Alerts",       kAccentAmber  },
        { "branding",    "Branding",     kAccentViolet },
    };

    auto *bar = new QWidget;
    bar->setObjectName("CategoryBar");
    auto *layout = new QHBoxLayout(bar);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(4);

    m_catGroup = new QButtonGroup(this);
    m_catGroup->setExclusive(true);

    for (int i = 0; i < cats.size(); ++i) {
        const auto &cat = cats[i];
        auto *btn = new QPushButton(cat.label);
        btn->setCheckable(true);
        btn->setChecked(i == 0);
        btn->setObjectName("CatBtn_" + cat.id);
        btn->setProperty("catId", cat.id);
        btn->setProperty("catColor", cat.color.name());
        btn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        btn->setFixedHeight(22);
        m_catGroup->addButton(btn, i);
        layout->addWidget(btn);
    }
    layout->addStretch();

    connect(m_catGroup, &QButtonGroup::idToggled,
            this, &OverlayBrowser::onCategoryToggled);

    return bar;
}

// ── Theme / QSS ───────────────────────────────────────────────────────────────
void OverlayBrowser::applyTheme()
{
    // Dock title bar
    setStyleSheet(R"(
        QDockWidget {
            color: #F8F8FF;
            font-family: 'DM Sans', sans-serif;
            font-size: 11px;
            font-weight: 700;
            letter-spacing: 0.06em;
            text-transform: uppercase;
        }
        QDockWidget::title {
            background: #1A2035;
            border-bottom: 1px solid #2A3350;
            padding: 6px 8px;
        }
        QDockWidget::close-button, QDockWidget::float-button {
            background: #1E2640;
            border: 1px solid #303D5A;
            border-radius: 3px;
        }
        QDockWidget::close-button:hover, QDockWidget::float-button:hover {
            background: #303D5A;
        }

        #OverlayBrowserContainer {
            background: #141928;
        }

        #SearchRow {
            background: #1A2035;
            border-bottom: 1px solid #1E2640;
        }

        #OverlaySearch {
            background: #1E2640;
            border: 1px solid #303D5A;
            border-radius: 4px;
            color: #A0A0B8;
            font-family: 'DM Sans', sans-serif;
            font-size: 11px;
            padding: 3px 8px;
            height: 24px;
        }
        #OverlaySearch:focus {
            border-color: #4F9EFF60;
            color: #F8F8FF;
        }

        #CategoryBar {
            background: #1A2035;
            border-bottom: 1px solid #1E2640;
        }

        QPushButton[objectName^="CatBtn_"] {
            background: #1E2640;
            border: 1px solid #303D5A;
            border-radius: 11px;
            color: #606078;
            font-family: 'DM Sans', sans-serif;
            font-size: 10px;
            font-weight: 600;
            padding: 0 8px;
        }
        QPushButton[objectName^="CatBtn_"]:checked {
            background: rgba(79, 158, 255, 0.13);
            border-color: rgba(79, 158, 255, 0.38);
            color: #4F9EFF;
        }
        QPushButton[objectName^="CatBtn_"]:hover:!checked {
            background: #303D5A;
            color: #A0A0B8;
        }

        #BrowserSep {
            background: #1E2640;
            max-height: 1px;
        }

        #OverlayList {
            background: #141928;
            border: none;
            padding: 4px;
        }
        #OverlayList::item {
            background: #1E2640;
            border: 1px solid #303D5A;
            border-radius: 4px;
            color: #A0A0B8;
            font-family: 'DM Sans', sans-serif;
            font-size: 9px;
            padding: 2px;
        }
        #OverlayList::item:selected {
            background: rgba(79, 158, 255, 0.15);
            border-color: rgba(79, 158, 255, 0.50);
            color: #F8F8FF;
        }
        #OverlayList::item:hover:!selected {
            background: #303D5A;
            border-color: #4A5570;
        }

        QScrollBar:vertical {
            background: #141928;
            width: 6px;
            margin: 0;
        }
        QScrollBar::handle:vertical {
            background: #303D5A;
            border-radius: 3px;
            min-height: 20px;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }

        #OverlayHelp {
            background: #0D1220;
            border-top: 1px solid #1E2640;
            color: #50506A;
            font-family: 'DM Sans', sans-serif;
            font-size: 9px;
            padding: 5px 8px;
        }
    )");
}

// ── Slots ─────────────────────────────────────────────────────────────────────
void OverlayBrowser::onSearchChanged(const QString &text)
{
    m_searchQuery = text.trimmed();
    refreshList();
}

void OverlayBrowser::onCategoryToggled(int categoryIndex, bool checked)
{
    if (!checked) return;
    // Map button index → category id
    const QStringList cats = { "", "scoreboard", "lowerthird", "ticker", "alert", "branding" };
    if (categoryIndex >= 0 && categoryIndex < cats.size())
        m_activeCategory = cats[categoryIndex];
    refreshList();
}

void OverlayBrowser::onTemplateDoubleClicked(const OverlayTemplate &tmpl)
{
    emit addTemplateRequested(tmpl);
}

void OverlayBrowser::refreshList()
{
    m_listWidget->clear();

    for (const auto &tmpl : m_templates) {
        // Category filter
        if (!m_activeCategory.isEmpty() && tmpl.category != m_activeCategory)
            continue;
        // Search filter
        if (!m_searchQuery.isEmpty() &&
            !tmpl.name.contains(m_searchQuery, Qt::CaseInsensitive) &&
            !tmpl.description.contains(m_searchQuery, Qt::CaseInsensitive))
            continue;

        new OverlayThumbnailItem(tmpl, m_listWidget);
    }

    // Empty state
    if (m_listWidget->count() == 0) {
        auto *empty = new QListWidgetItem("No overlays found");
        empty->setFlags(Qt::NoItemFlags);
        empty->setForeground(QColor(0x50, 0x50, 0x6A));
        m_listWidget->addItem(empty);
    }
}

// ── Lookup ────────────────────────────────────────────────────────────────────
const OverlayTemplate *OverlayBrowser::findTemplate(const QString &id) const
{
    for (const auto &t : m_templates)
        if (t.id == id) return &t;
    return nullptr;
}

} // namespace RailShot
