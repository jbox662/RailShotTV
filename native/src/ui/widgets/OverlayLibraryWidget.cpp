#include "ui/widgets/OverlayLibraryWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QButtonGroup>
#include <QIcon>
#include <QListView>

namespace railshot {

namespace {
QVector<OverlayTemplateInfo> catalog()
{
    QVector<OverlayTemplateInfo> out;
    auto add = [&](const char* id, const char* cat, const char* name, const char* desc,
                   const char* accent, SourceType type, const char* html = "") {
        OverlayTemplateInfo t;
        t.id = QString::fromLatin1(id);
        t.category = QString::fromLatin1(cat);
        t.name = QString::fromLatin1(name);
        t.description = QString::fromLatin1(desc);
        t.accent = QString::fromLatin1(accent);
        t.sourceType = type;
        t.htmlResource = QString::fromLatin1(html);
        out.append(t);
    };
    add("sb-lower", "scoreboard", "Lower Third Score", "Score strip along bottom", "#FF5A2C", SourceType::Scoreboard);
    add("sb-center", "scoreboard", "Center Banner Score", "Centered match banner", "#FF5A2C", SourceType::Scoreboard);
    add("sb-corner", "scoreboard", "Corner Compact", "Compact corner scorebug", "#FF5A2C", SourceType::Scoreboard);
    add("sb-full", "scoreboard", "Full Width Score", "Full-width scoreboard", "#FF5A2C", SourceType::Scoreboard);
    add("lt-player", "lowerthird", "Player Lower Third", "Name + role plate", "#4F9EFF", SourceType::LowerThird, "player-intro.html");
    add("lt-commentator", "lowerthird", "Commentator LT", "Desk talent plate", "#4F9EFF", SourceType::LowerThird);
    add("lt-sponsor", "lowerthird", "Sponsor LT", "Sponsor mention plate", "#4F9EFF", SourceType::LowerThird);
    add("tk-score", "ticker", "Score Ticker", "Scrolling scores", "#22D3EE", SourceType::Browser, "match-point.html");
    add("tk-news", "ticker", "News Ticker", "Breaking news strip", "#22D3EE", SourceType::Browser);
    add("al-sub", "alert", "Sub Alert", "Subscription celebration", "#FBBF24", SourceType::Alert);
    add("al-donate", "alert", "Donation Alert", "Donation flash", "#FBBF24", SourceType::Alert);
    add("al-follow", "alert", "Follow Alert", "New follower", "#FBBF24", SourceType::Alert);
    add("br-logo", "branding", "Logo Overlay", "Corner logo bug", "#A855F7", SourceType::Image);
    add("br-frame", "branding", "Camera Frame", "Decorative frame", "#A855F7", SourceType::Browser);
    add("br-sponsor", "branding", "Sponsor Bar", "Top sponsor rail", "#A855F7", SourceType::Browser);
    add("br-break", "branding", "Break and Run", "Billiards highlight", "#A855F7", SourceType::Browser, "break-and-run.html");
    add("br-intro", "branding", "Player Intro", "Player intro card", "#A855F7", SourceType::Browser, "player-intro.html");
    return out;
}

QPixmap thumbFor(const OverlayTemplateInfo& t, int w = 160, int h = 90)
{
    QPixmap pix(w, h);
    pix.fill(QColor(QStringLiteral("#0D1220")));
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    const QColor ac(t.accent);
    if (t.category == QLatin1String("scoreboard")) {
        p.fillRect(0, h * 2 / 3, w, h / 3, QColor(26, 32, 53, 230));
        p.fillRect(0, h * 2 / 3, 4, h / 3, ac);
        p.setPen(Qt::white);
        p.drawText(QRect(8, h * 2 / 3, w - 16, h / 3), Qt::AlignCenter, QStringLiteral("3 — 1"));
    } else if (t.category == QLatin1String("lowerthird")) {
        p.fillRect(0, h - 28, w * 3 / 4, 28, QColor(26, 32, 53, 230));
        p.fillRect(0, h - 28, 4, 28, ac);
    } else if (t.category == QLatin1String("ticker")) {
        p.fillRect(0, h - 14, w, 14, QColor(26, 32, 53, 240));
        p.fillRect(0, h - 14, 36, 14, ac);
    } else if (t.category == QLatin1String("alert")) {
        p.fillRect(w / 4, h / 4, w / 2, h / 2, QColor(26, 32, 53, 230));
        p.fillRect(w / 4, h / 4, w / 2, 3, ac);
    } else {
        p.fillRect(0, 0, w, 16, QColor(26, 32, 53, 220));
        p.setBrush(ac);
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPoint(w - 10, h - 10), 5, 5);
    }
    p.setPen(Qt::NoPen);
    p.setBrush(ac);
    p.drawEllipse(QPoint(6, h - 6), 3, 3);
    return pix;
}
} // namespace

