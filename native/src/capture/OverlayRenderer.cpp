#include "capture/OverlayRenderer.h"
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QtMath>

namespace railshot {

namespace {

QColor themeBg(const QString& theme)
{
    if (theme == QLatin1String("classic")) return QColor(245, 245, 248, 230);
    if (theme == QLatin1String("broadcast")) return QColor(8, 20, 40, 230);
    if (theme == QLatin1String("neon")) return QColor(8, 4, 20, 235);
    if (theme == QLatin1String("carbon")) return QColor(18, 18, 20, 235);
    if (theme == QLatin1String("gold")) return QColor(28, 22, 10, 235);
    return QColor(10, 12, 16, 220);
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
            if (c.alpha() == 255) c.setAlpha(235);
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

QString clockText(int clock)
{
    return QStringLiteral("%1:%2")
        .arg(clock / 60, 2, 10, QChar('0'))
        .arg(clock % 60, 2, 10, QChar('0'));
}

QString shortName(QString name, int maxChars = 12)
{
    name = name.trimmed();
    if (name.size() <= maxChars) return name;
    return name.left(maxChars - 1) + QChar(0x2026);
}

/// Placement for sport bugs based on layout id.
QRect placeBoard(const QString& layout, int W, int H, int bw, int bh)
{
    if (layout == QLatin1String("compact"))
        return QRect(24, 24, bw, bh);
    if (layout == QLatin1String("center"))
        return QRect((W - bw) / 2, (H - bh) / 2, bw, bh);
    if (layout == QLatin1String("wide"))
        return QRect(0, H - bh, W, bh);
    // standard lower-third
    return QRect(qMax(0, (W - bw) / 2), H - bh - 28, qMin(bw, W), bh);
}

void drawRoundRect(QPainter& p, const QRect& r, int radius, const QColor& fill, const QColor& border = Qt::transparent)
{
    p.setPen(border.alpha() ? QPen(border, 1.5) : Qt::NoPen);
    p.setBrush(fill);
    p.drawRoundedRect(r, radius, radius);
}

void drawBaseDiamond(QPainter& p, const QPointF& c, qreal size, bool occupied, const QColor& lit, const QColor& dim)
{
    QPolygonF poly;
    poly << QPointF(c.x(), c.y() - size)
         << QPointF(c.x() + size, c.y())
         << QPointF(c.x(), c.y() + size)
         << QPointF(c.x() - size, c.y());
    p.setPen(QPen(QColor(255, 255, 255, 40), 1));
    p.setBrush(occupied ? lit : dim);
    p.drawPolygon(poly);
}

void drawCueBall(QPainter& p, const QPointF& c, qreal r)
{
    QRadialGradient g(c.x() - r * 0.3, c.y() - r * 0.3, r * 1.4);
    g.setColorAt(0, QColor(255, 255, 255));
    g.setColorAt(1, QColor(200, 205, 215));
    p.setPen(QPen(QColor(40, 44, 52), 1));
    p.setBrush(g);
    p.drawEllipse(c, r, r);
}

void drawEightBall(QPainter& p, const QPointF& c, qreal r)
{
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(12, 12, 14));
    p.drawEllipse(c, r, r);
    p.setBrush(QColor(245, 245, 248));
    p.drawEllipse(c, r * 0.45, r * 0.45);
    p.setPen(QColor(12, 12, 14));
    p.setFont(QFont(QStringLiteral("Segoe UI"), int(r * 0.55), QFont::Bold));
    p.drawText(QRectF(c.x() - r, c.y() - r, r * 2, r * 2), Qt::AlignCenter, QStringLiteral("8"));
}

// ── Billiards / pool (primary) ───────────────────────────────────
void renderBilliards(QPainter& p, const QJsonObject& state, int W, int H)
{
    const QString layout = state.value(QStringLiteral("layout")).toString(QStringLiteral("standard"));
    const QString theme = state.value(QStringLiteral("theme")).toString(QStringLiteral("railshot"));
    const QString a = shortName(state.value(QStringLiteral("playerA")).toString(QStringLiteral("Player A")), 14);
    const QString b = shortName(state.value(QStringLiteral("playerB")).toString(QStringLiteral("Player B")), 14);
    const int sa = state.value(QStringLiteral("scoreA")).toInt();
    const int sb = state.value(QStringLiteral("scoreB")).toInt();
    const int race = state.value(QStringLiteral("raceTo")).toInt(7);
    const int active = state.value(QStringLiteral("activeSide")).toInt(1);
    const int clock = state.value(QStringLiteral("clockSeconds")).toInt();
    QColor accentA(state.value(QStringLiteral("colorA")).toString(QStringLiteral("#FF5A2C")));
    QColor accentB(state.value(QStringLiteral("colorB")).toString(QStringLiteral("#4F9EFF")));
    if (!accentA.isValid()) accentA = QColor(QStringLiteral("#FF5A2C"));
    if (!accentB.isValid()) accentB = QColor(QStringLiteral("#4F9EFF"));
    const QColor fg = resolveFg(state, theme);
    const QColor felt(16, 90, 48);
    const QColor feltDeep(8, 48, 28);
    QColor bg = resolveBg(state, theme);
    // Prefer felt-tinted plate for pool when using default dark themes
    if (state.value(QStringLiteral("bgColor")).toString().isEmpty()
        && (theme == QLatin1String("railshot") || theme == QLatin1String("broadcast")
            || theme == QLatin1String("carbon"))) {
        bg = QColor(10, 18, 14, 240);
    }

    const bool wide = layout == QLatin1String("wide");
    const int bh = wide ? 88 : 100;
    const int bw = wide ? W : qMin(1180, int(W * 0.92));
    const QRect board = placeBoard(layout, W, H, bw, bh);

    // Outer shell + felt rail
    drawRoundRect(p, board, 8, bg, QColor(255, 255, 255, 28));
    p.fillRect(QRect(board.left() + 6, board.top() + 4, board.width() - 12, 3), felt);
    p.fillRect(QRect(board.left() + 6, board.bottom() - 6, board.width() - 12, 3), feltDeep);

    const int midX = board.center().x();
    const int y0 = board.top() + 14;
    const int rowH = board.height() - 28;

    // Center race pill + 8-ball
    const int raceW = 150;
    const QRect raceR(midX - raceW / 2, y0 + 8, raceW, rowH - 16);
    drawRoundRect(p, raceR, 6, QColor(6, 10, 8, 220), felt);
    drawEightBall(p, QPointF(raceR.center().x(), raceR.top() + 22), 11);
    p.setPen(felt.lighter(140));
    p.setFont(QFont(QStringLiteral("DM Sans"), 9, QFont::Bold));
    p.drawText(QRect(raceR.left(), raceR.top() + 34, raceR.width(), 16), Qt::AlignHCenter | Qt::AlignVCenter,
               QStringLiteral("RACE TO"));
    p.setPen(fg);
    p.setFont(QFont(QStringLiteral("Bebas Neue"), 22, QFont::Bold));
    p.drawText(QRect(raceR.left(), raceR.top() + 48, raceR.width(), 28), Qt::AlignHCenter | Qt::AlignVCenter,
               QString::number(race));

    // Left player
    const QRect leftR(board.left() + 12, y0, midX - raceW / 2 - board.left() - 20, rowH);
    if (active == 1) {
        p.fillRect(QRect(leftR.left(), leftR.top(), 4, leftR.height()), accentA);
        p.fillRect(QRect(leftR.left() + 4, leftR.top(), leftR.width() - 4, leftR.height()),
                   QColor(accentA.red(), accentA.green(), accentA.blue(), 35));
    }
    p.setPen(active == 1 ? accentA : fg);
    p.setFont(QFont(QStringLiteral("DM Sans"), 13, QFont::Bold));
    p.drawText(QRect(leftR.left() + 14, leftR.top() + 6, leftR.width() - 70, 28),
               Qt::AlignLeft | Qt::AlignVCenter, a.toUpper());
    p.setPen(fg);
    p.setFont(QFont(QStringLiteral("Bebas Neue"), 42, QFont::Bold));
    p.drawText(QRect(leftR.right() - 72, leftR.top(), 68, leftR.height()),
               Qt::AlignRight | Qt::AlignVCenter, QString::number(sa));
    p.setPen(QColor(fg.red(), fg.green(), fg.blue(), 140));
    p.setFont(QFont(QStringLiteral("DM Sans"), 9, QFont::Bold));
    p.drawText(QRect(leftR.left() + 14, leftR.bottom() - 26, 80, 18), Qt::AlignLeft | Qt::AlignVCenter,
               QStringLiteral("RACKS"));

    // Right player
    const QRect rightR(midX + raceW / 2 + 8, y0, board.right() - (midX + raceW / 2) - 20, rowH);
    if (active == 2) {
        p.fillRect(QRect(rightR.right() - 3, rightR.top(), 4, rightR.height()), accentB);
        p.fillRect(QRect(rightR.left(), rightR.top(), rightR.width() - 4, rightR.height()),
                   QColor(accentB.red(), accentB.green(), accentB.blue(), 35));
    }
    p.setPen(fg);
    p.setFont(QFont(QStringLiteral("Bebas Neue"), 42, QFont::Bold));
    p.drawText(QRect(rightR.left(), rightR.top(), 68, rightR.height()),
               Qt::AlignLeft | Qt::AlignVCenter, QString::number(sb));
    p.setPen(active == 2 ? accentB : fg);
    p.setFont(QFont(QStringLiteral("DM Sans"), 13, QFont::Bold));
    p.drawText(QRect(rightR.left() + 72, rightR.top() + 6, rightR.width() - 86, 28),
               Qt::AlignRight | Qt::AlignVCenter, b.toUpper());
    p.setPen(QColor(fg.red(), fg.green(), fg.blue(), 140));
    p.setFont(QFont(QStringLiteral("DM Sans"), 9, QFont::Bold));
    p.drawText(QRect(rightR.right() - 90, rightR.bottom() - 26, 80, 18), Qt::AlignRight | Qt::AlignVCenter,
               QStringLiteral("RACKS"));

    // Shot clock chip (if running / non-zero)
    if (clock > 0 || state.value(QStringLiteral("clockRunning")).toBool()) {
        const QString ct = clockText(clock);
        p.setPen(QColor(134, 239, 172));
        p.setFont(QFont(QStringLiteral("JetBrains Mono"), 10, QFont::Bold));
        p.drawText(QRect(board.left(), board.bottom() - 22, board.width(), 16), Qt::AlignCenter,
                   QStringLiteral("SHOT  %1").arg(ct));
    }

    // Cue ball accents on active side
    if (active == 1)
        drawCueBall(p, QPointF(leftR.left() + 22, leftR.bottom() - 18), 6);
    else if (active == 2)
        drawCueBall(p, QPointF(rightR.right() - 22, rightR.bottom() - 18), 6);
}

// ── Baseball (diamond bug) ───────────────────────────────────────
void renderBaseball(QPainter& p, const QJsonObject& state, int W, int H)
{
    const QString layout = state.value(QStringLiteral("layout")).toString(QStringLiteral("standard"));
    const QString theme = state.value(QStringLiteral("theme")).toString(QStringLiteral("railshot"));
    const QString a = shortName(state.value(QStringLiteral("playerA")).toString(QStringLiteral("HOME")), 10);
    const QString b = shortName(state.value(QStringLiteral("playerB")).toString(QStringLiteral("AWAY")), 10);
    const int sa = state.value(QStringLiteral("scoreA")).toInt();
    const int sb = state.value(QStringLiteral("scoreB")).toInt();
    const int balls = qBound(0, state.value(QStringLiteral("balls")).toInt(), 3);
    const int strikes = qBound(0, state.value(QStringLiteral("strikes")).toInt(), 2);
    const int outs = qBound(0, state.value(QStringLiteral("outs")).toInt(), 2);
    const int inning = qMax(1, state.value(QStringLiteral("inning")).toInt(1));
    const bool top = state.value(QStringLiteral("topHalf")).toBool(true);
    const bool on1 = state.value(QStringLiteral("onFirst")).toBool();
    const bool on2 = state.value(QStringLiteral("onSecond")).toBool();
    const bool on3 = state.value(QStringLiteral("onThird")).toBool();
    QColor accentA(state.value(QStringLiteral("colorA")).toString(QStringLiteral("#22C55E")));
    QColor accentB(state.value(QStringLiteral("colorB")).toString(QStringLiteral("#EF4444")));
    if (!accentA.isValid()) accentA = QColor(QStringLiteral("#22C55E"));
    if (!accentB.isValid()) accentB = QColor(QStringLiteral("#EF4444"));
    const QColor fg = resolveFg(state, theme);
    const QColor bg = resolveBg(state, theme);
    const QColor litBase(QStringLiteral("#F97316"));
    const QColor dimBase(40, 44, 52);

    const int bh = 92;
    const int bw = layout == QLatin1String("wide") ? W : qMin(720, int(W * 0.55));
    const QRect board = placeBoard(layout, W, H, bw, bh);
    drawRoundRect(p, board, 6, bg, QColor(255, 255, 255, 30));

    // Team stack (left)
    const int leftW = int(board.width() * 0.38);
    const QRect homeR(board.left() + 6, board.top() + 8, leftW - 8, 36);
    const QRect awayR(board.left() + 6, board.top() + 48, leftW - 8, 36);
    drawRoundRect(p, homeR, 3, QColor(0, 0, 0, 160));
    drawRoundRect(p, awayR, 3, QColor(0, 0, 0, 160));
    p.fillRect(QRect(homeR.left(), homeR.top(), 4, homeR.height()), accentA);
    p.fillRect(QRect(awayR.left(), awayR.top(), 4, awayR.height()), accentB);
    p.setPen(fg);
    p.setFont(QFont(QStringLiteral("DM Sans"), 12, QFont::Bold));
    p.drawText(homeR.adjusted(12, 0, -40, 0), Qt::AlignVCenter | Qt::AlignLeft, a.toUpper());
    p.drawText(awayR.adjusted(12, 0, -40, 0), Qt::AlignVCenter | Qt::AlignLeft, b.toUpper());
    p.setFont(QFont(QStringLiteral("Bebas Neue"), 22, QFont::Bold));
    p.drawText(homeR.adjusted(0, 0, -8, 0), Qt::AlignVCenter | Qt::AlignRight, QString::number(sa));
    p.drawText(awayR.adjusted(0, 0, -8, 0), Qt::AlignVCenter | Qt::AlignRight, QString::number(sb));

    // Diamond
    const int dx = board.left() + leftW + int(board.width() * 0.14);
    const int dy = board.center().y();
    const qreal s = 14;
    // dirt pad
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(180, 120, 70, 80));
    p.drawEllipse(QPointF(dx, dy), s * 1.8, s * 1.8);
    drawBaseDiamond(p, QPointF(dx, dy - s * 1.35), 7, on2, litBase, dimBase);       // 2nd
    drawBaseDiamond(p, QPointF(dx + s * 1.35, dy), 7, on1, litBase, dimBase);        // 1st
    drawBaseDiamond(p, QPointF(dx - s * 1.35, dy), 7, on3, litBase, dimBase);        // 3rd
    // home plate
    p.setBrush(QColor(240, 240, 245));
    QPolygonF home;
    home << QPointF(dx, dy + s * 1.55) << QPointF(dx + 5, dy + s * 1.15)
         << QPointF(dx + 5, dy + s * 0.85) << QPointF(dx - 5, dy + s * 0.85)
         << QPointF(dx - 5, dy + s * 1.15);
    p.drawPolygon(home);

