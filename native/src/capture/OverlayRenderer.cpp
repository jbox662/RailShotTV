#include "capture/OverlayRenderer.h"
#include <QPainter>
#include <QLinearGradient>

namespace railshot {

namespace {
QColor themeBg(const QString& theme)
{
    if (theme == QLatin1String("classic")) return QColor(245, 245, 248, 230);
    if (theme == QLatin1String("broadcast")) return QColor(8, 20, 40, 230);
    if (theme == QLatin1String("neon")) return QColor(8, 4, 20, 235);
    if (theme == QLatin1String("carbon")) return QColor(18, 18, 20, 235);
    if (theme == QLatin1String("gold")) return QColor(28, 22, 10, 235);
    return QColor(10, 12, 16, 220); // railshot / dark
}
QColor themeFg(const QString& theme)
{
    if (theme == QLatin1String("classic")) return QColor(20, 22, 28);
    if (theme == QLatin1String("gold")) return QColor(255, 236, 179);
    return QColor(255, 255, 255);
}
QColor resolveBg(const QJsonObject& state, const QString& theme)
{
    const QString custom = state.value(QStringLiteral("bgColor")).toString();
    if (!custom.isEmpty()) {
        QColor c(custom);
        if (c.isValid()) {
            if (c.alpha() == 255) c.setAlpha(230);
            return c;
        }
    }
    return themeBg(theme);
}
QColor resolveFg(const QJsonObject& state, const QString& theme)
{
    const QString custom = state.value(QStringLiteral("textColor")).toString();
    if (!custom.isEmpty()) {
        QColor c(custom);
        if (c.isValid()) return c;
    }
    return themeFg(theme);
}
} // namespace

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

    const QString a = state.value(QStringLiteral("playerA")).toString(QStringLiteral("A"));
    const QString b = state.value(QStringLiteral("playerB")).toString(QStringLiteral("B"));
    const int sa = state.value(QStringLiteral("scoreA")).toInt();
    const int sb = state.value(QStringLiteral("scoreB")).toInt();
    const QString layout = state.value(QStringLiteral("layout")).toString(QStringLiteral("standard"));
    const QString theme = state.value(QStringLiteral("theme")).toString(QStringLiteral("railshot"));
    const QColor colA(state.value(QStringLiteral("colorA")).toString(QStringLiteral("#FF5A2C")));
    const QColor colB(state.value(QStringLiteral("colorB")).toString(QStringLiteral("#4F9EFF")));
    const int clock = state.value(QStringLiteral("clockSeconds")).toInt();
    const QString clockText = QStringLiteral("%1:%2")
                                  .arg(clock / 60, 2, 10, QChar('0'))
                                  .arg(clock % 60, 2, 10, QChar('0'));
    const QColor bg = resolveBg(state, theme);
    const QColor fg = resolveFg(state, theme);
    const bool neon = theme == QLatin1String("neon");
    const QColor accentA = colA.isValid() ? colA : QColor(QStringLiteral("#FF5A2C"));
    const QColor accentB = colB.isValid() ? colB : QColor(QStringLiteral("#4F9EFF"));

    auto drawTeamBlock = [&](const QRect& r, const QString& name, int score, const QColor& accent, bool right) {
        p.fillRect(r, bg);
        p.fillRect(right ? QRect(r.right() - 4, r.top(), 4, r.height())
                         : QRect(r.left(), r.top(), 4, r.height()),
                   accent);
        if (neon) {
            p.setPen(QPen(accent, 2));
            p.drawRect(r.adjusted(1, 1, -2, -2));
        }
        p.setPen(fg);
        p.setFont(QFont(QStringLiteral("Segoe UI"), qMax(14, r.height() / 4), QFont::Bold));
        p.drawText(r.adjusted(14, 0, -14, 0),
                   (right ? Qt::AlignRight : Qt::AlignLeft) | Qt::AlignVCenter,
                   QStringLiteral("%1  %2").arg(right ? QString::number(score) : name,
                                               right ? name : QString::number(score)));
    };

