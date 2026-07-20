#include "ui/widgets/SidebarRail.h"
#include "ui/Theme.h"
#include <QLabel>
#include <QToolTip>

namespace railshot {

SidebarRail::SidebarRail(QWidget* parent)
    : QWidget(parent)
{
    setFixedWidth(56);
    setStyleSheet(QStringLiteral("background:#0F1114; border-right:1px solid rgba(255,255,255,0.08);"));
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    auto* logo = new QLabel(QStringLiteral("●"), this);
    logo->setAlignment(Qt::AlignCenter);
    logo->setStyleSheet(QStringLiteral("color:#FF5A2C; font-size:22px;"));
    layout->addWidget(logo);
    layout->addSpacing(8);

    addNav(QStringLiteral("dashboard"), QStringLiteral("Dashboard"), QColor(theme::kBrand));
    addNav(QStringLiteral("chat"), QStringLiteral("Chat"), QColor(theme::kViolet));
    addNav(QStringLiteral("analytics"), QStringLiteral("Analytics"), QColor(theme::kCyan));
    addNav(QStringLiteral("scoreboard"), QStringLiteral("Scoreboard"), QColor(theme::kEmerald));
    addNav(QStringLiteral("schedule"), QStringLiteral("Schedule"), QColor(theme::kAmber));
    addNav(QStringLiteral("settings"), QStringLiteral("Settings"), QColor(theme::kTextMuted));
    layout->addStretch();

    auto* ver = new QLabel(QStringLiteral("v0.1"), this);
    ver->setObjectName(QStringLiteral("mono"));
    ver->setAlignment(Qt::AlignCenter);
    layout->addWidget(ver);
}

QPushButton* SidebarRail::addNav(const QString& id, const QString& tip, const QColor& color)
{
    auto* btn = new QPushButton(this);
    btn->setFixedSize(40, 40);
    btn->setToolTip(tip);
    btn->setCursor(Qt::PointingHandCursor);
    const bool active = (id == m_active);
    btn->setStyleSheet(QStringLiteral(
        "QPushButton { background:%1; border:1px solid %2; border-radius:8px; color:%3; }"
        "QPushButton:hover { background:rgba(255,255,255,0.06); }")
        .arg(active ? color.name() + QStringLiteral("22") : QStringLiteral("transparent"),
             active ? color.name() + QStringLiteral("55") : QStringLiteral("transparent"),
             active ? color.name() : QStringLiteral("#6B7280")));
    btn->setText(tip.left(1));
    static_cast<QVBoxLayout*>(layout())->addWidget(btn, 0, Qt::AlignHCenter);
    connect(btn, &QPushButton::clicked, this, [this, id] { m_active = id; emit navigate(id); });
    return btn;
}

} // namespace railshot
