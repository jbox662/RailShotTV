#pragma once

#include <QEasingCurve>

class QWidget;

namespace railshot {
namespace motion {

/** Chromatic Command §1.6 cubic-bezier(0.23, 1, 0.32, 1). */
QEasingCurve themeEasing();

/** live-pulse: opacity 1.0 ↔ 0.3 over periodMs (default 1800). */
void pulseOpacity(QWidget* widget, int periodMs = 1800);

/** Stop pulseOpacity / live ripple on widget (and children for ripple host). */
void stopPulse(QWidget* widget);

/**
 * liveRipple: expanding ring under host (sidebar LIVE block), 1.4s loop.
 * Call when going live; stopLiveRipple when leaving live.
 */
void startLiveRipple(QWidget* host);
void stopLiveRipple(QWidget* host);

/** live-border-in: reveal top live border with scaleX-like width anim (400ms). */
void animateLiveBorderIn(QWidget* border);

/** Modal in: opacity + translateY 8→0 over 220ms. */
void playModalEnter(QWidget* dialog);

/** Button press: brief 0.96 scale via geometry nudge (120ms). */
void installPressScale(QWidget* widget);

} // namespace motion
} // namespace railshot
