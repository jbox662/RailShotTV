# Nexus Broadcast — UI Design Specification for Cursor

> This document is a complete, authoritative reference for the Nexus Broadcast app UI. Use it as the single source of truth when implementing, extending, or modifying any screen. Every design decision is intentional — do not deviate without a strong reason.

---

## 1. Design Philosophy

**Theme:** Obsidian Studio — Professional Broadcast Hardware UI

The design is inspired by high-end broadcast control surfaces: Blackmagic ATEM switchers, Ross Video production systems, and DaVinci Resolve. The goal is to feel like **a physical piece of broadcast equipment rendered in software** — not a generic dark SaaS dashboard.

Every screen must feel like a **control room panel**: dense, purposeful, always showing operational state. Empty space is not "clean" — it is "waiting for signal." Fill it with instrumentation.

**Four Core Principles:**
1. **Information density without clutter** — every pixel earns its place; no decorative whitespace
2. **Hierarchy through luminosity** — brighter = more important; dim = secondary/inactive
3. **Precision over decoration** — functional beauty; no gradients unless they carry meaning
4. **Status at a glance** — critical states (LIVE, errors, health) are always visible

---

## 2. Color System

All colors are defined as exact hex or rgba values. Do **not** substitute with Tailwind color names — always use inline styles or CSS variables for these.

### Surface Hierarchy

| Layer | Color | Usage |
|---|---|---|
| App background | `#0E0F14` | Root background, main content area |
| Sidebar / top-level panels | `#111318` | Sidebar, panel backgrounds |
| Elevated panels / cards | `#16181F` | Secondary panels, settings tabs |
| Input / control backgrounds | `#0E0F14` | Text inputs, selects, buttons |
| Active/selected state | `#1A1D2B` | Highlighted nav item, selected row |
| Canvas / workspace | `#0A0B0F` | Program output canvas, scene editor workspace |

### Borders

All borders are **whisper-thin** — they define structure without adding visual weight.

```
Primary border:   rgba(255,255,255,0.07)   — panel edges, sidebar border
Secondary border: rgba(255,255,255,0.08)   — card/panel borders
Subtle divider:   rgba(255,255,255,0.05)   — inner section dividers
Input border:     rgba(255,255,255,0.12)   — form inputs (slightly more visible)
```

### Accent Colors (Semantic — Never Swap These)

| Color | Hex | Semantic Meaning | Use For |
|---|---|---|---|
| **Nexus Blue** | `#3B82F6` | Primary interaction / brand | Active states, CTAs, selected items, primary buttons, active nav border |
| **Live Red** | `#EF4444` | Danger / live state | LIVE badges, End Stream button, critical alerts, dropped frames |
| **Health Green** | `#22C55E` | Good / connected | Signal OK, CPU healthy, connected status, success states |
| **Warning Amber** | `#F59E0B` | Caution / degraded | Low bitrate, high CPU, donation highlights |
| **Cyan** | `#06B6D4` | Secondary data / timecode | Timecodes, chart secondary lines, uptime counters |
| **Muted White** | `rgba(255,255,255,0.5)` | Secondary labels | Inactive nav items, secondary text |
| **Dim White** | `rgba(255,255,255,0.3)` | Tertiary labels | Metadata, timestamps, placeholder text |
| **Ghost White** | `rgba(255,255,255,0.1)` | Decorative | Subtle borders, disabled states |

> **Rule:** Third-party platform colors (Twitch `#9146FF`, YouTube `#FF0000`, Facebook `#1877F2`) are allowed **only** for platform-selector buttons and chat username colors. They must never dominate a panel — always subordinate to Nexus Blue.

---

## 3. Typography

Three fonts are loaded via Google Fonts. All three serve distinct roles — never substitute one for another.

```html
<link href="https://fonts.googleapis.com/css2?family=Space+Grotesk:wght@500;600;700&family=Inter:wght@400;500;600&family=JetBrains+Mono:wght@400;500;700&display=swap" rel="stylesheet" />
```

### Font Roles

| Font | Role | Where Used |
|---|---|---|
| **Space Grotesk** | Display / Brand | Page titles, section headers, logo wordmark, KPI values |
| **Inter** | UI / Body | Nav labels, form labels, body copy, button text |
| **JetBrains Mono** | Data / Precision | Timecodes, bitrate values, dB readings, FPS, resolution strings, version numbers |

### Type Scale