    // Inning + count + outs (right)
    const int rx = board.left() + int(board.width() * 0.62);
    const int rw = board.right() - rx - 8;
    p.setPen(fg);
    p.setFont(QFont(QStringLiteral("DM Sans"), 11, QFont::Bold));
    const QString half = top ? QStringLiteral("TOP") : QStringLiteral("BOT");
    p.drawText(QRect(rx, board.top() + 10, rw, 22), Qt::AlignLeft | Qt::AlignVCenter,
               QStringLiteral("%1 %2").arg(half).arg(inning));
    // arrow
    p.setBrush(fg);
    QPolygonF arrow;
    if (top) {
        arrow << QPointF(rx + rw - 18, board.top() + 26) << QPointF(rx + rw - 8, board.top() + 26)
              << QPointF(rx + rw - 13, board.top() + 14);
    } else {
        arrow << QPointF(rx + rw - 18, board.top() + 14) << QPointF(rx + rw - 8, board.top() + 14)
              << QPointF(rx + rw - 13, board.top() + 26);
    }
    p.drawPolygon(arrow);

    p.setFont(QFont(QStringLiteral("Bebas Neue"), 20, QFont::Bold));
    p.drawText(QRect(rx, board.top() + 36, rw, 26), Qt::AlignLeft | Qt::AlignVCenter,
               QStringLiteral("%1-%2").arg(balls).arg(strikes));
    p.setFont(QFont(QStringLiteral("DM Sans"), 11, QFont::Bold));
    p.drawText(QRect(rx, board.top() + 62, rw, 22), Qt::AlignLeft | Qt::AlignVCenter,
               outs == 1 ? QStringLiteral("1 OUT") : QStringLiteral("%1 OUTS").arg(outs));
}

// ── Basketball ───────────────────────────────────────────────────
void renderBasketball(QPainter& p, const QJsonObject& state, int W, int H)
{
    const QString layout = state.value(QStringLiteral("layout")).toString(QStringLiteral("standard"));
    const QString theme = state.value(QStringLiteral("theme")).toString(QStringLiteral("railshot"));
    const QString a = shortName(state.value(QStringLiteral("playerA")).toString(QStringLiteral("HOME")), 10);
    const QString b = shortName(state.value(QStringLiteral("playerB")).toString(QStringLiteral("AWAY")), 10);
    const int sa = state.value(QStringLiteral("scoreA")).toInt();
    const int sb = state.value(QStringLiteral("scoreB")).toInt();
    const int period = qMax(1, state.value(QStringLiteral("period")).toInt(1));
    const int clock = state.value(QStringLiteral("clockSeconds")).toInt();
    QColor accentA(state.value(QStringLiteral("colorA")).toString(QStringLiteral("#EA580C")));
    QColor accentB(state.value(QStringLiteral("colorB")).toString(QStringLiteral("#2563EB")));
    if (!accentA.isValid()) accentA = QColor(QStringLiteral("#EA580C"));
    if (!accentB.isValid()) accentB = QColor(QStringLiteral("#2563EB"));
    const QColor fg = resolveFg(state, theme);
    const QColor bg = resolveBg(state, theme);

    const int bh = 72;
    const int bw = layout == QLatin1String("wide") ? W : qMin(900, int(W * 0.72));
    const QRect board = placeBoard(layout, W, H, bw, bh);
    drawRoundRect(p, board, 4, bg, QColor(255, 255, 255, 25));

    const int third = board.width() / 3;
    p.fillRect(QRect(board.left(), board.top(), 5, board.height()), accentA);
    p.fillRect(QRect(board.right() - 5, board.top(), 5, board.height()), accentB);

    p.setPen(fg);
    p.setFont(QFont(QStringLiteral("DM Sans"), 14, QFont::Bold));
    p.drawText(QRect(board.left() + 16, board.top(), third - 40, board.height()),
               Qt::AlignVCenter | Qt::AlignLeft, a.toUpper());
    p.setFont(QFont(QStringLiteral("Bebas Neue"), 36, QFont::Bold));
    p.drawText(QRect(board.left() + third - 70, board.top(), 60, board.height()),
               Qt::AlignVCenter | Qt::AlignRight, QString::number(sa));

    // Center: Qx + clock
    p.setFont(QFont(QStringLiteral("DM Sans"), 11, QFont::Bold));
    p.setPen(accentA);
    p.drawText(QRect(board.left() + third, board.top() + 8, third, 22), Qt::AlignCenter,
               QStringLiteral("Q%1").arg(period));
    p.setPen(fg);
    p.setFont(QFont(QStringLiteral("JetBrains Mono"), 16, QFont::Bold));
    p.drawText(QRect(board.left() + third, board.top() + 30, third, 28), Qt::AlignCenter, clockText(clock));

    p.setFont(QFont(QStringLiteral("Bebas Neue"), 36, QFont::Bold));
    p.drawText(QRect(board.left() + 2 * third + 10, board.top(), 60, board.height()),
               Qt::AlignVCenter | Qt::AlignLeft, QString::number(sb));
    p.setFont(QFont(QStringLiteral("DM Sans"), 14, QFont::Bold));
    p.drawText(QRect(board.right() - third + 70, board.top(), third - 90, board.height()),
               Qt::AlignVCenter | Qt::AlignRight, b.toUpper());
}

// ── Soccer ───────────────────────────────────────────────────────
void renderSoccer(QPainter& p, const QJsonObject& state, int W, int H)
{
    const QString layout = state.value(QStringLiteral("layout")).toString(QStringLiteral("standard"));
    const QString theme = state.value(QStringLiteral("theme")).toString(QStringLiteral("railshot"));
    const QString a = shortName(state.value(QStringLiteral("playerA")).toString(QStringLiteral("HOME")), 12);
    const QString b = shortName(state.value(QStringLiteral("playerB")).toString(QStringLiteral("AWAY")), 12);
    const int sa = state.value(QStringLiteral("scoreA")).toInt();
    const int sb = state.value(QStringLiteral("scoreB")).toInt();
    const int clock = state.value(QStringLiteral("clockSeconds")).toInt();
    const int mins = clock / 60;
    QColor accentA(state.value(QStringLiteral("colorA")).toString(QStringLiteral("#16A34A")));
    QColor accentB(state.value(QStringLiteral("colorB")).toString(QStringLiteral("#E2E8F0")));
    if (!accentA.isValid()) accentA = QColor(QStringLiteral("#16A34A"));
    if (!accentB.isValid()) accentB = QColor(QStringLiteral("#E2E8F0"));
    const QColor fg = resolveFg(state, theme);
    const QColor bg = resolveBg(state, theme);

    const int bh = 64;
    const int bw = layout == QLatin1String("wide") ? W : qMin(860, int(W * 0.7));
    const QRect board = placeBoard(layout, W, H, bw, bh);
    drawRoundRect(p, board, 4, bg, QColor(255, 255, 255, 22));
    p.fillRect(QRect(board.left(), board.top(), board.width(), 3), accentA);

    p.setPen(fg);
    p.setFont(QFont(QStringLiteral("DM Sans"), 13, QFont::Bold));
    p.drawText(QRect(board.left() + 16, board.top(), int(board.width() * 0.32), board.height()),
               Qt::AlignVCenter | Qt::AlignLeft, a.toUpper());
    p.setFont(QFont(QStringLiteral("Bebas Neue"), 28, QFont::Bold));
    p.drawText(QRect(0, board.top(), board.width(), board.height()), Qt::AlignCenter,
               QStringLiteral("%1  -  %2").arg(sa).arg(sb));
    p.setFont(QFont(QStringLiteral("DM Sans"), 13, QFont::Bold));
    p.drawText(QRect(board.right() - int(board.width() * 0.32) - 16, board.top(),
                     int(board.width() * 0.32), board.height()),
               Qt::AlignVCenter | Qt::AlignRight, b.toUpper());

    // Minute badge
    const QString minute = QStringLiteral("%1'").arg(mins);
    const QRect badge(board.center().x() - 28, board.top() - 14, 56, 20);
    drawRoundRect(p, badge, 3, accentA, Qt::transparent);
    p.setPen(Qt::white);
    p.setFont(QFont(QStringLiteral("DM Sans"), 10, QFont::Bold));
    p.drawText(badge, Qt::AlignCenter, minute);
}

// ── Tennis ───────────────────────────────────────────────────────
void renderTennis(QPainter& p, const QJsonObject& state, int W, int H)
{
    const QString layout = state.value(QStringLiteral("layout")).toString(QStringLiteral("standard"));
    const QString theme = state.value(QStringLiteral("theme")).toString(QStringLiteral("railshot"));
    const QString a = shortName(state.value(QStringLiteral("playerA")).toString(QStringLiteral("P1")), 14);
    const QString b = shortName(state.value(QStringLiteral("playerB")).toString(QStringLiteral("P2")), 14);
    const int sa = state.value(QStringLiteral("scoreA")).toInt(); // games
    const int sb = state.value(QStringLiteral("scoreB")).toInt();
    const int setA = state.value(QStringLiteral("balls")).toInt();
    const int setB = state.value(QStringLiteral("strikes")).toInt();
    const int active = state.value(QStringLiteral("activeSide")).toInt(1);
    QColor accentA(state.value(QStringLiteral("colorA")).toString(QStringLiteral("#F59E0B")));
    QColor accentB(state.value(QStringLiteral("colorB")).toString(QStringLiteral("#D97706")));
    if (!accentA.isValid()) accentA = QColor(QStringLiteral("#F59E0B"));
    if (!accentB.isValid()) accentB = QColor(QStringLiteral("#D97706"));
    const QColor fg = resolveFg(state, theme);
    const QColor bg = resolveBg(state, theme);

    const int bh = 78;
    const int bw = layout == QLatin1String("wide") ? W : qMin(640, int(W * 0.5));
    const QRect board = placeBoard(layout, W, H, bw, bh);
    drawRoundRect(p, board, 4, bg, QColor(255, 255, 255, 25));

    auto row = [&](int y, const QString& name, int games, int sets, bool serve, const QColor& accent) {
        const QRect r(board.left() + 6, y, board.width() - 12, 32);
        drawRoundRect(p, r, 2, QColor(0, 0, 0, 120));
        if (serve) {
            p.setBrush(accent);
            p.setPen(Qt::NoPen);
            p.drawEllipse(QPointF(r.left() + 14, r.center().y()), 5, 5);
        }
        p.setPen(fg);
        p.setFont(QFont(QStringLiteral("DM Sans"), 12, QFont::Bold));
        p.drawText(QRect(r.left() + 28, r.top(), r.width() - 120, r.height()),
                   Qt::AlignVCenter | Qt::AlignLeft, name.toUpper());
        p.setFont(QFont(QStringLiteral("Bebas Neue"), 18, QFont::Bold));
        p.drawText(QRect(r.right() - 100, r.top(), 40, r.height()), Qt::AlignCenter, QString::number(sets));
        p.drawText(QRect(r.right() - 50, r.top(), 44, r.height()), Qt::AlignCenter, QString::number(games));
    };
    row(board.top() + 6, a, sa, setA, active == 1, accentA);
    row(board.top() + 40, b, sb, setB, active == 2, accentB);
}

// ── Generic fallback (existing layouts) ──────────────────────────
void renderGeneric(QPainter& p, const QJsonObject& state, int W, int H)
{
    const QString a = state.value(QStringLiteral("playerA")).toString(QStringLiteral("A"));
    const QString b = state.value(QStringLiteral("playerB")).toString(QStringLiteral("B"));
    const int sa = state.value(QStringLiteral("scoreA")).toInt();
    const int sb = state.value(QStringLiteral("scoreB")).toInt();
    const QString layout = state.value(QStringLiteral("layout")).toString(QStringLiteral("standard"));
    const QString theme = state.value(QStringLiteral("theme")).toString(QStringLiteral("railshot"));
    const QColor colA(state.value(QStringLiteral("colorA")).toString(QStringLiteral("#FF5A2C")));
    const QColor colB(state.value(QStringLiteral("colorB")).toString(QStringLiteral("#4F9EFF")));
    const int clock = state.value(QStringLiteral("clockSeconds")).toInt();
    const QString ct = clockText(clock);
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
        const int boxW = int(W * 0.28);
        const int boxH = 56;
        drawTeamBlock(QRect(16, 16, boxW, boxH), a, sa, accentA, false);
        drawTeamBlock(QRect(W - boxW - 16, 16, boxW, boxH), b, sb, accentB, true);
        p.setPen(fg);
        p.setFont(QFont(QStringLiteral("JetBrains Mono"), 14, QFont::Bold));
        p.drawText(QRect(0, 20, W, 40), Qt::AlignHCenter | Qt::AlignVCenter, ct);
    } else if (layout == QLatin1String("center")) {
        const int h = 96;
        const int y = (H - h) / 2;
        const int x = int(W * 0.12);
        const int w = int(W * 0.76);
        p.setBrush(bg);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(x, y, w, h, 8, 8);
        p.fillRect(x, y, 6, h, accentA);
        p.fillRect(x + w - 6, y, 6, h, accentB);
        p.setPen(fg);
        p.setFont(QFont(QStringLiteral("Bebas Neue"), 36, QFont::Bold));
        p.drawText(QRect(x + 24, y, w / 2 - 40, h), Qt::AlignVCenter | Qt::AlignLeft,
                   QStringLiteral("%1  %2").arg(a).arg(sa));
        p.drawText(QRect(x + w / 2 + 16, y, w / 2 - 40, h), Qt::AlignVCenter | Qt::AlignRight,
                   QStringLiteral("%1  %2").arg(sb).arg(b));
        p.setFont(QFont(QStringLiteral("JetBrains Mono"), 16, QFont::Bold));
        p.setPen(accentA);
        p.drawText(QRect(x, y, w, h), Qt::AlignCenter, ct);
    } else if (layout == QLatin1String("wide")) {
        const int h = 72;
        const int y = H - h;
        p.fillRect(0, y, W, h, bg);
        p.fillRect(0, y, 6, h, accentA);
        p.fillRect(W - 6, y, 6, h, accentB);
        p.setPen(fg);
        p.setFont(QFont(QStringLiteral("Bebas Neue"), 28, QFont::Bold));
        p.drawText(QRect(24, y, W / 2 - 40, h), Qt::AlignVCenter | Qt::AlignLeft,
                   QStringLiteral("%1   %2").arg(a).arg(sa));
        p.drawText(QRect(W / 2 + 20, y, W / 2 - 40, h), Qt::AlignVCenter | Qt::AlignRight,
                   QStringLiteral("%1   %2").arg(sb).arg(b));
        p.setFont(QFont(QStringLiteral("JetBrains Mono"), 16, QFont::Bold));
        p.drawText(QRect(0, y, W, h), Qt::AlignCenter, ct);
    } else {
        const int h = 80;
        const int y = H - h;
        p.fillRect(0, y, W, h, bg);
        p.fillRect(0, y, 4, h, accentA);
        p.fillRect(W - 4, y, 4, h, accentB);
        QLinearGradient g(0, y, W, y);
        g.setColorAt(0.0, QColor(accentA.red(), accentA.green(), accentA.blue(), 55));
        g.setColorAt(0.5, Qt::transparent);
        g.setColorAt(1.0, QColor(accentB.red(), accentB.green(), accentB.blue(), 55));
        p.fillRect(0, y, W, h, g);
        p.setPen(fg);
        p.setFont(QFont(QStringLiteral("Segoe UI"), 22, QFont::Bold));
        p.drawText(QRect(20, y, W / 2 - 40, h), Qt::AlignVCenter | Qt::AlignLeft,
                   QStringLiteral("%1  %2").arg(a).arg(sa));
        p.drawText(QRect(W / 2, y, W / 2 - 20, h), Qt::AlignVCenter | Qt::AlignRight,
                   QStringLiteral("%1  %2").arg(sb).arg(b));
        p.setFont(QFont(QStringLiteral("JetBrains Mono"), 14, QFont::Bold));
        p.setPen(accentA);
        p.drawText(QRect(0, y, W, h), Qt::AlignCenter, ct);
    }
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
    p.setRenderHint(QPainter::TextAntialiasing);

    const QString sport = state.value(QStringLiteral("sport")).toString(QStringLiteral("generic"));
    if (sport == QLatin1String("8ball") || sport == QLatin1String("pool") || sport == QLatin1String("9ball")
        || sport == QLatin1String("snooker")) {
        renderBilliards(p, state, width, height);
    } else if (sport == QLatin1String("baseball")) {
        renderBaseball(p, state, width, height);
    } else if (sport == QLatin1String("basketball")) {
        renderBasketball(p, state, width, height);
    } else if (sport == QLatin1String("soccer")) {
        renderSoccer(p, state, width, height);
    } else if (sport == QLatin1String("tennis")) {
        renderTennis(p, state, width, height);
    } else {
        renderGeneric(p, state, width, height);
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
