#include "ui/Theme.h"
#include <QFile>
#include <QColor>
#include <QFontDatabase>
#include <QDebug>

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
    const char* tint = "rgba(79,158,255,0.14)";
    switch (accent) {
    case PanelAccent::Brand:
        border = kBrand; tint = "rgba(255,90,44,0.14)"; break;
    case PanelAccent::Blue:
        border = kBlue; tint = "rgba(79,158,255,0.14)"; break;
    case PanelAccent::Violet:
        border = kViolet; tint = "rgba(168,85,247,0.14)"; break;
    case PanelAccent::Emerald:
        border = kEmerald; tint = "rgba(34,197,94,0.14)"; break;
    case PanelAccent::Cyan:
        border = kCyan; tint = "rgba(34,211,238,0.14)"; break;
    case PanelAccent::Amber:
        border = kAmber; tint = "rgba(251,191,36,0.14)"; break;
    case PanelAccent::Pink:
        border = "#EC4899"; tint = "rgba(236,72,153,0.14)"; break;
    }
    return QStringLiteral(
               "border-left: 2px solid %1;"
               "background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 %2, stop:0.65 transparent);")
        .arg(QString::fromLatin1(border), QString::fromLatin1(tint));
}

void applyPanelHeader(QWidget* widget, PanelAccent accent)
{
    if (!widget) return;
    widget->setStyleSheet(panelHeaderStyle(accent));
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
