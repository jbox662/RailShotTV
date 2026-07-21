#include "capture/OverlayRenderer.h"
#include "scoreboard/ScoreboardModel.h"
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
/// Dedicated scoreboard textures: use the requested board size (centered), never
/// inflate to the full image — that stretched bars/fonts and clipped race text.
QRect placeBoard(const QString& layout, int W, int H, int bw, int bh)
{
    if (H <= 320) {
        const int padX = 10;
        const int w = qBound(1, bw, W - 2 * padX);
        const int h = qBound(1, bh, H - 8);
        return QRect((W - w) / 2, (H - h) / 2, w, h);
    }
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

/// Draw text clipped to `box`, shrinking from maxPx until it fits (no overflow).
void drawTextInBox(QPainter& p, const QRect& box, const QString& text,
                   const QString& family, int maxPx, int minPx,
                   const QColor& color, int align = Qt::AlignCenter)
{
    if (box.width() < 2 || box.height() < 2 || text.isEmpty())
        return;
    p.save();
    p.setClipRect(box);
    p.setPen(color);
    QFont f(family);
    f.setBold(true);
    f.setStyleStrategy(QFont::PreferAntialias);
    int px = qMax(minPx, maxPx);
    for (; px >= minPx; --px) {
        f.setPixelSize(px);
        p.setFont(f);
        const QRect br = p.fontMetrics().boundingRect(box, align | Qt::TextSingleLine, text);
        if (br.width() <= box.width() && br.height() <= box.height())
            break;
    }
    if (px < minPx) {
        f.setPixelSize(minPx);
        p.setFont(f);
    }
    p.drawText(box, align | Qt::TextSingleLine, text);
    p.restore();
}

/// Standard race tab: "RACE TO" label over a fitted race number — always inside `raceR`.
void drawRaceToTab(QPainter& p, const QRect& raceR, int race,
                   const QColor& labelColor, const QColor& numberColor,
                   bool lightShell = false)
{
    if (raceR.height() < 20)
        return;
    if (lightShell) {
        p.setBrush(QColor(248, 250, 252));
        p.setPen(QPen(QColor(200, 205, 215), 1));
        p.drawRoundedRect(raceR, 4, 4);
    }
    const int pad = 4;
    const int labelH = qBound(12, raceR.height() / 3, 16);
    const QRect labelR(raceR.left() + pad, raceR.top() + pad, raceR.width() - 2 * pad, labelH);
    const QRect numR(raceR.left() + pad, labelR.bottom() + 1,
                     raceR.width() - 2 * pad, raceR.bottom() - pad - (labelR.bottom() + 1));
    drawTextInBox(p, labelR, QStringLiteral("RACE TO"), QStringLiteral("DM Sans"),
                  11, 7, labelColor);
    drawTextInBox(p, numR, QString::number(race), QStringLiteral("Bebas Neue"),
                  qMin(28, numR.height()), 12, numberColor);
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

QColor poolBallColor(int n)
{
    static const QColor k[] = {
        QColor(0, 0, 0),
        QColor(242, 196, 28),  // 1
        QColor(30, 96, 210),   // 2
        QColor(210, 40, 45),   // 3
        QColor(110, 40, 160),  // 4
        QColor(230, 120, 20),  // 5
        QColor(30, 140, 55),   // 6
        QColor(130, 30, 40),   // 7
        QColor(18, 18, 20),    // 8
    };
    const int base = (n <= 8) ? n : (n - 8);
    return k[qBound(1, base, 8)];
}

void drawPoolBall(QPainter& p, const QPointF& c, qreal r, int number, bool pocketed)
{
    const bool stripe = number >= 9 && number <= 15;
    const QColor col = poolBallColor(number);
    p.setPen(QPen(QColor(0, 0, 0, pocketed ? 60 : 160), 1));
    if (stripe) {
        p.setBrush(Qt::white);
        p.drawEllipse(c, r, r);
        p.setBrush(pocketed ? QColor(col.red(), col.green(), col.blue(), 90) : col);
        p.setPen(Qt::NoPen);
        p.drawRect(QRectF(c.x() - r, c.y() - r * 0.38, r * 2, r * 0.76));
        p.setPen(QPen(QColor(0, 0, 0, pocketed ? 60 : 160), 1));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(c, r, r);
    } else if (number == 8) {
        p.setBrush(pocketed ? QColor(18, 18, 20, 100) : QColor(18, 18, 20));
        p.drawEllipse(c, r, r);
    } else {
        p.setBrush(pocketed ? QColor(col.red(), col.green(), col.blue(), 90) : col);
        p.drawEllipse(c, r, r);
    }
    // Number disc
    const qreal nr = r * (stripe || number == 8 ? 0.42 : 0.38);
    p.setPen(Qt::NoPen);
    p.setBrush(pocketed ? QColor(255, 255, 255, 120) : Qt::white);
    p.drawEllipse(c, nr, nr);
    p.setPen(pocketed ? QColor(40, 40, 40, 120) : QColor(20, 20, 24));
    p.setFont(QFont(QStringLiteral("Segoe UI"), qMax(6, int(r * 0.7)), QFont::Bold));
    p.drawText(QRectF(c.x() - r, c.y() - r, r * 2, r * 2), Qt::AlignCenter, QString::number(number));
}

void drawBallRackStrip(QPainter& p, const QRect& strip, int pocketedMask, int ballCount = 15)
{
    const int n = qBound(1, ballCount, 15);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(245, 247, 250));
    p.drawRoundedRect(strip, 2, 2);
    const qreal gap = 5;
    const qreal usableW = strip.width() - gap * (n + 1);
    // Keep balls circular: diameter limited by strip height and available width
    const qreal ballD = qMin(strip.height() - 6.0, usableW / n);
    const qreal r = ballD * 0.5;
    const qreal span = n * ballD + (n - 1) * gap;
    const qreal startX = strip.center().x() - span * 0.5 + r;
    const qreal cy = strip.center().y();
    for (int i = 1; i <= n; ++i) {
        const bool pocketed = (pocketedMask & (1 << (i - 1))) != 0;
        drawPoolBall(p, QPointF(startX + (i - 1) * (ballD + gap), cy), r, i, pocketed);
    }
}

void drawActiveChevron(QPainter& p, const QPointF& tip, bool pointRight, const QColor& col)
{
    QPolygonF poly;
    if (pointRight) {
        poly << tip << QPointF(tip.x() - 10, tip.y() - 8) << QPointF(tip.x() - 10, tip.y() + 8);
    } else {
        poly << tip << QPointF(tip.x() + 10, tip.y() - 8) << QPointF(tip.x() + 10, tip.y() + 8);
    }
    p.setPen(Qt::NoPen);
    p.setBrush(col);
    p.drawPolygon(poly);
}

// Mosconi / match-style: dark wings, white score tiles, Race-to tab, ball rack
void renderBilliardsMosconi(QPainter& p, const QJsonObject& state, int W, int H)
{
    const QString layout = state.value(QStringLiteral("layout")).toString(QStringLiteral("standard"));
    const QString a = shortName(state.value(QStringLiteral("playerA")).toString(QStringLiteral("Player A")), 16);
    const QString b = shortName(state.value(QStringLiteral("playerB")).toString(QStringLiteral("Player B")), 16);
    const int sa = state.value(QStringLiteral("scoreA")).toInt();
    const int sb = state.value(QStringLiteral("scoreB")).toInt();
    const int race = state.value(QStringLiteral("raceTo")).toInt(7);
    const int active = state.value(QStringLiteral("activeSide")).toInt(1);
    const int pocketed = state.value(QStringLiteral("pocketedMask")).toInt();
    const int ballCount = poolObjectBallCount(state.value(QStringLiteral("sport")).toString(QStringLiteral("8ball")));
    QColor wingA(state.value(QStringLiteral("colorA")).toString(QStringLiteral("#1B3A6B")));
    QColor wingB(state.value(QStringLiteral("colorB")).toString(QStringLiteral("#2A1F4D")));
    if (!wingA.isValid()) wingA = QColor(27, 58, 107);
    if (!wingB.isValid()) wingB = QColor(42, 31, 77);

    // Fixed proportions — never derive bar height from a stretched texture
    const QString theme = state.value(QStringLiteral("theme")).toString(QStringLiteral("broadcast"));
    const QString sport = state.value(QStringLiteral("sport")).toString(QStringLiteral("8ball"));
    const bool showRack = scoreboardShowsBallRack(sport, theme, layout);
    constexpr int mainH = 58;
    constexpr int rackH = 36;
    constexpr int scoreBox = 46;
    constexpr int raceW = 120;
    constexpr int gap = 8;
    const int bh = mainH + (showRack ? rackH : 0);
    const int bw = layout == QLatin1String("wide") ? (W - 20) : qMin(1180, int(W * 0.86));
    const QRect board = placeBoard(layout, W, H, bw, bh);
    const QRect main(board.left(), board.top(), board.width(), mainH);

    p.setRenderHint(QPainter::Antialiasing, true);

    QPainterPath shell;
    shell.addRoundedRect(main, 6, 6);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(12, 14, 18));
    p.drawPath(shell);
    p.setClipPath(shell);

    const int mid = main.center().x();
    const QRect raceR(mid - raceW / 2, main.top() + 5, raceW, mainH - 10);
    const QRect scoreAR(raceR.left() - gap - scoreBox, main.top() + (mainH - scoreBox) / 2, scoreBox, scoreBox);
    const QRect scoreBR(raceR.right() + gap + 1, scoreAR.top(), scoreBox, scoreBox);
    const QRect leftWing(main.left(), main.top(), scoreAR.left() - main.left() - 4, mainH);
    const QRect rightWing(scoreBR.right() + 5, main.top(), main.right() - scoreBR.right() - 4, mainH);

    p.setBrush(wingA);
    p.drawRect(leftWing);
    p.setBrush(wingB);
    p.drawRect(rightWing);

    QLinearGradient lg(main.topLeft(), main.bottomLeft());
    lg.setColorAt(0, QColor(255, 255, 255, 16));
    lg.setColorAt(1, QColor(0, 0, 0, 36));
    p.fillRect(main, lg);

    p.setBrush(Qt::white);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(scoreAR, 3, 3);
    p.drawRoundedRect(scoreBR, 3, 3);
    drawTextInBox(p, scoreAR.adjusted(2, 2, -2, -2), QString::number(sa),
                  QStringLiteral("Bebas Neue"), 28, 14, QColor(15, 18, 24));
    drawTextInBox(p, scoreBR.adjusted(2, 2, -2, -2), QString::number(sb),
                  QStringLiteral("Bebas Neue"), 28, 14, QColor(15, 18, 24));

    drawRaceToTab(p, raceR, race, QColor(30, 34, 42), QColor(30, 34, 42), true);

    p.setPen(Qt::white);
    p.setFont(QFont(QStringLiteral("DM Sans"), 15, QFont::Bold));
    p.drawText(leftWing.adjusted(18, 0, -8, 0), Qt::AlignVCenter | Qt::AlignLeft, a.toUpper());
    p.drawText(rightWing.adjusted(8, 0, -18, 0), Qt::AlignVCenter | Qt::AlignRight, b.toUpper());

    if (active == 1)
        drawActiveChevron(p, QPointF(scoreAR.left() - 3, main.center().y()), true, Qt::white);
    else if (active == 2)
        drawActiveChevron(p, QPointF(scoreBR.right() + 3, main.center().y()), false, Qt::white);

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 255, 220));
    p.drawEllipse(QPointF(leftWing.left() + 10, main.center().y()), 3.5, 3.5);
    p.drawEllipse(QPointF(rightWing.right() - 10, main.center().y()), 3.5, 3.5);

    p.setClipping(false);

    if (showRack) {
        const QRect strip(main.left() + 12, main.bottom() + 4, main.width() - 24, rackH - 6);
        drawBallRackStrip(p, strip, pocketed, ballCount);
    }
}

