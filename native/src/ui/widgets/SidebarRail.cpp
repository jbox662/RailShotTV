#include "ui/widgets/SidebarRail.h"
#include "ui/Theme.h"
#include <QLabel>
#include <QPainter>
#include <QPaintEvent>
#include <QLinearGradient>
#include <QRadialGradient>

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

        p.setPen(QPen(Qt::white, 2.2, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(15, 8), QPointF(15, 22));
        p.drawLine(QPointF(8, 15), QPointF(22, 15));
        p.setPen(QPen(QColor(255, 255, 255, 100), 1.4, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(10, 20), QPointF(20, 10));
    }
};
} // namespace

SidebarRail::SidebarRail(QWidget* parent)
    : QWidget(parent)
{
    setFixedWidth(56);
    setStyleSheet(QStringLiteral(
        "background: #0F1114;"
        "border-right: 1px solid rgba(255,255,255,0.08);"));
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 12, 8, 8);
    layout->setSpacing(8);

    layout->addWidget(new LogoBadge(this), 0, Qt::AlignHCenter);
    layout->addSpacing(8);

    addNav(QStringLiteral("dashboard"), QStringLiteral("Dashboard"), QColor(theme::kBrand));
    addNav(QStringLiteral("chat"), QStringLiteral("Chat"), QColor(theme::kViolet));
    addNav(QStringLiteral("analytics"), QStringLiteral("Analytics"), QColor(theme::kCyan));
    addNav(QStringLiteral("scoreboard"), QStringLiteral("Scoreboard"), QColor(theme::kEmerald));
    addNav(QStringLiteral("schedule"), QStringLiteral("Schedule"), QColor(theme::kAmber));
    addNav(QStringLiteral("settings"), QStringLiteral("Settings"), QColor(QStringLiteral("#94A3B8")));
    layout->addStretch();

    auto* ver = new QLabel(QStringLiteral("v0.1"), this);
    ver->setAlignment(Qt::AlignCenter);
    ver->setStyleSheet(QStringLiteral("color:#3A3D45; font-size:9px;"));
    layout->addWidget(ver);
}

QPushButton* SidebarRail::addNav(const QString& id, const QString& tip, const QColor& color)
{
    auto* btn = new QPushButton(this);
    btn->setFixedSize(40, 40);
    btn->setToolTip(tip);
    btn->setCursor(Qt::PointingHandCursor);
    const bool active = (id == m_active);
    const QString fill = active
                             ? QStringLiteral("rgba(%1,%2,%3,0.16)").arg(color.red()).arg(color.green()).arg(color.blue())
                             : QStringLiteral("transparent");
    const QString border = active
                               ? QStringLiteral("rgba(%1,%2,%3,0.50)").arg(color.red()).arg(color.green()).arg(color.blue())
                               : QStringLiteral("transparent");
    const QString text = active ? color.name() : QStringLiteral("#6B7280");
    btn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background:%1; border:1px solid %2; border-radius:10px; color:%3;"
        "  font-weight:700; font-size:14px;"
        "}"
        "QPushButton:hover { background:rgba(255,255,255,0.06); color:%3; }")
                           .arg(fill, border, text));

    QString glyph = QStringLiteral("•");
    if (id == QLatin1String("dashboard")) glyph = QStringLiteral("▣");
    else if (id == QLatin1String("chat")) glyph = QStringLiteral("◎");
    else if (id == QLatin1String("analytics")) glyph = QStringLiteral("▥");
    else if (id == QLatin1String("scoreboard")) glyph = QStringLiteral("◆");
    else if (id == QLatin1String("schedule")) glyph = QStringLiteral("☰");
    else if (id == QLatin1String("settings")) glyph = QStringLiteral("⚙");
    btn->setText(glyph);
    static_cast<QVBoxLayout*>(layout())->addWidget(btn, 0, Qt::AlignHCenter);
    connect(btn, &QPushButton::clicked, this, [this, id] {
        m_active = id;
        emit navigate(id);
    });
    return btn;
}

} // namespace railshot
