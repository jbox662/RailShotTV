#include "ui/widgets/SidebarRail.h"
#include "ui/Theme.h"
#include <QLabel>
#include <QPainter>
#include <QPaintEvent>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QHBoxLayout>
#include <QTimer>

namespace railshot {

namespace {
class LogoBadge : public QWidget {
public:
    explicit LogoBadge(QWidget* parent = nullptr) : QWidget(parent) { setFixedSize(30, 30); }
protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        QRadialGradient glow(15, 15, 20);
        glow.setColorAt(0, QColor(255, 90, 44, 160));
        glow.setColorAt(1, QColor(255, 90, 44, 0));
        p.setPen(Qt::NoPen);
        p.setBrush(glow);
        p.drawEllipse(QRectF(-6, -6, 42, 42));

        QLinearGradient g(0, 0, 30, 30);
        g.setColorAt(0, QColor(QStringLiteral("#FF5A2C")));
        g.setColorAt(1, QColor(QStringLiteral("#FF8C42")));
        p.setBrush(g);
        p.drawEllipse(0, 0, 30, 30);

        p.setPen(QPen(QColor(255, 255, 255, 230), 1.5, Qt::SolidLine, Qt::RoundCap));
        p.setBrush(QColor(255, 255, 255, 230));
        p.drawEllipse(QPointF(15, 15), 3, 3);
        p.drawLine(QPointF(1, 15), QPointF(6, 15));
        p.drawLine(QPointF(10, 15), QPointF(15, 15));
        p.drawLine(QPointF(1 + 9, 15), QPointF(6 + 9, 15));
        p.drawLine(QPointF(15, 1), QPointF(15, 6));
        p.drawLine(QPointF(15, 10), QPointF(15, 15));
    }
};

class SignalBars : public QWidget {
public:
    explicit SignalBars(QWidget* parent = nullptr) : QWidget(parent)
    {
        setFixedSize(22, 14);
    }
protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(Qt::NoPen);
        const int heights[] = {3, 5, 7, 9, 11};
        for (int i = 0; i < 5; ++i) {
            const int h = heights[i];
            p.setBrush(i < 4 ? QColor(QStringLiteral("#22C55E")) : QColor(QStringLiteral("#2D3748")));
            p.drawRoundedRect(i * 4, 14 - h, 3, h, 1.5, 1.5);
        }
    }
};
} // namespace

SidebarRail::SidebarRail(QWidget* parent)
    : QWidget(parent)
{
    setFixedWidth(56);
    setObjectName(QStringLiteral("sidebarRail"));
    setStyleSheet(QStringLiteral(
        "QWidget#sidebarRail {"
        "  background: #0F1114;"
        "  border-right: 1px solid rgba(255,255,255,0.08);"
        "}"));

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 12);
    layout->setSpacing(0);

    auto* logoCell = new QWidget(this);
    logoCell->setFixedSize(56, 46);
    logoCell->setStyleSheet(QStringLiteral("border-bottom: 1px solid rgba(255,255,255,0.08); background: transparent;"));
    auto* logoLay = new QHBoxLayout(logoCell);
    logoLay->setContentsMargins(0, 0, 0, 0);
    logoLay->addWidget(new LogoBadge(logoCell), 0, Qt::AlignCenter);
    layout->addWidget(logoCell);
    layout->addSpacing(8);

    m_liveBlock = new QWidget(this);
    m_liveBlock->setFixedWidth(56);
    auto* liveLay = new QVBoxLayout(m_liveBlock);
    liveLay->setContentsMargins(0, 0, 0, 8);
    liveLay->setSpacing(3);
    liveLay->setAlignment(Qt::AlignHCenter);
    auto* liveDot = new QLabel(m_liveBlock);
    liveDot->setObjectName(QStringLiteral("sidebarLiveDot"));
    liveDot->setFixedSize(10, 10);
    liveDot->setStyleSheet(QStringLiteral(
        "background: #FF5A2C; border-radius: 5px; border: 2px solid #FF3A0C;"));
    liveLay->addWidget(liveDot, 0, Qt::AlignHCenter);
    auto* liveText = new QLabel(QStringLiteral("LIVE"), m_liveBlock);
    liveText->setAlignment(Qt::AlignCenter);
    liveText->setStyleSheet(QStringLiteral(
        "color: #FF5A2C; font-family: 'DM Sans'; font-size: 7px; font-weight: 800; letter-spacing: 1px;"));
    liveLay->addWidget(liveText);
    m_liveBlock->setStyleSheet(QStringLiteral("border-bottom: 1px solid rgba(255,90,44,0.2);"));
    m_liveBlock->setVisible(false);
    layout->addWidget(m_liveBlock);

    auto* navWrap = new QWidget(this);
    auto* navLay = new QVBoxLayout(navWrap);
    navLay->setContentsMargins(8, 4, 8, 0);
    navLay->setSpacing(4);
    layout->addWidget(navWrap, 1);

    auto add = [&](const QString& id, const QString& tip, const QColor& c) {
        m_colors.insert(id, c);
        auto* btn = addNav(id, tip, c);
        navLay->addWidget(btn, 0, Qt::AlignHCenter);
    };
    add(QStringLiteral("dashboard"), QStringLiteral("Dashboard"), QColor(theme::kBrand));
    add(QStringLiteral("chat"), QStringLiteral("Chat"), QColor(theme::kViolet));
    add(QStringLiteral("analytics"), QStringLiteral("Analytics"), QColor(theme::kCyan));
    add(QStringLiteral("scoreboard"), QStringLiteral("Scoreboard"), QColor(theme::kEmerald));
    add(QStringLiteral("schedule"), QStringLiteral("Schedule"), QColor(QStringLiteral("#FACC15")));
    add(QStringLiteral("settings"), QStringLiteral("Settings"), QColor(QStringLiteral("#94A3B8")));
    navLay->addStretch();

    auto* footer = new QWidget(this);
    auto* footLay = new QVBoxLayout(footer);
    footLay->setContentsMargins(0, 0, 0, 4);
    footLay->setSpacing(2);
    footLay->setAlignment(Qt::AlignHCenter);
    footLay->addWidget(new SignalBars(footer), 0, Qt::AlignHCenter);
    m_version = new QLabel(QStringLiteral("v2.5"), footer);
    m_version->setAlignment(Qt::AlignCenter);
    m_version->setStyleSheet(QStringLiteral(
        "color: #4B5563; font-family: 'JetBrains Mono'; font-size: 8px; letter-spacing: 0.5px;"));
    footLay->addWidget(m_version);
    layout->addWidget(footer);

    restyleNav();
}

