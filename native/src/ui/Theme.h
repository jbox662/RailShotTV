#pragma once

#include <QString>
#include <QColor>

namespace railshot {
namespace theme {

// Chromatic Command — RailShotTV brand tokens (from ideas.md)
inline constexpr const char* kBgRoot = "#0B0D0F";
inline constexpr const char* kBgPanel = "#0F1114";
inline constexpr const char* kBgElevated = "#1A1D22";
inline constexpr const char* kBgControl = "#1E2128";
inline constexpr const char* kBorder = "#2A2D35";
inline constexpr const char* kBorderStrong = "#3A3D45";

inline constexpr const char* kBrand = "#FF5A2C";
inline constexpr const char* kBlue = "#4F9EFF";
inline constexpr const char* kViolet = "#A855F7";
inline constexpr const char* kEmerald = "#22C55E";
inline constexpr const char* kCyan = "#22D3EE";
inline constexpr const char* kAmber = "#FBBF24";
inline constexpr const char* kCrimson = "#EF4444";

inline constexpr const char* kTextPrimary = "#F0F0F0";
inline constexpr const char* kTextSecondary = "#A0A8B8";
inline constexpr const char* kTextMuted = "#606878";

QString loadStyleSheet();
QColor color(const char* hex);

} // namespace theme
} // namespace railshot