```
9px   — Micro labels (badge text, version, signal indicators)  — Inter or JetBrains Mono
10px  — Small labels (section headers, instrumentation strip)  — Inter 600 uppercase letter-spaced
11px  — Secondary body (status text, timestamps)               — Inter 400/500
12px  — Primary body (form labels, nav items, settings values) — Inter 400/500
13px  — Nav labels, panel headings                             — Inter 500
15px  — Page titles (old style — now replaced by 12px uppercase) — Space Grotesk 600
20px  — KPI values                                             — Space Grotesk 700
28px+ — Hero numbers (peak viewers, revenue)                   — Space Grotesk 700
```

### Section Header Pattern

All panel section headers follow this exact pattern:
```tsx
<span style={{
  fontSize: 10,
  fontWeight: 600,
  color: "rgba(255,255,255,0.4)",
  letterSpacing: "0.1em",
  fontFamily: "'Inter', sans-serif",
  textTransform: "uppercase"
}}>
  SECTION NAME
</span>
```

### Page Title Bar Pattern (Consistent Across All Screens)

Every screen has a top bar with this exact structure:
```tsx
<div style={{ minHeight: 46, background: "#0D0E12", borderBottom: "1px solid rgba(255,255,255,0.07)" }}>
  {/* Page name — all caps, Space Grotesk Bold */}
  <span style={{ fontFamily: "'Space Grotesk', sans-serif", fontWeight: 700, fontSize: 12, color: "#fff", letterSpacing: "0.1em" }}>
    PAGE NAME
  </span>

  {/* Vertical divider */}
  <div style={{ width: 1, height: 16, background: "rgba(255,255,255,0.1)" }} />

  {/* Subtitle or breadcrumb — JetBrains Mono, dim */}
  <span style={{ fontSize: 10, color: "rgba(255,255,255,0.3)", fontFamily: "'JetBrains Mono', monospace" }}>
    subtitle / context
  </span>

  {/* Right side: status indicators + actions */}
</div>
```

---

## 4. Layout Structure

### Global Shell

```
┌─────────────────────────────────────────────────────────┐
│  Sidebar (220px fixed)  │  Main Content Area (flex-1)   │
│  background: #111318    │  background: #0E0F14           │
│                         │  ┌──────────────────────────┐ │
│  [Logo Block]           │  │ Top Bar (46px)           │ │
│  [Nav Items]            │  │ background: #0D0E12      │ │
│  [Connection Status]    │  ├──────────────────────────┤ │
│                         │  │ Page Content (flex-1)    │ │
│                         │  │ overflow: hidden/scroll  │ │
│                         │  └──────────────────────────┘ │
└─────────────────────────────────────────────────────────┘
```

### Sidebar Anatomy (220px)

```
┌──────────────────────┐
│ [Icon] NEXUS         │  ← Logo block, 56px min-height
│        BROADCAST PRO │    border-bottom: rgba(255,255,255,0.07)
├──────────────────────┤
│ ▌ Dashboard          │  ← Active: 3px blue left border + #1A1D2B bg
│   Scene Editor       │  ← Inactive: transparent border, 50% opacity text
│   Chat & Audience    │
│   Analytics          │
│   Settings           │
├──────────────────────┤
│ ● Connected          │  ← Connection status footer
│ ● LIVE · 01:23:47    │    border-top: rgba(255,255,255,0.07)
│ ▐▐▐▐▐ NEXUS v2.4.1  │    Signal bars + version
└──────────────────────┘
```

**Sidebar nav item active state:**
```tsx
style={{ background: "#1A1D2B", borderLeft: "3px solid #3B82F6", paddingLeft: 13 }}
// Icon: strokeWidth 2 (active) vs 1.5 (inactive)
// Text: fontWeight 500 (active) vs 400 (inactive), color white vs white/50
```

---

## 5. Screen-by-Screen Specification

### Screen 1: Dashboard (`/`)

**Layout:** 3-column: [Center: Preview + Scenes + Mixer] | [Right: Stream Status 300px]

**Top Bar extras:** LIVE badge + timecode + resolution + fps + codec + SIG OK indicator

**Program Output Panel:**
- Aspect ratio: 16/9, background `#0A0B0F`
- SVG grid overlay: `rgba(255,255,255,0.06)` opacity, 40px grid
- 4 corner bracket markers: `rgba(59,130,246,0.6)` blue, 16×16px L-shapes
- Center placeholder: Monitor icon + resolution string in JetBrains Mono
- **Instrumentation strip** below canvas: timecode (cyan) | scene name | SRC ACTIVE (green) | viewer count (blue) | bitrate (green/amber)

**Scene Switcher:**
- Horizontal scrollable row of 110×70px thumbnails
- Active scene: 2px `#3B82F6` border + `box-shadow: 0 0 12px rgba(59,130,246,0.3)` + red LIVE badge
- Inactive: `rgba(255,255,255,0.08)` border

