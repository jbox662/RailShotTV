# Nexus Broadcast — Qt 6 Widgets Developer Handoff

**Target:** Native C++ application using Qt 6 Widgets + QSS (Qt Style Sheets). No QML. No web technologies.
**Prototype reference:** React/Tailwind prototype at `nexus-broadcast-ui/` (see attached ZIP).
**Instruction:** Document every measurement, color, and behavior exactly as implemented. Do not simplify or redesign.

---

## Table of Contents

1. [Design Token Reference](#1-design-token-reference)
2. [Typography System](#2-typography-system)
3. [Global Application Shell & Layout](#3-global-application-shell--layout)
4. [Sidebar Component](#4-sidebar-component)
5. [Top Bar (Page Header)](#5-top-bar-page-header)
6. [Dashboard Screen — Full Specification](#6-dashboard-screen--full-specification)
7. [Scene Editor Screen — Full Specification](#7-scene-editor-screen--full-specification)
8. [Settings Screen — Full Specification](#8-settings-screen--full-specification)
9. [Chat & Audience Screen — Full Specification](#9-chat--audience-screen--full-specification)
10. [Analytics Screen — Full Specification](#10-analytics-screen--full-specification)
11. [Reusable Component Specifications](#11-reusable-component-specifications)
12. [All Widget State Specifications](#12-all-widget-state-specifications)
13. [SVG Icon Inventory](#13-svg-icon-inventory)
14. [Static vs. Dynamic Data Guide](#14-static-vs-dynamic-data-guide)
15. [Qt Widget & Layout Mapping](#15-qt-widget--layout-mapping)
16. [Complete QSS Stylesheet](#16-complete-qss-stylesheet)
17. [Responsive Rules](#17-responsive-rules)

---

## 1. Design Token Reference

All colors are defined here. Use these exact values everywhere. Do not approximate.

### 1.1 Surface Colors

| Token Name | Hex / RGBA | Qt `QColor` Constructor | Usage |
|---|---|---|---|
| `surface-root` | `#0E0F14` | `QColor(14, 15, 20)` | App root background, main content area |
| `surface-topbar` | `#0D0E12` | `QColor(13, 14, 18)` | Top bar background (all screens) |
| `surface-sidebar` | `#111318` | `QColor(17, 19, 24)` | Sidebar background |
| `surface-panel` | `#111318` | `QColor(17, 19, 24)` | Panel / card backgrounds |
| `surface-elevated` | `#16181F` | `QColor(22, 24, 31)` | Settings tab content area, elevated panels |
| `surface-input` | `#0E0F14` | `QColor(14, 15, 20)` | Text inputs, selects, number fields |
| `surface-active` | `#1A1D2B` | `QColor(26, 29, 43)` | Selected nav item, selected source row |
| `surface-canvas` | `#0A0B0F` | `QColor(10, 11, 15)` | Program output canvas, scene editor workspace |
| `surface-channel` | `#0E0F14` | `QColor(14, 15, 20)` | Individual audio channel cards |
| `surface-chroma-card` | `#0E0F14` | `QColor(14, 15, 20)` | Chroma key sub-panel |

### 1.2 Border Colors

| Token Name | RGBA | Qt `QColor` Constructor | Usage |
|---|---|---|---|
| `border-primary` | `rgba(255,255,255,0.07)` | `QColor(255,255,255,18)` | Panel edges, sidebar border |
| `border-panel` | `rgba(255,255,255,0.08)` | `QColor(255,255,255,20)` | Card/panel borders |
| `border-section` | `rgba(255,255,255,0.06)` | `QColor(255,255,255,15)` | Inner section dividers within panels |
| `border-subtle` | `rgba(255,255,255,0.05)` | `QColor(255,255,255,13)` | FormRow bottom dividers |
| `border-input` | `rgba(255,255,255,0.12)` | `QColor(255,255,255,31)` | Text inputs, selects |
| `border-input-focus` | `rgba(59,130,246,0.5)` | `QColor(59,130,246,128)` | Input focus ring |
| `border-divider` | `rgba(255,255,255,0.10)` | `QColor(255,255,255,26)` | Vertical dividers in top bar |
| `border-ghost` | `rgba(255,255,255,0.08)` | `QColor(255,255,255,20)` | Inactive scene card, inactive tool button |

### 1.3 Accent Colors (Semantic — Never Swap)

| Token Name | Hex | Qt `QColor` | Semantic Meaning |
|---|---|---|---|
| `accent-blue` | `#3B82F6` | `QColor(59,130,246)` | Primary interaction, active states, CTAs, nav active border |
| `accent-blue-bg` | `rgba(59,130,246,0.10)` | `QColor(59,130,246,26)` | Blue chip/badge background |
| `accent-blue-border` | `rgba(59,130,246,0.25)` | `QColor(59,130,246,64)` | Blue chip/badge border |
| `accent-blue-glow` | `rgba(59,130,246,0.30)` | `QColor(59,130,246,77)` | Active scene card box-shadow |
| `accent-red` | `#EF4444` | `QColor(239,68,68)` | LIVE badge, End Stream button, critical alerts |
| `accent-red-bg` | `rgba(239,68,68,0.15)` | `QColor(239,68,68,38)` | LIVE badge background |
| `accent-red-border` | `rgba(239,68,68,0.35)` | `QColor(239,68,68,89)` | LIVE badge border |
| `accent-green` | `#22C55E` | `QColor(34,197,94)` | Connected, healthy, SRC ACTIVE, CPU/GPU good |
| `accent-amber` | `#F59E0B` | `QColor(245,158,11)` | Warning, degraded bitrate, donation highlight |
| `accent-cyan` | `#06B6D4` | `QColor(6,182,212)` | Timecodes, uptime counter, secondary data |
| `accent-purple` | `#9146FF` | `QColor(145,70,255)` | Twitch platform color (platform selector only) |
| `accent-youtube` | `#FF0000` | `QColor(255,0,0)` | YouTube platform color (platform selector only) |
| `accent-facebook` | `#1877F2` | `QColor(24,119,242)` | Facebook platform color (platform selector only) |

### 1.4 Text Colors

| Token Name | RGBA | Qt `QColor` | Usage |
|---|---|---|---|
| `text-primary` | `#FFFFFF` | `QColor(255,255,255)` | Active nav item, selected source, KPI values |
| `text-secondary` | `rgba(255,255,255,0.60)` | `QColor(255,255,255,153)` | Inactive nav items, secondary labels |
| `text-tertiary` | `rgba(255,255,255,0.40)` | `QColor(255,255,255,102)` | Section headers, form labels |
| `text-muted` | `rgba(255,255,255,0.35)` | `QColor(255,255,255,89)` | Metadata, timestamps, placeholder |
| `text-dim` | `rgba(255,255,255,0.25)` | `QColor(255,255,255,64)` | Instrumentation strip labels (TC, SCENE, etc.) |
| `text-ghost` | `rgba(255,255,255,0.15)` | `QColor(255,255,255,38)` | Placeholder icons, empty state text |
| `text-separator` | `rgba(255,255,255,0.12)` | `QColor(255,255,255,31)` | Pipe `|` separators in top bar |
| `text-disabled` | `rgba(255,255,255,0.20)` | `QColor(255,255,255,51)` | Disabled button labels |

### 1.5 Spacing & Radius Tokens

| Token | Value | Usage |
|---|---|---|
| `spacing-xs` | 4px | Icon-to-label gap, badge padding |
| `spacing-sm` | 8px | Gap between controls, inner padding |
| `spacing-md` | 12px | Panel padding, section gap |
| `spacing-lg` | 16px | Outer content padding |
| `radius-sm` | 4px | Badges, chips, small buttons |
| `radius-md` | 6px | Buttons, inputs, number fields |
| `radius-lg` | 8px | Panels, cards, scene cards |
| `radius-full` | 9999px | Toggle switches, health bars, VU bar segments |

---

## 2. Typography System

Three fonts are required. Embed them as application resources (`.ttf` or `.otf` files) and load via `QFontDatabase::addApplicationFont()`.

| Font Family | Weights Required | Role | Qt Usage |
|---|---|---|---|
| **Space Grotesk** | 500, 600, 700 | Display / Brand — page titles, KPI values, logo wordmark | `QFont("Space Grotesk", size, QFont::Bold)` |
| **Inter** | 400, 500, 600 | UI / Body — nav labels, form labels, body copy, button text | `QFont("Inter", size)` |
| **JetBrains Mono** | 400, 500, 700 | Data / Precision — timecodes, bitrate, dB, FPS, resolution | `QFont("JetBrains Mono", size)` |

### 2.1 Type Scale

| Size | Font | Weight | Color Token | Usage |
|---|---|---|---|---|
| 9px | Inter or JetBrains Mono | 400–600 | `text-muted` / `text-dim` | Micro labels, badge text, version, signal indicators, VU dB readouts |
| 10px | Inter (uppercase) | 600 | `text-tertiary` | Section headers (`SCENES`, `AUDIO MIXER`, `STREAM STATUS`), instrumentation strip labels |
| 10px | JetBrains Mono | 400 | `text-muted` | Top bar metadata (YT, FPS, resolution, codec, SIG OK) |
| 11px | Inter | 400–500 | `text-secondary` | Status text, timestamps, channel sub-labels |
| 12px | Inter | 400–500 | `text-secondary` | Form labels, nav items, settings values, source names |
| 12px | Space Grotesk | 700 | `text-primary` | Page title in top bar (all-caps, letter-spacing 0.1em) |
| 13px | Inter | 500–600 | `text-primary` | End Stream button text, bitrate value label |
| 13px | JetBrains Mono | 700 | `accent-blue` | Bitrate kbps value in Stream Status panel |
| 15px | JetBrains Mono | 700 | `text-primary` | Viewers count, uptime in stat cards |
| 20px | Space Grotesk | 700 | `text-primary` | KPI card values (Analytics screen) |
| 28px+ | Space Grotesk | 700 | `text-primary` | Hero numbers (peak viewers, revenue) |

### 2.2 Section Header Pattern

All panel section headers (e.g., "SCENES", "AUDIO MIXER", "STREAM STATUS", "SOURCES") use this exact style:

```
Font:           Inter
Weight:         600
Size:           10px
Color:          rgba(255,255,255,0.40)   [text-tertiary]
Letter-spacing: 0.1em (approximately 1px tracking per 10px)
Transform:      UPPERCASE
```

In Qt: `QLabel` with `QFont("Inter", 8, QFont::DemiBold)`, `setStyleSheet("color: rgba(255,255,255,102); letter-spacing: 1px;")`.

---

## 3. Global Application Shell & Layout

### 3.1 Window Structure

The application window is a fixed horizontal split: a persistent left sidebar and a right main content area. There is no top-level menu bar or title bar chrome — use `Qt::FramelessWindowHint` with a custom title bar or `QMainWindow` with `menuBar()->hide()`.

```
QMainWindow (min: 1024×706, default: 1440×900)
└── Central Widget: QWidget
    └── QHBoxLayout (spacing: 0, margins: 0)
        ├── SidebarWidget          (fixed width: 220px)
        └── MainContentStack       (QStackedWidget, flex-1)
            ├── DashboardPage
            ├── SceneEditorPage
            ├── ChatPage
            ├── AnalyticsPage
            └── SettingsPage
```

### 3.2 Dimensional Constants

| Constant | Value | Description |
|---|---|---|
| `SIDEBAR_WIDTH` | 220px | Fixed, never collapses on desktop |
| `TOPBAR_HEIGHT` | 46px | Minimum height for all page top bars |
| `SIDEBAR_LOGO_HEIGHT` | 56px | Logo block minimum height |
| `DASHBOARD_RIGHT_PANEL_WIDTH` | 240px | Stream Status right panel |
| `SCENE_EDITOR_RIGHT_PANEL_WIDTH` | 320px | Sources + Properties right panel |
| `SETTINGS_TAB_NAV_WIDTH` | 180px | Settings left tab navigation |
| `CHAT_LEFT_PANEL_WIDTH` | 220px | Chat left utility rail |
| `CHAT_RIGHT_PANEL_WIDTH` | 260px | Chat right activity rail |
| `SCENE_CARD_WIDTH` | 110px | Scene switcher card width |
| `SCENE_CARD_HEIGHT` | 70px | Scene switcher card height |
| `TOOL_BUTTON_SIZE` | 32px | Scene editor vertical tool buttons |
| `HEALTH_BAR_HEIGHT` | 4px | Stream health progress bars |
| `VU_BAR_WIDTH` | 3px | Individual VU meter bar width |
| `VU_BAR_COUNT` | 20 | Number of bars per VU meter |
| `VU_METER_HEIGHT` | 16px | Total VU meter container height |
| `MASTER_SLIDER_WIDTH` | 80px | Master volume slider width |
| `BITRATE_SPARKLINE_HEIGHT` | 40px | Bitrate SVG sparkline height |
| `TOGGLE_WIDTH` | 36px | Toggle switch width (Settings) / 32px (Scene Editor) |
| `TOGGLE_HEIGHT` | 20px | Toggle switch height (Settings) / 18px (Scene Editor) |
| `TOGGLE_THUMB_SIZE` | 16px | Toggle thumb diameter (Settings) / 14px (Scene Editor) |

---

## 4. Sidebar Component

### 4.1 Structure

```
SidebarWidget (QWidget, fixed width: 220px, background: #111318)
├── LogoBlock (QWidget, min-height: 56px, border-bottom: 1px rgba(255,255,255,18))
│   ├── LogoIcon (QLabel, 28×28px, background: #3B82F6, border-radius: 4px)
│   │   └── Radio SVG icon (15×15px, white)
│   ├── BrandStack (QVBoxLayout, spacing: 0)
│   │   ├── "NEXUS" (QLabel, Space Grotesk Bold, 13px, #fff, letter-spacing: 0.04em)
│   │   └── "BROADCAST" (QLabel, Inter Regular, 9px, rgba(255,255,255,102), letter-spacing: 0.12em)
│   └── ProBadge (QLabel, "PRO", 9px Inter Bold, #fff, background: #3B82F6, border-radius: 4px, padding: 2px 6px)
├── NavList (QVBoxLayout, top-padding: 8px, bottom-padding: 8px)
│   └── NavItem × 5 (see 4.2)
└── StatusFooter (QWidget, border-top: 1px rgba(255,255,255,18), padding: 12px 16px)
    ├── ConnectionRow (QHBoxLayout)
    │   ├── WiFi icon (13×13px, #22C55E)
    │   └── "Connected · Excellent" (Inter 500, 11px, #22C55E)
    ├── LiveRow (QHBoxLayout, margin-top: 4px)
    │   ├── LiveDot (QLabel, 8×8px, background: #EF4444, border-radius: 4px, animated pulse)
    │   └── "LIVE · 01:23:47" (JetBrains Mono, 11px, rgba(255,255,255,128))
    └── SignalRow (QHBoxLayout, margin-top: 8px, border-top: 1px rgba(255,255,255,15))
        ├── SignalBars (custom QWidget — see 4.3)
        └── "NEXUS v2.4.1" (JetBrains Mono, 9px, rgba(255,255,255,77))
```

### 4.2 Nav Item States

Each nav item is a `QPushButton` or `QWidget` with mouse event handling. Exact pixel values:

| State | Background | Left Border | Text Color | Icon strokeWidth | Padding-left |
|---|---|---|---|---|---|
| **Default** | transparent | 3px transparent | rgba(255,255,255,128) | 1.5 | 16px |
| **Hover** | rgba(255,255,255,13) | 3px transparent | rgba(255,255,255,204) | 1.5 | 16px |
| **Active/Selected** | #1A1D2B | 3px solid #3B82F6 | #FFFFFF | 2.0 | 13px (compensates for border) |
| **Pressed** | rgba(255,255,255,20) | 3px transparent | rgba(255,255,255,204) | 1.5 | 16px |

Nav item internal layout:
- Height: 38px minimum
- Icon: 16×16px, left-aligned
- Gap between icon and label: 12px
- Label: Inter, 13px, weight 400 (default) / 500 (active)
- Transition: background color 120ms ease-out

### 4.3 Signal Bars Widget

Five vertical bars, rendered as a custom `QWidget::paintEvent`:

| Bar index (0–4) | Width | Height | Color (active) | Color (inactive) |
|---|---|---|---|---|
| 0 | 3px | 10px | #3B82F6 | rgba(255,255,255,26) |
| 1 | 3px | 12px | #3B82F6 | rgba(255,255,255,26) |
| 2 | 3px | 14px | #3B82F6 | rgba(255,255,255,26) |
| 3 | 3px | 16px | #3B82F6 | rgba(255,255,255,26) |
| 4 | 3px | 18px | rgba(255,255,255,26) | rgba(255,255,255,26) |

Bars 0–3 are active (blue), bar 4 is inactive (dim). Gap between bars: 1px. Border-radius: 1px on each bar. Bars are bottom-aligned.

---

## 5. Top Bar (Page Header)

Every screen has an identical-height top bar. The content changes per screen but the container is always:

```
TopBarWidget (QWidget)
  height:           46px minimum
  background:       #0D0E12
  border-bottom:    1px solid rgba(255,255,255,18)
  padding:          0 16px
  layout:           QHBoxLayout, spacing: 8px, vertical-center
```

### 5.1 Top Bar — Dashboard

Left-to-right element sequence with exact spacing:

```
[PAGE TITLE] [DIVIDER] [LIVE BADGE] [PLATFORM] [|] [TIMECODE] [|] [RESOLUTION] [|] [FPS] [|] [CODEC]   ←→   [SIG OK] [|] [PROFILE] [SETTINGS ICON]
```

| Element | Widget | Font | Size | Color | Notes |
|---|---|---|---|---|---|
| "DASHBOARD" | QLabel | Space Grotesk Bold | 12px | #fff | letter-spacing: 0.1em |
| Vertical divider | QFrame (VLine) | — | 1×16px | rgba(255,255,255,26) | |
| LIVE badge | Custom QWidget | Inter Bold | 10px | #EF4444 | bg: rgba(239,68,68,38), border: 1px rgba(239,68,68,89), radius: 4px, padding: 2px 8px, includes pulsing dot |
| "YT" | QLabel | JetBrains Mono | 10px | rgba(255,255,255,77) | Platform abbreviation |
| Pipe separators | QLabel | JetBrains Mono | 10px | rgba(255,255,255,31) | "|" character |
| Timecode | QLabel | JetBrains Mono | 10px | #06B6D4 | **DYNAMIC** — updates every second |
| "1920×1080" | QLabel | JetBrains Mono | 10px | rgba(255,255,255,89) | **DYNAMIC** — from encoder config |
| "60fps" | QLabel | JetBrains Mono | 10px | rgba(255,255,255,89) | **DYNAMIC** — from encoder config |
| "H.264" | QLabel | JetBrains Mono | 10px | #22C55E | **DYNAMIC** — from encoder config |
| [spacer] | QSpacerItem | — | — | — | pushes right side |
| SIG OK dot | QLabel | — | 6×6px | #22C55E | circle, border-radius: 3px |
| "SIG OK" | QLabel | JetBrains Mono | 9px | rgba(255,255,255,89) | **DYNAMIC** — from network status |
| Divider | QFrame | — | 1×16px | rgba(255,255,255,20) | |
| "Default Profile" | QLabel | Inter | 10px | rgba(255,255,255,77) | **DYNAMIC** — from profile name |
| Settings2 icon | QPushButton (icon-only) | — | 13×13px | rgba(255,255,255,77) | Opens profile settings |

### 5.2 Top Bar — Scene Editor

```
[SCENE EDITOR] [DIVIDER] [PROJECT CHIP] [→ spacer →] [● LIVE 00:00:00] [|] [REC 00:00:00] [|] [CPU x.x%] [|] [xx.xx FPS] [|] [xxxx KB/s]
```

| Element | Color | Notes |
|---|---|---|
| "SCENE EDITOR" | #fff | Space Grotesk Bold 12px, letter-spacing 0.1em |
| Project chip | bg: rgba(59,130,246,26), border: 1px rgba(59,130,246,64) | JetBrains Mono 10px, #3B82F6, radius: 4px, padding: 2px 8px |
| LIVE timecode | #EF4444 dot + rgba(255,255,255,77) text | **DYNAMIC** |
| REC timecode | rgba(255,255,255,77) | **DYNAMIC** |
| CPU % | label: rgba(255,255,255,77), value: #22C55E | **DYNAMIC** |
| FPS | label: rgba(255,255,255,77), value: #3B82F6 | **DYNAMIC** |
| KB/s | label: rgba(255,255,255,77), value: #06B6D4 | **DYNAMIC** |

---

## 6. Dashboard Screen — Full Specification

### 6.1 Layout

```
DashboardPage (QWidget, background: #0E0F14)
├── TopBar (46px, see §5.1)
└── ContentArea (QHBoxLayout, spacing: 0)
    ├── CenterColumn (QVBoxLayout, padding: 12px, spacing: 12px, flex-1)
    │   ├── ProgramOutputPanel
    │   ├── ScenesPanel
    │   └── AudioMixerPanel
    └── StreamStatusPanel (fixed width: 240px, border-left: 1px rgba(255,255,255,18))
```

### 6.2 Program Output Panel

```
ProgramOutputPanel (QWidget, background: #111318, border: 1px rgba(255,255,255,20), border-radius: 8px)
├── PanelHeader (QHBoxLayout, height: 32px, padding: 0 12px, border-bottom: 1px rgba(255,255,255,15))
│   ├── "PROGRAM OUTPUT" label (section header style)
│   └── LIVE badge (right-aligned)
├── CanvasArea (custom QWidget, aspect-ratio: 16:9, background: #0A0B0F)
│   ├── GridOverlay (SVG-style grid, opacity: 6%, see §6.2.1)
│   ├── PlaceholderContent (centered Monitor icon + resolution string)
│   └── CornerMarkers × 4 (see §6.2.2)
└── InstrumentationStrip (QWidget, height: 28px, background: #0A0B0F, border-top: 1px rgba(255,255,255,13))
    ├── LeftGroup: TC [timecode] | SCENE [name] | SRC ACTIVE
    └── RightGroup: VIEWERS [count] | BITRATE [kbps]
```

**Canvas aspect ratio enforcement:** Use `QWidget::heightForWidth()` returning `width * 9 / 16`, or constrain with a `QSizePolicy` with `setHeightForWidth(true)`.

#### 6.2.1 Canvas Grid Overlay

Render in `paintEvent` using `QPainter`:
- Grid cell size: 40×40px
- Line color: `QColor(255,255,255,15)` (opacity ~6%)
- Line width: 0.5px (use `QPen` with `setWidthF(0.5)`)
- Pattern: horizontal + vertical lines at every 40px interval
- Rendering: `QPainter::Antialiasing` off for crisp pixel lines

#### 6.2.2 Corner Bracket Markers

Four L-shaped brackets, one at each corner of the canvas. Render in `paintEvent`:
- Size: 16×16px each
- Color: `QColor(59,130,246,153)` (rgba(59,130,246,0.6))
- Line width: 1.5px
- Position: 8px inset from each corner edge
- Shape: top-left = horizontal right + vertical down; top-right = horizontal left + vertical down; bottom-left = horizontal right + vertical up; bottom-right = horizontal left + vertical up

#### 6.2.3 Instrumentation Strip

Height: 28px. Background: `#0A0B0F`. Border-top: 1px `rgba(255,255,255,13)`. Padding: 0 12px.

Left group (JetBrains Mono, 9px):
- "TC" label: `rgba(255,255,255,64)` [text-dim]
- Timecode value: `#06B6D4` [accent-cyan] — **DYNAMIC**, format `HH:MM:SS:FF`
- "|" separator: `rgba(255,255,255,26)` [text-separator]
- "SCENE" label: `rgba(255,255,255,64)`
- Scene name: `#FFFFFF` — **DYNAMIC**
- "|" separator
- "SRC ACTIVE": `#22C55E` [accent-green] — **DYNAMIC** (green = active, red = no source)

Right group (JetBrains Mono, 9px):
- "VIEWERS" label: `rgba(255,255,255,64)`
- Viewer count: `#3B82F6` [accent-blue], weight 600 — **DYNAMIC**
- "|" separator
- "BITRATE" label: `rgba(255,255,255,64)`
- Bitrate value: `#22C55E` if ≥7000 kbps, `#F59E0B` if <7000 kbps — **DYNAMIC**

### 6.3 Scenes Panel

```
ScenesPanel (QWidget, background: #111318, border: 1px rgba(255,255,255,20), border-radius: 8px)
├── PanelHeader (QHBoxLayout, height: 32px, padding: 0 12px, border-bottom: 1px rgba(255,255,255,15))
│   ├── "SCENES" label (section header style)
│   └── ControlGroup (QHBoxLayout, spacing: 4px)
│       ├── ChevronLeft button (13×13px icon)
│       ├── ChevronRight button (13×13px icon)
│       ├── Vertical divider (1×12px, rgba(255,255,255,26))
│       ├── GridView button (13×13px icon)
│       ├── ListView button (13×13px icon)
│       └── AddScene button (Plus icon, 13×13px)
└── SceneList (QScrollArea horizontal, padding: 8px, spacing: 8px)
    └── SceneCard × N (see §6.3.1)
```

#### 6.3.1 Scene Card Specification

```
SceneCard (QPushButton or QWidget with click handler)
  Size:             110×70px (fixed)
  Background:       #0A0B0F
  Border-radius:    6px
  Overflow:         hidden (clip children)
```

| State | Border | Box Shadow | Icon Color | Label Color | Label Weight |
|---|---|---|---|---|---|
| **Default** | 2px solid rgba(255,255,255,20) | none | rgba(255,255,255,51) | rgba(255,255,255,102) | 400 |
| **Hover** | 2px solid rgba(255,255,255,40) | none | rgba(255,255,255,102) | rgba(255,255,255,153) | 400 |
| **Active/Selected** | 2px solid #3B82F6 | 0 0 12px rgba(59,130,246,77) | #3B82F6 | #FFFFFF | 600 |

Active card also shows a "LIVE" badge in top-right corner:
- Size: auto, padding: 1px 4px
- Background: #EF4444
- Font: Inter Bold, 8px, #fff, letter-spacing: 0.06em
- Border-radius: 3px
- Position: 4px from top, 4px from right (absolute)

Card internal layout (centered vertically and horizontally):
- Monitor icon: 16×16px
- Gap: 4px
- Scene name label: Inter, 10px

### 6.4 Audio Mixer Panel

```
AudioMixerPanel (QWidget, background: #111318, border: 1px rgba(255,255,255,20), border-radius: 8px, flex-1)
├── PanelHeader (QHBoxLayout, height: 32px, padding: 0 12px, border-bottom: 1px rgba(255,255,255,15))
│   ├── "AUDIO MIXER" label
│   └── MasterControls (QHBoxLayout, spacing: 8px)
│       ├── "MASTER" label (Inter, 10px, rgba(255,255,255,102))
│       ├── Volume2 icon (12×12px, rgba(255,255,255,102))
│       ├── MasterSlider (QSlider, width: 80px, height: 4px — see §11.3)
│       ├── "-6.0 dB" label (JetBrains Mono, 10px, rgba(255,255,255,128))
│       └── Settings2 icon button (12×12px)
└── ChannelGrid (QGridLayout, 4 columns, padding: 8px, spacing: 8px)
    └── AudioChannel × 4 (see §6.4.1)
```

#### 6.4.1 Audio Channel Card

```
AudioChannel (QWidget, background: #0E0F14, border: 1px rgba(255,255,255,15), border-radius: 6px, padding: 8px)
├── ChannelHeader (QHBoxLayout, margin-bottom: 8px)
│   ├── ChannelIcon (12×12px, color: #3B82F6)
│   └── ChannelInfo (QVBoxLayout, spacing: 0)
│       ├── Channel name (Inter 600, 11px, #fff)
│       └── Channel sub-label (Inter 400, 9px, rgba(255,255,255,89))
├── VUMeter (custom QWidget, height: 16px — see §11.1)
├── VolumeSlider (QSlider, height: 3px, margin-top: 6px — see §11.3)
└── ChannelFooter (QHBoxLayout, margin-top: 6px)
    ├── dB readout (JetBrains Mono, 9px, rgba(255,255,255,102))
    └── ButtonGroup (QHBoxLayout, spacing: 4px)
        ├── SoloButton "S" (see §6.4.2)
        └── MuteButton "M" (see §6.4.2)
```

Channel data (static labels, dynamic values):

| Channel | Icon | Sub-label | Level (static) | dB (static) |
|---|---|---|---|---|
| Mic | Mic | Shure SM7B | 72% | -3.2 dB |
| Desktop | Monitor | System Audio | 45% | -10.4 dB |
| Music | Music | Spotify | 30% | -14.7 dB |
| Alert | Bell | Stream Elements | 55% | -6.8 dB |

> **Note:** Channel names, sub-labels, and icon types are **DYNAMIC** from the application's audio source configuration. The level (VU meter) and dB values are **DYNAMIC** from real-time audio analysis. The static values above are prototype defaults only.

#### 6.4.2 Solo / Mute Buttons

| State | Background | Text Color | Border |
|---|---|---|---|
| Default | rgba(255,255,255,15) | rgba(255,255,255,128) | none |
| Hover | rgba(255,255,255,25) | rgba(255,255,255,178) | none |
| Active (Solo) | #F59E0B | #fff | none |
| Active (Mute) | #EF4444 | #fff | none |

Size: auto, padding: 2px 4px. Font: Inter Bold, 9px. Border-radius: 3px.

### 6.5 Stream Status Panel

```
StreamStatusPanel (QWidget, fixed width: 240px, background: #0E0F14, border-left: 1px rgba(255,255,255,18))
  padding: 12px
  layout: QVBoxLayout, spacing: 12px

├── SectionHeader "STREAM STATUS" (section header style)
├── PlatformCard (QWidget, background: #111318, border: 1px rgba(255,255,255,20), border-radius: 8px, padding: 10px)
│   ├── LiveDot (8×8px, #EF4444, animated pulse)
│   ├── "LIVE" (Inter 600, 12px, #EF4444)
│   └── "on YouTube" (Inter 400, 11px, rgba(255,255,255,153))
├── BitrateCard (QWidget, background: #111318, border: 1px rgba(255,255,255,20), border-radius: 8px, padding: 12px)
│   ├── BitrateHeader (QHBoxLayout)
│   │   ├── "BITRATE" label (Inter 400, 10px, rgba(255,255,255,102), letter-spacing: 0.08em)
│   │   └── BitrateValue (JetBrains Mono Bold, 13px, #3B82F6) — DYNAMIC
│   └── BitrateSparkline (custom QWidget, width: 100%, height: 40px — see §11.4)
├── StatGrid (QGridLayout, 2 columns, spacing: 8px)
│   ├── ViewersCard (see §6.5.1)
│   └── UptimeCard (see §6.5.1)
├── StreamHealthCard (QWidget, background: #111318, border: 1px rgba(255,255,255,20), border-radius: 8px, padding: 12px)
│   ├── HealthHeader: "STREAM HEALTH" + Activity icon
│   └── HealthRows × 3 (see §6.5.2)
└── EndStreamButton (QPushButton, full-width, height: 44px — see §6.5.3)
```

#### 6.5.1 Stat Card (Viewers / Uptime)

```
StatCard (QWidget, background: #111318, border: 1px rgba(255,255,255,20), border-radius: 8px, padding: 10px)
├── StatHeader (QHBoxLayout)
│   ├── Icon (10×10px, color per card)
│   └── Label (Inter, 9px, rgba(255,255,255,102), letter-spacing: 0.08em, UPPERCASE)
├── Value (JetBrains Mono Bold, 15px, #fff) — DYNAMIC
└── SubLabel (Inter, 9px, accent color) — DYNAMIC
```

| Card | Icon | Icon Color | Sub-label |
|---|---|---|---|
| VIEWERS | Users | #3B82F6 | "Peak: 3,152" (DYNAMIC) |
| UPTIME | Clock | #06B6D4 | "Session Live" (static label) |

#### 6.5.2 Health Row

Each row is a `QHBoxLayout` with height 20px, margin-bottom 8px:

```
[Label (50px fixed width, Inter 10px, rgba(255,255,255,128))]
[HealthBar (flex-1, height: 4px)]
[Value (40px fixed width, right-aligned, JetBrains Mono 10px)]
```

| Row | Label | Value | Bar Color | Value Color |
|---|---|---|---|---|
| CPU | "CPU" | "28%" (DYNAMIC) | #22C55E if <60%, #F59E0B if <80%, #EF4444 if ≥80% | same as bar |
| GPU | "GPU" | "42%" (DYNAMIC) | same thresholds | same as bar |
| Network | "Network" | "Excellent" (DYNAMIC) | #22C55E | #22C55E |

#### 6.5.3 End Stream Button

```
QPushButton
  width:            100% (stretch)
  height:           44px
  background:       #EF4444
  color:            #FFFFFF
  font:             Space Grotesk Bold, 13px, letter-spacing: 0.04em
  border-radius:    8px
  border:           none
  box-shadow:       0 4px 16px rgba(239,68,68,77)
  icon:             Square (filled white, 14×14px) — left of text
  text:             "END STREAM"
```

| State | Background | Shadow |
|---|---|---|
| Default | #EF4444 | 0 4px 16px rgba(239,68,68,77) |
| Hover | #DC2626 (darken 8%) | 0 6px 20px rgba(239,68,68,100) |
| Pressed | scale(0.97) + #B91C1C | reduced shadow |
| Disabled | rgba(239,68,68,77) | none |

---

## 7. Scene Editor Screen — Full Specification

### 7.1 Layout

```
SceneEditorPage (QWidget, background: #0E0F14)
├── TopBar (46px, see §5.2)
└── ContentArea (QHBoxLayout, spacing: 0)
    ├── CanvasArea (QVBoxLayout, flex-1)
    │   ├── CanvasWorkspace (flex-1, background: #0A0B0F, padding: 12px)
    │   │   ├── CanvasFrame (QWidget with border: 1px rgba(255,255,255,26))
    │   │   │   └── CanvasContent (background: #111318, see §7.2)
    │   │   └── ToolSidebar (absolute right, vertically centered, see §7.4)
    │   └── TransitionsBar (fixed height: 44px, border-top: 1px rgba(255,255,255,18), background: #111318)
    └── RightPanel (fixed width: 320px, border-left: 1px rgba(255,255,255,18))
        ├── SourcesPanel (see §7.5)
        └── PropertiesPanel (see §7.6)
```

### 7.2 Canvas Content

The canvas content area fills `CanvasFrame` and contains:

1. **Background gradient:** `linear-gradient(135deg, #0D1117 0%, #161B22 50%, #0D1117 100%)`
2. **Grid overlay:** 60×60px grid, line color `QColor(255,255,255,10)` (opacity ~4%), line width 0.5px
3. **Crosshair:** horizontal line 20×1px + vertical line 1×20px, centered, color `QColor(59,130,246,64)` (rgba(59,130,246,0.25))
4. **Canvas metadata strip** (absolute bottom, height: 22px, background: rgba(0,0,0,153), border-top: 1px rgba(255,255,255,15)):
   - Left: "1920×1080 · 16:9 · 60fps" (JetBrains Mono, 9px, rgba(255,255,255,89))
   - Center: "SCENE 1 · 5 SOURCES" (JetBrains Mono, 9px, rgba(59,130,246,179))
   - Right: "SAFE AREA: ON" (JetBrains Mono, 9px, rgba(255,255,255,89))
5. **Safe area corner brackets:** 20×20px L-shapes, color `QColor(59,130,246,102)` (rgba(59,130,246,0.4)), 8px inset from canvas edges

### 7.3 Source Overlay Elements (Canvas)

These are rendered as child `QWidget`s positioned absolutely within the canvas:

**Selected source (Webcam) — blue selection box:**
- Position: left 5%, bottom 10%, width 28%, height 38% of canvas
- Border: 2px solid #3B82F6
- Outer ring: 1px solid rgba(59,130,246,77) (box-shadow equivalent — use second border or QPainter)
- Background: #1A1D2B
- 8 resize handles: 8×8px white squares with 1px #3B82F6 border, border-radius: 2px, positioned at 4 corners + 4 edge midpoints

**Text overlay element:**
- Position: top 16px, horizontally centered
- Border: 1px dashed rgba(255,255,255,51)
- Padding: 4px 12px
- Text: "LIVE BROADCAST", Space Grotesk Bold, 16px, rgba(255,255,255,179)

**Logo element:**
- Position: top 12px, right 12px
- Border: 1px dashed rgba(255,255,255,38)
- Padding: 6px
- Inner: 48×48px rounded square, background rgba(59,130,246,51), Monitor icon 18×18px, #3B82F6

### 7.4 Tool Sidebar (Vertical)

Positioned absolutely on the right edge of the canvas workspace, vertically centered. 6 buttons stacked vertically with 4px gap.

```
ToolButton (QPushButton, 32×32px, border-radius: 6px)
```

| State | Background | Border | Icon Color |
|---|---|---|---|
| Default | rgba(255,255,255,15) | 1px rgba(255,255,255,26) | rgba(255,255,255,128) |
| Hover | rgba(255,255,255,25) | 1px rgba(255,255,255,40) | rgba(255,255,255,178) |
| Active/Selected | #3B82F6 | 1px #3B82F6 | #FFFFFF |

Tools in order (top to bottom): Move, Resize (Maximize2), Crop, Rotate (RotateCw), Align (AlignLeft), Lock

### 7.5 Sources Panel

```
SourcesPanel (QWidget, border-bottom: 1px rgba(255,255,255,18))
├── SourcesHeader (QHBoxLayout, height: 32px, padding: 0 12px, border-bottom: 1px rgba(255,255,255,15))
│   ├── "SOURCES" label (section header style)
│   └── ButtonGroup: Plus button + MoreHorizontal button (12×12px icons)
└── SourceList (QListWidget or custom QWidget list)
    └── SourceRow × N (see §7.5.1)
```

#### 7.5.1 Source Row

Height: 34px. Padding: 0 12px.

| State | Background | Left Border | Padding-left |
|---|---|---|---|
| Default | transparent | 3px transparent | 12px |
| Hover | rgba(255,255,255,8) | 3px transparent | 12px |
| Selected | #1A1D2B | 3px solid #3B82F6 | 9px |

Row element sequence (left to right):
1. **Drag handle:** ⠿ character or 6-dot grid icon, 16×16px, rgba(255,255,255,51), cursor: grab
2. **Visibility toggle:** Eye / EyeOff icon, 13×13px. Visible: rgba(255,255,255,128). Hidden: rgba(255,255,255,64)
3. **Lock toggle:** Lock / Unlock icon, 12×12px. Locked: rgba(255,255,255,77). Unlocked: rgba(255,255,255,51)
4. **Type icon:** 13×13px. Selected: #3B82F6. Default: rgba(255,255,255,128)
5. **Source name:** Inter, 12px. Selected: #fff, weight 500. Default: rgba(255,255,255,153), weight 400
6. **More button:** MoreHorizontal icon, 12×12px, rgba(255,255,255,51), visible on hover only

Source list (static prototype data — **DYNAMIC** in production):

| Order | Name | Icon | Visible | Locked |
|---|---|---|---|---|
| 1 | Game Capture | Monitor | true | false |
| 2 | Webcam | Camera | true | false |
| 3 | Text Overlay | Type | true | false |
| 4 | Logo Image | Image | true | true |
| 5 | Alert Overlay | Globe | false | false |

### 7.6 Source Properties Panel

```
PropertiesPanel (QScrollArea, flex-1)
├── PropertiesHeader (QWidget, height: 32px, border-bottom: 1px rgba(255,255,255,15), padding: 0 12px)
│   └── "SOURCE PROPERTIES: [NAME]" (section header style, name in uppercase)
└── PropertiesContent (QVBoxLayout, padding: 12px, spacing: 12px)
    ├── PositionSizeGroup (QGridLayout, 2 columns, gap: 8px)
    │   ├── PositionGroup: POSITION label + [X input] [Y input]
    │   └── SizeGroup: SIZE label + [W input] [H input]
    ├── RotationOpacityGroup (QGridLayout, 2 columns, gap: 8px)
    │   ├── RotationGroup: ROTATION label + [degrees input]
    │   └── OpacityGroup: OPACITY label + [slider] [% label]
    ├── ChromaKeyCard (see §7.6.1)
    ├── FlipGroup (see §7.6.2)
    └── CropGroup (see §7.6.3)
```

**NumberInput field:**
- Background: #0A0B0F
- Border: 1px rgba(255,255,255,26)
- Border-radius: 4px
- Font: JetBrains Mono, 11px, #fff
- Padding: 4px 8px
- Focus border: rgba(59,130,246,128)
- Label above: Inter, 9px, rgba(255,255,255,102), margin-bottom: 3px

#### 7.6.1 Chroma Key Card

```
ChromaKeyCard (QWidget, background: #0E0F14, border: 1px rgba(255,255,255,18), border-radius: 4px, padding: 10px)
├── ChromaHeader (QHBoxLayout)
│   ├── "Chroma Key" (Inter 600, 11px, #fff)
│   └── ChromaToggle (Toggle widget, defaultChecked: true, color: #22C55E)
└── ChromaControls (QGridLayout, 2 columns, gap: 8px)
    ├── ColorPicker: "Color" label + [20×20px color swatch (#22C55E)] + "#22C55E" text
    └── Similarity: "Similarity" label + [slider, 50%] + "50%" label
```

Color swatch: 20×20px `QLabel` with background `#22C55E`, border: 1px rgba(255,255,255,51), border-radius: 3px.

#### 7.6.2 Flip Group

Two buttons side by side:
- "↔ Flip Horizontal" and "↕ Flip Vertical"
- Style: background #0E0F14, border: 1px rgba(255,255,255,26), border-radius: 6px, padding: 6px 12px
- Font: Inter, 12px, rgba(255,255,255,153)
- Icon: FlipHorizontal2 / FlipVertical2, 12×12px, left of text

#### 7.6.3 Crop Group

Label "CROP (px)" (section header style). 2×2 grid of NumberInputs: Top, Bottom, Left, Right, all defaulting to "0". Below: "Reset Crop" button (full width, background rgba(255,255,255,10), border: 1px rgba(255,255,255,20), border-radius: 6px, color rgba(255,255,255,128), height: 28px).

### 7.7 Transitions Bar

```
TransitionsBar (QWidget, height: 44px, background: #111318, border-top: 1px rgba(255,255,255,18))
  layout: QHBoxLayout, padding: 0 12px, spacing: 16px

├── "TRANSITIONS" label (section header style, no-wrap)
├── TransitionButtons (QHBoxLayout, spacing: 8px)
│   └── TransitionButton × 5: Cut, Fade, Slide, Wipe, Stinger (see §7.7.1)
└── DurationGroup (QHBoxLayout, spacing: 8px, ml-auto)
    ├── "Duration" label (Inter, 10px, rgba(255,255,255,102))
    ├── DurationInput (QLineEdit, width: 60px, defaultValue: "300", JetBrains Mono 11px)
    ├── "ms" label (Inter, 10px, rgba(255,255,255,102))
    └── ApplyButton ("Apply Transition", background: #3B82F6, color: #fff, Inter 600, 11px, border-radius: 6px, padding: 6px 12px)
```

#### 7.7.1 Transition Button

| State | Background | Border | Color | Font Weight |
|---|---|---|---|---|
| Default | #0E0F14 | 1px rgba(255,255,255,20) | rgba(255,255,255,102) | 400 |
| Hover | rgba(255,255,255,10) | 1px rgba(255,255,255,30) | rgba(255,255,255,153) | 400 |
| Active | rgba(59,130,246,51) | 1px #3B82F6 | #3B82F6 | 600 |

Size: auto, padding: 6px 12px, border-radius: 6px, font: Inter 11px.

---

## 8. Settings Screen — Full Specification

### 8.1 Layout

```
SettingsPage (QWidget, background: #0E0F14)
├── TopBar (46px)
└── ContentArea (QHBoxLayout, spacing: 0, flex-1)
    ├── TabNav (QWidget, fixed width: 180px, background: #111318, border-right: 1px rgba(255,255,255,18))
    │   └── TabList (QVBoxLayout, padding: 8px 0)
    │       └── TabItem × 8 (see §8.2)
    ├── FormContent (QScrollArea, flex-1, padding: 24px, background: #0E0F14)
    │   └── StreamTab (see §8.3) [or other tab content]
    └── ActionBar (QWidget, fixed height: 60px, border-top: 1px rgba(255,255,255,7), background: #0D0E12)
        └── QHBoxLayout (right-aligned)
            ├── CancelButton (ghost style, see §11.5)
            └── SaveButton ("Save Settings", primary blue style, see §11.5)
```

### 8.2 Settings Tab Item

Same visual rules as sidebar nav items. Width: 180px, height: 36px, padding: 0 16px.

| State | Background | Left Border | Text Color |
|---|---|---|---|
| Default | transparent | 3px transparent | rgba(255,255,255,128) |
| Hover | rgba(255,255,255,8) | 3px transparent | rgba(255,255,255,178) |
| Active | #1A1D2B | 3px solid #3B82F6 | #FFFFFF |

### 8.3 Stream Tab Content

**SERVICE section:**
- Section label: "SERVICE" (section header style), margin-bottom: 10px
- Platform buttons: 4 pill buttons (Twitch, YouTube, Facebook, Custom RTMP)
  - Default: background #0E0F14, border: 1px rgba(255,255,255,20), color rgba(255,255,255,153)
  - Active: background `{platformColor}22`, border: 1px `{platformColor}`, color `{platformColor}`
  - Padding: 8px 16px, border-radius: 6px, Inter 13px

**FormRow pattern** (used for all settings fields):
- Height: auto, min-height: 44px
- Border-bottom: 1px rgba(255,255,255,13)
- Padding: 12px 0
- Label column: fixed width 160px, Inter 500, 12px, rgba(255,255,255,153)
- Control column: flex-1

**Stream Key row:**
- Password input (full width minus 120px for buttons)
- "Show" button: background #0E0F14, border: 1px rgba(255,255,255,20), Inter 12px, padding: 6px 12px
- "Get Key ↗" button: background rgba(59,130,246,20), border: 1px #3B82F6, color #3B82F6, Inter 12px

**ENCODER section:**
- Three segmented buttons: NVENC (NVIDIA), QSV (Intel), x264 (Software)
- Active: background #3B82F6, color #fff, border: 1px #3B82F6
- Default: background #0E0F14, color rgba(255,255,255,102), border: 1px rgba(255,255,255,20)
- Padding: 8px 16px, border-radius: 6px

**Encoder params grid (4 columns):**
- Rate Control, Bitrate (Target), Bitrate (Min), Keyframe Interval
- Each: label above (Inter 10px, rgba(255,255,255,77)) + StyledInput below
- Bitrate inputs have "kbps" suffix label; Keyframe has "s" suffix

**OUTPUT section:**
- Recording Path: full-width input + "Browse" button
- Recording Format: MKV / MP4 / MOV segmented buttons (same style as encoder)
- Replay Buffer: Toggle (36×20px) + "60 seconds" label

**VIDEO section:**
- Canvas Resolution: QComboBox (min-width: 200px)
- Output Resolution: QComboBox (min-width: 200px)
- FPS: 30 / 60 / 120 segmented buttons

---

## 9. Chat & Audience Screen — Full Specification

### 9.1 Layout

```
ChatPage (QWidget, background: #0E0F14)
├── TopBar (46px)
└── ContentArea (QHBoxLayout, spacing: 0)
    ├── LeftPanel (fixed width: 220px, border-right: 1px rgba(255,255,255,18), background: #111318)
    │   ├── ProgramPreview (16:9 thumbnail, border-bottom: 1px rgba(255,255,255,7))
    │   └── QuickActions (see §9.2)
    ├── ChatCenter (flex-1, QVBoxLayout)
    │   ├── ChatHeader (see §9.3)
    │   ├── MessageList (QScrollArea, flex-1)
    │   │   └── MessageRow × N (see §9.4)
    │   ├── ChatInput (see §9.5)
    │   └── ModToolbar (see §9.6)
    └── RightPanel (fixed width: 260px, border-left: 1px rgba(255,255,255,18))
        ├── ActivityFeed (see §9.7)
        ├── PollsPanel (see §9.8)
        └── CommandsPanel (see §9.9)
```

### 9.2 Quick Actions

2×2 grid of action buttons (Clip, Raid, Host, Poll) + full-width Marker button below. Each button: background rgba(255,255,255,6), border: 1px rgba(255,255,255,10), border-radius: 6px, padding: 12px 8px, icon 20×20px centered above label (Inter 11px, rgba(255,255,255,153)).

### 9.3 Chat Header

Height: 44px. Contains: "LIVE CHAT" label (Space Grotesk Bold, 13px, #fff) + platform color dots (T=purple, Y=red, F=blue, 8×8px circles) + viewer count badge (Users icon + count, background rgba(255,255,255,10), border-radius: 12px, padding: 4px 10px).

### 9.4 Message Row

Three variants:

| Type | Background | Border | Notes |
|---|---|---|---|
| Regular | transparent | none | Standard chat message |
| Donation | rgba(245,158,11,26) | 1px solid rgba(245,158,11,51) | Amber tint |
| Subscription | rgba(145,70,255,26) | 1px solid rgba(145,70,255,51) | Purple tint |

Row anatomy: `[timestamp] [badge?] [username:] [message]`
- Timestamp: JetBrains Mono, 10px, rgba(255,255,255,51)
- MOD badge: background rgba(34,197,94,26), border: 1px rgba(34,197,94,51), color #22C55E, Inter Bold 9px, padding: 1px 4px, border-radius: 3px
- SUB badge: background rgba(145,70,255,26), border: 1px rgba(145,70,255,51), color #9146FF, same sizing
- Username: Inter 600, 13px, color varies per user (platform-assigned)
- Message: Inter 400, 13px, rgba(255,255,255,204)

### 9.5 Chat Input

Height: 44px. Background: #111318. Border-top: 1px rgba(255,255,255,7). Padding: 0 12px.
- Text input: flex-1, background #0E0F14, border: 1px rgba(255,255,255,12), border-radius: 6px, Inter 12px, #fff
- Emoji button: Smile icon, 20×20px, rgba(255,255,255,77)
- Send button: 32×32px, background #3B82F6, border-radius: 6px, Send icon 14×14px white

### 9.6 Mod Toolbar

Height: 36px. Row of icon-only buttons (Shield, Ban, Clock, Users, MessageSquare, Trash2), each 28×28px, background rgba(255,255,255,6), border-radius: 4px, icon 14×14px rgba(255,255,255,77).

### 9.7 Activity Feed

Section header "ACTIVITY FEED". Each event row: icon (16×16px, colored) + text (Inter 12px). Event types: Follow (Heart, #EF4444), Sub (Star, #9146FF), Donation (DollarSign, #F59E0B), Raid (Zap, #06B6D4). Username in event: colored bold. Timestamp: JetBrains Mono 9px, rgba(255,255,255,51).

### 9.8 Polls & Predictions

Section header "POLLS & PREDICTIONS" + "ACTIVE" badge (background rgba(34,197,94,26), border: 1px rgba(34,197,94,51), color #22C55E, 9px Inter Bold). Poll question: Inter 600, 13px, #fff. Each option: label + progress bar (4px height, #3B82F6) + percentage. Total votes + countdown timer in JetBrains Mono 10px.

### 9.9 Chat Commands

Section header "CHAT COMMANDS". 2-column grid of command cards. Each card: background rgba(255,255,255,6), border: 1px rgba(255,255,255,10), border-radius: 6px, padding: 8px 10px. Command: Inter 600, 12px, #3B82F6. Description: Inter 400, 10px, rgba(255,255,255,77).

---

## 10. Analytics Screen — Full Specification

### 10.1 Layout

```
AnalyticsPage (QWidget, background: #0E0F14)
├── TopBar (46px)
└── ScrollContent (QScrollArea)
    └── QVBoxLayout (padding: 12px, spacing: 12px)
        ├── KPIGrid (QGridLayout, 4 columns, spacing: 12px)
        │   └── KPICard × 4 (see §10.2)
        ├── ViewerChart (see §10.3)
        ├── MiddleRow (QHBoxLayout, spacing: 12px)
        │   ├── StreamHealthPanel (flex-1)
        │   └── AudienceBreakdownPanel (flex-1)
        └── SessionsTable (see §10.4)
```

### 10.2 KPI Card

```
KPICard (QWidget, background: #111318, border: 1px solid {accentColor}48, border-radius: 8px, padding: 12px)
├── CardHeader (QHBoxLayout)
│   ├── Label (Inter 600, 9px, rgba(255,255,255,102), UPPERCASE, letter-spacing: 0.1em)
│   └── Icon (13×13px, color: accentColor)
├── Value (Space Grotesk Bold, 20px, #fff) — DYNAMIC
└── Delta (Inter, 11px, color: green if positive, red if negative) — DYNAMIC
```

| Card | Label | Icon | Accent Color |
|---|---|---|---|
| PEAK VIEWERS | Users | #3B82F6 | #3B82F6 |
| AVG. WATCH TIME | Clock | #06B6D4 | #06B6D4 |
| NEW FOLLOWERS | UserPlus | #22C55E | #22C55E |
| TOTAL REVENUE | DollarSign | #F59E0B | #F59E0B |

### 10.3 Viewer Chart

Full-width panel (background: #111318, border: 1px rgba(255,255,255,20), border-radius: 8px, padding: 12px). Chart height: 140px. Use `QCustomPlot` or `QtCharts::QChart`.

- Line color: #3B82F6, width: 1.5px
- Fill: gradient from rgba(59,130,246,102) at top to rgba(59,130,246,0) at bottom
- Grid lines: rgba(255,255,255,13), 1px dashed
- Axis labels: JetBrains Mono, 9px, rgba(255,255,255,77)
- Time range selector: 6 pill buttons (5m, 15m, 1h, 3h, 6h, 12h), active: background rgba(59,130,246,51), border: 1px #3B82F6, color #3B82F6
- LIVE DATA badge: red pulse dot + "LIVE DATA" text, same as LIVE badge style
- Instrumentation strip above chart: PEAK / AVG / CURRENT values in JetBrains Mono 10px

### 10.4 Sessions Table

Panel with "RECENT STREAM SESSIONS" header. Standard HTML-style table rendered via `QTableWidget`.

| Column | Alignment | Color | Notes |
|---|---|---|---|
| Date | Left | rgba(255,255,255,153) | Inter 12px |
| Duration | Left | rgba(255,255,255,153) | Inter 12px |
| Peak Viewers | Left | #3B82F6 | JetBrains Mono 12px Bold |
| Avg Viewers | Left | rgba(255,255,255,153) | Inter 12px |
| Revenue | Left | #22C55E | JetBrains Mono 12px Bold |
| Quality Score | Center | Badge widget | Excellent=green, Good=blue, Fair=amber |

Table styling: background #111318, header background #0E0F14, row separator: 1px rgba(255,255,255,7), alternating row: rgba(255,255,255,4), hover row: rgba(255,255,255,8).

---

## 11. Reusable Component Specifications

### 11.1 VU Meter (Audio Level Indicator)

Implement as a custom `QWidget` subclass with `paintEvent`.

```
VUMeter
  Container size:   variable width × 16px height
  Bar count:        20
  Bar width:        3px each
  Bar gap:          1px
  Bar border-radius: 1px
  Bar heights:      4px + (index % 3)px  [4, 5, 6, 4, 5, 6, ... pattern]
  Alignment:        bottom-aligned within container
```

Color thresholds (based on bar index, 0–19):

| Bar index | Active color | Inactive color |
|---|---|---|
| 0–13 | #22C55E | rgba(255,255,255,26) |
| 14–16 | #F59E0B | rgba(255,255,255,26) |
| 17–19 | #EF4444 | rgba(255,255,255,26) |

A bar is "active" when `(barIndex / totalBars) * 100 < currentLevel`. `currentLevel` is 0–100 (percentage).

Update rate: 50ms timer (20fps minimum, 60fps preferred). Transition: `background` color change, 50ms.

In `paintEvent`:
```cpp
void VUMeter::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);
    int barW = 3, gap = 1, totalW = barW + gap;
    for (int i = 0; i < 20; i++) {
        int barH = 4 + (i % 3);
        float threshold = (float)i / 20.0f * 100.0f;
        bool active = threshold < m_level;
        QColor color = active
            ? (i < 14 ? QColor(34,197,94) : i < 17 ? QColor(245,158,11) : QColor(239,68,68))
            : QColor(255,255,255,26);
        p.fillRect(i * totalW, height() - barH, barW, barH,
                   QBrush(color, Qt::SolidPattern));
    }
}
```

### 11.2 Toggle Switch

Implement as a custom `QWidget` subclass or `QPushButton` with custom `paintEvent`.

**Settings screen variant (larger):**
- Container: 36×20px, border-radius: 10px
- Thumb: 16×16px circle, border-radius: 8px, color #fff
- Thumb position ON: left = 18px (2px from right edge)
- Thumb position OFF: left = 2px
- Background ON: #3B82F6
- Background OFF: rgba(255,255,255,38)
- Transition: thumb left position, 200ms ease

**Scene Editor variant (smaller, chroma key):**
- Container: 32×18px, border-radius: 9px
- Thumb: 14×14px circle
- Thumb position ON: left = 16px
- Thumb position OFF: left = 2px
- Background ON: #22C55E (green for chroma key active)
- Background OFF: rgba(255,255,255,38)

### 11.3 Slider (Volume / Opacity)

Implement via `QSlider` with custom QSS:

```qss
QSlider::groove:horizontal {
    height: 4px;
    background: rgba(255,255,255,20);
    border-radius: 2px;
}
QSlider::sub-page:horizontal {
    background: #3B82F6;
    border-radius: 2px;
}
QSlider::handle:horizontal {
    width: 12px;
    height: 12px;
    background: #fff;
    border-radius: 6px;
    margin: -4px 0;
    border: 2px solid #3B82F6;
}
QSlider::handle:horizontal:hover {
    background: #3B82F6;
}
```

For the 3px-height variant (channel volume, opacity, similarity): set groove height to 3px, handle margin to -4.5px 0.

### 11.4 Bitrate Sparkline

Custom `QWidget` subclass. Stores a rolling buffer of 30 bitrate samples. On each `paintEvent`:

1. Normalize values to 0–1 range based on max in buffer
2. Draw filled area: `QPainterPath` from left to right, closed at bottom
   - Fill: `QLinearGradient` from `QColor(59,130,246,102)` at top to `QColor(59,130,246,0)` at bottom
3. Draw line: `QPen(QColor(59,130,246), 1.5f)`, `Qt::SolidLine`, `Qt::RoundCap`
4. Update via 1-second `QTimer`, append new sample, remove oldest

Widget size: full panel width × 40px.

### 11.5 Button Variants

**Primary (Blue):**
```qss
QPushButton[variant="primary"] {
    background: #3B82F6;
    color: #ffffff;
    border: none;
    border-radius: 6px;
    padding: 8px 16px;
    font-family: "Inter";
    font-size: 12px;
    font-weight: 600;
}
QPushButton[variant="primary"]:hover { background: #2563EB; }
QPushButton[variant="primary"]:pressed { background: #1D4ED8; }
QPushButton[variant="primary"]:disabled { background: rgba(59,130,246,77); color: rgba(255,255,255,128); }
```

**Ghost (Cancel):**
```qss
QPushButton[variant="ghost"] {
    background: transparent;
    color: rgba(255,255,255,153);
    border: 1px solid rgba(255,255,255,20);
    border-radius: 6px;
    padding: 8px 16px;
    font-family: "Inter";
    font-size: 12px;
}
QPushButton[variant="ghost"]:hover { background: rgba(255,255,255,8); color: rgba(255,255,255,204); }
QPushButton[variant="ghost"]:pressed { background: rgba(255,255,255,13); }
```

**Danger (End Stream):**
```qss
QPushButton[variant="danger"] {
    background: #EF4444;
    color: #ffffff;
    border: none;
    border-radius: 8px;
    padding: 10px 16px;
    font-family: "Space Grotesk";
    font-size: 13px;
    font-weight: 700;
    letter-spacing: 0.04em;
}
QPushButton[variant="danger"]:hover { background: #DC2626; }
QPushButton[variant="danger"]:pressed { background: #B91C1C; }
```

**Icon-only toolbar button:**
```qss
QPushButton[variant="tool"] {
    background: rgba(255,255,255,15);
    border: 1px solid rgba(255,255,255,26);
    border-radius: 6px;
    padding: 0;
    min-width: 32px;
    min-height: 32px;
    max-width: 32px;
    max-height: 32px;
}
QPushButton[variant="tool"]:hover { background: rgba(255,255,255,25); }
QPushButton[variant="tool"]:checked { background: #3B82F6; border-color: #3B82F6; }
```

### 11.6 Health / Progress Bar

Custom `QWidget` or `QProgressBar` with QSS:
```qss
QProgressBar {
    background: rgba(255,255,255,20);
    border-radius: 2px;
    border: none;
    max-height: 4px;
    min-height: 4px;
    text-visible: false;
}
QProgressBar::chunk { border-radius: 2px; }
QProgressBar[health="good"]::chunk { background: #22C55E; }
QProgressBar[health="warn"]::chunk { background: #F59E0B; }
QProgressBar[health="bad"]::chunk  { background: #EF4444; }
```

### 11.7 LIVE Pulse Animation

The red dot in LIVE badges and the sidebar footer dot must animate. Use `QPropertyAnimation` on a custom property or `QTimer`-driven opacity oscillation:

```cpp
// Pseudocode — drive with QTimer at 50ms intervals
float t = (elapsed_ms % 2000) / 2000.0f;
float opacity = 0.5f + 0.5f * cos(t * 2 * M_PI);  // 1.0 → 0.5 → 1.0 over 2 seconds
dot->setOpacity(opacity);
// Also scale: 1.0 → 1.3 → 1.0
```

---

## 12. All Widget State Specifications

### 12.1 Complete State Matrix

| Widget | Default | Hover | Active/Selected | Pressed | Disabled | LIVE | Offline | Warning |
|---|---|---|---|---|---|---|---|---|
| Nav item | transparent bg, 50% text | rgba(255,255,255,5%) bg | #1A1D2B bg + 3px blue border | rgba(255,255,255,8%) | 30% opacity | — | — | — |
| Scene card | rgba(255,255,255,8%) border | rgba(255,255,255,16%) border | 2px #3B82F6 border + blue glow | scale(0.98) | 40% opacity | red LIVE badge | no badge | — |
| Source row | transparent | rgba(255,255,255,3%) | #1A1D2B + 3px blue border | — | 40% opacity | — | — | — |
| Tool button | rgba(255,255,255,6%) | rgba(255,255,255,10%) | #3B82F6 bg + white icon | scale(0.97) | 40% opacity | — | — | — |
| Transition btn | #0E0F14 | rgba(255,255,255,4%) | rgba(blue,20%) + blue border | — | 40% opacity | — | — | — |
| Platform btn | #0E0F14 | rgba(255,255,255,4%) | platform-color bg/border | — | 40% opacity | — | — | — |
| Toggle (off) | rgba(255,255,255,15%) | rgba(255,255,255,20%) | — | — | 40% opacity | — | — | — |
| Toggle (on) | #3B82F6 | #2563EB | — | — | rgba(blue,40%) | — | — | — |
| Primary button | #3B82F6 | #2563EB | — | #1D4ED8 + scale(0.97) | rgba(blue,30%) | — | — | — |
| Danger button | #EF4444 | #DC2626 | — | #B91C1C + scale(0.97) | rgba(red,30%) | — | — | — |
| Input field | #0E0F14 + rgba(white,12%) border | rgba(white,18%) border | — | — | 40% opacity | — | — | rgba(amber,50%) border |
| Health bar | — | — | — | — | — | — | rgba(white,8%) fill | #F59E0B fill |
| VU bar | rgba(white,10%) | — | — | — | — | — | — | — |
| LIVE badge | red bg/border + pulse | — | — | — | hidden | shown | hidden | — |
| SIG OK dot | green | — | — | — | — | — | red dot + "NO SIGNAL" | amber dot + "DEGRADED" |
| Bitrate value | #3B82F6 | — | — | — | — | — | rgba(white,40%) | #F59E0B |

### 12.2 LIVE State (Streaming Active)

When the application is in LIVE state:
- Top bar LIVE badge: visible, red, pulsing
- Sidebar footer: red dot + "LIVE · HH:MM:SS" (timecode updating)
- Active scene card: red "LIVE" badge in top-right corner
- Stream Status panel: all values updating in real time
- End Stream button: fully opaque and enabled
- Instrumentation strip: timecode, viewer count, bitrate all live

### 12.3 Offline State (Not Streaming)

When the application is offline (not streaming):
- Top bar LIVE badge: hidden or replaced with "OFFLINE" in rgba(255,255,255,40%)
- Sidebar footer: gray dot + "OFFLINE"
- Active scene card: no LIVE badge
- Stream Status panel: all values show "—" or last known values grayed out
- End Stream button: replaced with "GO LIVE" button (same size, #22C55E background)
- Bitrate sparkline: flat line or empty

### 12.4 Warning State

Triggered when bitrate drops below 4000 kbps or CPU exceeds 80%:
- Affected health bar: turns #F59E0B (amber)
- Affected value label: turns #F59E0B
- SIG OK indicator: amber dot + "DEGRADED" text
- Bitrate value in top bar: turns #F59E0B

### 12.5 Error/Critical State

Triggered when bitrate drops below 2000 kbps, dropped frames exceed 5%, or network disconnects:
- Affected health bar: turns #EF4444 (red)
- SIG OK indicator: red dot + "NO SIGNAL" or "DROPPED FRAMES"
- Top bar LIVE badge: flashes rapidly (200ms pulse instead of 2000ms)
- Alert toast: appears at top-right, background #EF4444, white text

---

## 13. SVG Icon Inventory

All icons are from the **Lucide** icon set (v0.453.0). All icons use `stroke="currentColor"`, `stroke-width="2"`, `stroke-linecap="round"`, `stroke-linejoin="round"`, `fill="none"`, `viewBox="0 0 24 24"`.

To use in Qt: load SVG via `QSvgRenderer` or `QIcon(QPixmap::fromImage(...))`. Set color by replacing `currentColor` with the desired hex before loading, or use `QSvgRenderer` with a custom color filter.

### 13.1 Icon Usage Map

| Icon Name | File | Used In | Size(s) |
|---|---|---|---|
| `radio` | radio.svg | Sidebar logo | 15px |
| `layout-dashboard` | layout-dashboard.svg | Sidebar nav | 16px |
| `layers` | layers.svg | Sidebar nav | 16px |
| `message-square` | message-square.svg | Sidebar nav | 16px |
| `bar-chart-2` | bar-chart-2.svg | Sidebar nav | 16px |
| `settings` | settings.svg | Sidebar nav | 16px |
| `wifi` | wifi.svg | Sidebar footer | 13px |
| `wifi-off` | wifi-off.svg | Sidebar footer (offline) | 13px |
| `monitor` | monitor.svg | Scene card placeholder, canvas placeholder, audio channel | 16–32px |
| `camera` | camera.svg | Webcam source icon, canvas | 13–20px |
| `type` | type.svg | Text Overlay source icon | 13px |
| `image` | image.svg | Logo Image source icon | 13px |
| `globe` | globe.svg | Alert Overlay source icon | 13px |
| `eye` | eye.svg | Source visibility (visible) | 13px |
| `eye-off` | eye-off.svg | Source visibility (hidden) | 13px |
| `lock` | lock.svg | Source locked | 12px |
| `unlock` | unlock.svg | Source unlocked | 12px |
| `more-horizontal` | more-horizontal.svg | Source row overflow menu | 12px |
| `plus` | plus.svg | Add scene, add source | 13px |
| `move` | move.svg | Canvas tool: Move | 14px |
| `maximize-2` | maximize-2.svg | Canvas tool: Resize | 14px |
| `crop` | crop.svg | Canvas tool: Crop | 14px |
| `rotate-cw` | rotate-cw.svg | Canvas tool: Rotate | 14px |
| `align-left` | align-left.svg | Canvas tool: Align | 14px |
| `flip-horizontal-2` | flip-horizontal-2.svg | Flip Horizontal button | 12px |
| `flip-vertical-2` | flip-vertical-2.svg | Flip Vertical button | 12px |
| `volume-2` | volume-2.svg | Master volume icon | 12px |
| `users` | users.svg | Viewers stat, KPI card | 10–13px |
| `clock` | clock.svg | Uptime stat, KPI card | 10–11px |
| `zap` | zap.svg | Raid event icon | 16px |
| `heart` | heart.svg | Follow event icon | 16px |
| `star` | star.svg | Sub event icon | 16px |
| `dollar-sign` | dollar-sign.svg | Donation event, revenue KPI | 13–16px |
| `user-plus` | user-plus.svg | New followers KPI | 13px |
| `shield` | shield.svg | Mod tool | 14px |
| `ban` | ban.svg | Ban tool | 14px |
| `trash-2` | trash-2.svg | Clear chat tool | 14px |
| `send` | send.svg | Chat send button | 14px |
| `smile` | smile.svg | Chat emoji button | 20px |
| `flag` | flag.svg | Chat report | 14px |
| `refresh-cw` | refresh-cw.svg | Analytics refresh | 11–12px |
| `download` | download.svg | Analytics export | 10–11px |
| `alert-circle` | alert-circle.svg | Error/warning state | 14px |
| `alert-triangle` | alert-triangle.svg | Warning state | 14px |
| `check` | check.svg | Success state | 14px |
| `chevron-down` | chevron-down.svg | Select dropdowns | 12px |
| `chevron-right` | chevron-right.svg | Breadcrumb separators | 12px |
| `arrow-left` | arrow-left.svg | Back navigation | 16px |
| `arrow-right` | arrow-right.svg | Scene scroll | 13px |
| `sliders` | sliders.svg | Advanced settings tab | 16px |
| `keyboard` | keyboard.svg | Hotkeys settings tab | 16px |
| `plug` | plug.svg | Plugins settings tab | 16px |
| `scissors` | scissors.svg | Clip action | 20px |
| `rotate-ccw` | rotate-ccw.svg | Undo | 14px |
| `camera` | camera.svg | Camera action | 20px |

### 13.2 Loading SVG Icons in Qt

```cpp
// Method 1: QIcon from SVG file (recommended for scalable icons)
QIcon icon(":/icons/radio.svg");
button->setIcon(icon);
button->setIconSize(QSize(15, 15));

// Method 2: Colorize SVG at runtime (for dynamic color changes)
QSvgRenderer renderer(QString(":/icons/radio.svg"));
QPixmap pixmap(16, 16);
pixmap.fill(Qt::transparent);
QPainter painter(&pixmap);
renderer.render(&painter);
// Apply color via QPainter::CompositionMode_SourceIn
painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
painter.fillRect(pixmap.rect(), QColor(59, 130, 246));  // accent-blue
painter.end();
QIcon coloredIcon(pixmap);
```

---

## 14. Static vs. Dynamic Data Guide

This section identifies every value in the prototype and classifies it as either **STATIC** (a hardcoded design example) or **DYNAMIC** (must be driven by live application data).

### 14.1 Dashboard Screen

| Element | Value in Prototype | Classification | Data Source |
|---|---|---|---|
| Platform abbreviation "YT" | "YT" | **DYNAMIC** | Active streaming service config |
| Timecode in top bar | "01:23:49" | **DYNAMIC** | `QElapsedTimer` since stream start |
| Resolution "1920×1080" | "1920×1080" | **DYNAMIC** | Encoder output resolution setting |
| FPS "60fps" | "60fps" | **DYNAMIC** | Encoder FPS setting |
| Codec "H.264" | "H.264" | **DYNAMIC** | Encoder codec setting |
| SIG OK indicator | green dot + "SIG OK" | **DYNAMIC** | Network/encoder health check |
| Profile name "Default Profile" | "Default Profile" | **DYNAMIC** | Active profile name |
| Canvas content | black + grid + placeholder | **DYNAMIC** | Live video frame from encoder |
| TC in instrumentation strip | "01:23:49:02" | **DYNAMIC** | Timecode + frame counter |
| Scene name in strip | "Scene 1" | **DYNAMIC** | Active scene name |
| SRC ACTIVE in strip | "SRC ACTIVE" | **DYNAMIC** | Whether active scene has sources |
| Viewer count in strip | "2,843" | **DYNAMIC** | Platform API viewer count |
| Bitrate in strip | "8300 kbps" | **DYNAMIC** | Encoder output bitrate |
| Scene list names | "Scene 1" … "End Screen" | **DYNAMIC** | User-configured scene list |
| Scene card thumbnails | black placeholder | **DYNAMIC** | Scene preview frames |
| Active scene indicator | "Scene 1" | **DYNAMIC** | Currently active scene |
| Audio channel names | "Mic", "Desktop", etc. | **DYNAMIC** | Configured audio sources |
| Audio channel sub-labels | "Shure SM7B", etc. | **DYNAMIC** | Audio device names |
| VU meter levels | 72%, 45%, 30%, 55% | **DYNAMIC** | Real-time audio level analysis |
| dB readouts | "-3.2 dB", etc. | **DYNAMIC** | Real-time audio level in dB |
| Master volume | 70% / "-6.0 dB" | **DYNAMIC** | Master volume setting |
| LIVE platform label | "on YouTube" | **DYNAMIC** | Active streaming platform |
| Bitrate value | "8,347 kbps" | **DYNAMIC** | Encoder output bitrate |
| Bitrate sparkline | animated waveform | **DYNAMIC** | Rolling 30-sample bitrate history |
| Viewer count | "2,842" | **DYNAMIC** | Platform API |
| Peak viewers | "Peak: 3,152" | **DYNAMIC** | Session max viewer count |
| Uptime | "01:23:49" | **DYNAMIC** | `QElapsedTimer` since stream start |
| CPU % | "28%" | **DYNAMIC** | System CPU usage (e.g., `GetSystemTimes`) |
| GPU % | "42%" | **DYNAMIC** | GPU usage (NVAPI / D3DKMT) |
| Network health | "Excellent" | **DYNAMIC** | Packet loss / RTT to ingest server |

### 14.2 Scene Editor Screen

| Element | Value in Prototype | Classification | Data Source |
|---|---|---|---|
| Project name chip | "My Broadcast Project" | **DYNAMIC** | Active project/profile name |
| LIVE timecode | "00:00:00" | **DYNAMIC** | Stream elapsed time |
| REC timecode | "00:00:00" | **DYNAMIC** | Recording elapsed time |
| CPU % | "2.1%" | **DYNAMIC** | System CPU usage |
| FPS | "60.00" | **DYNAMIC** | Encoder output FPS |
| KB/s | "5120" | **DYNAMIC** | Encoder output bitrate |
| Canvas background | gradient placeholder | **DYNAMIC** | Composited scene preview |
| Canvas metadata | "1920×1080 · 16:9 · 60fps" | **DYNAMIC** | Canvas resolution settings |
| Scene name in strip | "SCENE 1" | **DYNAMIC** | Active scene name |
| Source count | "5 SOURCES" | **DYNAMIC** | Count of sources in active scene |
| SAFE AREA toggle | "ON" | **DYNAMIC** | User setting |
| Source list | 5 sources | **DYNAMIC** | Scene source list |
| Source visibility | eye/eye-off state | **DYNAMIC** | Per-source visibility setting |
| Source lock state | lock/unlock icon | **DYNAMIC** | Per-source lock setting |
| Selected source | "Webcam" | **DYNAMIC** | User selection |
| Source position X/Y | "80.0", "720.0" | **DYNAMIC** | Source transform data |
| Source size W/H | "640.0", "360.0" | **DYNAMIC** | Source transform data |
| Rotation | "0.0°" | **DYNAMIC** | Source transform data |
| Opacity | "100%" | **DYNAMIC** | Source opacity setting |
| Chroma key enabled | true | **DYNAMIC** | Source filter setting |
| Chroma key color | "#22C55E" | **DYNAMIC** | Source filter setting |
| Similarity | "50%" | **DYNAMIC** | Source filter setting |
| Crop values | "0" (all sides) | **DYNAMIC** | Source crop setting |
| Active transition | "Cut" | **DYNAMIC** | User-selected transition |
| Duration | "300" ms | **DYNAMIC** | Transition duration setting |
| Text overlay content | "LIVE BROADCAST" | **DYNAMIC** | Text source content |

### 14.3 Settings Screen

| Element | Value in Prototype | Classification | Data Source |
|---|---|---|---|
| Active platform | "YouTube" | **DYNAMIC** | Stream service config |
| Stream key | "••••••••••••••••••••" | **DYNAMIC** | Stored stream key (masked) |
| Server | "Auto-select (recommended)" | **DYNAMIC** | Selected ingest server |
| Active encoder | "NVENC (NVIDIA)" | **DYNAMIC** | Encoder config |
| Rate Control | "CBR" | **DYNAMIC** | Encoder config |
| Bitrate Target | "6000" | **DYNAMIC** | Encoder config |
| Bitrate Min | "4500" | **DYNAMIC** | Encoder config |
| Keyframe Interval | "2" | **DYNAMIC** | Encoder config |
| Recording Path | "D:\StreamPro\Recordings" | **DYNAMIC** | User-configured path |
| Recording Format | "MKV" | **DYNAMIC** | User config |
| Replay Buffer | enabled + "60 seconds" | **DYNAMIC** | User config |
| Canvas Resolution | "1920×1080 (16:9)" | **DYNAMIC** | Video config |
| Output Resolution | "1920×1080 (16:9)" | **DYNAMIC** | Video config |
| FPS | "60" | **DYNAMIC** | Video config |
| LIVE indicator in top bar | red dot | **DYNAMIC** | Stream state |
| "Changes apply on next stream" | static notice | **STATIC** | UI copy |

### 14.4 Chat Screen

| Element | Classification | Notes |
|---|---|---|
| Chat messages | **DYNAMIC** | Platform chat API (Twitch IRC / YouTube Live Chat API) |
| Viewer count badge | **DYNAMIC** | Platform API |
| Activity feed events | **DYNAMIC** | Platform events API |
| Poll question and results | **DYNAMIC** | Platform polls API |
| Poll countdown | **DYNAMIC** | Timer from poll end time |
| Chat command list | **DYNAMIC** | User-configured commands |
| Program preview thumbnail | **DYNAMIC** | Live video frame |
| Timecode on preview | **DYNAMIC** | Stream elapsed time |

### 14.5 Analytics Screen

| Element | Classification | Notes |
|---|---|---|
| All KPI values | **DYNAMIC** | Platform API + local session data |
| Delta percentages | **DYNAMIC** | Calculated vs. previous session |
| Viewer count chart | **DYNAMIC** | Time-series from platform API |
| Bitrate / CPU / GPU / FPS lines | **DYNAMIC** | Local encoder telemetry |
| Dropped frames % | **DYNAMIC** | Encoder dropped frame counter |
| Audience breakdown (Twitch 54% etc.) | **DYNAMIC** | Platform API |
| Top countries | **DYNAMIC** | Platform API |
| Recent sessions table | **DYNAMIC** | Local session history database |
| Time range selector state | **DYNAMIC** | User selection |

---

## 15. Qt Widget & Layout Mapping

### 15.1 Widget Selection Guide

| UI Element | Recommended Qt Widget | Notes |
|---|---|---|
| Application window | `QMainWindow` | Use `setCentralWidget()` for the main split |
| Sidebar | `QWidget` with `QVBoxLayout` | Fixed width via `setFixedWidth(220)` |
| Page navigation | `QStackedWidget` | Switch pages via `setCurrentIndex()` |
| Nav items | `QPushButton` or `QLabel` with mouse events | Custom `paintEvent` for left-border active state |
| Top bar | `QWidget` with `QHBoxLayout` | Fixed height via `setFixedHeight(46)` |
| Panel/card | `QFrame` with `QVBoxLayout` | Set `setFrameShape(QFrame::NoFrame)`, use QSS for border |
| Section header | `QLabel` | Uppercase via `text().toUpper()` or QSS `text-transform` |
| Scene card | `QPushButton` | Custom `paintEvent` for LIVE badge overlay |
| Scene list | `QScrollArea` + `QWidget` + `QHBoxLayout` | Horizontal scroll, `setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded)` |
| Audio channel | `QWidget` with `QVBoxLayout` | Custom card |
| VU meter | `QWidget` subclass | Custom `paintEvent` (see §11.1) |
| Volume slider | `QSlider` | Custom QSS (see §11.3) |
| Toggle switch | `QWidget` subclass | Custom `paintEvent` + `mousePressEvent` (see §11.2) |
| Bitrate sparkline | `QWidget` subclass | Custom `paintEvent` (see §11.4) |
| Health bar | `QProgressBar` | Custom QSS (see §11.6) |
| Text input | `QLineEdit` | Custom QSS for border/background |
| Password input | `QLineEdit` with `setEchoMode(QLineEdit::Password)` | |
| Dropdown select | `QComboBox` | Custom QSS for dark styling |
| Settings form | `QWidget` + `QFormLayout` or `QGridLayout` | Fixed label column width |
| Chat messages | `QScrollArea` + `QWidget` + `QVBoxLayout` | Auto-scroll to bottom on new message |
| Chat input | `QLineEdit` | |
| Analytics chart | `QCustomPlot` (third-party) or `QtCharts::QChartView` | `QCustomPlot` recommended for performance |
| Sessions table | `QTableWidget` | Custom item delegates for colored cells |
| Color swatch | `QLabel` | Set background via QSS |
| Donut chart | `QWidget` subclass | Custom `paintEvent` with `QPainter::drawArc` |
| Tooltip | `QToolTip` | Style via `QToolTip` QSS |
| Toast/notification | `QFrame` (overlay) | Position via `QPropertyAnimation` |
| Separator (vertical) | `QFrame` with `setFrameShape(QFrame::VLine)` | |
| Separator (horizontal) | `QFrame` with `setFrameShape(QFrame::HLine)` | |
| Scroll area | `QScrollArea` | Hide scrollbar via QSS when not needed |
| Icon button | `QPushButton` with `setIcon()` | `setFlat(true)` + custom QSS |
| Badge/chip | `QLabel` | Rounded via QSS `border-radius` |

### 15.2 Layout Hierarchy (Dashboard)

```cpp
// Pseudocode structure
QWidget* dashPage = new QWidget;
QVBoxLayout* dashLayout = new QVBoxLayout(dashPage);
dashLayout->setSpacing(0);
dashLayout->setContentsMargins(0,0,0,0);
dashLayout->addWidget(topBar);  // fixed 46px

QHBoxLayout* content = new QHBoxLayout;
content->setSpacing(0);
content->setContentsMargins(0,0,0,0);

// Center column
QWidget* center = new QWidget;
QVBoxLayout* centerLayout = new QVBoxLayout(center);
centerLayout->setSpacing(12);
centerLayout->setContentsMargins(12,12,12,12);
centerLayout->addWidget(programOutputPanel);  // aspect-ratio constrained
centerLayout->addWidget(scenesPanel);         // fixed height ~100px
centerLayout->addWidget(audioMixerPanel, 1);  // flex-1

// Right panel
QWidget* rightPanel = new QWidget;
rightPanel->setFixedWidth(240);
// ... add stream status widgets

content->addWidget(center, 1);
content->addWidget(rightPanel);
dashLayout->addLayout(content, 1);
```

---

## 16. Complete QSS Stylesheet

Apply this stylesheet to `QApplication::setStyleSheet()` at startup. Use `setProperty()` with variant attributes for state-specific rules.

```qss
/* ============================================================
   NEXUS BROADCAST — Global QSS Stylesheet
   Target: Qt 6 Widgets
   Version: 1.0
   ============================================================ */

/* --- Reset & Base --- */
* {
    outline: none;
    box-sizing: border-box;
}

QWidget {
    background-color: #0E0F14;
    color: #FFFFFF;
    font-family: "Inter";
    font-size: 12px;
    border: none;
}

/* --- Scrollbars --- */
QScrollBar:vertical {
    background: transparent;
    width: 6px;
    margin: 0;
}
QScrollBar::handle:vertical {
    background: rgba(255,255,255,40);
    border-radius: 3px;
    min-height: 20px;
}
QScrollBar::handle:vertical:hover { background: rgba(255,255,255,70); }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }

QScrollBar:horizontal {
    background: transparent;
    height: 6px;
    margin: 0;
}
QScrollBar::handle:horizontal {
    background: rgba(255,255,255,40);
    border-radius: 3px;
    min-width: 20px;
}
QScrollBar::handle:horizontal:hover { background: rgba(255,255,255,70); }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }

/* --- QLineEdit (Text Input) --- */
QLineEdit {
    background: #0E0F14;
    border: 1px solid rgba(255,255,255,31);
    border-radius: 6px;
    color: #FFFFFF;
    font-family: "Inter";
    font-size: 12px;
    padding: 6px 12px;
    selection-background-color: rgba(59,130,246,100);
}
QLineEdit:focus {
    border: 1px solid rgba(59,130,246,128);
}
QLineEdit:disabled {
    color: rgba(255,255,255,51);
    border-color: rgba(255,255,255,13);
}
QLineEdit[monospace="true"] {
    font-family: "JetBrains Mono";
}

/* --- QComboBox (Select / Dropdown) --- */
QComboBox {
    background: #0E0F14;
    border: 1px solid rgba(255,255,255,31);
    border-radius: 6px;
    color: #FFFFFF;
    font-family: "Inter";
    font-size: 12px;
    padding: 6px 12px;
    min-width: 200px;
}
QComboBox:focus { border-color: rgba(59,130,246,128); }
QComboBox::drop-down {
    border: none;
    width: 24px;
}
QComboBox::down-arrow {
    image: url(:/icons/chevron-down.svg);
    width: 12px;
    height: 12px;
}
QComboBox QAbstractItemView {
    background: #16181F;
    border: 1px solid rgba(255,255,255,20);
    border-radius: 6px;
    color: #FFFFFF;
    selection-background-color: rgba(59,130,246,51);
    padding: 4px;
}

/* --- QPushButton (Primary) --- */
QPushButton[variant="primary"] {
    background: #3B82F6;
    color: #FFFFFF;
    border: none;
    border-radius: 6px;
    padding: 8px 16px;
    font-family: "Inter";
    font-size: 12px;
    font-weight: 600;
}
QPushButton[variant="primary"]:hover { background: #2563EB; }
QPushButton[variant="primary"]:pressed { background: #1D4ED8; }
QPushButton[variant="primary"]:disabled {
    background: rgba(59,130,246,77);
    color: rgba(255,255,255,128);
}

/* --- QPushButton (Ghost) --- */
QPushButton[variant="ghost"] {
    background: transparent;
    color: rgba(255,255,255,153);
    border: 1px solid rgba(255,255,255,51);
    border-radius: 6px;
    padding: 8px 16px;
    font-family: "Inter";
    font-size: 12px;
}
QPushButton[variant="ghost"]:hover {
    background: rgba(255,255,255,20);
    color: rgba(255,255,255,204);
}
QPushButton[variant="ghost"]:pressed { background: rgba(255,255,255,33); }

/* --- QPushButton (Danger) --- */
QPushButton[variant="danger"] {
    background: #EF4444;
    color: #FFFFFF;
    border: none;
    border-radius: 8px;
    padding: 10px 16px;
    font-family: "Space Grotesk";
    font-size: 13px;
    font-weight: 700;
}
QPushButton[variant="danger"]:hover { background: #DC2626; }
QPushButton[variant="danger"]:pressed { background: #B91C1C; }
QPushButton[variant="danger"]:disabled { background: rgba(239,68,68,77); }

/* --- QPushButton (Tool) --- */
QPushButton[variant="tool"] {
    background: rgba(255,255,255,15);
    border: 1px solid rgba(255,255,255,26);
    border-radius: 6px;
    min-width: 32px;
    max-width: 32px;
    min-height: 32px;
    max-height: 32px;
    padding: 0;
}
QPushButton[variant="tool"]:hover { background: rgba(255,255,255,25); }
QPushButton[variant="tool"]:checked {
    background: #3B82F6;
    border-color: #3B82F6;
}

/* --- QPushButton (Icon-only small) --- */
QPushButton[variant="icon"] {
    background: transparent;
    border: none;
    padding: 4px;
    border-radius: 4px;
}
QPushButton[variant="icon"]:hover { background: rgba(255,255,255,13); }
QPushButton[variant="icon"]:pressed { background: rgba(255,255,255,20); }

/* --- QPushButton (Segmented / Toggle group) --- */
QPushButton[variant="segment"] {
    background: #0E0F14;
    color: rgba(255,255,255,102);
    border: 1px solid rgba(255,255,255,51);
    border-radius: 6px;
    padding: 8px 16px;
    font-family: "Inter";
    font-size: 12px;
}
QPushButton[variant="segment"]:hover {
    background: rgba(255,255,255,10);
    color: rgba(255,255,255,153);
}
QPushButton[variant="segment"]:checked {
    background: #3B82F6;
    color: #FFFFFF;
    border-color: #3B82F6;
    font-weight: 600;
}

/* --- QProgressBar (Health Bar) --- */
QProgressBar {
    background: rgba(255,255,255,20);
    border: none;
    border-radius: 2px;
    max-height: 4px;
    min-height: 4px;
    text-align: left;
}
QProgressBar::chunk {
    border-radius: 2px;
    background: #22C55E;
}
QProgressBar[health="warn"]::chunk { background: #F59E0B; }
QProgressBar[health="bad"]::chunk  { background: #EF4444; }

/* --- QSlider (Volume / Opacity) --- */
QSlider::groove:horizontal {
    height: 4px;
    background: rgba(255,255,255,20);
    border-radius: 2px;
    border: none;
}
QSlider::sub-page:horizontal {
    background: #3B82F6;
    border-radius: 2px;
}
QSlider::handle:horizontal {
    width: 12px;
    height: 12px;
    background: #FFFFFF;
    border-radius: 6px;
    margin: -4px 0;
    border: 2px solid #3B82F6;
}
QSlider::handle:horizontal:hover { background: #3B82F6; }
QSlider::groove:horizontal[thin="true"] { height: 3px; }
QSlider::handle:horizontal[thin="true"] { margin: -4px 0; }

/* --- QLabel (Section Header) --- */
QLabel[role="section-header"] {
    color: rgba(255,255,255,102);
    font-family: "Inter";
    font-size: 10px;
    font-weight: 600;
    letter-spacing: 1px;
    text-transform: uppercase;
}

/* --- QLabel (Monospace data) --- */
QLabel[role="mono"] {
    font-family: "JetBrains Mono";
    font-size: 10px;
    color: rgba(255,255,255,89);
}

/* --- QLabel (Page title) --- */
QLabel[role="page-title"] {
    font-family: "Space Grotesk";
    font-size: 12px;
    font-weight: 700;
    color: #FFFFFF;
    letter-spacing: 1px;
    text-transform: uppercase;
}

/* --- QTableWidget (Sessions Table) --- */
QTableWidget {
    background: #111318;
    border: none;
    gridline-color: rgba(255,255,255,18);
    color: rgba(255,255,255,153);
    font-family: "Inter";
    font-size: 12px;
}
QTableWidget::item:selected {
    background: rgba(59,130,246,26);
    color: #FFFFFF;
}
QTableWidget::item:hover { background: rgba(255,255,255,20); }
QHeaderView::section {
    background: #0E0F14;
    color: rgba(255,255,255,77);
    font-family: "Inter";
    font-size: 10px;
    font-weight: 600;
    letter-spacing: 1px;
    border: none;
    border-bottom: 1px solid rgba(255,255,255,18);
    padding: 8px 12px;
}

/* --- QScrollArea --- */
QScrollArea {
    background: transparent;
    border: none;
}
QScrollArea > QWidget > QWidget { background: transparent; }

/* --- QFrame (Panel border) --- */
QFrame[role="panel"] {
    background: #111318;
    border: 1px solid rgba(255,255,255,20);
    border-radius: 8px;
}

/* --- QFrame (Divider) --- */
QFrame[role="divider-v"] {
    background: rgba(255,255,255,26);
    max-width: 1px;
    min-width: 1px;
    border: none;
}
QFrame[role="divider-h"] {
    background: rgba(255,255,255,18);
    max-height: 1px;
    min-height: 1px;
    border: none;
}

/* --- QToolTip --- */
QToolTip {
    background: #16181F;
    color: rgba(255,255,255,204);
    border: 1px solid rgba(255,255,255,31);
    border-radius: 4px;
    font-family: "Inter";
    font-size: 11px;
    padding: 4px 8px;
}
```

---

## 17. Responsive Rules

The prototype was designed at 1440×900. The following rules govern behavior at smaller sizes.

### 17.1 Minimum Window Size

```cpp
window->setMinimumSize(1024, 706);
```

Below 1024×706, show a "Window too small" overlay or disable resize below this threshold.

### 17.2 Dashboard at 1024×706

At 1024px wide, the available main content width is `1024 - 220 (sidebar) = 804px`.

| Panel | 1440px behavior | 1024px behavior |
|---|---|---|
| Stream Status panel | 240px fixed | 220px fixed (reduce by 20px) |
| Center column | flex-1 (~980px) | flex-1 (~584px) |
| Scene cards | all 6 visible | horizontal scroll activates at ~4 visible |
| Audio mixer | 4-column grid | 4-column grid maintained (cards narrow) |
| Program output | 16:9 aspect | 16:9 aspect maintained, height reduces |

### 17.3 Scene Editor at 1024×706

| Panel | 1440px behavior | 1024px behavior |
|---|---|---|
| Right panel (Sources + Properties) | 320px fixed | 280px fixed |
| Canvas workspace | flex-1 (~900px) | flex-1 (~524px) |
| Properties panel | full scroll | full scroll (same) |
| Transitions bar | all 5 buttons + duration | all 5 buttons + duration (tighter spacing) |

### 17.4 General Rules

1. **Sidebar never collapses** — always 220px at all supported sizes.
2. **Top bar never wraps** — at narrow widths, right-side elements may be clipped. Priority: LIVE badge > timecode > resolution/fps/codec > SIG OK > profile.
3. **Panels never overlap** — use `QSplitter` for resizable panels if desired, but default to fixed widths.
4. **Minimum panel widths:** Center column min-width 400px; right panels min-width 200px.
5. **Font sizes never scale** — all sizes are fixed pixel values, not em/rem.
6. **Scroll areas activate** when content overflows — never clip content silently.

---

*Document version: 1.0 — Generated from Nexus Broadcast UI Prototype (React/Tailwind) for Qt 6 Widgets C++ implementation.*
*Prototype source: `nexus-broadcast-ui-source.zip`*
*Icon sources: `nexus_icons.zip` (54 Lucide SVG files)*
