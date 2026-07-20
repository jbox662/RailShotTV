# RailShotTV — Original UI Restore Spec

**Source of truth:** `Professional UI Designs for Livestreaming Windows App/`  
**Design system name:** Chromatic Command — Bright Edition  
**Purpose:** Pixel- and structure-accurate reference to restore the original livestream UI in the native Qt app (or any reimplementation).

This document audits every screen, panel, modal, and component in that folder. Where React and Qt references diverge, both are documented and a restore priority is stated.

---

## Table of contents

1. [Design system](#1-design-system)
2. [App shell & navigation](#2-app-shell--navigation)
3. [Dashboard (unified broadcast surface)](#3-dashboard-unified-broadcast-surface)
4. [Scene Manager Panel](#4-scene-manager-panel)
5. [Input Settings Drawer](#5-input-settings-drawer)
6. [Go Live Modal](#6-go-live-modal)
7. [Add Input / Input Select Modal](#7-add-input--input-select-modal)
8. [Dashboard side panels & overlays](#8-dashboard-side-panels--overlays)
9. [Scene Editor](#9-scene-editor)
10. [Chat & Audience](#10-chat--audience)
11. [Scoreboard](#11-scoreboard)
12. [Schedule](#12-schedule)
13. [Analytics](#13-analytics)
14. [Settings](#14-settings)
15. [Shared primitives](#15-shared-primitives)
16. [Qt reference mapping](#16-qt-reference-mapping)
17. [Restore checklist](#17-restore-checklist)
18. [File inventory](#18-file-inventory)

---

## 1. Design system

### 1.1 Brand & product copy

| Element | Spec |
|---------|------|
| Product name | **RAILSHOT** (white/near-white) + **TV** (brand orange) |
| Wordmark font | Bebas Neue, typically 16–18px, letter-spacing `0.04em`–`0.08em` |
| Theme tagline in CSS | “Chromatic Command — Bright Edition” |
| App version (rail footer) | `v2.5` |

### 1.2 Typography

| Role | Family | Weights | Typical sizes |
|------|--------|---------|---------------|
| Display / wordmark / scores / countdown | **Bebas Neue** | 400 | 13–72px |
| UI body / buttons / labels | **DM Sans** | 300–700 | 9–14px |
| Metrics / timecodes / keys / dB | **JetBrains Mono** | 400–600 | 7–13px |

**Google Fonts load** (`index.html`):

```
Bebas+Neue
DM+Sans:ital,opsz,wght@0,9..40,300;0,9..40,400;0,9..40,500;0,9..40,600;0,9..40,700;1,9..40,400
JetBrains+Mono:wght@400;500;600
```

### 1.3 Core palette (hex — use these in native QSS)

| Token | Hex | Primary use |
|-------|-----|-------------|
| Brand | `#FF5A2C` | CTAs, live, PROGRAM accent, logo TV |
| Brand light | `#FF8C42` / `#FF6B35` | Gradients, logo glow |
| Blue | `#4F9EFF` | Selection, scenes header, links |
| Electric blue (controls) | `#3A6AFF` / `#1A3AFF` | Active transition, GO-adjacent, Add Scene |
| Violet | `#A855F7` | Chat nav, overlays, playlist active |
| Emerald | `#22C55E` | Preview, success, GO button |
| Emerald dark | `#16A34A` | GO / Preview gradients |
| Cyan | `#22D3EE` | Analytics, timers, tickers |
| Amber | `#FBBF24` / `#FACC15` | Warnings, schedule nav, solo |
| Pink | `#EC4899` | Accent swatches |
| Danger | `#EF4444` | Delete, mute, clip |
| Recording red | `#7F1D1D` / `#5A1010` | Record active / End Stream |

### 1.4 Surfaces & borders

| Token | Hex | Use |
|-------|-----|-----|
| Body / rail | `#0F1114` | App background, sidebar |
| Dashboard shell | `#0B0D0F` | Dashboard root |
| Deep canvas | `#080A0D` / `#060608` | Preview/program wells |
| Panel deep | `#0A0C0F` / `#0D0F12` | Scene list, tiles row |
| Panel mid | `#141619` / `#1A1D22` | Bars, headers |
| Card (pages) | `#1E2640` / `#1A2035` | Analytics/Settings cards |
| Border primary | `#2A2D35` | Most dividers (Dashboard) |
| Border page | `#2A3350` / `#303D5A` | Analytics/Settings/Scene Editor |
| Text primary | `#F8F8FF` / `#E0E2E8` / `#F0F0F0` | Titles |
| Text secondary | `#8892A4` / `#A0A0B8` / `#C8CAD0` | Labels |
| Text muted | `#606878` / `#50506A` / `#404450` | Hints, empty |

### 1.5 CSS utility classes (`index.css`)

| Class | Spec |
|-------|------|
| `.panel-header-brand` | Left 2px `#FF5A2C`, gradient tint `rgba(255,90,44,0.14)` → transparent 65% |
| `.panel-header-blue` | `#4F9EFF` / `rgba(79,158,255,0.14)` |
| `.panel-header-violet` | `#A855F7` / `rgba(168,85,247,0.14)` |
| `.panel-header-emerald` | `#22C55E` / `rgba(34,197,94,0.14)` |
| `.panel-header-cyan` | `#22D3EE` / `rgba(34,211,238,0.14)` |
| `.panel-header-amber` | `#FBBF24` / `rgba(251,191,36,0.14)` |
| `.panel-header-pink` | `#EC4899` / `rgba(236,72,153,0.14)` |
| `.live-dot` | `live-pulse` 1.8s opacity 1↔0.3 |
| `.live-top-border` | Fixed 3px bar, gradient `#FF5A2C → #FF8C42 → #FBBF24`, glow, `live-border-in` 0.4s |
| `.nav-glow-*` | `0 0 16px` colored glow |
| `.gradient-brand` | Text clip `#FF5A2C → #FF8C42` |
| `.vu-bar` | 3px wide, 1px radius, 0.05s height transition |
| `button:active` | `scale(0.96)` |

### 1.6 Motion tokens

| Name | Timing | Behavior |
|------|--------|----------|
| `liveRipple` | 1.4s ease-out infinite | Scale 1→2.4, opacity fade (sidebar LIVE) |
| `live-pulse` | 1.8s | Opacity pulse for `.live-dot` |
| `live-border-in` | 0.4s cubic-bezier(0.23,1,0.32,1) | Top live border scaleX 0→1 |
| Nav hover/active | 0.18s same easing | Background / glow |
| Mixer slide | 0.25s same easing | `max-width` 0↔320 |
| Modal in | 0.22s | Scale 0.95→1, translateY 8→0 |
| Button press | 0.12s | Scale 0.96 |

### 1.7 Control chrome (Dashboard “hardware” look)

Default toolbar / menu button:

```
background: linear-gradient(180deg, #1E2128, #16181E)
border: 1px solid #3A3D45
border-radius: 3px
color: #C8CAD0
font: DM Sans 11px / weight 500
box-shadow: 0 1px 3px rgba(0,0,0,0.4), inset 0 1px 0 rgba(255,255,255,0.06)
padding: 3px 10px (top bar) or 5px 12px (bottom)
```

Elevated / primary split buttons use border `#4A4D55` and stronger outer shadow.

### 1.8 Semantic state colors (broadcast)

| State | Color | Where |
|-------|-------|-------|
| Preview | `#22C55E` | PREVIEW label, borders, scene badges, GO |
| Program / On Air | `#FF5A2C` | PROGRAM label, tiles, ON AIR badge |
| Selected (edit) | `#4F9EFF` | Source selection, scenes active |
| Transition active | `#3A6AFF` | Cut/Fade/… buttons |

---

## 2. App shell & navigation

### 2.1 Routing (`App.tsx`)

| Path | Screen |
|------|--------|
| `/` | Dashboard |
| `/scenes` | Redirect → `/` |
| `/chat` | ChatPanel |
| `/analytics` | Analytics |
| `/scoreboard` | ScoreboardPage |
| `/schedule` | SchedulePage |
| `/settings` | Settings |
| fallback | NotFound |

**Providers:** ErrorBoundary → ThemeProvider(dark) → SceneProvider → StreamingProvider → TooltipProvider → Toaster → Router.

### 2.2 AppSidebar — 56px icon rail

**File:** `AppSidebar.tsx`

```
┌────56px────┐  ┌──────── content (flex:1) ────────┐
│ Logo 46h   │  │                                  │
│ LIVE?      │  │         Page children            │
│ Nav icons  │  │                                  │
│            │  │                                  │
│ Signal+ver │  │                                  │
└────────────┘  └──────────────────────────────────┘
```

| Element | Spec |
|---------|------|
| Rail | `width/minWidth: 56`, `height: 100vh`, bg `#0F1114`, border-right `1px solid rgba(255,255,255,0.08)` |
| Logo cell | 56×46, bottom border same as rail |
| Logo mark | 30×30 circle, gradient `#FF5A2C → #FF8C42`, shadow `0 0 18px rgba(255,90,44,0.55)` |
| Cue-ball SVG | 16×16 white circle + crosshair lines |
| Nav button | 40×40, radius 8, gap 4 between items |
| Inactive icon | `#6B7280`, stroke 1.8 |
| Active | bg `{accent}22`, border `{accent}55`, glow `0 0 16px {glow}`, icon stroke 2.2 |
| Tooltip | bg `#1A1D22`, border `rgba(255,255,255,0.12)`, text `#E2E8F0` 12px, side right |
| LIVE block | Pulsing `#FF5A2C` / `#FF3A0C` dot + “LIVE” 7px weight 800 letter-spacing 0.12em |
| Signal bars | Heights 3,5,7,9,11; first 4 `#22C55E`, last `#2D3748`; width 3, radius 1.5 |
| Version | JetBrains Mono 8px `#4B5563` |

**Nav items (order):**

| Path | Label | Active / glow |
|------|-------|---------------|
| `/` | Dashboard | `#FF5A2C` / `rgba(255,90,44,0.65)` |
| `/chat` | Chat | `#A855F7` / `rgba(168,85,247,0.65)` |
| `/analytics` | Analytics | `#22D3EE` / `rgba(34,211,238,0.65)` |
| `/scoreboard` | Scoreboard | `#22C55E` / `rgba(34,197,94,0.65)` |
| `/schedule` | Schedule | `#FACC15` / `rgba(250,204,21,0.65)` |
| `/settings` | Settings | `#94A3B8` / `rgba(120,120,160,0.45)` |

**Qt note:** `SidebarRail.h` lists Dashboard, Scene Editor, Chat, Analytics, Settings (no Scoreboard/Schedule). React is the richer nav; restore should include Scoreboard + Schedule and either a Scene Editor entry or keep editor inside Dashboard.

---

## 3. Dashboard (unified broadcast surface)

**File:** `Dashboard.tsx` (~2000 lines)  
**Layout metaphor:** vMix-style Preview | Transitions | Program + input tiles + mixer + bottom action bar.

### 3.1 Overall structure

```
[AppSidebar 56]
  ┌─ Top menu bar (36px) ─────────────────────────────────────────┐
  ├─ Main: PREVIEW (flex) │ Transitions (120px) │ PROGRAM (flex) ─┤
  ├─ Input tiles row: Scenes(180) │ Source tiles │ Mixer(0–320) ──┤
  └─ Bottom action toolbar ───────────────────────────────────────┘
  + Modals/drawers (Add Input, Go Live, Input Settings, shortcuts…)
```

Root: `column`, `height: 100vh`, `overflow: hidden`, bg `#0B0D0F`, font DM Sans.

### 3.2 Top menu bar (36px)

| Spec | Value |
|------|-------|
| Height | 36 |
| Background | `linear-gradient(180deg, #1A1D22, #141619)` |
| Border | bottom `1px solid #2A2D35` |
| Shadow | `0 1px 0 rgba(0,0,0,0.5)` |
| Padding | `0 8px`, gap 2 |

**Left — brand:** RAILSHOT `#F0F0F0` + TV `#FF5A2C`, Bebas Neue 16px.

**Menu buttons (left):** Preset ▾ | New | Open | Save | Save As | Last  
(Preset is elevated gradient `#2A2D35→#1E2128`; others standard chrome.)

**Preset menu items:** Save New Preset… | Load Last Preset | Manage Presets…

**Right cluster:**

| Control | Behavior / style |
|---------|------------------|
| Fullscreen | Green tint when active (`#22C55E`) |
| Divider | 1×20 `#3A3D45` |
| Status text | Mono 10px `#606878`: `1080p29.97` · `EX FPS: 30` · `CPU: 3%`/`12%` |
| Pause Inputs | Orange when paused (`#F97316`) |
| Basic | Toast “coming soon” |
| Settings | Navigates `/settings` |
| `?` | 26×26; active blue gradient; toggles shortcuts overlay |

### 3.3 Preview monitor (left, flex:1)

| Region | Spec |
|--------|------|
| Column bg | `#0D0F12`, border-right `#2A2D35` |
| Label bar | pad `3px 10px`, gradient `#1A1D22→#141619` |
| Label | **PREVIEW** DM Sans 11px 700 `#22C55E` letter-spacing 0.06em |
| Scene name | Mono 10px `#A0A8B8` |
| CLEAR | Border `#22C55E40`, text `#22C55E70`, 9px 700 |
| Canvas well | `#080A0D`, centered 16:9 |
| Frame | Border `2px solid #22C55E30`, bg `#000` |
| Badge | Top-left: bg `#22C55E`, text black 9px 700 “PREVIEW” |
| Empty | Monitor icon `#2A3550` + “No Preview” `#3A4560` uppercase |

**Interaction:** `ProgramCanvas` with selectable/draggable sources when a preview scene is set.

### 3.4 Center transition column (120px)

| Element | Spec |
|---------|------|
| Width | 120 fixed |
| Background | `linear-gradient(180deg, #141619, #0F1114)` |
| Borders | left+right `#2A2D35` |

**GO button**

- Margin `8px 6px 4px`, pad `10px 0`, radius 4, font 15px weight 900 letter-spacing 0.08em
- **Armed** (preview set): gradient `#22C55E→#16A34A`, border `#22C55E`, text `#000`, glow `0 0 16px rgba(34,197,94,0.5)` (hover → 24px / 0.8)
- **Disarmed:** muted green `#2A3A2A`, text `#3A5A3A`, not-allowed

**Active transition label:** 9px `#606878` uppercase centered.

**Transition types (split buttons):** Cut | Fade | Merge | Wipe | CubeZoom | FTB

- Main: flex 1, pad `7px 0`, radius `3px 0 0 3px`
- Active: gradient `#3A6AFF→#2A50CC`, border `#5A8AFF`, glow blue
- Inactive: `#2A2D35→#1E2128`, border `#3A3D45`, text `#C0C2C8`
- Options chevron: width 18, radius `0 3px 3px 0`

**Per-type options menus:**

| Type | Options |
|------|---------|
| Cut | “No options” |
| Fade | Duration 250–2000ms; Curve ease-in-out/linear/ease-in/ease-out |
| Merge | Blend: normal, screen, multiply, overlay, hard-light |
| Wipe | Direction L/R/U/D/diagonals; Border 0/2/4/8px |
| CubeZoom | Axis X/Y/Z; Depth 25–100% |
| FTB | Hold at black 0–1000ms |

**Scene number pad:** 4×2 grid buttons 1–8

- Preview: green gradient + glow
- Program: orange gradient + glow
- Empty slot: muted `#404450`

**Speed fader:** Label “Speed”; range 100–2000ms; `accentColor: #3A6AFF`.

### 3.5 Program monitor (right, flex:1)

| Spec | Value |
|------|-------|
| Label | **PROGRAM** `#FF5A2C` (dim `#FF5A2C80` when not live) |
| Live cue | `● LIVE` mono `#FF5A2C` + `LiveTimecode` `HH:MM:SS` |
| CLEAR | Orange-tinted like Preview CLEAR |
| Frame border | `#FF5A2C50` live / `#FF5A2C20` idle; live glow `0 0 30px rgba(255,90,44,0.15)` |
| ON AIR badge | `#FF5A2C`, white text, pulse animation |
| Corner markers | 14×14 L-brackets; live orange / idle blue `#4F9EFF` @ 0.3 |
| Empty | “No Output” |

### 3.6 ProgramCanvas / source transforms

**Default normalized transforms** (`x,y,w,h` 0–1):

| Type | Transform |
|------|-----------|
| camera | 0.05, 0.05, 0.4, 0.4 |
| display | 0, 0, 1, 1 |
| browser | 0.1, 0.1, 0.8, 0.8 |
| image | 0.05, 0.05, 0.3, 0.3 |
| text | 0.1, 0.7, 0.5, 0.12 |
| alert | 0.2, 0.05, 0.6, 0.25 |
| scoreboard | 0.05, 0.05, 0.9, 0.9 |
| lowerthird | 0, 0.72, 1, 0.2 |

Browser sources use double-buffered iframes (`SmoothBrowserFrame`) for seamless URL swaps.

### 3.7 Source type catalogue (core)

| type | Label | Color |
|------|-------|-------|
| display | Display Capture | `#4F9EFF` |
| camera | Camera / Webcam | `#22C55E` |
| browser | Browser Source | `#22D3EE` |
| text | Text (GDI+) | `#A855F7` |
| image | Image | `#FBBF24` |
| alert | Alert / Stinger | `#FF5A2C` |
| scoreboard | Scoreboard | `#FF5A2C` |
| lowerthird | Lower Third | `#4F9EFF` |

### 3.8 Input tiles row

**Row:** bg `#0D0F12`, border-top `#2A2D35`.

**Left:** SceneManagerPanel (180px) — see §4.

**Center:** horizontal source tiles (`minWidth 160`, `maxWidth 200`, `flex 1 1 160`).

| Tile state | Header bg | Outline | Body tint |
|------------|-----------|---------|-----------|
| In Program | orange gradient `#FF5A2C→#CC3A14` | `#FF5A2C60` | `#1A0A0A` |
| In Preview | green `#22C55E→#16A34A` | `#22C55E60` | `#0A150A` |
| Selected | blue-ish `#1A2A3A` | `#4F9EFF60` | `#0A1220` |
| Default | `#1E2128→#181B22` | none | `#0D0F12` |

**Tile chrome:**

- Header: icon + `{index} {name}` 10px 700
- Gear → Input Settings; Eye visibility; X delete
- Preview area: minH 60 maxH 80, type icon + type label
- Footer: ◀ ▶ reorder, mini **GO**, **Cut**
- Context menu: Select, Hide/Show, Duplicate, Input Settings…, Delete Input (red)

**Empty copy:**

- No scene: “No scene selected — click + in the Scenes panel to create one”
- No sources: “No inputs — click Add Input to begin”

### 3.9 Audio mixer (collapsible, right of tiles)

| Spec | Value |
|------|-------|
| Open width | max-width 320 |
| Closed | max-width 0 |
| Transition | 0.25s cubic-bezier(0.23,1,0.32,1) |
| Vertical labels | “OUTPUTS” / “INPUTS”, 8px 700 `#3A6AFF`, writing-mode vertical |

**Master strip (68px):**

- Badge MASTER green (or red when muted)
- `VUMeterVertical` color `#22C55E`
- dB readout mono 8px
- Volume 0–100; Pan −50..50 accent `#4F9EFF`
- Mute: `M` / `MUTED`

**Channel strips (min 58px):** per Desktop Audio (−1, `#4F9EFF`), Mic/Aux (−2, `#22C55E`), then scene sources

- VU + dB + volume + pan label C/Ln/Rn + Gain number (−12..12, amber when ≠0)
- Name 8px truncated
- **S** solo amber / **M** mute red

### 3.10 VUMeterVertical

| Spec | Value |
|------|-------|
| Height | 80 |
| Channels | 2 bars × width 8 |
| Well | `#0A0E1A`, border `rgba(255,255,255,0.06)` |
| Fill | Gradient bottom→top: channel color → `#84CC16` → `#FBBF24` → `#EF4444` |
| Peak hold | 2px white (red if >0.9) |
| dB marks | 0, −6, −12, −18, −24, −30, −42, −54 |
| Update | ~60ms when active |

### 3.11 Bottom action toolbar

Background: `linear-gradient(180deg, #141619, #0F1114)`, border-top `#2A2D35`, pad `4px 8px`.

| Control | Notes |
|---------|-------|
| Add Input + ▾ | Opens modal; menu also Virtual Input / NDI Scan (toasts) |
| Cpu icon 28×28 | → Settings |
| Record + ▾ | Red when recording; `RecordTimecode`; MP4/MKV/Settings |
| External + ▾ | Green when on; HDMI / NDI |
| Stream | Idle: orange gradient “● Stream” opens Go Live; Live: “■ End Stream” dark red |
| MultiCorder + ▾ | Blue tint when open |
| PlayList | Violet tint when open |
| Overlay | Toggles overlay browser state |
| Square / Activity / Layout / Cpu / Lock | Compact tiles, stats, layout presets, perf toast, UI lock |
| Audio Mixer | Blue glow when open (`#1A3AFF`) |
| Status pill | Mono 9px: 1080p29.97, EX FPS, Render, GPU, CPU, Total |

---

## 4. Scene Manager Panel

**File:** `SceneManagerPanel.tsx`

| Spec | Value |
|------|-------|
| Width | 180 |
| Background | `#0A0C0F` |
| Border | right `#2A2D35` |
| Header | gradient `#1A1D22→#141619`, pad `5px 8px` |
| Title | **SCENES** 9px 700 `#4F9EFF` uppercase 0.12em |
| Add (+) | 20×20, gradient `#1A3AFF→#1230CC`, border `#3A6AFF`, glow |

**Row states:**

| State | Left accent | Name color | Row gradient |
|-------|-------------|------------|--------------|
| Program | `#FF5A2C` 3px | `#FF8A6A` | `#2A0A0A→#1A0808` |
| Preview | `#22C55E` | `#6EE7A0` | `#0A1A0A→#081208` |
| Active | `#4F9EFF` | `#C8DAFF` | `#0D1A2A→#0A1220` |
| Idle | transparent | `#8892A4` | none |

Features: drag reorder, rename inline, move up/down chevrons, GripVertical, source-count pill, context menu (Set Active, Rename, Duplicate, Delete `#EF4444`).

Empty: “No scenes yet. Click + to add one.” 10px `#404450`.

---

## 5. Input Settings Drawer

**File:** `InputSettingsDrawer.tsx`

| Spec | Value |
|------|-------|
| Position | Fixed right, full height |
| Width | 460 |
| Background | gradient `#13151A → #0F1114` |
| Border | left `#2A2D35` |
| Shadow | `-8px 0 40px rgba(0,0,0,0.7)` |
| Backdrop | `rgba(0,0,0,0.4)` |
| Slide | translateX 100%→0, 0.25s |

**Tabs:** Source Config | General | Position | Colour Adjust | Effects | Layers | Audio Settings  
Active tab: bottom border 2px `#4F9EFF`, text `#4F9EFF` 10px 700.

**Shared field styles:**

- Input: bg `#0A0C10`, border `#3A3D45`, text `#D0D2D8`, 11px, radius 3
- Section title: 9px 700 `#4F9EFF` uppercase
- Slider accent: `#4F9EFF`

**Tab contents (summary):**

| Tab | Controls |
|-----|----------|
| General | Name, Aspect (Source/16:9/4:3/1:1/9:16/21:9/Custom), category swatches (`#1A1A1A`,`#EF4444`,`#22C55E`,`#FBBF24`,`#EC4899`,`#3B82F6`,`#A855F7`), playback checkboxes |
| Position | X/Y/W/H/Z/Rot/Opacity, Crop L/R/T/B, Reset |
| Colour | Brightness/Contrast/Saturation/Hue/Alpha, Reset Colour |
| Effects | Blur/Sharpen/Pixelate, Luma/Chroma key (`#00FF00` default) |
| Layers | Scene source list with visibility dots |
| Audio | Volume 0–200%, Pan, Gain, Delay, Mute, bus routing |
| Source Config | Type-specific (camera, NDI, browser URL, scoreboard, etc.) |

Footer: Copy From… | Cancel | **Apply** (blue gradient + glow).

---

## 6. Go Live Modal

**File:** `GoLiveModal.tsx`

| Spec | Value |
|------|-------|
| Backdrop | `rgba(0,0,0,0.72)`, blur 6px |
| Panel | width 520, radius 12, bg `#141926`, border `#2A3350` |
| Top accent | 3px `#FF5A2C → #FF6B35 → #A855F7` |
| Shadow | `0 32px 80px rgba(0,0,0,0.7)` + orange ring |

**Steps:**

1. **config** — Title “GO LIVE” Bebas 20px; platforms YouTube `#FF0000`, Twitch `#9146FF`, Facebook `#1877F2`, Custom `#FF5A2C` (buttons 72×64); Stream Title; Category; Stream Key (mono, focus orange); readiness chips; CTA **RUN CHECKS & GO LIVE** orange gradient h:42 glow.
2. **checking** — Sequential checks: Audio `#A855F7`, Scene `#4F9EFF`, Encoder `#22C55E`, Network `#22D3EE`, Stream Key `#FBBF24`.
3. **countdown** — 120px circle, Bebas 72px `#FF5A2C`, “GOING LIVE IN”.
4. **live** — “YOU ARE LIVE” Bebas 36px + Radio icon pulse.

---

## 7. Add Input / Input Select Modal

**File:** embedded in `Dashboard.tsx` as `AddSourceModal`

| Spec | Value |
|------|-------|
| Backdrop | `rgba(0,0,0,0.82)` |
| Panel | width 780, maxH 88vh, bg `#141618`, border `#3A3D45`, radius 4 |
| Title | “Input Select” |
| Left list | width 210, bg `#0F1114` |
| Selected row | bg `#1A3A6A`, left 3px type color |
| Footer | Cancel | OK (blue gradient `#1A6AFF→#1050CC`) |

**VMIX_INPUT_TYPES** (26 entries): Video, DVD, List, Camera, NDI/OMT/Desktop, Stream/SRT, Instant Replay, Image Sequence/Stinger, Video Delay, Image, Photos, PowerPoint, Colour, Audio, Audio Input, Title, Virtual Set, Web Browser, Video Call, Zoom, Telestrator, Scoreboard, Lower Third, Overlay, Display Capture, Alert/Stinger — each with emoji icon + accent color.

Right pane: type-colored title + `InputConfigPanel` (name + type-specific fields).

---

## 8. Dashboard side panels & overlays

### 8.1 PropertiesPanel (inline compact)

Transform section (violet-tinted number inputs): X Y W H Rot Opacity; Flip H / Flip V / Reset.  
Settings section type-specific (camera device/RTSP, browser URL/size, image path, text, display monitor, alert URL, scoreboard sport, lower-third title/subtitle/duration). Empty: “Select a source”.

### 8.2 New Project AlertDialog

Title “New Project”; warn clear scenes; Cancel / **Clear & Start New** (red).

### 8.3 MultiCorder panel

Fixed right, width 320, top offset 36; header “MULTICORDER” `#93C5FD`; per-source ● REC buttons.

### 8.4 PlayList panel

Width 300; header “PLAYLIST” `#D8B4FE`; scene list; **▶ GO (Preview → Program)** green.

### 8.5 Keyboard shortcuts overlay

560px panel; sections Transitions / Stream / UI; key chips mono on `#1E2128`; section headers `#3A6AFF`.

| Key | Action |
|-----|--------|
| Space / Enter | GO |
| 1–8 | Preview scene N |
| F | Fullscreen |
| Shift+? | This overlay |
| Ctrl+Shift+S | Stream |
| Ctrl+Shift+R | Record |
| Esc | Close |
| Ctrl+S / Ctrl+O | Save / Open |

### 8.6 Overlay templates (Dashboard catalogue)

Billiards Scoreboard, Basketball Board, Player/Team Lower Third, Score Ticker, Sub Alert, Logo Overlay, Camera Frame.

---

## 9. Scene Editor

**File:** `SceneEditor.tsx` (not routed in App — standalone / Qt page equivalent)

### 9.1 Layout

```
TopBar 46px
Tool rail 40px | Overlay library 220px (optional) | Canvas flex | Sources/Props 240px
Transitions rail 44px
```

### 9.2 Overlay library

Categories: All `#8892A4`, Scoreboard `#FF5A2C`, Lower Thirds `#4F9EFF`, Tickers `#22D3EE`, Alerts `#FBBF24`, Branding `#A855F7`.

**17 templates** with SVG thumbs (160×90): sb-lower, sb-center, sb-corner, sb-full, lt-player, lt-commentator, lt-sponsor, plus tickers/alerts/branding variants.

Canvas: bg `#060608`, grid 40px `#4F9EFF` @ 0.06, corner brackets, drag-drop flash green.

Transitions: Cut, Fade, Slide, Wipe, Stinger; duration 300ms; trigger button.

### 9.3 Qt SceneEditorPage columns (restore target)

180 (scenes+sources) | canvas | 240 overlay | 320 properties | transitions 44px.  
QSS surfaces: `#1A2035`, `#1E2640`, `#161B2E`, borders `#2A3350`.

---

## 10. Chat & Audience

**File:** `ChatPanel.tsx`

### 10.1 Layout

```
Top bar 46px (#141619)
Left 220px (Connections / Filter / Session Stats)
Center (tabs: Live Chat | Activity | Moderation + feed + composer)
Right 220px (Pinned / Highlights / Recent Activity)
```

### 10.2 Platform colors

| Platform | Color | Tint |
|----------|-------|------|
| Twitch | `#9146FF` | `#9146FF20` |
| YouTube | `#FF0000` | `#FF000020` |
| Facebook | `#1877F2` | `#1877F220` |

### 10.3 Connection states

| Status | Color |
|--------|-------|
| connected | `#22C55E` |
| connecting | `#FBBF24` (+ spin) |
| disconnected | `#606878` |
| error | `#FF5A2C` |

### 10.4 Message accents

- Donation: amber border/bg
- Sub/highlight: violet
- Pinned: blue
- MOD badge: emerald
- Platform chip: 9px platform color

### 10.5 Moderation toggles

Slow Mode (amber), Subscribers Only (violet), Emote Only (cyan), Mute Alerts (brand); Clear All Chat (brand tint).

### 10.6 ApiKeyDialog

maxWidth 440; bg `#141619`; channel + API key (JetBrains Mono); Connect uses platform accent.

### 10.7 Qt ChatPage mapping

Simpler split: chat 340px (violet header underline) + activity flex (cyan underline). React is richer; restore React layout for “original” Chromatic Command.

---

## 11. Scoreboard

**File:** `ScoreboardPage.tsx`

### 11.1 Layout

```
Top bar 46px (border-top 2px #22C55E)
Left 280px controls | Center preview | Right 240px style
Timer bar under preview (h ~56)
```

Page bg `#0D1117`. Wordmark: RAILSHOT `#FF5A2C`, TV `#E2E8F0`, **SCOREBOARD** Bebas 13px `#22C55E`.

Top actions: OVERLAY ON/OFF | SAVE PRESET (blue) | LOAD PRESET (violet).

### 11.2 Sport presets

Generic, Pool/Billiards, Basketball, Soccer, Tennis, Custom — active amber `#F59E0B`.

Period/score labels: Rack/Racks (pool), Quarter/Points, Half/Goals, Set/Games.

### 11.3 Teams

Color swatches: `#FF5A2C #4F9EFF #A855F7 #22C55E #22D3EE #F59E0B #EF4444 #EC4899 #FFFFFF #94A3B8`.  
Score ± buttons 36×36 tinted with team color.

### 11.4 Layouts & themes

Layouts: Lower Third, Center Banner, Corner Compact, Full Width.  
Themes:

| Theme | Overlay BG | Accent |
|-------|------------|--------|
| Dark | `rgba(10,12,20,0.92)` | `#4F9EFF` |
| Light | `rgba(240,244,255,0.95)` | `#1E40AF` |
| Team | `rgba(20,10,40,0.92)` | `#A855F7` |
| Neon | `rgba(0,8,24,0.95)` | `#22D3EE` |
| Minimal | `rgba(15,15,15,0.88)` | `#FF5A2C` |

Hidden overlay: diagonal hatch + “OVERLAY HIDDEN”.

### 11.5 Data model (`ScoreboardState.h`)

Enums mirror React; defaults Team Alpha/Beta, event “LIVE EVENT”.

---

## 12. Schedule

**File:** `SchedulePage.tsx`

| Spec | Value |
|------|-------|
| Page bg | `#0F1623` |
| Top bar | `#111827`, 46px |
| Platforms | YT `#FF0000`, Twitch `#9147FF`, FB `#1877F2`, Custom `#22D3EE` |

**NextUpBanner:** blue→violet gradient tint; countdown Bebas 32px `#4F9EFF`; Start Stream brand gradient.

**Stats cards:** Total Events, Upcoming, Est. Viewers, Completed.

**EventCard statuses:** upcoming / live (pulse `#FF5A2C`) / completed / cancelled.

**EventModal:** 520px, bg `#1A2035`, fields Title*, Description, Sport, Platform, Start*, Duration, Venue, Tags, reminders toggle.

Empty: dashed border, Calendar icon, “No events found”.

---

## 13. Analytics

**File:** `Analytics.tsx`

| Spec | Value |
|------|-------|
| Top bar | `#1A2035`, 46px |
| Body | `#161B2E` |
| Cards | `#1E2640` / border `#2A3350` |

**Range pills:** 5m 15m 1h 3h 6h 12h — active cyan.  
**KPIs:** Peak Viewers `#4F9EFF`, Avg Watch `#A855F7`, Followers `#22C55E`, Revenue `#FBBF24` — Bebas 28px values.  
**Panels:** Viewer Count (`panel-header-cyan`), Stream Health (emerald), Audience (violet), Sessions (brand).  
**Health lines:** Bitrate blue, CPU green, GPU cyan, FPS amber.  
Empty: “—”, “No data yet”, “NO DATA” badge.

---

## 14. Settings

**File:** `Settings.tsx`

```
Top 46px | Left tabs 168px | Content max 640 | Footer Save/Cancel
```

Surfaces: `#1A2035` chrome, content `#141928`, cards `#1E2640`.

**Tabs:** General, Stream, Output, Video, Audio, Hotkeys, Advanced, Plugins.  
Active: left border `#FF5A2C`, bg `#FF5A2C0F`.

**Controls:** Toggle 36×20 ON `#FF5A2C`; Seg bordered `#303D5A`; inputs h:30.

| Tab | Key fields |
|-----|------------|
| General | Theme dark/light/system, Language, startup toggles |
| Stream | Platform, Stream Key, Encoder NVENC/QSV/AMF/x264, CBR/VBR/CQP, bitrates, keyframe |
| Output | Path, MKV/MP4/MOV/FLV, Replay Buffer seconds |
| Video | Canvas/Output res, FPS 24/30/60/120 |
| Audio | Sample rate, channels, desktop/mic devices |
| Hotkeys | Stream, Record, Scene 1–3, Switch Cam |
| Advanced | Priority, network optimize, low latency, bind IP |
| Plugins | Toggle list + Install |

Footer **SAVE SETTINGS** glows orange when dirty.

---

## 15. Shared primitives

### 15.1 Dropdown menus (`dropdown-menu.tsx`)

Popover tokens; Dashboard overrides: bg `#1A1D22`, border `#3A3D45`, text `#C8CAD0`, separators `#2A2D35`.

### 15.2 Alert dialog (`alert-dialog.tsx`)

Centered modal, fade/zoom; Dashboard New Project uses dark overrides above.

### 15.3 Page top bar pattern (secondary pages)

Height **46px**; Bebas wordmark + divider + uppercase page title DM Sans 11px `#8892A4` letter-spacing 0.1em.

---

## 16. Qt reference mapping

| Qt file | Role | React equivalent |
|---------|------|------------------|
| `MainWindow.cpp` | 56 rail + stack; 3px live border | AppSidebar + Router; `.live-top-border` |
| `SidebarRail.h` | Icon nav | AppSidebar (add Scoreboard/Schedule) |
| `DashboardPage.h` | Single program + stream status 240 | Prefer React dual Preview/Program |
| `SceneEditorPage.*` | 4-column editor | SceneEditor.tsx |
| `SourcePropertiesPanel.h` | 320px props | InputSettingsDrawer / PropertiesPanel |
| `GoLiveDialog.h` | 4-step stack | GoLiveModal.tsx (visual SoT) |
| `VUMeter.h` | 20 LED segments | VUMeterVertical continuous (pick one) |
| `SceneCard.h` | 100×70 thumbs | SceneManagerPanel rows |
| `ChatPage.cpp` | 340 + activity | ChatPanel (richer) |
| `OverlayBrowser.*` | Full QSS + 17 templates | SceneEditor overlay column |
| `ScoreboardState.h` | Data model | ScoreboardPage |

**Decision for native restore:** Treat **React Chromatic Command Dashboard** (Preview/Program/GO) as the primary production UX; use Qt Scene Editor column widths and OverlayBrowser QSS as the overlay-library stylesheet reference.

---

## 17. Restore checklist

Use this as an implementation order for native Qt:

1. **Tokens & fonts** — Bebas / DM Sans / JetBrains; palette table §1.3; QSS variables.
2. **Shell** — 56px rail, logo glow, per-route accents, LIVE ripple, signal + v2.5, optional 3px live top border.
3. **Dashboard chrome** — 36px top bar, dual monitors, 120px transition column, GO/Cut/Fade styling.
4. **Scenes + tiles** — SceneManagerPanel states; input tiles program/preview/selected; context menus.
5. **Mixer** — Collapsible 320px, master + channels, vertical VU, S/M.
6. **Bottom toolbar** — Add Input, Record, Stream, MultiCorder, PlayList, Audio Mixer, status pill.
7. **Modals** — Go Live 4-step; Input Select 780px; Input Settings 460 drawer; shortcuts overlay.
8. **Secondary pages** — Chat / Scoreboard / Schedule / Analytics / Settings layouts + accents.
9. **Scene Editor / overlays** — Library categories + 17 templates; transitions rail.
10. **Motion** — Button press, live pulse, mixer slide, modal in — match timings in §1.6. **Done (Phase 5).**

### Acceptance criteria (visual)

- [x] RAILSHOT + orange TV wordmark on every page top bar / dashboard
- [x] Preview always green, Program always orange, selection blue
- [x] GO button glows green when Preview armed
- [x] Stream CTA glows brand orange when idle
- [x] Panel headers use chromatic left-border + gradient tint classes
- [x] No flat single-color toolbars — use 180° gradients + inset highlight
- [x] Empty states use muted copy + icon, never blank white
- [x] Motion tokens (§1.6): live ripple/pulse, live-border-in, mixer slide easing, modal enter, primary CTA press

---

## 18. File inventory

### React / design system (authoritative visuals)

| File | Component |
|------|-----------|
| `index.css` | Tokens, panel headers, live animations |
| `index.html` | Fonts |
| `App.tsx` | Routes / providers |
| `AppSidebar.tsx` | Icon rail |
| `Dashboard.tsx` | Main broadcast UI + modals |
| `SceneManagerPanel.tsx` | Scenes list |
| `InputSettingsDrawer.tsx` | Source drawer |
| `GoLiveModal.tsx` | Go live flow |
| `SceneEditor.tsx` | Overlay editor |
| `ChatPanel.tsx` | Chat page |
| `ScoreboardPage.tsx` | Scoreboard |
| `SchedulePage.tsx` | Schedule |
| `Analytics.tsx` | Analytics |
| `Settings.tsx` | Settings |
| `SceneContext.tsx` | Scene/source model |
| `StreamingContext.tsx` | isLive |
| `dropdown-menu.tsx` / `alert-dialog.tsx` | Primitives |

### Qt references (structure / QSS hints)

| File | Notes |
|------|-------|
| `MainWindow.cpp` | Shell + live border |
| `SidebarRail.h` | Rail structure |
| `DashboardPage.h` | Alternate dashboard layout |
| `SceneEditorPage.h/.cpp` | Editor columns + QSS |
| `SourcePropertiesPanel.h` | Props dock |
| `GoLiveDialog.h` | Dialog steps |
| `VUMeter.h` | Segmented meter |
| `SceneCard.h` | Thumbnail card |
| `ChatPage.cpp` | Chat split QSS |
| `OverlayBrowser.h/.cpp` | Overlay library QSS + templates |
| `ScoreboardState.h` | Scoreboard model |
| `main.cpp` | Window size defaults (1440×900, min 1024×640) |

### Non-UI (skip for visual restore)

OBSCore, OBSSourceManager, OBSAudioManager, OBSOutputManager, OBSStatsMonitor, OBSDisplay, TwitchChatClient, EventModel, SceneModel — engine stubs unless needed for labels.

---

*Generated from a full audit of `Professional UI Designs for Livestreaming Windows App/` for RailShotTV native UI restoration.*
