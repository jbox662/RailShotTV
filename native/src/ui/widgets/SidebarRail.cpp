#include "ui/widgets/SidebarRail.h"
#include "ui/Theme.h"
#include "ui/Motion.h"
#include <QLabel>
#include <QPainter>
#include <QPaintEvent>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QHBoxLayout>
#include <QTimer>
#include <QVariant>
#include <QEnterEvent>

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

/** Lucide-style stroked icons (Tv2 / MessageSquare / BarChart2 / Trophy / Calendar / Settings). */
class NavIconButton : public QPushButton {
public:
    NavIconButton(const QString& id, QWidget* parent = nullptr)
        : QPushButton(parent), m_id(id)
    {
        setFixedSize(40, 40);
        setCursor(Qt::PointingHandCursor);
        setFocusPolicy(Qt::NoFocus);
        setText(QString());
        setFlat(true);
    }
    void setNavActive(bool on, const QColor& accent)
    {
        m_active = on;
        m_accent = accent;
        update();
    }

protected:
    void enterEvent(QEnterEvent* e) override { QPushButton::enterEvent(e); update(); }
    void leaveEvent(QEvent* e) override { QPushButton::leaveEvent(e); update(); }
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const QColor accent = m_accent.isValid() ? m_accent : QColor(QStringLiteral("#6B7280"));
        if (m_active) {
            QRadialGradient glow(rect().center(), 26);
            glow.setColorAt(0, QColor(accent.red(), accent.green(), accent.blue(), 160));
            glow.setColorAt(1, QColor(accent.red(), accent.green(), accent.blue(), 0));
            p.setPen(Qt::NoPen);
            p.setBrush(glow);
            p.drawEllipse(rect().adjusted(-4, -4, 4, 4));
            p.setBrush(QColor(accent.red(), accent.green(), accent.blue(), 70));
            p.setPen(QPen(QColor(accent.red(), accent.green(), accent.blue(), 220), 2));
            p.drawRoundedRect(rect().adjusted(1, 1, -2, -2), 8, 8);
        } else if (underMouse()) {
            p.setPen(QPen(QColor(255, 255, 255, 40), 1));
            p.setBrush(QColor(255, 255, 255, 22));
            p.drawRoundedRect(rect().adjusted(1, 1, -2, -2), 8, 8);
        }
        const QColor stroke = m_active ? accent : QColor(QStringLiteral("#6B7280"));
        p.setPen(QPen(stroke, m_active ? 2.1 : 1.7, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.setBrush(Qt::NoBrush);
        const QRectF ic = QRectF(11, 11, 18, 18);
        if (m_id == QLatin1String("dashboard")) {
            // Tv2
            p.drawRoundedRect(QRectF(ic.left(), ic.top() + 3, 18, 12), 2, 2);
            p.drawLine(QPointF(ic.center().x() - 3, ic.bottom() - 1), QPointF(ic.center().x() + 3, ic.bottom() - 1));
            p.drawLine(QPointF(ic.center().x(), ic.top() + 15), QPointF(ic.center().x(), ic.bottom() - 1));
        } else if (m_id == QLatin1String("chat")) {
            p.drawRoundedRect(QRectF(ic.left(), ic.top(), 16, 12), 2.5, 2.5);
            p.drawLine(QPointF(ic.left() + 3, ic.top() + 12), QPointF(ic.left() + 3, ic.bottom()));
            p.drawLine(QPointF(ic.left() + 3, ic.bottom()), QPointF(ic.left() + 8, ic.top() + 12));
        } else if (m_id == QLatin1String("analytics")) {
            p.drawLine(QPointF(ic.left() + 2, ic.bottom()), QPointF(ic.left() + 2, ic.top() + 8));
            p.drawLine(QPointF(ic.left() + 8, ic.bottom()), QPointF(ic.left() + 8, ic.top() + 3));
            p.drawLine(QPointF(ic.left() + 14, ic.bottom()), QPointF(ic.left() + 14, ic.top() + 6));
        } else if (m_id == QLatin1String("scoreboard")) {
            // Trophy cup
            p.drawLine(QPointF(ic.left() + 4, ic.top() + 2), QPointF(ic.right() - 4, ic.top() + 2));
            p.drawArc(QRectF(ic.left() + 3, ic.top() + 2, 12, 10), 0, -180 * 16);
            p.drawLine(QPointF(ic.center().x(), ic.top() + 12), QPointF(ic.center().x(), ic.bottom() - 3));
            p.drawLine(QPointF(ic.left() + 5, ic.bottom() - 2), QPointF(ic.right() - 5, ic.bottom() - 2));
        } else if (m_id == QLatin1String("schedule")) {
            p.drawRoundedRect(QRectF(ic.left() + 1, ic.top() + 3, 16, 14), 2, 2);
            p.drawLine(QPointF(ic.left() + 1, ic.top() + 7), QPointF(ic.right() - 1, ic.top() + 7));
            p.drawLine(QPointF(ic.left() + 5, ic.top() + 1), QPointF(ic.left() + 5, ic.top() + 5));
            p.drawLine(QPointF(ic.right() - 5, ic.top() + 1), QPointF(ic.right() - 5, ic.top() + 5));
        } else {
            // Settings gear (simplified)
            p.drawEllipse(QRectF(ic.center().x() - 3, ic.center().y() - 3, 6, 6));
            p.drawEllipse(QRectF(ic.center().x() - 7, ic.center().y() - 7, 14, 14));
        }
    }

private:
    QString m_id;
    bool m_active = false;
    QColor m_accent;
};
} // namespace