**Audio Mixer:**
- 4 channels: Mic, Desktop, Music, Alert
- Each channel: VU meter (20 bars, green→amber→red threshold), dB readout in JetBrains Mono, S/M buttons
- Master volume slider with dB display

**Stream Status Panel (right, 300px):**
- LIVE on YouTube indicator
- Bitrate: large value in blue + sparkline SVG chart
- Viewers + Uptime in 2-column grid
- Stream Health: CPU/GPU/Network progress bars with color-coded values
- END STREAM button: full-width, `#EF4444` background, bold white text

---

### Screen 2: Settings (`/settings`)

**Layout:** [Left tab nav 180px] | [Content area flex-1] | [Bottom action bar]

**Tab nav:** `background: #111318`, active tab: 3px blue left border + `#1A1D2B` bg

**Tabs:** General, Stream, Output, Video, Audio, Hotkeys, Advanced, Plugins

**Stream Tab content:**
- Platform selector: pill buttons (Twitch/YouTube/Facebook/Custom RTMP), active uses platform color
- Stream Key: password input + Show/Hide + "Get Key ↗" button in blue
- Server: dropdown select
- Encoder: NVENC / QSV / x264 toggle buttons, active in blue
- Encoder params: Rate Control, Bitrate Target/Min, Keyframe Interval — 4-column grid
- Output: Recording path + Browse, Format buttons (MKV/MP4/MOV), Replay Buffer toggle
- Video: Canvas Resolution, Output Resolution dropdowns + FPS toggle (30/60/120)

**Form row pattern:**
```tsx
// Label (160px fixed width) + control (flex-1)
// Divider: borderBottom: "1px solid rgba(255,255,255,0.05)"
```

**Bottom action bar:** Cancel (ghost) + Save Settings (blue primary) — fixed to bottom

---

### Screen 3: Chat & Audience (`/chat`)

**Layout:** [Left panel 220px] | [Center chat flex-1] | [Right panel 280px]

**Left panel:**
- Mini program preview (16/9 with timecode overlay)
- Quick Actions grid: Clip, Raid, Host, Poll (2×2) + Marker (full width)

**Chat center:**
- Header: platform color dots (T/Y/F) + viewer count badge
- Message types:
  - Regular chat: transparent background
  - Donation: `rgba(245,158,11,0.1)` bg + amber border
  - Sub: `rgba(145,70,255,0.1)` bg + purple border
- Message anatomy: `[timestamp] [MOD/SUB badge] [username:] [message]`
- MOD badge: green bg/border; SUB badge: purple bg/border
- Input: dark input + emoji button + send button (blue)
- Mod tools row: shield, ban, timeout, slow mode, clear icons

**Right panel:**
- Activity Feed: follow/sub/donation/raid events with colored icons
- Polls & Predictions: progress bars with percentage + countdown timer
- Chat Commands: grid of `!command` buttons with descriptions

---

### Screen 4: Analytics (`/analytics`)

**Layout:** Full-width scrollable content, 3-row structure

**KPI Cards (4-column grid):**
- Each card: colored top-left icon + metric label + large value (Space Grotesk 700) + delta badge
- Delta: green for positive (`+18.7% vs yesterday`), red for negative
- Card border uses accent color at 30% opacity: `border: 1px solid ${color}30`

**Viewer Chart:**
- Full-width recharts AreaChart/LineChart
- LIVE DATA badge (red pulse) in header
- Instrumentation strip: PEAK / AVG / CURRENT values in JetBrains Mono
- Time range selector: 5m / 15m / 1h / 3h / 6h / 12h pill buttons

**Middle row (2-column):**
- Stream Health: list of metrics (Bitrate, CPU, GPU, FPS, Dropped Frames) with colored right-aligned values
- Audience Breakdown: donut chart + platform legend + Top Countries bar chart

**Recent Sessions table:**
- Columns: Date, Duration, Peak Viewers (blue), Avg Viewers, Revenue (green), Quality Score (badge)
- Quality Score badges: Excellent (green), Good (blue), Fair (amber)

---

### Screen 5: Scene Editor (`/scenes`)

**Layout:** [Canvas + toolbar flex-1] | [Right inspector 300px]

**Top bar extras:** Project name chip (blue border) + live telemetry (LIVE/REC timers, CPU%, FPS, KB/s)