void SidebarRail::setActivePage(const QString& pageId)
{
    if (m_active == pageId) return;
    m_active = pageId;
    restyleNav();
}

void SidebarRail::setLive(bool live)
{
    m_live = live;
    if (m_liveBlock) m_liveBlock->setVisible(live);
    if (live) {
        if (!findChild<QTimer*>(QStringLiteral("livePulseTimer"))) {
            auto* t = new QTimer(this);
            t->setObjectName(QStringLiteral("livePulseTimer"));
            connect(t, &QTimer::timeout, this, [this] {
                if (auto* dot = m_liveBlock ? m_liveBlock->findChild<QLabel*>() : nullptr) {
                    static bool bright = false;
                    bright = !bright;
                    dot->setStyleSheet(bright
                        ? QStringLiteral("background:#FF8C42;border-radius:5px;border:2px solid #FF5A2C;")
                        : QStringLiteral("background:#FF5A2C;border-radius:5px;border:2px solid #FF3A0C;"));
                }
            });
            t->start(700);
        }
    } else if (auto* t = findChild<QTimer*>(QStringLiteral("livePulseTimer"))) {
        t->stop();
        t->deleteLater();
    }
}

QPushButton* SidebarRail::addNav(const QString& id, const QString& tip, const QColor& /*color*/)
{
    auto* btn = new QPushButton(this);
    btn->setFixedSize(40, 40);
    btn->setToolTip(tip);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setFocusPolicy(Qt::NoFocus);

    QString glyph = QStringLiteral("•");
    if (id == QLatin1String("dashboard")) glyph = QStringLiteral("▣");
    else if (id == QLatin1String("chat")) glyph = QStringLiteral("◎");
    else if (id == QLatin1String("analytics")) glyph = QStringLiteral("▥");
    else if (id == QLatin1String("scoreboard")) glyph = QStringLiteral("◆");
    else if (id == QLatin1String("schedule")) glyph = QStringLiteral("☰");
    else if (id == QLatin1String("settings")) glyph = QStringLiteral("⚙");
    btn->setText(glyph);

    m_buttons.insert(id, btn);
    connect(btn, &QPushButton::clicked, this, [this, id] {
        setActivePage(id);
        emit navigate(id);
    });
    return btn;
}

void SidebarRail::restyleNav()
{
    for (auto it = m_buttons.begin(); it != m_buttons.end(); ++it) {
        const QColor color = m_colors.value(it.key(), QColor(QStringLiteral("#6B7280")));
        const bool active = (it.key() == m_active);
        const QString fill = active
                                 ? QStringLiteral("rgba(%1,%2,%3,0.13)").arg(color.red()).arg(color.green()).arg(color.blue())
                                 : QStringLiteral("transparent");
        const QString border = active
                                   ? QStringLiteral("rgba(%1,%2,%3,0.33)").arg(color.red()).arg(color.green()).arg(color.blue())
                                   : QStringLiteral("transparent");
        const QString text = active ? color.name() : QStringLiteral("#6B7280");
        const QString glow = active
                                 ? QStringLiteral("0 0 16px rgba(%1,%2,%3,0.65)")
                                       .arg(color.red()).arg(color.green()).arg(color.blue())
                                 : QStringLiteral("none");
        it.value()->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  background:%1; border:1px solid %2; border-radius:8px; color:%3;"
            "  font-weight:700; font-size:14px;"
            "}"
            "QPushButton:hover {"
            "  background:rgba(255,255,255,0.06); border:1px solid rgba(255,255,255,0.1); color:%3;"
            "}")
                                      .arg(fill, border, text));
        Q_UNUSED(glow); // Qt stylesheets don't support box-shadow; glow approximated via border tint
    }
}

void SidebarRail::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
}

} // namespace railshot
