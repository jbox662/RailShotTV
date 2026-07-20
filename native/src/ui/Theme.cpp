#include "ui/Theme.h"
#include <QFile>
#include <QColor>

namespace railshot {
namespace theme {

QColor color(const char* hex)
{
    return QColor(QString::fromLatin1(hex));
}

QString loadStyleSheet()
{
    QFile f(QStringLiteral(":/styles/railshot.qss"));
    if (f.open(QIODevice::ReadOnly))
        return QString::fromUtf8(f.readAll());
    // Fallback inline sheet if resource missing
    return QStringLiteral(
        "QWidget { background: #0B0D0F; color: #F0F0F0; font-family: 'Segoe UI'; font-size: 12px; }"
        "QPushButton { background: qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #2A2D35, stop:1 #1E2128);"
        "  border: 1px solid #3A3D45; border-radius: 3px; padding: 4px 10px; color: #C8CAD0; }"
        "QPushButton:hover { border-color: #4F9EFF; }"
        "QPushButton#goButton { background: #22C55E; color: #000; font-weight: 800; }"
        "QPushButton#streamButton { background: #FF5A2C; color: #fff; font-weight: 700; }"
        "QListWidget { background: #0A0C0F; border: 1px solid #2A2D35; }"
        "QListWidget::item:selected { background: #1A2A3A; color: #4F9EFF; }"
        "QSlider::groove:vertical { width: 4px; background: #1A1D24; }"
        "QSlider::handle:vertical { height: 12px; background: #C8CAD0; margin: 0 -4px; border-radius: 2px; }"
        "QLabel#mono { font-family: 'Consolas'; font-size: 10px; color: #606878; }"
    );
}

} // namespace theme
} // namespace railshot
