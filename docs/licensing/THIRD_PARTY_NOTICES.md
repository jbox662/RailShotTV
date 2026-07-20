# Third-Party Notices — RailShotTV

This product includes software developed by third parties. Their licenses apply
to those components only; RailShotTV application code remains proprietary.

## Qt 6
- License: Qt commercial **or** LGPLv3 (dynamic linking)
- Modules used: Core, Gui, Widgets, Network, Concurrent, Svg, Test
- Obligations if using LGPL: ship Qt DLLs separately, provide Qt source offer,
  allow user relinking. Prefer Qt commercial for closed distribution comfort.
- Website: https://www.qt.io/licensing

## FFmpeg (optional, LGPL build only)
- License: LGPLv2.1+ when configured **without** `--enable-gpl` and **without** `--enable-nonfree`
- Dynamic linking required for proprietary app compliance
- Do **not** enable x264 (GPL) or nonfree components in shipping builds
- See `docs/licensing/FFMPEG_BUILD.md`
- Website: https://ffmpeg.org/legal.html

## Windows / Microsoft APIs
- Media Foundation, WASAPI, DXGI Desktop Duplication, Windows.Graphics.Capture
- Covered by Windows SDK redistributable terms

## Fonts
- Bundle licensed fonts locally (Segoe UI is a Windows system font)
- If shipping Bebas Neue / DM Sans / JetBrains Mono, include their OFL/SIL notices

## Icons
- Lucide icons in the React prototype — if ported, retain their ISC license notice

## Hardware encoder SDKs (optional, gated)
- NVIDIA Video Codec SDK — NVIDIA license + driver runtime
- AMD AMF — AMD license
- Intel oneVPL — Intel license

## Explicitly excluded
- OBS Studio / libobs (GPL) — **not linked, not copied**
- GPL-licensed FFmpeg builds
- GPL-only Qt modules
