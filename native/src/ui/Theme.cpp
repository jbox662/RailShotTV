#include "ui/Theme.h"
#include <QFile>
#include <QColor>
#include <QFontDatabase>
#include <QDebug>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>

namespace railshot {
namespace theme {

QColor color(const char* hex)
{
    return QColor(QString::fromLatin1(hex));
}

void registerFonts()
{
    const QStringList paths = {
        QStringLiteral(":/fonts/BebasNeue-Regular.ttf"),
        QStringLiteral(":/fonts/DMSans-Variable.ttf"),
        QStringLiteral(":/fonts/JetBrainsMono-Regular.ttf"),
        QStringLiteral(":/fonts/JetBrainsMono-Medium.ttf"),
    };
    for (const auto& path : paths) {
        const int id = QFontDatabase::addApplicationFont(path);
        if (id < 0)
            qWarning() << "RailShotTV: failed to load font" << path;
    }
}

QString panelHeaderStyle(PanelAccent accent)
{
    const char* border = kBlue;
    const char* tint = "rgba(79,158,255,0.28)";
    switch (accent) {
    case PanelAccent::Brand:
        border = kBrand; tint = "rgba(255,90,44,0.28)"; break;
    case PanelAccent::Blue:
        border = kBlue; tint = "rgba(79,158,255,0.28)"; break;
    case PanelAccent::Violet:
        border = kViolet; tint = "rgba(168,85,247,0.28)"; break;
    case PanelAccent::Emerald:
        border = kEmerald; tint = "rgba(34,197,94,0.28)"; break;
    case PanelAccent::Cyan:
        border = kCyan; tint = "rgba(34,211,238,0.28)"; break;
    case PanelAccent::Amber:
        border = kAmber; tint = "rgba(251,191,36,0.28)"; break;
    case PanelAccent::Pink:
        border = "#EC4899"; tint = "rgba(236,72,153,0.28)"; break;
    }
    return QStringLiteral(
               "border-left: 3px solid %1;"
               "background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 %2, stop:0.55 transparent);"
               "border-bottom: 1px solid #3A3D45;")
        .arg(QString::fromLatin1(border), QString::fromLatin1(tint));
}

void applyPanelHeader(QWidget* widget, PanelAccent accent)
{
    if (!widget) return;
    widget->setStyleSheet(panelHeaderStyle(accent));
}

QWidget* makePageHeader(const QString& pageTitle, PanelAccent accent, QWidget* parent)
{
    auto* bar = new QWidget(parent);
    bar->setObjectName(QStringLiteral("pageHeader"));
    bar->setFixedHeight(46);
    const char* accentHex = kBlue;
    switch (accent) {
    case PanelAccent::Brand: accentHex = kBrand; break;
    case PanelAccent::Violet: accentHex = kViolet; break;
    case PanelAccent::Emerald: accentHex = kEmerald; break;
    case PanelAccent::Cyan: accentHex = kCyan; break;
    case PanelAccent::Amber: accentHex = kAmber; break;
    case PanelAccent::Pink: accentHex = "#EC4899"; break;
    default: break;
    }
    bar->setStyleSheet(QStringLiteral(
        "QWidget#pageHeader {"
        "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #1A1D22, stop:1 #0F1114);"
        "  border-bottom: 1px solid #3A3D45;"
        "  border-top: 3px solid %1;"
        "}").arg(QString::fromLatin1(accentHex)));

    auto* lay = new QHBoxLayout(bar);
    lay->setContentsMargins(16, 0, 16, 0);
    lay->setSpacing(10);

    auto* brand = new QLabel(QStringLiteral("RAILSHOT"), bar);
    brand->setStyleSheet(QStringLiteral(
        "font-family:'Bebas Neue'; font-size:18px; letter-spacing:1px; color:#F0F0F0; background:transparent;"));
    auto* tv = new QLabel(QStringLiteral("TV"), bar);
    tv->setStyleSheet(QStringLiteral(
        "font-family:'Bebas Neue'; font-size:18px; letter-spacing:1px; color:#FF5A2C; background:transparent;"));
    auto* div = new QLabel(QStringLiteral("|"), bar);
    div->setStyleSheet(QStringLiteral("color:#3A3D45; font-size:14px; background:transparent;"));
    auto* title = new QLabel(pageTitle.toUpper(), bar);
    title->setObjectName(QStringLiteral("pageHeaderTitle"));
    title->setStyleSheet(QStringLiteral(
        "font-family:'DM Sans'; font-size:12px; font-weight:800; letter-spacing:2px;"
        "color:%1; background:transparent;").arg(QString::fromLatin1(accentHex)));

    lay->addWidget(brand);
    lay->addWidget(tv);
    lay->addWidget(div);
    lay->addWidget(title);
    lay->addStretch();
    return bar;
}

QLabel* makeEmptyState(const QString& glyph, const QString& message, QWidget* parent)
{
    auto* wrap = new QLabel(parent);
    wrap->setAlignment(Qt::AlignCenter);
    wrap->setWordWrap(true);
    wrap->setMinimumHeight(72);
    wrap->setText(QStringLiteral("<div style='color:#4A5060;font-size:22px;padding-bottom:6px;'>%1</div>"
                                 "<div style='color:#606878;font-size:12px;'>%2</div>")
                      .arg(glyph.toHtmlEscaped(), message.toHtmlEscaped()));
    wrap->setStyleSheet(QStringLiteral("background:transparent; padding:16px;"));
    wrap->setTextFormat(Qt::RichText);
    return wrap;
}

QString loadStyleSheet()
{
    QFile f(QStringLiteral(":/styles/railshot.qss"));
    if (f.open(QIODevice::ReadOnly))
        return QString::fromUtf8(f.readAll());
    return QStringLiteral(
        "QWidget { background: #0B0D0F; color: #F0F0F0; font-family: 'Segoe UI'; font-size: 12px; }"
        "QPushButton { background: qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #2A2D35, stop:1 #1E2128);"
        "  border: 1px solid #3A3D45; border-radius: 3px; padding: 4px 10px; color: #C8CAD0; }"
        "QPushButton:hover { border-color: #4F9EFF; }"
        "QPushButton#goButton { background: #22C55E; color: #000; font-weight: 800; }"
        "QPushButton#streamButton { background: #FF5A2C; color: #fff; font-weight: 700; }"
    );
}

} // namespace theme
} // namespace railshot