    if (layout == QLatin1String("compact")) {
        const int boxW = int(width * 0.28);
        const int boxH = 56;
        const QRect ra(16, 16, boxW, boxH);
        const QRect rb(width - boxW - 16, 16, boxW, boxH);
        drawTeamBlock(ra, a, sa, accentA, false);
        drawTeamBlock(rb, b, sb, accentB, true);
        p.setPen(fg);
        p.setFont(QFont(QStringLiteral("JetBrains Mono"), 14, QFont::Bold));
        p.drawText(QRect(0, 20, width, 40), Qt::AlignHCenter | Qt::AlignVCenter, clockText);
    } else if (layout == QLatin1String("center")) {
        // Mid-screen broadcast banner
        const int h = 96;
        const int y = (height - h) / 2;
        const int x = int(width * 0.12);
        const int w = int(width * 0.76);
        p.setBrush(bg);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(x, y, w, h, 8, 8);
        p.fillRect(x, y, 6, h, accentA);
        p.fillRect(x + w - 6, y, 6, h, accentB);
        QLinearGradient g(x, y, x + w, y);
        g.setColorAt(0.0, QColor(accentA.red(), accentA.green(), accentA.blue(), 50));
        g.setColorAt(0.5, Qt::transparent);
        g.setColorAt(1.0, QColor(accentB.red(), accentB.green(), accentB.blue(), 50));
        p.fillRect(x, y, w, h, g);
        if (neon) {
            p.setPen(QPen(accentA, 2));
            p.drawRoundedRect(x + 1, y + 1, w - 2, h - 2, 8, 8);
        }
        p.setPen(fg);
        p.setFont(QFont(QStringLiteral("Bebas Neue"), 36, QFont::Bold));
        p.drawText(QRect(x + 24, y, w / 2 - 40, h), Qt::AlignVCenter | Qt::AlignLeft,
                   QStringLiteral("%1  %2").arg(a).arg(sa));
        p.drawText(QRect(x + w / 2 + 16, y, w / 2 - 40, h), Qt::AlignVCenter | Qt::AlignRight,
                   QStringLiteral("%1  %2").arg(sb).arg(b));
        p.setFont(QFont(QStringLiteral("JetBrains Mono"), 16, QFont::Bold));
        p.setPen(accentA);
        p.drawText(QRect(x, y, w, h), Qt::AlignCenter, clockText);
    } else if (layout == QLatin1String("wide")) {
        const int h = 72;
        const int y = height - h;
        p.fillRect(0, y, width, h, bg);
        p.fillRect(0, y, width / 2, h, QColor(accentA.red(), accentA.green(), accentA.blue(), 40));
        p.fillRect(width / 2, y, width / 2, h, QColor(accentB.red(), accentB.green(), accentB.blue(), 40));
        p.fillRect(0, y, 6, h, accentA);
        p.fillRect(width - 6, y, 6, h, accentB);
        p.setPen(fg);
        p.setFont(QFont(QStringLiteral("Bebas Neue"), 28, QFont::Bold));
        p.drawText(QRect(24, y, width / 2 - 40, h), Qt::AlignVCenter | Qt::AlignLeft,
                   QStringLiteral("%1   %2").arg(a).arg(sa));
        p.drawText(QRect(width / 2 + 20, y, width / 2 - 40, h), Qt::AlignVCenter | Qt::AlignRight,
                   QStringLiteral("%1   %2").arg(sb).arg(b));
        p.setFont(QFont(QStringLiteral("JetBrains Mono"), 16, QFont::Bold));
        p.drawText(QRect(0, y, width, h), Qt::AlignCenter, clockText);
    } else {
        // standard / lower-third strip
        const int h = 80;
        const int y = height - h;
        p.fillRect(0, y, width, h, bg);
        p.fillRect(0, y, 4, h, accentA);
        p.fillRect(width - 4, y, 4, h, accentB);
        QLinearGradient g(0, y, width, y);
        g.setColorAt(0.0, QColor(accentA.red(), accentA.green(), accentA.blue(), 55));
        g.setColorAt(0.5, Qt::transparent);
        g.setColorAt(1.0, QColor(accentB.red(), accentB.green(), accentB.blue(), 55));
        p.fillRect(0, y, width, h, g);
        p.setPen(fg);
        p.setFont(QFont(QStringLiteral("Segoe UI"), 22, QFont::Bold));
        p.drawText(QRect(20, y, width / 2 - 40, h), Qt::AlignVCenter | Qt::AlignLeft,
                   QStringLiteral("%1  %2").arg(a).arg(sa));
        p.drawText(QRect(width / 2, y, width / 2 - 20, h), Qt::AlignVCenter | Qt::AlignRight,
                   QStringLiteral("%1  %2").arg(sb).arg(b));
        p.setFont(QFont(QStringLiteral("JetBrains Mono"), 14, QFont::Bold));
        p.setPen(accentA);
        p.drawText(QRect(0, y, width, h), Qt::AlignCenter, clockText);
    }
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