**Canvas workspace:**
- Background: `#0A0B0F`
- SVG grid: 60px grid, `rgba(255,255,255,0.04)` opacity
- Crosshair at center: 20px lines, `rgba(59,130,246,0.25)`
- Selected source: blue dashed border + 8 resize handles (corner + edge)
- Canvas metadata strip (bottom): resolution · aspect ratio · fps | scene name · source count | SAFE AREA: ON
- Corner bracket markers: `rgba(59,130,246,0.6)` L-shapes

**Right toolbar (vertical icon strip):**
Move, Scale, Crop, Rotate, Align, Lock — icon buttons with hover states

**Sources panel (right, 300px):**
- Header: SOURCES label + Add (+) + More (···) buttons
- Source rows: drag handle + visibility eye + lock + type icon + name + more menu
- Active source: `#1A1D2B` background + blue left accent
- Source types: Game Capture, Webcam, Text Overlay, Logo Image, Alert Overlay

**Source Properties panel:**
- Header: "SOURCE PROPERTIES: [NAME]" in uppercase
- Position (X/Y), Size (W/H), Rotation, Opacity — numeric inputs in JetBrains Mono
- Chroma Key toggle (green when active)
- Color picker + Similarity slider
- Flip Horizontal / Flip Vertical buttons
- Crop inputs (Top/Bottom/Left/Right) + Reset Crop button

**Transitions rail (bottom):**
- Transition types: Cut (active, blue), Fade, Slide, Wipe, Stinger
- Duration input (ms) + Apply Transition button

---

## 6. Reusable Component Patterns

### Panel / Card
```tsx
<div style={{
  background: "#111318",
  border: "1px solid rgba(255,255,255,0.08)",
  borderRadius: 8  // 0.5rem
}}>
  {/* Panel header */}
  <div style={{ borderBottom: "1px solid rgba(255,255,255,0.06)", padding: "8px 12px" }}>
    <span style={{ fontSize: 10, fontWeight: 600, color: "rgba(255,255,255,0.4)", letterSpacing: "0.1em" }}>
      PANEL TITLE
    </span>
  </div>
  {/* Panel content */}
</div>
```

### LIVE Badge
```tsx
<div style={{ display: "flex", alignItems: "center", gap: 6, background: "rgba(239,68,68,0.15)", border: "1px solid rgba(239,68,68,0.35)", borderRadius: 4, padding: "2px 8px" }}>
  <div style={{ width: 6, height: 6, borderRadius: "50%", background: "#EF4444", animation: "pulse 2s infinite" }} />
  <span style={{ fontSize: 10, fontWeight: 700, color: "#EF4444", letterSpacing: "0.1em" }}>LIVE</span>
</div>
```

### Status Indicator (inline)
```tsx
// Green = good, Red = bad, Amber = warning
<div style={{ display: "flex", alignItems: "center", gap: 6 }}>
  <div style={{ width: 6, height: 6, borderRadius: "50%", background: "#22C55E" }} />
  <span style={{ fontSize: 10, color: "rgba(255,255,255,0.4)", fontFamily: "'JetBrains Mono', monospace" }}>
    SIG OK
  </span>
</div>
```

### Instrumentation Strip (below canvas/chart)
```tsx
<div style={{ display: "flex", justifyContent: "space-between", padding: "6px 12px", borderTop: "1px solid rgba(255,255,255,0.05)", background: "#0A0B0F", fontFamily: "'JetBrains Mono', monospace", fontSize: 9 }}>
  <div style={{ display: "flex", gap: 12 }}>
    <span style={{ color: "rgba(255,255,255,0.25)" }}>TC</span>
    <span style={{ color: "#06B6D4" }}>01:23:48:01</span>
    <span style={{ color: "rgba(255,255,255,0.1)" }}>|</span>
    <span style={{ color: "#22C55E" }}>SRC ACTIVE</span>
  </div>
  <div style={{ display: "flex", gap: 12 }}>
    <span style={{ color: "rgba(255,255,255,0.25)" }}>BITRATE</span>
    <span style={{ color: "#22C55E", fontWeight: 600 }}>8500 kbps</span>
  </div>
</div>
```

### Primary Button
```tsx
<button style={{
  background: "#3B82F6",
  color: "#fff",
  border: "none",
  borderRadius: 6,
  padding: "8px 16px",
  fontSize: 12,
  fontWeight: 600,
  fontFamily: "'Inter', sans-serif",
  cursor: "pointer",
  transition: "transform 0.1s ease, opacity 0.1s ease",
  // On :active: transform: scale(0.97)
}}>
  Save Settings
</button>
```

### Danger Button (End Stream)
```tsx
<button style={{
  background: "#EF4444",
  color: "#fff",
  width: "100%",
  padding: "10px",
  borderRadius: 6,
  fontSize: 13,
  fontWeight: 700,
  letterSpacing: "0.04em",
  border: "none",
  cursor: "pointer",
}}>
  ■ END STREAM
</button>
```