// Clean navy: name | white score | RACE TO | white score | name
void renderBilliardsClean(QPainter& p, const QJsonObject& state, int W, int H)
{
    const QString layout = state.value(QStringLiteral("layout")).toString(QStringLiteral("standard"));
    const QString a = shortName(state.value(QStringLiteral("playerA")).toString(QStringLiteral("Team A")), 14);
    const QString b = shortName(state.value(QStringLiteral("playerB")).toString(QStringLiteral("Team B")), 14);
    const int sa = state.value(QStringLiteral("scoreA")).toInt();
    const int sb = state.value(QStringLiteral("scoreB")).toInt();
    const int race = state.value(QStringLiteral("raceTo")).toInt(7);
    const int active = state.value(QStringLiteral("activeSide")).toInt(1);
    QColor navy(state.value(QStringLiteral("bgColor")).toString());
    if (!navy.isValid()) navy = QColor(18, 28, 48, 245);
    QColor mid(state.value(QStringLiteral("colorA")).toString(QStringLiteral("#3B6EA5")));
    if (!mid.isValid()) mid = QColor(59, 110, 165);

    const int barH = 58;
    const int bh = barH + 10;
    const int bw = layout == QLatin1String("wide") ? (W - 20) : qMin(1000, int(W * 0.78));
    const QRect board = placeBoard(layout, W, H, bw, bh);
    const QRect bar(board.left(), board.top(), board.width(), barH);

    constexpr int scoreBox = 46;
    constexpr int raceW = 108;
    const int midX = bar.center().x();
    const int nameW = (bar.width() - raceW - scoreBox * 2 - 8) / 2;
    const QRect leftName(bar.left(), bar.top(), nameW, barH);
    const QRect scoreA(leftName.right() + 2, bar.top() + (barH - scoreBox) / 2, scoreBox, scoreBox);
    const QRect raceR(midX - raceW / 2, bar.top(), raceW, barH);
    const QRect scoreB(raceR.right() + 2, scoreA.top(), scoreBox, scoreBox);
    const QRect rightName(scoreB.right() + 2, bar.top(), bar.right() - scoreB.right(), barH);

    p.setPen(Qt::NoPen);
    p.setBrush(navy);
    p.drawRoundedRect(bar, 8, 8);
    p.setBrush(mid);
    p.drawRect(raceR);
    // clip race edges into bar
    p.setBrush(navy);
    p.drawRect(leftName);
    p.drawRect(rightName);
    p.setBrush(mid);
    p.drawRect(raceR);

    p.setBrush(Qt::white);
    p.drawRoundedRect(scoreA, 2, 2);
    p.drawRoundedRect(scoreB, 2, 2);

    p.setBrush(Qt::white);
    p.drawRect(QRect(leftName.left() + 20, bar.bottom() + 2, leftName.width() - 40, 4));
    p.drawRect(QRect(rightName.left() + 20, bar.bottom() + 2, rightName.width() - 40, 4));

    p.setPen(Qt::white);
    p.setFont(QFont(QStringLiteral("DM Sans"), 14, QFont::Bold));
    p.drawText(leftName, Qt::AlignCenter, a);
    p.drawText(rightName, Qt::AlignCenter, b);
    drawRaceToTab(p, raceR.adjusted(4, 4, -4, -4), race, Qt::white, Qt::white, false);
    drawTextInBox(p, scoreA.adjusted(2, 2, -2, -2), QString::number(sa),
                  QStringLiteral("Bebas Neue"), 26, 14, QColor(15, 18, 24));
    drawTextInBox(p, scoreB.adjusted(2, 2, -2, -2), QString::number(sb),
                  QStringLiteral("Bebas Neue"), 26, 14, QColor(15, 18, 24));

    if (active == 1) {
        p.setBrush(Qt::white);
        p.setPen(Qt::NoPen);
        p.drawRect(QRect(leftName.left(), leftName.top(), 4, leftName.height()));
    } else if (active == 2) {
        p.setBrush(Qt::white);
        p.setPen(Qt::NoPen);
        p.drawRect(QRect(rightName.right() - 3, rightName.top(), 4, rightName.height()));
    }
}

