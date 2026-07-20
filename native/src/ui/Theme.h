#pragma once

#include <QString>
#include <QColor>
#include <QWidget>

namespace railshot {
namespace theme {

// Chromatic Command — Bright Edition tokens
inline constexpr const char* kBgRoot = "#0B0D0F";
inline constexpr const char* kBgPanel = "#0F1114";
inline constexpr const char* kBgElevated = "#1A1D22";
inline constexpr const char* kBgControl = "#1E2128";
inline constexpr const char* kBgDeep = "#0A0C0F";
inline constexpr const char* kBgCanvas = "#080A0D";
inline constexpr const char* kBgMonitor = "#0D0F12";
inline constexpr const char* kBorder = "#2A2D35";
inline constexpr const char* kBorderStrong = "#3A3D45";
inline constexpr const char* kBorderEmphasis = "#4A4D55";

inline constexpr const char* kBrand = "#FF5A2C";
inline constexpr const char* kBrandLight = "#FF8C42";
inline constexpr const char* kBrandDeep = "#CC3A18";
inline constexpr const char* kBlue = "#4F9EFF";
inline constexpr const char* kBlueActive = "#3A6AFF";
inline constexpr const char* kViolet = "#A855F7";
inline constexpr const char* kEmerald = "#22C55E";
inline constexpr const char* kEmeraldDeep = "#16A34A";
inline constexpr const char* kCyan = "#22D3EE";
inline constexpr const char* kAmber = "#FBBF24";
inline constexpr const char* kCrimson = "#EF4444";

inline constexpr const char* kTextPrimary = "#F0F0F0";
inline constexpr const char* kTextSecondary = "#A0A8B8";
inline constexpr const char* kTextMuted = "#606878";

inline constexpr const char* kFontDisplay = "Bebas Neue";
inline constexpr const char* kFontSans = "DM Sans";
inline constexpr const char* kFontMono = "JetBrains Mono";

enum class PanelAccent { Brand, Blue, Violet, Emerald, Cyan, Amber, Pink };

void registerFonts();
QString loadStyleSheet();
QColor color(const char* hex);
QString panelHeaderStyle(PanelAccent accent);
void applyPanelHeader(QWidget* widget, PanelAccent accent);

} // namespace theme
} // namespace railshot
