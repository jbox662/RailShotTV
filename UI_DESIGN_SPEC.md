# Nexus Broadcast UI — Complete Design Specification

> **Purpose:** This document is the authoritative reference for restoring the original Nexus Broadcast UI design. It covers every screen, panel, modal, component, color token, typography rule, and interaction pattern. Cursor must use this document as the ground-truth spec when restoring or rebuilding any part of the UI.

---

## Table of Contents

1. [Design System — Tokens & Typography](#1-design-system--tokens--typography)
2. [Global Layout Shell](#2-global-layout-shell)
3. [AppSidebar (Left Nav)](#3-appsidebar-left-nav)
4. [Dashboard — Main Screen](#4-dashboard--main-screen)
   - 4.1 [Top Toolbar Bar](#41-top-toolbar-bar)
   - 4.2 [Main Monitor Area (Preview | Transitions | Program)](#42-main-monitor-area-preview--transitions--program)
   - 4.3 [Input Tiles Row](#43-input-tiles-row)
   - 4.4 [Audio Mixer Panel](#44-audio-mixer-panel)
   - 4.5 [Bottom Action Toolbar](#45-bottom-action-toolbar)
   - 4.6 [Scenes Panel (Left Column)](#46-scenes-panel-left-column)
5. [Modals & Drawers](#5-modals--drawers)
   - 5.1 [Add Source Modal](#51-add-source-modal)
   - 5.2 [Go Live Modal](#52-go-live-modal)
   - 5.3 [Input Settings Drawer](#53-input-settings-drawer)
   - 5.4 [New Project Confirmation Dialog](#54-new-project-confirmation-dialog)
   - 5.5 [Keyboard Shortcuts Overlay](#55-keyboard-shortcuts-overlay)
6. [Chat Panel Page](#6-chat-panel-page)
7. [Analytics Page](#7-analytics-page)
8. [Settings Page](#8-settings-page)
9. [Scoreboard Page](#9-scoreboard-page)
10. [Schedule Page](#10-schedule-page)
11. [Context Menus](#11-context-menus)
12. [Shared Sub-Components](#12-shared-sub-components)

---

## 1. Design System — Tokens & Typography

### Color Palette (OKLCH)

All colors are defined as CSS custom properties in `client/src/index.css` under `:root`. The theme is **dark-only** — there is no light mode toggle.

| Token | OKLCH Value | Hex Equivalent | Usage |
|---|---|---|---|
| `--background` | `oklch(0.09 0.004 260)` | ~`#0A0C10` | Page/app background |
| `--panel` | `oklch(0.11 0.004 260)` | ~`#0D0F14` | Panel backgrounds |
| `--card` | `oklch(0.14 0.005 260)` | ~`#141619` | Card/tile backgrounds |
| `--control` | `oklch(0.18 0.006 260)` | ~`#1A1D22` | Input/button backgrounds |
| `--border` | `oklch(0.22 0.006 260)` | ~`#2A2D35` | Default borders |
| `--border-elevated` | `oklch(0.28 0.008 260)` | ~`#3A3D45` | Elevated/hover borders |
| `--border-active` | `oklch(0.46 0.030 265)` | ~`#4A6A9A` | Active/focused borders |
| `--foreground` | `oklch(0.97 0.003 270)` | ~`#F0F2F8` | Primary text |
| `--foreground-secondary` | `oklch(0.75 0.012 270)` | ~`#C0C2C8` | Secondary text |
| `--foreground-muted` | `oklch(0.52 0.012 270)` | ~`#808898` | Muted/label text |
| `--brand` | `oklch(0.68 0.24 28)` | ~`#FF5A2C` | Brand orange-red (Stream button, ON AIR badge) |
| `--blue` | `oklch(0.65 0.21 250)` | ~`#4F9EFF` | Blue accent (selection, links) |
| `--emerald` | `oklch(0.72 0.20 162)` | ~`#22C55E` | Green (Preview label, GO button, live indicator) |
| `--violet` | `oklch(0.62 0.26 292)` | ~`#A855F7` | Violet accent |
| `--amber` | `oklch(0.82 0.19 78)` | ~`#FBBF24` | Amber/warning (Solo button) |
| `--crimson` | `oklch(0.66 0.24 25)` | ~`#EF4444` | Red/destructive (Mute active, delete) |
| `--cyan` | `oklch(0.76 0.17 210)` | ~`#22D3EE` | Cyan accent |
| `--pink` | `oklch(0.66 0.26 350)` | ~`#EC4899` | Pink accent |

### Typography

Three font families are loaded via Google Fonts CDN in `client/index.html`:

| Role | Font | Weights | Usage |
|---|---|---|---|
| Display / Headers | **Bebas Neue** | 400 | Large labels (PREVIEW, PROGRAM, ON AIR), section titles |
| Body / UI | **DM Sans** | 400, 600, 700, 800 | All UI text, buttons, labels, tooltips |
| Monospace | **JetBrains Mono** | 400 | Timecodes, dB values, FPS/CPU stats, version numbers |

**Font size scale used throughout the UI:**

- `7px` — channel strip dB readout, channel name, S/M button text
- `8px` — MASTER label, INPUTS/OUTPUTS rotated labels, signal strength version
- `9px` — section sub-labels, context menu labels, status bar stats
- `10px` — tooltip content, secondary info, AppSidebar version
- `11px` — standard button text, input labels, dropdown items, tile controls
- `12px` — Settings page inputs
- `13px` — keyboard shortcut `?` button

### Border Radius

`--radius: 0.4rem` (≈ 6px). Most UI elements use `borderRadius: 3` (3px) for a sharp, broadcast-tool aesthetic. Modals use `borderRadius: 6–8px`. The AppSidebar nav icons use `borderRadius: 8px`.

### Shadows & Glows

- Active/live elements use colored glow: `0 0 16px rgba(255,90,44,0.4)` for brand-orange, `0 0 10px rgba(58,106,255,0.4)` for blue.
- Standard button shadow: `0 2px 6px rgba(0,0,0,0.5), inset 0 1px 0 rgba(255,255,255,0.08)`.
- Modal/dropdown shadow: `0 8px 32px rgba(0,0,0,0.7)`.
- Selected source in canvas: `border: 2px solid #4F9EFF`.
- Program canvas when live: `box-shadow: 0 0 30px rgba(255,90,44,0.15)`.

### Button Gradient Convention

All buttons use a subtle top-to-bottom linear gradient to give depth:

- **Default (inactive):** `linear-gradient(180deg, #1E2128, #16181E)` with `border: 1px solid #3A3D45`
- **Active/on state:** Colored gradient matching the function (e.g., green for recording, orange for stream)
- **Hover:** Slightly lighter gradient or border color change
- **Active press:** `transform: scale(0.97)` with 160ms ease-out

---

## 2. Global Layout Shell

The entire app is wrapped in `<AppSidebar>` which renders a fixed 52px-wide left sidebar and a full-height right content area. The root element is `display: flex; height: 100vh; overflow: hidden; background: #0A0C10`.

```
┌────────────────────────────────────────────────────────┐
│ AppSidebar (52px) │ Page Content (flex: 1)             │
│  [Logo]           │                                    │
│  [Nav Icons]      │  <Dashboard /> or other page       │
│  [Signal]         │                                    │
└────────────────────────────────────────────────────────┘
```

---

## 3. AppSidebar (Left Nav)

**File:** `client/src/components/AppSidebar.tsx`

**Container:** `width: 52px`, `height: 100vh`, `background: #0D0F14`, `border-right: 1px solid #1A1D24`, `display: flex; flex-direction: column; align-items: center; padding: 8px 0`, `position: relative`, `z-index: 50`, `flex-shrink: 0`.

### Logo Area (top)

A 32×32px circular icon at the top. The icon is an SVG with a central white circle (r=3) and four radiating lines (horizontal and vertical), representing a broadcast signal. The circle background is `linear-gradient(135deg, #FF5A2C 0%, #FF8C42 100%)` with `box-shadow: 0 0 18px rgba(255,90,44,0.55)`.

### Live Indicator (conditional)

Only visible when `isLive === true`. Shown between the logo and nav items. Contains:
- A 10×10px pulsing red dot: outer ring animates with `liveRipple` keyframe (scale + opacity), inner dot is solid `#FF3A0C`.
- Text label "LIVE" in `DM Sans 800 7px #FF5A2C letterSpacing: 0.12em uppercase`.
- Separated from nav by `border-bottom: 1px solid rgba(255,90,44,0.2)`.

### Navigation Items

Six icon buttons in a vertical column. Each is a 40×40px rounded square (`borderRadius: 8px`).

| Icon | Route | Active Color | Glow |
|---|---|---|---|
| `Tv2` | `/` (Dashboard) | `#FF5A2C` | `rgba(255,90,44,0.3)` |
| `MessageSquare` | `/chat` | `#A855F7` | `rgba(168,85,247,0.3)` |
| `BarChart2` | `/analytics` | `#22D3EE` | `rgba(34,211,238,0.3)` |
| `Trophy` | `/scoreboard` | `#FBBF24` | `rgba(251,191,36,0.3)` |
| `Calendar` | `/schedule` | `#22C55E` | `rgba(34,197,94,0.3)` |
| `Settings` | `/settings` | `#4F9EFF` | `rgba(79,158,255,0.3)` |

**Active state:** `background: {color}22`, `border: 1px solid {color}55`, `box-shadow: 0 0 16px {glow}`.
**Hover state (inactive):** `background: rgba(255,255,255,0.06)`, `border: 1px solid rgba(255,255,255,0.1)`.
**Tooltip:** Radix `<Tooltip>` on the right side, `background: #1A1D22`, `border: 1px solid rgba(255,255,255,0.12)`, `color: #E2E8F0`, `fontSize: 12`.

### Signal Strength (bottom)

Five vertical bars of increasing height `[3, 5, 7, 9, 11]px`, width 3px, `borderRadius: 1.5px`. First four bars are `#22C55E` (green), fifth is `#2D3748` (dark). Below them: version label "v2.5" in `JetBrains Mono 8px #4B5563`.

---

## 4. Dashboard — Main Screen

**File:** `client/src/pages/Dashboard.tsx`

The Dashboard is a single full-height flex column (`height: 100vh; display: flex; flex-direction: column; overflow: hidden; background: #0A0C10`). It has five horizontal rows stacked top-to-bottom:

```
┌─────────────────────────────────────────────────────────┐
│ 1. Top Toolbar Bar (flexShrink: 0, ~34px tall)          │
├──────────────────────────────────────────────────────────┤
│ 2. Main Monitor Area (flex: 1, overflow: hidden)         │
│    [SCENES panel] [PREVIEW] [TRANSITIONS] [PROGRAM]      │
├──────────────────────────────────────────────────────────┤
│ 3. Input Tiles Row (flexShrink: 0, ~110px tall)          │
│    [tile 1][tile 2]...[Audio Mixer slides in from right] │
├──────────────────────────────────────────────────────────┤
│ 4. Bottom Action Toolbar (flexShrink: 0, ~36px tall)     │
└──────────────────────────────────────────────────────────┘
```

---

### 4.1 Top Toolbar Bar

**Container:** `display: flex; align-items: center; gap: 2px; padding: 4px 8px; background: linear-gradient(180deg, #141619, #0F1114); border-bottom: 1px solid #2A2D35; flex-shrink: 0`.

Left-to-right contents:

1. **App title** — "RailShotTV" in `Bebas Neue 16px #FF5A2C letterSpacing: 0.08em`, followed by version "v2.5" in `JetBrains Mono 9px #606878`.
2. **Vertical divider** — `width: 1px; height: 20px; background: #3A3D45; margin: 0 6px`.
3. **Status readouts** — three `JetBrains Mono 10px #606878` spans: "1080p29.97", "EX FPS: 30", "CPU: 3%" (or "12%" when live).
4. **Vertical divider.**
5. **Pause Inputs button** — toggles `inputsPaused`. When active: `background: linear-gradient(180deg,#3A2A1A,#281E14)`, `border: 1px solid #F9731660`, `color: #F97316`, text "▐▐ Paused". When inactive: default gradient, text "Pause Inputs".
6. **Basic button** — default gradient, shows `toast.info("Basic mode — simplified layout coming soon")` on click.
7. **Settings button** — default gradient, navigates to `/settings`.
8. **`?` button** — 26×26px, toggles `showShortcuts`. When active: `background: linear-gradient(180deg,#3A6AFF,#2A50CC)`, `border: 1px solid #5A8AFF`, `color: #fff`. Shows the keyboard shortcuts overlay panel.

---

### 4.2 Main Monitor Area (Preview | Transitions | Program)

**Container:** `display: flex; flex: 1; overflow: hidden; min-height: 0`.

Three columns side by side:

#### Left Column — Scenes Panel + Preview Monitor

`flex: 1; display: flex; flex-direction: column; overflow: hidden; background: #0D0F12; border-right: 1px solid #2A2D35`.

**Top sub-row (Scenes + Preview side by side):**
`display: flex; flex: 1; overflow: hidden; min-height: 0`.

- **Scenes Panel** (left, 190px wide) — see [Section 4.6](#46-scenes-panel-left-column).
- **Preview Monitor** (right, flex: 1):
  - **Label bar:** `display: flex; align-items: center; justify-content: space-between; padding: 3px 10px; background: linear-gradient(180deg,#1A1D22,#141619); border-bottom: 1px solid #2A2D35`.
    - Left: "PREVIEW" in `DM Sans 700 11px #22C55E letterSpacing: 0.06em`.
    - Right: scene name in `JetBrains Mono 10px #A0A8B8` + optional "CLEAR" button (`border: 1px solid #22C55E40`, `color: #22C55E70`, `fontSize: 9`, `fontWeight: 700`).
  - **Canvas area:** `flex: 1; display: flex; align-items: center; justify-content: center; background: #080A0D; position: relative; overflow: hidden`.
    - Inner canvas: `width: 100%; max-width: 100%; aspect-ratio: 16/9; background: #000; border: 2px solid #22C55E30; overflow: hidden`.
    - Empty state: centered `Monitor` icon (size 32, `color: #2A3550, opacity: 0.4`) + "No Preview" text (`fontSize: 11, color: #3A4560, letterSpacing: 0.06em, textTransform: uppercase`).
    - When a scene is loaded: renders `<ProgramCanvas>` with drag-to-reposition and resize handles.
    - Corner markers: four 14×14px L-shaped brackets at each corner, `border-color: rgba(79,158,255,0.3)`.

#### Center Column — Transitions Panel

`width: 140px; display: flex; flex-direction: column; align-items: center; background: #0A0C10; border-right: 1px solid #2A2D35; overflow-y: auto; padding: 6px 4px; gap: 4px; flex-shrink: 0`.

**Contents (top to bottom):**

1. **Transition speed slider** — labeled "Speed" (`DM Sans 9px #606878`), current value in `JetBrains Mono 9px #4F9EFF`. `<input type="range" min={100} max={2000}` with `accentColor: #4F9EFF`.
2. **Transition type buttons** — one per transition type: `["Cut","Fade","Merge","Slide","Wipe","CubeZoom","FTB"]`. Each is a split button:
   - Left part (label): `flex: 1; padding: 3px 0; fontSize: 10; fontWeight: 600`. Active: `background: linear-gradient(180deg,#1A3AFF,#1230CC)`, `border: 1px solid #3A6AFF`, `color: #fff`. Inactive: default gradient, `color: #808898`.
   - Right part (▾ dropdown): `width: 16px; fontSize: 9`. Opens a `<DropdownMenu>` with transition-specific options (duration, curve, direction, blend mode, etc.).
3. **Auto-transition toggle** — "Auto" label + checkbox. When on: `color: #4F9EFF`.
4. **Vertical divider.**
5. **FTB button** — "FTB" (Fade to Black). Same split-button style as transition types.

#### Right Column — Program Monitor

`flex: 1; display: flex; flex-direction: column; overflow: hidden; background: #0D0F12`.

- **Label bar:** Same structure as Preview label bar.
  - Left: "PROGRAM" in `DM Sans 700 11px #FF5A2C letterSpacing: 0.06em`.
  - Right: scene name + optional "CLEAR" button (`border: 1px solid #FF5A2C40`, `color: #FF5A2C70`).
  - Also shows `<LiveTimecode />` component (self-contained, renders current time in `JetBrains Mono 10px #FF5A2C` when live, else `#606878`).
- **Canvas area:** Same structure as Preview.
  - Inner canvas: `border: 2px solid ${isLive ? "#FF5A2C50" : "#FF5A2C20"}`. When live: `box-shadow: 0 0 30px rgba(255,90,44,0.15)`.
  - **ON AIR badge** (when live): `position: absolute; top: 6px; left: 6px; padding: 1px 6px; background: #FF5A2C; borderRadius: 2; fontFamily: DM Sans; fontWeight: 700; fontSize: 9; color: #fff; letterSpacing: 0.08em; animation: pulse 1.5s infinite`.
  - Corner markers: same as Preview but `border-color: rgba(255,90,44,0.5)` when live, `rgba(79,158,255,0.3)` otherwise.
  - Transition animation: CSS transitions applied to the canvas wrapper div during `transitionPhase === "out"` / `"in"`.

---

### 4.3 Input Tiles Row

**Container:** `display: flex; align-items: stretch; background: #0A0C10; border-top: 1px solid #1A1D24; flex-shrink: 0; min-height: 0; overflow: hidden`.

Each tile represents one source in the active scene. Tiles are rendered in a horizontal scrollable row.

#### Individual Tile Structure

`display: flex; flex-direction: column; border-right: 1px solid #1A1D24; flex-shrink: 0; cursor: pointer; transition: background 0.15s`.

**Tile header (top bar):**
`display: flex; align-items: center; gap: 3px; padding: 2px 4px; background: linear-gradient(180deg,#1A1D22,#141619); border-bottom: 1px solid #1A1D24; flex-shrink: 0`.

- Source index number badge: `background: {source.color}22; border: 1px solid {source.color}40; borderRadius: 3; padding: 0 4px; fontSize: 8; color: {source.color}; fontFamily: JetBrains Mono`.
- Source type icon: `<source.icon size={10} color={source.color} opacity={0.8}>`.
- Source name: truncated, `DM Sans 10px #C0C2C8; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; flex: 1`.
- Settings gear icon: `<Settings size={9} color="#606878">` — opens Input Settings Drawer on click.
- Eye icon: `<Eye size={9}>` — toggles source visibility. Hidden sources show `opacity: 0.5` on the icon.
- × close icon: `<X size={9} color="#606878">` — removes source from scene.

**Preview area (middle):**
`flex: 1; background: #080A0D; display: flex; align-items: center; justify-content: center; position: relative; overflow: hidden; min-height: 52px`.

Content varies by source type:
- `browser`: renders `<SmoothBrowserFrame url={src.settings.url}>` (live iframe with opacity fade-in).
- `camera`: dark green background `#0A1A0A` + source icon (size 24, `color: src.color, opacity: 0.5`) + source name below.
- `image`: `<img>` with `object-fit: contain`.
- `text`: centered text span with `font-size` scaled to tile size, `color: #F8F8FF`, optional bold.
- All others: `background: {src.color}18` + icon (size 20, `opacity: 0.6`) + name.

**Tile controls (bottom bar):**
`display: flex; align-items: center; gap: 2px; padding: 2px 3px; border-top: 1px solid #1E2028; background: #080A0E; flex-shrink: 0`.

- **◀ / ▶ layer order buttons** — 16×16px, `background: linear-gradient(180deg,#2A2D35,#1E2128)`, `border: 1px solid #3A3D45`, `borderRadius: 2`. Disabled state: `color: #303540`.
- **GO button** — `padding: 2px 6px; background: linear-gradient(180deg,#22C55E,#16A34A); border: 1px solid #22C55E80; borderRadius: 2; color: #000; fontSize: 9; fontWeight: 700; box-shadow: 0 0 8px rgba(34,197,94,0.3); margin-left: auto`. Disabled (transitioning): muted green.
- **Cut button** — `padding: 2px 6px; background: linear-gradient(180deg,#2A2D35,#1E2128); border: 1px solid #3A3D45; borderRadius: 2; color: #C0C2C8; fontSize: 9; fontWeight: 700`.

**Right-click context menu on tile:** See [Section 11](#11-context-menus).

---

### 4.4 Audio Mixer Panel

The Audio Mixer slides in horizontally from the right edge of the Input Tiles Row. It is **not** a popup — it is an inline panel that expands the tiles row.

**Toggle:** "Audio Mixer" button in the Bottom Action Toolbar. When open: `background: linear-gradient(180deg,#1A3AFF,#1230CC)`, `border: 1px solid #3A6AFF`, `color: #fff`, `box-shadow: 0 0 10px rgba(58,106,255,0.4)`.

**Container:** `display: flex; overflow: hidden; flex-shrink: 0; max-width: 480px (open) or 0 (closed); transition: max-width 0.25s ease-in-out; background: #0A0C10; border-left: 1px solid #2A2D35`.

**Inner layout:** `display: flex; height: 100%`.

#### OUTPUTS Label (leftmost)
`width: 20px; display: flex; align-items: center; justify-content: center; background: #22C55E15; border-right: 1px solid #2A2D35; flex-shrink: 0`.
Text: "OUTPUTS" in `DM Sans 8px 700 #22C55E letterSpacing: 0.1em writing-mode: vertical-rl textTransform: uppercase`.

#### Master Strip
`width: 52px; display: flex; flex-direction: column; align-items: center; gap: 2px; padding: 4px 3px 3px; border-right: 1px solid #2A2D35; flex-shrink: 0; background: #0D0F14`.

- **MASTER badge:** `width: 100%; padding: 1px 4px; background: linear-gradient(180deg,#22C55E,#16A34A) (unmuted) or linear-gradient(180deg,#7F1D1D,#5A1010) (muted); borderRadius: 2; DM Sans 700 8px color: #000 (unmuted) or #FCA5A5 (muted); letterSpacing: 0.06em; text-align: center`.
- **Master volume slider:** `<input type="range" min={0} max={100}` with `accentColor: #22C55E`. Styled as `mixer-fader` class (native vertical slider via `writing-mode: vertical-lr; direction: rtl; width: 20px; height: 100px`).
- **VU meter:** `<VUMeterVertical color="#22C55E" active={isLive && !masterMuted} volume={masterVolume}>`.
- **dB readout:** `JetBrains Mono 7px color: #22C55E`. Formula: `volume === 0 ? "-∞" : volume >= 100 ? "0.0" : (20 * Math.log10(volume / 100)).toFixed(1)`.
- **M (mute) button:** `width: 100%; height: 13px; borderRadius: 2; fontSize: 8; fontWeight: 700`. Active: `background: #EF4444; border: 1px solid #EF4444; color: #fff`. Inactive: `background: #1E2128; border: 1px solid #3A3D45; color: #606878`.

#### Channel Strips (scrollable)
`display: flex; overflow-x: auto; min-width: 0`.

Each channel strip: `width: 44px; display: flex; flex-direction: column; align-items: center; gap: 0; padding: 4px 2px 3px; border-right: 1px solid #1A1D24; flex-shrink: 0; background: #0A0C10 (normal) or #1A1400 (solo active); opacity: 0.45 (muted) or 1 (active); transition: opacity 0.15s`.

**Channel strip contents (top to bottom):**
1. **Meter + fader side-by-side row:** `display: flex; gap: 3px; align-items: flex-end; flex: 1; width: 100%; justify-content: center; min-height: 0`.
   - `<VUMeterVertical color={ch.color} active={isLive && !cs.muted} volume={cs.volume}>` — see VUMeterVertical spec below.
   - Native vertical fader: `<input type="range" min={0} max={100}` with `className="mixer-fader"` and `accentColor: ch.color`. The `mixer-fader` CSS class applies `writing-mode: vertical-lr; direction: rtl; -webkit-appearance: slider-vertical; appearance: slider-vertical; width: 20px; height: 100px; cursor: pointer`.
2. **dB readout:** `JetBrains Mono 7px` — same formula as master. Color: `ch.color` (active) or `#404450` (muted).
3. **Channel name:** `DM Sans 7px color: #808898 (active) or #404450 (muted); overflow: hidden; text-overflow: ellipsis; white-space: nowrap; width: 100%; text-align: center; letterSpacing: 0.03em; margin-top: 2px`.
4. **S / M buttons row:** `display: flex; gap: 1px; width: 100%; margin-top: 2px`.
   - **S (solo):** `flex: 1; height: 13px; borderRadius: 2; fontSize: 7; fontWeight: 700`. Active: `background: #FBBF24; border: 1px solid #FBBF24; color: #000`. Inactive: `background: #141619; border: 1px solid #2A2D35; color: #505868`.
   - **M (mute):** Same dimensions. Active: `background: #EF4444; border: 1px solid #EF4444; color: #fff`. Inactive: same as S inactive.

**Default audio channels (always present):**

| id | name | color |
|---|---|---|
| 1001 | "Desktop A…" | `#22C55E` |
| 1002 | "Mic/Aux" | `#4F9EFF` |

Additional channels are added dynamically from sources in the active scene that have audio (cameras, NDI, etc.).

#### INPUTS Label (rightmost)
`width: 20px; display: flex; align-items: center; justify-content: center; background: #1A3AFF15; border-left: 1px solid #2A2D35; flex-shrink: 0`.
Text: "INPUTS" in `DM Sans 8px 700 #3A6AFF letterSpacing: 0.1em writing-mode: vertical-rl textTransform: uppercase`.

#### VUMeterVertical Sub-Component

`memo`-wrapped. Props: `{ color: string; active: boolean; volume: number }`.

Renders an SVG VU meter: `width: 6px; height: 120px`. The meter is a vertical bar divided into segments. When `active === true`, segments light up proportionally to `volume`. The color transitions from the channel color at low levels to amber at -6dB to red at 0dB. When `active === false`, all segments are dark (`#1A1D24`).

---

### 4.5 Bottom Action Toolbar

**Container:** `display: flex; align-items: center; gap: 2px; padding: 4px 8px; background: linear-gradient(180deg,#141619,#0F1114); border-top: 1px solid #2A2D35; flex-shrink: 0`.

Left-to-right contents:

1. **Add Input split button:**
   - Main: "＋ Add Input" — opens `<AddSourceModal>`.
   - Dropdown (▾): options "Add Camera", "Add Browser Source", "Add Image", "Add Text".

2. **Snapshot button** — camera icon + "Snapshot" text. `toast.success("Snapshot saved")`.

3. **Record split button:**
   - Main: "⏺ Record" (inactive) or "⏹ Stop Rec" (recording). When recording: `background: linear-gradient(180deg,#1A3A2A,#122A1E)`, `border: 1px solid #22C55E40`, `color: #86EFAC`. Contains `<RecordTimecode isRecording={isRecording}>` component.
   - Dropdown (▾): "Record to MP4", "Record to MKV", separator, "Recording Settings…" (navigates to `/settings`).

4. **External split button:**
   - Main: "External" (inactive) or "● External" (active). When active: `background: linear-gradient(180deg,#1A3A2A,#122A1E)`, `border: 1px solid #22C55E40`, `color: #86EFAC`.
   - Dropdown (▾): "HDMI / Capture Card", "NDI Output", separator, "Output Settings…".

5. **Stream split button:**
   - When NOT live: "● Stream" button (orange gradient) + ▾ dropdown.
     - Main: `background: linear-gradient(180deg,#FF5A2C,#CC3A18)`, `border: 1px solid #FF5A2C80`, `color: #fff`, `fontWeight: 700`, `box-shadow: 0 0 16px rgba(255,90,44,0.4)`.
   - When live: "⏹ End Stream" button (dark red) + ▾ dropdown.
     - Main: `background: linear-gradient(180deg,#7F1D1D,#5A1010)`, `border: 1px solid #EF444450`, `color: #FCA5A5`.

6. **Vertical spacer** (`flex: 1`).

7. **Stats toggle** — `<Activity size={12}>` icon button, 28×28px. When active: `border: 1px solid #22C55E50`, `color: #86EFAC`.

8. **Layout presets** — `<LayoutTemplate size={12}>` icon button, 28×28px. Shows "coming soon" toast.

9. **Performance monitor** — `<Cpu size={12}>` icon button, 28×28px. Shows CPU/GPU stats toast.

10. **UI Lock** — `<Lock>` / `<Unlock>` icon button, 28×28px. When locked: `background: linear-gradient(180deg,#3A2A1A,#281E14)`, `border: 1px solid #F9731650`, `color: #FCD34D`.

11. **Audio Mixer toggle** — `<Volume2 size={12}>` + "Audio Mixer" text. When open: blue gradient. See [Section 4.4](#44-audio-mixer-panel).

12. **Status bar** — `background: #0A0C0F; border: 1px solid #2A2D35; borderRadius: 3; padding: 2px 8px`. Contains: `1080p29.97` (green), `EX FPS: 30`, `Render: 1ms`, `GPU: 2%`, `CPU: 3%/12%`, `Total: 8%/63%` — all in `JetBrains Mono 9px`.

---

### 4.6 Scenes Panel (Left Column)

**File:** `client/src/components/SceneManagerPanel.tsx`

**Container:** `width: 190px; display: flex; flex-direction: column; background: #0A0C0F; border-right: 1px solid #2A2D35; overflow: hidden; flex-shrink: 0`.

**Header:** `display: flex; align-items: center; justify-content: space-between; padding: 5px 8px; background: linear-gradient(180deg,#1A1D22,#141619); border-bottom: 1px solid #2A2D35; flex-shrink: 0`.
- Left: "SCENES" in `DM Sans 700 10px #808898 letterSpacing: 0.1em uppercase`.
- Right: "＋" add button — `width: 22px; height: 22px; background: linear-gradient(180deg,#1A3AFF,#1230CC); border: 1px solid #3A6AFF; borderRadius: 4; color: #fff; fontSize: 14; fontWeight: 700`.

**Scene list:** `flex: 1; overflow-y: auto; padding: 4px`.

Each scene row is a `<ContextMenu>` wrapper. The row itself:
`display: flex; align-items: center; gap: 4px; padding: 5px 6px; borderRadius: 4; cursor: pointer; border: 1px solid transparent; transition: all 0.12s`.

**Scene row states:**
- **Program (on-air):** `background: #FF5A2C18; border: 1px solid #FF5A2C40`.
- **Preview:** `background: #22C55E18; border: 1px solid #22C55E40`.
- **Active/editing:** `background: #4F9EFF18; border: 1px solid #4F9EFF40`.
- **Hover (inactive):** `background: rgba(255,255,255,0.04)`.

**Scene row contents:**
- Scene index badge: `width: 18px; height: 18px; borderRadius: 3; background: {color}22; border: 1px solid {color}40; DM Sans 700 9px; text-align: center; flex-shrink: 0`.
- Scene name: `DM Sans 11px; flex: 1; overflow: hidden; text-overflow: ellipsis; white-space: nowrap`. Color: `#FF8A6A` (program), `#6EE7A0` (preview), `#C8DAFF` (active), `#8892A4` (inactive).
- Source count badge: `background: #1A2030; border: 1px solid #2A3050; borderRadius: 8; JetBrains Mono 8px color: #4F9EFF80; padding: 0 4px`.
- Up/Down chevron buttons: 9px icons, `color: #606878`, opacity 0.4 → 1 on hover.

**Renaming mode:** Clicking the scene name (or "Rename" from context menu) replaces the name span with an `<input>` field: `background: #0A0C10; border: 1px solid #4F9EFF; borderRadius: 2; color: #C8DAFF; fontSize: 11; padding: 1px 4px; width: 100%`. Confirmed on Enter or blur.

**Footer:** `padding: 4px 8px; border-top: 1px solid #2A2D35; background: #0A0C0F`. Text: `{n} scene(s)` in `JetBrains Mono 9px #404450`.

---

## 5. Modals & Drawers

### 5.1 Add Source Modal

**Trigger:** "＋ Add Input" button in Bottom Action Toolbar.
**Type:** Full-screen overlay modal (not a dialog — renders as a fixed overlay div).

**Overlay:** `position: fixed; inset: 0; background: rgba(0,0,0,0.75); display: flex; align-items: center; justify-content: center; z-index: 1000`.

**Modal box:** `background: #0F1114; border: 1px solid #3A3D45; borderRadius: 6; width: 720px; max-height: 80vh; display: flex; flex-direction: column; box-shadow: 0 20px 60px rgba(0,0,0,0.8)`.

**Header:** `display: flex; align-items: center; justify-content: space-between; padding: 10px 14px; background: linear-gradient(180deg,#1A1D22,#141619); border-bottom: 1px solid #3A3D45`.
- Title: "Add Input" in `DM Sans 700 13px #F0F2F8`.
- Close button: `×` in `color: #606878; fontSize: 18`.

**Body:** Two-column layout.
- **Left column (input type grid):** `width: 220px; padding: 8px; border-right: 1px solid #2A2D35; overflow-y: auto`.
  - Grid of type buttons (4 columns). Each button: `display: flex; flex-direction: column; align-items: center; gap: 3px; padding: 8px 4px; borderRadius: 4; border: 1px solid transparent; cursor: pointer; transition: all 0.12s`.
  - Selected: `background: {type.color}18; border: 1px solid {type.color}40; box-shadow: 0 0 8px {type.color}30`.
  - Icon: `<type.icon size={20} color={type.color}>`.
  - Label: `DM Sans 9px color: #808898 (inactive) or {type.color} (active); text-align: center`.

**Input types available:**

| Type ID | Label | Icon | Color |
|---|---|---|---|
| `camera` | Camera | `Camera` | `#22C55E` |
| `ndi` | NDI | `Wifi` | `#4F9EFF` |
| `stream` | Stream/SRT | `Radio` | `#A855F7` |
| `browser` | Browser | `Globe` | `#22D3EE` |
| `display` | Display | `Monitor` | `#FBBF24` |
| `image` | Image | `ImageIcon` | `#EC4899` |
| `video` | Video | `Video` | `#FF5A2C` |
| `audio` | Audio File | `Music` | `#A855F7` |
| `audioinput` | Audio Device | `Mic` | `#22C55E` |
| `text` | Title/Text | `Type` | `#F8F8FF` |
| `lowerthird` | Lower Third | `AlignLeft` | `#4F9EFF` |
| `title` | Title | `Type` | `#FBBF24` |
| `virtualset` | Virtual Set | `Layers` | `#A855F7` |
| `zoom` | Zoom | `Video` | `#2D8CFF` |
| `scoreboard` | Scoreboard | `Trophy` | `#FBBF24` |
| `telestrator` | Telestrator | `Pencil` | `#EC4899` |
| `color` | Color/BG | `Palette` | `#FF5A2C` |
| `counter` | Counter | `Hash` | `#22D3EE` |

- **Right column (config form):** `flex: 1; padding: 12px 14px; overflow-y: auto`.
  - Each input type renders a specific config form. Common fields: Name (text input), Number (numeric input 1–1000).
  - **Browser source fields:** URL (text), Width (number, default 1920), Height (number, default 1080), Transparent Background (checkbox). **No "Capture Audio" field.**
  - Form inputs: `background: #0A0C10; border: 1px solid #3A3D45; borderRadius: 3; color: #D0D2D8; fontSize: 11; fontFamily: DM Sans; padding: 4px 8px`.
  - Hint text: `DM Sans 10px italic color: #606878; margin-bottom: 8px`.

**Footer:** `display: flex; justify-content: flex-end; gap: 8px; padding: 10px 14px; border-top: 1px solid #2A2D35; background: #0A0C0F`.
- Cancel: `padding: 5px 16px; background: #1A1D22; border: 1px solid #3A3D45; borderRadius: 3; color: #808898; fontSize: 11`.
- Add: `padding: 5px 16px; background: linear-gradient(180deg,#FF5A2C,#CC3A18); border: 1px solid #FF5A2C80; borderRadius: 3; color: #fff; fontSize: 11; fontWeight: 700`.

---

### 5.2 Go Live Modal

**File:** `client/src/components/GoLiveModal.tsx`
**Trigger:** "● Stream" button in Bottom Action Toolbar.
**Type:** Radix `<Dialog>` (shadcn/ui).

**Dialog content:** `background: #0F1114; border: 1px solid #3A3D45; borderRadius: 8; width: 520px; max-height: 90vh; overflow-y: auto; padding: 0`.

The modal has four sequential steps:

#### Step 1: Config
- **Platform selector:** Four buttons in a row — YouTube, Twitch, Facebook, Custom. Each: `width: 72px; height: 64px; borderRadius: 8`. Selected: `background: {platform.color}18; border: 1.5px solid {platform.color}; box-shadow: 0 0 12px {platform.color}30; transform: scale(1.04)`.
- **Stream title input:** Full-width text input.
- **Stream key input:** Password-type input.
- **Category select:** Dropdown.
- **Start Stream button:** Full-width, orange gradient.

#### Step 2: Checking (pre-flight)
Five animated check items run sequentially: Audio Devices, Active Scene, Video Encoder, Network, Stream Key. Each shows a spinner → checkmark/warning/fail icon.

#### Step 3: Countdown
Large centered countdown number (3→2→1) with a progress ring animation.

#### Step 4: Live
Confirmation screen with "YOU ARE LIVE" in large text and a "Stop Stream" button.

---

### 5.3 Input Settings Drawer

**File:** `client/src/components/InputSettingsDrawer.tsx`
**Trigger:** Settings gear icon on a tile header, or "⚙ Input Settings…" from right-click context menu.
**Type:** Slides in from the right edge of the screen (not a modal overlay).

**Drawer container:** `position: fixed; top: 0; right: 0; height: 100vh; width: 380px; background: #0F1114; border-left: 1px solid #3A3D45; z-index: 500; display: flex; flex-direction: column; box-shadow: -8px 0 32px rgba(0,0,0,0.6); transform: translateX(0) (open) or translateX(100%) (closed); transition: transform 0.25s cubic-bezier(0.23,1,0.32,1)`.

**Header:** `display: flex; align-items: center; justify-content: space-between; padding: 10px 14px; background: linear-gradient(180deg,#1A1D22,#141619); border-bottom: 1px solid #3A3D45`.
- Left: `<Settings size={14} color="#4F9EFF">` + source name in `DM Sans 700 13px #F0F2F8`.
- Right: `<RotateCcw size={12}>` reset button + `<X size={14}>` close button.

**Tab bar:** Six tabs — General | Position | Colour Adjust | Effects | Layers | Audio Settings. Each tab: `padding: 5px 10px; DM Sans 10px 600; border-bottom: 2px solid transparent (inactive) or #4F9EFF (active); color: #606878 (inactive) or #4F9EFF (active)`.

**Tab content area:** `flex: 1; overflow-y: auto; padding: 12px 14px`.

Form field styles:
- Label: `DM Sans 10px color: #808898; margin-bottom: 2px`.
- Input: `background: #0A0C10; border: 1px solid #3A3D45; borderRadius: 3; color: #D0D2D8; fontSize: 11; padding: 4px 8px; width: 100%`.
- Slider: `accentColor: #4F9EFF; height: 3px; width: 100%`.
- Section title: `DM Sans 9px 700 color: #4F9EFF; letterSpacing: 0.1em; uppercase; border-bottom: 1px solid #2A2D35; margin-top: 12px; margin-bottom: 8px; padding-bottom: 4px`.
- Two-column grid: `display: grid; grid-template-columns: 1fr 1fr; gap: 8px`.

**Footer:** `padding: 10px 14px; border-top: 1px solid #2A2D35; display: flex; justify-content: flex-end; gap: 8px`.
- Cancel: default button style.
- Save: orange gradient button.

---

### 5.4 New Project Confirmation Dialog

Radix `<AlertDialog>`. Triggered by "New" option in a menu.
`background: #1A1D22; border: 1px solid #3A3D45; color: #D0D2D8`.
- Title: "New Project" in `color: #F0F0F0`.
- Description: "This will clear all scenes and inputs. Any unsaved changes will be lost. Are you sure?" in `color: #808898`.
- Cancel: default button.
- Confirm: destructive red button.

---

### 5.5 Keyboard Shortcuts Overlay

Triggered by the `?` button in the Top Toolbar or `Shift+?` hotkey. Renders as an inline panel that slides down below the toolbar (not a modal). Lists all keyboard shortcuts in a two-column table format.

---

## 6. Chat Panel Page

**Route:** `/chat`  
**File:** `client/src/pages/ChatPanel.tsx`

Wrapped in `<AppSidebar>`. Full-height flex layout.

**Three-column layout:**
1. **Platform connections sidebar** (left, ~220px) — connection status badges for Twitch, YouTube, Facebook. Each platform shows: colored icon, name, `<ConnBadge>` (Connected/Connecting/Disconnected/Error), viewer count, channel name input, API key input, Connect/Disconnect button.
2. **Chat feed** (center, flex: 1) — scrollable list of `<ChatMessage>` items. Each message: platform color left border, username in platform color, message text, timestamp. Pinned messages shown at top with a pin icon. Donation/sub messages have a colored background highlight.
3. **Activity feed** (right, ~240px) — follows, subs, donations, raids shown as event cards with icons and amounts.

**Bottom bar:** Text input + Send button + emoji picker toggle + moderation settings.

**Color scheme:** Same dark theme. Platform colors: Twitch `#9146FF`, YouTube `#FF0000`, Facebook `#1877F2`.

---

## 7. Analytics Page

**Route:** `/analytics`  
**File:** `client/src/pages/Analytics.tsx`

Wrapped in `<AppSidebar>`. Dark background `#0A0C10`.

**Layout:** Top summary cards row + tabbed chart area below.

**Summary cards:** Viewer count, peak viewers, watch time, revenue — each in a `background: #1A1D22; border: 1px solid #2A2D35; borderRadius: 6` card with a large number in `Bebas Neue` and a trend indicator.

**Chart area:** Line charts and bar charts using Recharts or a similar library. Chart backgrounds: `#0F1114`. Grid lines: `#2A2D35`. Chart colors follow the brand palette (blue, emerald, violet, amber).

---

## 8. Settings Page

**Route:** `/settings`  
**File:** `client/src/pages/Settings.tsx`

Wrapped in `<AppSidebar>`. Two-column layout: left tab nav + right content panel.

**Left tab nav** (~180px): vertical list of setting categories — General, Streaming, Recording, Video, Audio, Hotkeys, Advanced, Plugins. Active tab: `background: #FF5A2C18; border-left: 2px solid #FF5A2C; color: #FF5A2C`. Inactive: `color: #808898`.

**Right content panel:** `background: #0F1114; border: 1px solid #2A2D35; borderRadius: 6; padding: 20px 24px`.

**Shared form components:**
- `<Section>`: `background: #1A1D22; border: 1px solid #2A2D35; borderRadius: 4; padding: 16px; margin-bottom: 24px`.
- `<Toggle>`: 36×20px pill toggle. On: `background: #FF5A2C`. Off: `background: #141619`.
- `<Seg>`: Segmented control. Active segment: `background: {color}18; color: {color}`. Inactive: `background: #1A1D22; color: #606878`.
- `<TxtInput>`: `height: 30px; background: #1A1D22; border: 1px solid #3A3D45; borderRadius: 4; color: #C0C2C8; fontSize: 12; padding: 0 10px`.
- `<NumInput>`: Same as TxtInput but `type="number"; fontFamily: JetBrains Mono`.
- `<Sel>`: `height: 30px; background: #1A1D22; border: 1px solid #3A3D45; borderRadius: 4; color: #C0C2C8; fontSize: 12`.

---

## 9. Scoreboard Page

**Route:** `/scoreboard`  
**File:** `client/src/pages/ScoreboardPage.tsx`

Wrapped in `<AppSidebar>`. Two-panel layout: left controls + right live preview.

**Left controls panel:** Sport preset selector, layout style selector (Lower Third / Center Banner / Corner Compact / Full Width), color theme selector, team name/score/color inputs, period/timer controls, event title input, visibility toggle.

**Sport presets:** Generic, Pool/Billiards, Basketball, Soccer, Tennis, Custom.

**Layout options:** Lower Third, Center Banner, Corner Compact, Full Width — each with an icon button.

**Color themes:** Dark, Light, Team, Neon, Minimal.

**Right preview panel:** Shows a live preview of the scoreboard overlay rendered on a dark background. The overlay itself uses the selected theme colors and layout.

**Timer:** Count-up or count-down timer with Start/Stop/Reset controls.

---

## 10. Schedule Page

**Route:** `/schedule`  
**File:** `client/src/pages/SchedulePage.tsx`

Wrapped in `<AppSidebar>`. Calendar + event list layout.

**Top bar:** Month/week navigation, "New Event" button (orange gradient), search input, filter dropdown.

**Calendar view:** Month grid. Days with events show colored dots. Clicking a day shows events for that day.

**Event list:** Each event card: `background: #1A1D22; border: 1px solid #2A2D35; borderRadius: 6`. Shows: event title, sport type, venue, start time, duration, platform badge, status badge (Upcoming/Live/Completed/Cancelled), countdown timer, reminder toggle, "Start Stream" button.

**Platform colors:** YouTube `#FF0000`, Twitch `#9146FF`, Facebook `#1877F2`, Custom `#FF5A2C`.

**Status colors:** Upcoming `#4F9EFF`, Live `#FF5A2C` (pulsing), Completed `#22C55E`, Cancelled `#606878`.

**New/Edit Event modal:** Radix `<Dialog>`. Fields: title, description, sport, venue, start date/time, duration, platform, stream key, tags, reminder toggle.

---

## 11. Context Menus

All context menus use Radix `<ContextMenu>` with consistent styling:
`background: #1A1D22; border: 1px solid #3A3D45; borderRadius: 4; padding: 4px 0; box-shadow: 0 8px 32px rgba(0,0,0,0.7)`.

Item style: `padding: 5px 10px; fontSize: 11; fontFamily: DM Sans; cursor: pointer`.
Hover: `background: #2A2D35`.
Label (non-clickable): `padding: 3px 10px; fontSize: 9; color: #606878; letterSpacing: 0.08em; uppercase`.
Separator: `height: 1px; background: #2A2D35; margin: 2px 0`.
Destructive item: `color: #EF4444`.

### Scene Context Menu (right-click on scene row)
- Label: scene name
- "Set as Active" (`color: #4F9EFF`)
- Separator
- "Rename" (Pencil icon)
- "Duplicate" (Copy icon)
- Separator
- "Delete Scene" (Trash2 icon, `color: #EF4444`)

### Source Tile Context Menu (right-click on input tile)
- Label: source name
- "Select" (`color: #4F9EFF`)
- "Hide" / "Show"
- "Duplicate"
- "⚙ Input Settings…" (`color: #4F9EFF`)
- Separator
- "Delete Input" (`color: #EF4444`)

---

## 12. Shared Sub-Components

### SmoothBrowserFrame

Renders a live `<iframe>` for browser sources. Uses two iframe slots (A and B) that alternate to avoid flicker on URL changes. The active slot fades in with `opacity: 0 → 1` transition (300ms). The inactive slot stays at `opacity: 0`. Both iframes: `position: absolute; inset: 0; width: 100%; height: 100%; border: none`. The `handleLoad` callbacks are stable `useCallback` refs — they do not change identity on re-render.

### ProgramCanvas

`memo`-wrapped. Renders all visible sources in a scene as absolutely-positioned divs within a `position: relative; overflow: hidden` container. Supports:
- Drag to reposition (mouse down → move → up).
- Resize handles (8 cardinal/diagonal handles, 8×8px blue squares with `borderRadius: 2`).
- Source selection (click → blue border `2px solid #4F9EFF`).
- Click on empty canvas → deselect.
- Empty state: centered SVG icon + "Rack this scene" + "Add sources to build your layout" text.

### LiveTimecode

`memo`-wrapped. Self-contained component with its own `setInterval` (1-second tick). Renders current wall-clock time as `HH:MM:SS` in `JetBrains Mono 10px`. Color: `#FF5A2C` when live, `#606878` otherwise. **Does not cause Dashboard re-renders.**

### RecordTimecode

`memo`-wrapped. Self-contained component with its own `setInterval` (1-second tick). Renders elapsed recording time as `HH:MM:SS` in `JetBrains Mono 10px #86EFAC`. Only ticks when `isRecording === true`. **Does not cause Dashboard re-renders.**

### VUMeterVertical

`memo`-wrapped. SVG-based vertical VU meter. `width: 6px; height: 120px`. Segments light up proportionally to `volume` prop. Color: channel color at low levels → amber → red at peak. Dark when `active === false`.

### AppSidebar (wrapper)

Wraps every page. Renders the 52px left sidebar + a `div` for page content (`flex: 1; min-width: 0; height: 100vh; overflow: hidden`). The sidebar and content area are in a `display: flex; height: 100vh; overflow: hidden` root div.

---

## Appendix: Key Inline Style Patterns

These patterns appear throughout the codebase and must be preserved:

```
// Standard dark button
background: "linear-gradient(180deg,#1E2128,#16181E)"
border: "1px solid #3A3D45"
borderRadius: 3
color: "#C8CAD0"
fontSize: 11
fontFamily: "'DM Sans',sans-serif"
boxShadow: "0 2px 6px rgba(0,0,0,0.5), inset 0 1px 0 rgba(255,255,255,0.08)"

// Active/on button (example: recording)
background: "linear-gradient(180deg,#1A3A2A,#122A1E)"
border: "1px solid #22C55E40"
color: "#86EFAC"

// Stream/brand button
background: "linear-gradient(180deg,#FF5A2C,#CC3A18)"
border: "1px solid #FF5A2C80"
color: "#fff"
fontWeight: 700
boxShadow: "0 0 16px rgba(255,90,44,0.4), 0 2px 6px rgba(0,0,0,0.5), inset 0 1px 0 rgba(255,255,255,0.15)"

// Section/panel header
background: "linear-gradient(180deg,#1A1D22,#141619)"
borderBottom: "1px solid #2A2D35"

// Dropdown/context menu
background: "#1A1D22"
border: "1px solid #3A3D45"
borderRadius: 4
boxShadow: "0 8px 32px rgba(0,0,0,0.7)"
```

---

*Document generated from source code of commit `291c4214` (nexus-broadcast-ui). All measurements are exact values from the codebase.*