// Snooker-ish: names + white score chips + frames center + thin stats bar
void renderBilliardsSnooker(QPainter& p, const QJsonObject& state, int W, int H)
{
    const QString layout = state.value(QStringLiteral("layout")).toString(QStringLiteral("standard"));
    const QString a = shortName(state.value(QStringLiteral("playerA")).toString(QStringLiteral("Player A")), 14);
    const QString b = shortName(state.value(QStringLiteral("playerB")).toString(QStringLiteral("Player B")), 14);
    const int sa = state.value(QStringLiteral("scoreA")).toInt();
    const int sb = state.value(QStringLiteral("scoreB")).toInt();
    const int race = state.value(QStringLiteral("raceTo")).toInt(7);
    const int active = state.value(QStringLiteral("activeSide")).toInt(1);
    const QColor bg = QColor(28, 30, 34, 245);
    const QColor fg = Qt::white;

    constexpr int mainH = 48;
    constexpr int subH = 26;
    constexpr int chip = 40;
    const int bh = mainH + subH;
    const int bw = layout == QLatin1String("wide") ? (W - 20) : qMin(860, int(W * 0.68));
    const QRect board = placeBoard(layout, W, H, bw, bh);
    const QRect main(board.left(), board.top(), board.width(), mainH);
    const QRect sub(board.left() + 20, main.bottom(), board.width() - 40, subH);

    p.setPen(Qt::NoPen);
    p.setBrush(bg);
    p.drawRoundedRect(main, 6, 6);
    p.drawRoundedRect(sub, 0, 0);

    const int mid = main.center().x();
    constexpr int raceGap = 56;
    const QRect chipA(mid - raceGap / 2 - chip - 6, main.top() + (mainH - chip) / 2, chip, chip);
    const QRect chipB(mid + raceGap / 2 + 6, chipA.top(), chip, chip);
    const QRect raceMid(chipA.right(), main.top(), chipB.left() - chipA.right(), mainH);

    p.setBrush(Qt::white);
    p.drawRoundedRect(chipA, 3, 3);
    p.drawRoundedRect(chipB, 3, 3);
    drawTextInBox(p, chipA.adjusted(2, 2, -2, -2), QString::number(sa),
                  QStringLiteral("Bebas Neue"), 24, 12, QColor(20, 22, 28));
    drawTextInBox(p, chipB.adjusted(2, 2, -2, -2), QString::number(sb),
                  QStringLiteral("Bebas Neue"), 24, 12, QColor(20, 22, 28));

    drawTextInBox(p, QRect(raceMid.left(), raceMid.top() + 4, raceMid.width(), 14),
                  QStringLiteral("RACE"), QStringLiteral("DM Sans"), 10, 7, fg);
    drawTextInBox(p, QRect(raceMid.left(), raceMid.top() + 18, raceMid.width(), raceMid.height() - 22),
                  QString::number(race), QStringLiteral("Bebas Neue"), 18, 11, fg);

    p.setPen(fg);
    p.setFont(QFont(QStringLiteral("DM Sans"), 13, QFont::Bold));
    p.drawText(QRect(main.left() + 16, main.top(), chipA.left() - main.left() - 24, mainH),
               Qt::AlignVCenter | Qt::AlignLeft, a);
    p.drawText(QRect(chipB.right() + 12, main.top(), main.right() - chipB.right() - 28, mainH),
               Qt::AlignVCenter | Qt::AlignRight, b);

    if (active == 1) {
        p.setBrush(QColor(220, 50, 50));
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPointF(main.left() + 10, main.center().y()), 4, 4);
    } else if (active == 2) {
        p.setBrush(QColor(220, 50, 50));
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPointF(main.right() - 10, main.center().y()), 4, 4);
    }

    p.setPen(QColor(200, 205, 215));
    p.setFont(QFont(QStringLiteral("DM Sans"), 9, QFont::Bold));
    p.drawText(sub, Qt::AlignCenter, QStringLiteral("Race to %1  ·  frames").arg(race));
}

