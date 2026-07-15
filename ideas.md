# RailShotTV — Design Brief

## Chosen Approach: Chromatic Command

RailShotTV is a billiard-first livestreaming app for Windows. The design must feel like a premium sports broadcast control surface — not a generic dark SaaS tool. Every color carries meaning. Every panel has a role.

### Design Movement
**Precision Broadcast Product Design** — the intersection of professional broadcast hardware UI (ATEM Mini, DaVinci Resolve) and modern consumer software product design (Linear, Raycast). Dark, dense, colorful, purposeful.

### Core Principles
1. **Color as function** — Every accent color maps to a domain: orange-red=LIVE/streaming, blue=video/preview, violet=audio, emerald=health/signal, amber=warnings.
2. **Density with breathing room** — Information-dense panels separated by deliberate negative space.
3. **Broadcast hardware language** — Corner brackets, monospace readouts, status strips, VU bars.
4. **Sport energy** — The energy of a live sports broadcast: urgency, precision, performance.

### Color Philosophy
- **Base**: `#0A0A0F` (deepest bg), `#111118` (panel bg), `#1A1A24` (elevated card), `#242432` (control surface)
- **Borders**: `#2A2A3A` (default), `#3A3A50` (elevated), `#4A4A6A` (active)
- **Brand / LIVE**: `#FF4D1C` — hot orange-red. The signature RailShotTV color.
- **Blue**: `#3B82F6` / `#60A5FA` — video, preview, scene selection
- **Violet**: `#8B5CF6` / `#A78BFA` — audio, mixer, VU meters
- **Emerald**: `#10B981` / `#34D399` — health, connection, good states
- **Amber**: `#F59E0B` — warnings, caution
- **Crimson**: `#EF4444` — errors, dropped frames
- **Cyan**: `#06B6D4` — analytics, timecodes, data
- **Text**: `#F8F8FF` (primary), `#A0A0B8` (secondary), `#606078` (muted)

### Layout Paradigm
**Asymmetric multi-rail.** Left rail: icon-only nav (56px) with color-coded active glows. Main: full-bleed panels. Right: contextual status/properties. Top: global status strip. Bottom: screen-specific controls.

### Signature Elements
1. **The Rail** — Icon-only left nav, colored glows per section (Dashboard=orange-red, Audio=violet, Analytics=cyan).
2. **Chromatic panel headers** — 2px left border in domain color + subtle gradient wash into panel bg.
3. **Live State System** — Streaming active: 4px orange-red top border full-width, pulsing LIVE badge, END STREAM button.

### Typography System
- **Display**: `Bebas Neue` — condensed, athletic. KPI numbers, scene names, wordmark.
- **UI / Body**: `DM Sans` — geometric, clean. Labels, panel headers, body text.
- **Monospace**: `JetBrains Mono` — timecodes, bitrate, numeric data.

### Brand Essence
**RailShotTV — the broadcast tool built for billiards.** Precision, speed, sport.
Personality: **Focused. Athletic. Professional.**

### Wordmark & Logo
Stylized cue ball — circle with diagonal speed line. Wordmark: **RAILSHOT** in Bebas Neue white + **TV** in `#FF4D1C`.

## Style Decisions
- Left nav: icon-only, 56px, colored glows per section
- Panel headers: 2px left accent border + gradient wash
- All numeric readouts: JetBrains Mono
- LIVE state: orange-red top border, pulsing badge, transformed button
- Background: `#0A0A0F` — deep, high contrast, dramatic