SidebarRail::SidebarRail(QWidget* parent)
    : QWidget(parent)
{
    setFixedWidth(56);
    setObjectName(QStringLiteral("sidebarRail"));
    setStyleSheet(QStringLiteral(
        "QWidget#sidebarRail {"
        "  background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #141619, stop:1 #0F1114);"
        "  border-right: 2px solid #FF5A2C55;"
        "}"));

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 12);
    layout->setSpacing(0);

    auto* logoCell = new QWidget(this);
    logoCell->setFixedSize(56, 46);
    logoCell->setStyleSheet(QStringLiteral(
        "border-bottom: 1px solid #FF5A2C40; background: qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #1A1210,stop:1 transparent);"));
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
    auto* liveDot = m_liveBlock ? m_liveBlock->findChild<QLabel*>(QStringLiteral("sidebarLiveDot")) : nullptr;
    if (live) {
        if (liveDot) {
            liveDot->setStyleSheet(QStringLiteral(
                "background:#FF5A2C;border-radius:5px;border:2px solid #FF3A0C;"));
            motion::pulseOpacity(liveDot, 1800);
        }
        if (m_liveBlock)
            motion::startLiveRipple(m_liveBlock);
    } else {
        if (liveDot)
            motion::stopPulse(liveDot);
        if (m_liveBlock)
            motion::stopLiveRipple(m_liveBlock);
        if (auto* t = findChild<QTimer*>(QStringLiteral("livePulseTimer"))) {
            t->stop();
            t->deleteLater();
        }
    }
}

QPushButton* SidebarRail::addNav(const QString& id, const QString& tip, const QColor& /*color*/)
{
    auto* btn = new NavIconButton(id, this);
    btn->setToolTip(tip);
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
        it.value()->setStyleSheet(QStringLiteral("QPushButton{background:transparent;border:none;}"));
        static_cast<NavIconButton*>(it.value())->setNavActive(active, color);
        it.value()->setProperty("navActive", active);
        it.value()->setProperty("navColor", color);
    }
    update();
}

void SidebarRail::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
    // Glow is painted inside NavIconButton for correct z-order under the icon strokes.
}

} // namespace railshot