// Modern slanted translucent wings
void renderBilliardsSlant(QPainter& p, const QJsonObject& state, int W, int H)
{
    const QString layout = state.value(QStringLiteral("layout")).toString(QStringLiteral("standard"));
    const QString a = shortName(state.value(QStringLiteral("playerA")).toString(QStringLiteral("Team A")), 12);
    const QString b = shortName(state.value(QStringLiteral("playerB")).toString(QStringLiteral("Team B")), 12);
    const int sa = state.value(QStringLiteral("scoreA")).toInt();
    const int sb = state.value(QStringLiteral("scoreB")).toInt();
    const int race = state.value(QStringLiteral("raceTo")).toInt(7);
    const int active = state.value(QStringLiteral("activeSide")).toInt(1);
    QColor panel(state.value(QStringLiteral("bgColor")).toString());
    if (!panel.isValid()) panel = QColor(120, 200, 200, 200);
    QColor ink(state.value(QStringLiteral("textColor")).toString());
    if (!ink.isValid()) ink = QColor(20, 40, 50);
    QColor shield(state.value(QStringLiteral("colorA")).toString(QStringLiteral("#1E3A5F")));
    if (!shield.isValid()) shield = QColor(30, 50, 80);

    constexpr int barH = 64;
    constexpr int scoreBox = 44;
    const int bw = layout == QLatin1String("wide") ? (W - 20) : qMin(980, int(W * 0.78));
    const QRect board = placeBoard(layout, W, H, bw, barH);
    const int mid = board.center().x();
    const int half = (board.width() - 56) / 2;

    auto slant = [&](const QRect& r, bool leanRight) {
        QPolygonF poly;
        if (leanRight) {
            poly << r.topLeft() << QPointF(r.right() - 18, r.top()) << r.bottomRight() << r.bottomLeft();
        } else {
            poly << QPointF(r.left() + 18, r.top()) << r.topRight() << r.bottomRight() << r.bottomLeft();
        }
        p.setPen(Qt::NoPen);
        p.setBrush(panel);
        p.drawPolygon(poly);
    };

    const QRect leftR(board.left(), board.top() + 6, half, barH - 12);
    const QRect rightR(board.right() - half, board.top() + 6, half, barH - 12);
    slant(leftR, true);
    slant(rightR, false);

    // Center shield with race number
    QPainterPath sh;
    const QRect sc(mid - 24, board.top() + 2, 48, barH - 4);
    sh.moveTo(sc.center().x(), sc.top());
    sh.lineTo(sc.right(), sc.top() + 10);
    sh.lineTo(sc.right() - 4, sc.bottom() - 6);
    sh.lineTo(sc.center().x(), sc.bottom());
    sh.lineTo(sc.left() + 4, sc.bottom() - 6);
    sh.lineTo(sc.left(), sc.top() + 10);
    sh.closeSubpath();
    p.setBrush(shield);
    p.setPen(Qt::NoPen);
    p.drawPath(sh);
    p.setPen(Qt::white);
    p.setFont(QFont(QStringLiteral("DM Sans"), 8, QFont::Bold));
    p.drawText(QRect(sc.left(), sc.top() + 8, sc.width(), 12), Qt::AlignCenter, QStringLiteral("RACE"));
    drawTextInBox(p, QRect(sc.left() + 4, sc.top() + 20, sc.width() - 8, sc.height() - 28),
                  QString::number(race), QStringLiteral("Bebas Neue"), 18, 11, Qt::white);

    // White score tiles
    const QRect scoreAR(leftR.right() - scoreBox - 10, leftR.top() + (leftR.height() - scoreBox) / 2,
                        scoreBox, scoreBox);
    const QRect scoreBR(rightR.left() + 10, scoreAR.top(), scoreBox, scoreBox);
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::white);
    p.drawRoundedRect(scoreAR, 3, 3);
    p.drawRoundedRect(scoreBR, 3, 3);
    drawTextInBox(p, scoreAR.adjusted(2, 2, -2, -2), QString::number(sa),
                  QStringLiteral("Bebas Neue"), 26, 14, QColor(15, 18, 24));
    drawTextInBox(p, scoreBR.adjusted(2, 2, -2, -2), QString::number(sb),
                  QStringLiteral("Bebas Neue"), 26, 14, QColor(15, 18, 24));

    p.setPen(ink);
    p.setFont(QFont(QStringLiteral("DM Sans"), 13, QFont::Bold));
    p.drawText(leftR.adjusted(16, 0, -scoreBox - 16, 0), Qt::AlignVCenter | Qt::AlignLeft, a);
    p.drawText(rightR.adjusted(scoreBox + 16, 0, -16, 0), Qt::AlignVCenter | Qt::AlignRight, b);

    if (active == 1) {
        p.setBrush(shield);
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPointF(leftR.left() + 10, leftR.center().y()), 4, 4);
    } else if (active == 2) {
        p.setBrush(shield);
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPointF(rightR.right() - 10, rightR.center().y()), 4, 4);
    }
}