### Toggle Switch
```tsx
// Width: 36px, Height: 20px
// On: background #3B82F6, thumb left: 18px
// Off: background rgba(255,255,255,0.15), thumb left: 2px
// Thumb: 16×16px white circle, transition: left 0.2s ease
```

### Text Input
```tsx
<input style={{
  background: "#0E0F14",
  border: "1px solid rgba(255,255,255,0.12)",
  borderRadius: 6,
  color: "#fff",
  fontSize: 12,
  fontFamily: "'Inter', sans-serif",
  padding: "6px 12px",
  outline: "none",
  // On focus: border-color: rgba(59,130,246,0.5)
}} />
```

### VU Meter (Audio)
```tsx
// 20 bars, each 3px wide, 1px gap
// Heights: 4px + (i % 3)px — slight variation for organic feel
// Color thresholds: bars 0-13 = #22C55E, 14-16 = #F59E0B, 17-19 = #EF4444
// Inactive bar: rgba(255,255,255,0.1)
// Update at 60fps via requestAnimationFrame or setInterval(16ms)
```

### Health/Progress Bar
```tsx
// Container: height 4px, background rgba(255,255,255,0.08), border-radius 9999
// Fill: width = value%, color = green/amber/red based on threshold, transition: width 0.5s ease
```

---

## 7. Animation & Motion

```css
/* LIVE pulse — used on all red dot indicators */
@keyframes pulse {
  0%, 100% { opacity: 1; transform: scale(1); }
  50% { opacity: 0.5; transform: scale(1.3); }
}

/* Standard easing for all UI transitions */
--ease-out: cubic-bezier(0.23, 1, 0.32, 1);

/* Timing guidelines */
button hover/active:   100–160ms ease-out
nav item hover:        120ms ease-out
panel entrance:        150ms opacity + translateY(4px)
modal/drawer:          200–300ms ease-out
chart draw-in:         600ms ease-out
bitrate/health bars:   500ms ease transition on width
VU meters:             50ms (near-instant, feels live)
```

**Button press feedback (required on all interactive buttons):**
```css
button:active { transform: scale(0.97); transition: transform 0.1s ease-out; }
```

---

## 8. Canvas / Workspace Instrumentation Rules

Any large empty workspace (program output preview, scene editor canvas, chart area) **must** include:

1. **SVG grid overlay** — subtle `rgba(255,255,255,0.04–0.06)` grid pattern
2. **Corner bracket markers** — 4 L-shaped brackets in `rgba(59,130,246,0.6)` at corners
3. **Metadata strip** — bottom bar with resolution, scene name, status in JetBrains Mono
4. **Center placeholder** — icon + label when no content is loaded
5. **Crosshair** (scene editor only) — thin cross at canvas center in `rgba(59,130,246,0.25)`

---

## 9. Spacing & Radius

```
Panel padding:        12px (p-3)
Section gap:          12px (gap-3)
Inner element gap:    8px (gap-2)
Border radius:        8px (rounded-lg) for panels
                      6px (rounded-md) for buttons/inputs
                      4px (rounded) for badges/chips
                      9999px for toggles/pills
Top bar height:       46px minimum
Sidebar width:        220px (fixed, never collapses on desktop)
Right panel width:    280–300px (fixed)
```

---

## 10. Do's and Don'ts

### Do
- Use `#0E0F14` as the root background — never pure `#000000`
- Use `JetBrains Mono` for all numeric readouts, timecodes, and technical values
- Keep the LIVE badge visible on every screen while streaming
- Add instrumentation strips to any large empty area
- Use `rgba()` for borders — never opaque colored borders on dark surfaces
- Keep Nexus Blue `#3B82F6` as the **only** primary interaction color
- Use `Space Grotesk` for page titles and KPI numbers
- Make all top bars exactly 46px tall with `background: #0D0E12`

### Don't
- Don't use purple gradients, rounded hero sections, or centered landing-page layouts
- Don't use `Inter` for timecodes or numeric data — always `JetBrains Mono`
- Don't make panels float freely — they should feel anchored to a grid
- Don't use platform colors (Twitch purple, YouTube red) as dominant panel colors
- Don't add decorative illustrations or icons that don't serve a functional purpose
- Don't use `border-radius > 8px` on panels — keep corners sharp and professional
- Don't animate layout properties (`width`, `height`, `margin`) — only `transform` and `opacity`
- Don't use white text on anything brighter than `#1A1D2B` — contrast will fail
