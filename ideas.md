# Nexus Broadcast — Design Ideas

## Approaches Considered

### 1. Obsidian Studio (Probability: 0.04)
Ultra-dark, near-black surfaces with electric blue and cyan accents. Inspired by professional broadcast hardware (ATEM, Blackmagic). Dense information layout, hardware-panel aesthetic.

### 2. Neon Forge (Probability: 0.07)
Deep charcoal with vivid neon highlights (magenta, electric blue). Gaming-adjacent but professional. Glow effects, sharp corners, high-contrast typography.

### 3. Carbon Command (Probability: 0.02)
Matte carbon texture backgrounds, amber/orange accent color, military-grade precision aesthetic. Monospace typography, tight grid, industrial feel.

---

## Chosen Approach: Obsidian Studio

### Design Movement
Professional Broadcast Hardware UI — inspired by Blackmagic ATEM, Ross Video, and high-end NLE software (DaVinci Resolve). Dark, dense, purposeful.

### Core Principles
1. **Information density without clutter** — every pixel earns its place; no decorative whitespace
2. **Hierarchy through luminosity** — brighter = more important; dim = secondary/inactive
3. **Precision over decoration** — functional beauty; no gradients unless they carry meaning
4. **Status at a glance** — critical states (LIVE, errors, health) are always visible

### Color Philosophy
- **Background**: `#0E0F14` — near-black with a blue undertone (not pure black, avoids harshness)
- **Panel surfaces**: `#16181F` — slightly lifted, creates depth without contrast shock
- **Borders**: `rgba(255,255,255,0.08)` — whisper-thin, defines structure without weight
- **Primary accent**: `#3B82F6` (electric blue) — selection, active states, CTAs
- **Live/danger**: `#EF4444` (red) — LIVE badges, end stream, critical alerts
- **Success/health**: `#22C55E` (green) — good signal, connected, healthy
- **Warning**: `#F59E0B` (amber) — degraded performance, caution
- **Cyan highlight**: `#06B6D4` — secondary data, chart lines, subtle accents

### Layout Paradigm
Persistent left sidebar (icon + label, 220px wide) + main content area. No top navigation bar. Content areas use asymmetric multi-column layouts (not centered grids). Panels have defined roles — never free-floating cards.

### Signature Elements
1. **Pulsing LIVE badge** — red dot with CSS pulse animation, always visible when streaming
2. **VU meter bars** — animated green-to-red audio level indicators in the audio mixer
3. **Status indicator strip** — thin colored left-border on active/selected items

### Interaction Philosophy
Snappy, sub-200ms transitions. Hover states reveal secondary actions. Active states use blue left-border + subtle background lift. No page transitions — instant panel switching for broadcast-speed workflow.

### Animation
- Sidebar item hover: 120ms ease-out background fade
- Panel entrance: 150ms opacity + 4px translateY from below
- LIVE badge: 2s infinite pulse (scale 1→1.4, opacity 1→0)
- VU meters: 60fps CSS animation, smooth level tracking
- Chart lines: 600ms ease-out draw-in on mount
- Button press: scale(0.97) 100ms ease-out

### Typography System
- **Display/headings**: `Space Grotesk` — geometric, technical, distinctive
- **UI labels/body**: `Inter` — neutral, readable at small sizes
- **Monospace (timecodes, values)**: `JetBrains Mono` — precise, scannable
- Scale: 11px (micro labels) → 13px (body) → 15px (section headers) → 20px (KPI values) → 28px (hero numbers)

### Brand Essence
**Nexus Broadcast** — professional streaming control for creators who mean business. Bold, precise, reliable.
Personality: **Authoritative · Precise · Empowering**

### Brand Voice
Headlines are declarative and action-oriented: *"Your stream. Your control."* / *"Go live in seconds."*
No filler copy. Every label is a verb or a noun — never a sentence.

### Wordmark & Logo
Broadcast tower icon (signal waves radiating upward) paired with "NEXUS" in Space Grotesk Bold + "BROADCAST" in Inter Medium, letter-spaced. Icon in electric blue, text in white.

### Signature Brand Color
Electric Blue `#3B82F6` — unmistakably Nexus.

## Style Decisions
- All panels use `#16181F` background, not card component defaults
- Sidebar width: 220px, always visible (no collapse on desktop)
- Section headers: 11px uppercase letter-spaced labels in muted color
- Active nav item: blue left border (3px) + `#1E2130` background
- All numeric readouts use JetBrains Mono