OverlayLibraryWidget::OverlayLibraryWidget(QWidget* parent)
    : QWidget(parent)
{
    setFixedWidth(240);
    setStyleSheet(QStringLiteral("background:#141928; border-left:1px solid #2A3350;"));
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(6);

    auto* title = new QLabel(QStringLiteral("OVERLAY LIBRARY"), this);
    title->setStyleSheet(QStringLiteral("color:#8892A4; font-weight:800; font-size:10px; letter-spacing:1.5px;"));
    root->addWidget(title);

    auto* cats = new QHBoxLayout();
    auto* group = new QButtonGroup(this);
    group->setExclusive(true);
    struct Cat { const char* id; const char* label; const char* color; };
    const Cat catList[] = {
        {"all", "All", "#8892A4"},
        {"scoreboard", "SB", "#FF5A2C"},
        {"lowerthird", "LT", "#4F9EFF"},
        {"ticker", "TK", "#22D3EE"},
        {"alert", "AL", "#FBBF24"},
        {"branding", "BR", "#A855F7"},
    };
    for (const auto& c : catList) {
        auto* b = new QPushButton(QString::fromLatin1(c.label), this);
        b->setCheckable(true);
        b->setFixedHeight(22);
        b->setProperty("catId", QString::fromLatin1(c.id));
        b->setStyleSheet(QStringLiteral(
            "QPushButton{background:#1E2640;border:1px solid #2A3350;color:%1;font-size:9px;font-weight:700;padding:2px 4px;}"
            "QPushButton:checked{border-color:%1;}").arg(QString::fromLatin1(c.color)));
        if (QString::fromLatin1(c.id) == QLatin1String("all")) b->setChecked(true);
        group->addButton(b);
        cats->addWidget(b);
        connect(b, &QPushButton::clicked, this, [this, id = QString::fromLatin1(c.id)] {
            rebuild(id == QLatin1String("all") ? QString() : id);
        });
    }
    root->addLayout(cats);

    m_list = new QListWidget(this);
    m_list->setViewMode(QListView::IconMode);
    m_list->setIconSize(QSize(160, 90));
    m_list->setGridSize(QSize(176, 120));
    m_list->setResizeMode(QListView::Adjust);
    m_list->setMovement(QListView::Static);
    m_list->setStyleSheet(QStringLiteral(
        "QListWidget{background:#0D1220;border:1px solid #2A3350;}"
        "QListWidget::item{color:#C8CAD0;font-size:10px;}"));
    root->addWidget(m_list, 1);

    connect(m_list, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
        const auto all = catalog();
        const QString id = item->data(Qt::UserRole).toString();
        for (const auto& t : all) {
            if (t.id == id) {
                emit templateActivated(t);
                return;
            }
        }
    });

    rebuild({});
}

void OverlayLibraryWidget::rebuild(const QString& categoryFilter)
{
    m_filter = categoryFilter;
    m_list->clear();
    for (const auto& t : catalog()) {
        if (!m_filter.isEmpty() && t.category != m_filter) continue;
        auto* item = new QListWidgetItem(t.name, m_list);
        item->setData(Qt::UserRole, t.id);
        item->setToolTip(t.description);
        item->setIcon(QIcon(thumbFor(t)));
    }
}

} // namespace railshot