// Felt / neon — names + white score tiles + race pill + optional rack
void renderBilliardsFelt(QPainter& p, const QJsonObject& state, int W, int H)
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
    const int pocketed = state.value(QStringLiteral("pocketedMask")).toInt();
    const int ballCount = poolObjectBallCount(state.value(QStringLiteral("sport")).toString(QStringLiteral("8ball")));
    QColor accentA(state.value(QStringLiteral("colorA")).toString(QStringLiteral("#FF5A2C")));
    QColor accentB(state.value(QStringLiteral("colorB")).toString(QStringLiteral("#4F9EFF")));
    if (!accentA.isValid()) accentA = QColor(QStringLiteral("#FF5A2C"));
    if (!accentB.isValid()) accentB = QColor(QStringLiteral("#4F9EFF"));
    const QColor fg = resolveFg(state, theme);
    const bool neon = theme == QLatin1String("neon");
    const QColor felt(16, 90, 48);
    QColor bg = resolveBg(state, theme);
    if (state.value(QStringLiteral("bgColor")).toString().isEmpty())
        bg = neon ? QColor(8, 4, 20, 240) : QColor(10, 18, 14, 240);

    const bool showRack = scoreboardShowsBallRack(
        state.value(QStringLiteral("sport")).toString(QStringLiteral("8ball")), theme, layout);
    constexpr int mainH = 72;
    constexpr int rackHFixed = 34;
    constexpr int scoreBox = 48;
    constexpr int raceW = 110;
    const int bh = mainH + (showRack ? rackHFixed : 0);
    const int bw = layout == QLatin1String("wide") ? (W - 20) : qMin(1180, int(W * 0.88));
    const QRect board = placeBoard(layout, W, H, bw, bh);
    const int rackH = showRack ? rackHFixed : 0;
    const QRect main(board.left(), board.top(), board.width(), mainH);

    drawRoundRect(p, main, 8, bg, neon ? accentA : QColor(255, 255, 255, 28));
    p.fillRect(QRect(main.left() + 8, main.top() + 4, main.width() - 16, 3), felt);

    const int midX = main.center().x();
    const QRect raceR(midX - raceW / 2, main.top() + 8, raceW, mainH - 16);
    const QRect scoreAR(raceR.left() - scoreBox - 8, main.top() + (mainH - scoreBox) / 2, scoreBox, scoreBox);
    const QRect scoreBR(raceR.right() + 8, scoreAR.top(), scoreBox, scoreBox);
    const QRect leftR(main.left() + 10, main.top() + 8, scoreAR.left() - main.left() - 16, mainH - 16);
    const QRect rightR(scoreBR.right() + 8, main.top() + 8, main.right() - scoreBR.right() - 18, mainH - 16);

    if (active == 1)
        p.fillRect(QRect(leftR.left(), leftR.top(), 4, leftR.height()), accentA);
    if (active == 2)
        p.fillRect(QRect(rightR.right() - 3, rightR.top(), 4, rightR.height()), accentB);

    // Race pill — label + fitted number only (no 8-ball; it stole height and overflowed).
    drawRoundRect(p, raceR, 6, QColor(0, 0, 0, 180), felt);
    {
        p.save();
        p.setClipRect(raceR);
        drawRaceToTab(p, raceR.adjusted(2, 2, -2, -2), race,
                      felt.lighter(160), fg, false);
        p.restore();
    }

    // White score tiles
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::white);
    p.drawRoundedRect(scoreAR, 4, 4);
    p.drawRoundedRect(scoreBR, 4, 4);
    drawTextInBox(p, scoreAR.adjusted(2, 2, -2, -2), QString::number(sa),
                  QStringLiteral("Bebas Neue"), 28, 14, QColor(15, 18, 24));
    drawTextInBox(p, scoreBR.adjusted(2, 2, -2, -2), QString::number(sb),
                  QStringLiteral("Bebas Neue"), 28, 14, QColor(15, 18, 24));

    p.setPen(fg);
    p.setFont(QFont(QStringLiteral("DM Sans"), 13, QFont::Bold));
    p.drawText(leftR.adjusted(10, 0, -4, 0), Qt::AlignVCenter | Qt::AlignLeft, a.toUpper());
    p.drawText(rightR.adjusted(4, 0, -10, 0), Qt::AlignVCenter | Qt::AlignRight, b.toUpper());

    if (clock > 0 || state.value(QStringLiteral("clockRunning")).toBool()) {
        p.setPen(QColor(134, 239, 172));
        p.setFont(QFont(QStringLiteral("JetBrains Mono"), 8, QFont::Bold));
        p.drawText(QRect(main.left(), main.bottom() - 14, main.width(), 12), Qt::AlignCenter,
                   QStringLiteral("SHOT %1").arg(clockText(clock)));
    }
    if (showRack && rackH > 8) {
        drawBallRackStrip(p, QRect(main.left() + 8, main.bottom() + 2, main.width() - 16, rackH - 4),
                          pocketed, ballCount);
    }
}

