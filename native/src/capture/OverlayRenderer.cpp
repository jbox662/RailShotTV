#include "capture/OverlayRenderer.h"
#include <QPainter>

namespace railshot {

OverlayRenderer::OverlayRenderer(QObject* parent)
    : QObject(parent)
{
}

QImage OverlayRenderer::renderScoreboard(const QJsonObject& state, int width, int height) const
{
    QImage img(width, height, QImage::Format_ARGB32);
    img.fill(Qt::transparent);
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(0, height - 80, width, 80, QColor(10, 12, 16, 220));
    p.fillRect(0, height - 80, 4, 80, QColor(QStringLiteral("#FF5A2C")));
    p.setPen(Qt::white);
    QFont f(QStringLiteral("Segoe UI"), 22, QFont::Bold);
    p.setFont(f);
    const QString a = state.value(QStringLiteral("playerA")).toString(QStringLiteral("A"));
    const QString b = state.value(QStringLiteral("playerB")).toString(QStringLiteral("B"));
    const int sa = state.value(QStringLiteral("scoreA")).toInt();
    const int sb = state.value(QStringLiteral("scoreB")).toInt();
    p.drawText(QRect(20, height - 80, width / 2 - 40, 80), Qt::AlignVCenter | Qt::AlignLeft,
               QStringLiteral("%1  %2").arg(a).arg(sa));
    p.drawText(QRect(width / 2, height - 80, width / 2 - 20, 80), Qt::AlignVCenter | Qt::AlignRight,
               QStringLiteral("%1  %2").arg(sb).arg(b));
    return img;
}

QImage OverlayRenderer::renderLowerThird(const QString& title, const QString& subtitle, int width, int height) const
{
    QImage img(width, height, QImage::Format_ARGB32);
    img.fill(Qt::transparent);
    QPainter p(&img);
    const int y = int(height * 0.72);
    p.fillRect(0, y, width, int(height * 0.2), QColor(15, 17, 20, 230));
    p.fillRect(0, y, 6, int(height * 0.2), QColor(QStringLiteral("#4F9EFF")));
    p.setPen(Qt::white);
    p.setFont(QFont(QStringLiteral("Segoe UI"), 28, QFont::Bold));
    p.drawText(24, y + 48, title);
    p.setPen(QColor(QStringLiteral("#A0A8B8")));
    p.setFont(QFont(QStringLiteral("Segoe UI"), 16));
    p.drawText(24, y + 78, subtitle);
    return img;
}

QImage OverlayRenderer::renderAlert(const QString& title, const QString& body, int width, int height) const
{
    QImage img(width, height, QImage::Format_ARGB32);
    img.fill(Qt::transparent);
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);
    const int boxW = int(width * 0.42);
    const int boxH = 110;
    const int x = width - boxW - 32;
    const int y = 32;
    p.setBrush(QColor(12, 14, 18, 230));
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(x, y, boxW, boxH, 10, 10);
    p.fillRect(x, y, 5, boxH, QColor(QStringLiteral("#FF5A2C")));
    p.setPen(Qt::white);
    p.setFont(QFont(QStringLiteral("Segoe UI"), 18, QFont::Bold));
    p.drawText(QRect(x + 18, y + 18, boxW - 30, 36), Qt::AlignLeft | Qt::AlignVCenter, title);
    p.setPen(QColor(QStringLiteral("#A0A8B8")));
    p.setFont(QFont(QStringLiteral("Segoe UI"), 13));
    p.drawText(QRect(x + 18, y + 54, boxW - 30, 40), Qt::AlignLeft | Qt::TextWordWrap, body);
    return img;
}

} // namespace railshot