// ── Billiards / pool (primary) ───────────────────────────────────
void renderBilliards(QPainter& p, const QJsonObject& state, int W, int H)
{
    const QString theme = state.value(QStringLiteral("theme")).toString(QStringLiteral("railshot"));
    const QString layout = state.value(QStringLiteral("layout")).toString(QStringLiteral("standard"));
    // Center layout → snooker-style dual bar (names + white score chips)
    if (layout == QLatin1String("center"))
        renderBilliardsSnooker(p, state, W, H);
    else if (theme == QLatin1String("broadcast") || theme == QLatin1String("gold"))
        renderBilliardsMosconi(p, state, W, H);
    else if (theme == QLatin1String("carbon"))
        renderBilliardsClean(p, state, W, H);
    else if (theme == QLatin1String("classic"))
        renderBilliardsSlant(p, state, W, H);
    else
        renderBilliardsFelt(p, state, W, H);
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
    const int bw = layout == QLatin1String("wide") ? (W - 20) : qMin(820, int(W * 0.62));
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
    const int bw = layout == QLatin1String("wide") ? (W - 20) : qMin(1000, int(W * 0.78));
    const QRect board = placeBoard(layout, W, H, bw, bh);
    drawRoundRect(p, board, 4, bg, QColor(255, 255, 255, 25));

    constexpr int scoreBox = 48;
    const int mid = board.center().x();
    constexpr int clockW = 100;
    const QRect clockR(mid - clockW / 2, board.top() + 8, clockW, board.height() - 16);
    const QRect scoreAR(clockR.left() - 10 - scoreBox, board.top() + (board.height() - scoreBox) / 2,
                        scoreBox, scoreBox);
    const QRect scoreBR(clockR.right() + 10, scoreAR.top(), scoreBox, scoreBox);
    const QRect leftName(board.left() + 12, board.top(), scoreAR.left() - board.left() - 16, board.height());
    const QRect rightName(scoreBR.right() + 12, board.top(), board.right() - scoreBR.right() - 16, board.height());

    p.fillRect(QRect(board.left(), board.top(), 5, board.height()), accentA);
    p.fillRect(QRect(board.right() - 5, board.top(), 5, board.height()), accentB);

    p.setPen(Qt::NoPen);
    p.setBrush(Qt::white);
    p.drawRoundedRect(scoreAR, 3, 3);
    p.drawRoundedRect(scoreBR, 3, 3);
    drawTextInBox(p, scoreAR.adjusted(2, 2, -2, -2), QString::number(sa),
                  QStringLiteral("Bebas Neue"), 28, 14, QColor(15, 18, 24));
    drawTextInBox(p, scoreBR.adjusted(2, 2, -2, -2), QString::number(sb),
                  QStringLiteral("Bebas Neue"), 28, 14, QColor(15, 18, 24));

    p.setPen(accentA);
    p.setFont(QFont(QStringLiteral("DM Sans"), 11, QFont::Bold));
    p.drawText(QRect(clockR.left(), clockR.top(), clockR.width(), 18), Qt::AlignCenter,
               QStringLiteral("Q%1").arg(period));
    p.setPen(fg);
    p.setFont(QFont(QStringLiteral("JetBrains Mono"), 16, QFont::Bold));
    p.drawText(QRect(clockR.left(), clockR.top() + 22, clockR.width(), 28), Qt::AlignCenter, clockText(clock));

    p.setFont(QFont(QStringLiteral("DM Sans"), 14, QFont::Bold));
    p.drawText(leftName.adjusted(8, 0, 0, 0), Qt::AlignVCenter | Qt::AlignLeft, a.toUpper());
    p.drawText(rightName.adjusted(0, 0, -8, 0), Qt::AlignVCenter | Qt::AlignRight, b.toUpper());
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
    const int bw = layout == QLatin1String("wide") ? (W - 20) : qMin(960, int(W * 0.75));
    const QRect board = placeBoard(layout, W, H, bw, bh);
    drawRoundRect(p, board, 4, bg, QColor(255, 255, 255, 22));
    p.fillRect(QRect(board.left(), board.top(), board.width(), 3), accentA);

    constexpr int scoreBox = 44;
    const int mid = board.center().x();
    constexpr int midGap = 72;
    const QRect scoreAR(mid - midGap / 2 - scoreBox, board.top() + (bh - scoreBox) / 2, scoreBox, scoreBox);
    const QRect scoreBR(mid + midGap / 2, scoreAR.top(), scoreBox, scoreBox);
    const QRect leftName(board.left() + 12, board.top(), scoreAR.left() - board.left() - 16, bh);
    const QRect rightName(scoreBR.right() + 12, board.top(), board.right() - scoreBR.right() - 16, bh);

    p.setPen(Qt::NoPen);
    p.setBrush(Qt::white);
    p.drawRoundedRect(scoreAR, 3, 3);
    p.drawRoundedRect(scoreBR, 3, 3);
    drawTextInBox(p, scoreAR.adjusted(2, 2, -2, -2), QString::number(sa),
                  QStringLiteral("Bebas Neue"), 26, 14, QColor(15, 18, 24));
    drawTextInBox(p, scoreBR.adjusted(2, 2, -2, -2), QString::number(sb),
                  QStringLiteral("Bebas Neue"), 26, 14, QColor(15, 18, 24));

    p.setPen(fg);
    p.setFont(QFont(QStringLiteral("DM Sans"), 13, QFont::Bold));
    p.drawText(leftName, Qt::AlignVCenter | Qt::AlignLeft, a.toUpper());
    p.drawText(rightName, Qt::AlignVCenter | Qt::AlignRight, b.toUpper());

    // Minute between score tiles
    const QString minute = QStringLiteral("%1'").arg(mins);
    const QRect badge(mid - 26, board.top() + (bh - 20) / 2, 52, 20);
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
    const int bw = layout == QLatin1String("wide") ? (W - 20) : qMin(720, int(W * 0.55));
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
    const QString a = shortName(state.value(QStringLiteral("playerA")).toString(QStringLiteral("A")), 14);
    const QString b = shortName(state.value(QStringLiteral("playerB")).toString(QStringLiteral("B")), 14);
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

    constexpr int barH = 72;
    constexpr int scoreBox = 48;
    const int bw = layout == QLatin1String("wide") ? (W - 20)
                 : layout == QLatin1String("compact") ? qMin(720, int(W * 0.55))
                                                      : qMin(1100, int(W * 0.82));
    const QRect board = placeBoard(layout, W, H, bw, barH);

    drawRoundRect(p, board, 6, bg, neon ? accentA : QColor(255, 255, 255, 28));
    p.fillRect(QRect(board.left(), board.top(), 5, board.height()), accentA);
    p.fillRect(QRect(board.right() - 5, board.top(), 5, board.height()), accentB);

    const int mid = board.center().x();
    constexpr int clockW = 90;
    const QRect clockR(mid - clockW / 2, board.top() + 8, clockW, board.height() - 16);
    const QRect scoreAR(clockR.left() - 8 - scoreBox, board.top() + (barH - scoreBox) / 2, scoreBox, scoreBox);
    const QRect scoreBR(clockR.right() + 8, scoreAR.top(), scoreBox, scoreBox);
    const QRect leftName(board.left() + 14, board.top(), scoreAR.left() - board.left() - 18, barH);
    const QRect rightName(scoreBR.right() + 14, board.top(), board.right() - scoreBR.right() - 18, barH);

    p.setPen(Qt::NoPen);
    p.setBrush(Qt::white);
    p.drawRoundedRect(scoreAR, 3, 3);
    p.drawRoundedRect(scoreBR, 3, 3);
    drawTextInBox(p, scoreAR.adjusted(2, 2, -2, -2), QString::number(sa),
                  QStringLiteral("Bebas Neue"), 28, 14, QColor(15, 18, 24));
    drawTextInBox(p, scoreBR.adjusted(2, 2, -2, -2), QString::number(sb),
                  QStringLiteral("Bebas Neue"), 28, 14, QColor(15, 18, 24));

    p.setPen(fg);
    p.setFont(QFont(QStringLiteral("DM Sans"), 14, QFont::Bold));
    p.drawText(leftName, Qt::AlignVCenter | Qt::AlignLeft, a.toUpper());
    p.drawText(rightName, Qt::AlignVCenter | Qt::AlignRight, b.toUpper());

    p.setPen(accentA);
    p.setFont(QFont(QStringLiteral("JetBrains Mono"), 14, QFont::Bold));
    p.drawText(clockR, Qt::AlignCenter, ct);
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
        || sport == QLatin1String("10ball")
        || sport == QLatin1String("snooker") || sport == QLatin1String("straight")
        || sport == QLatin1String("onepocket")) {
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
