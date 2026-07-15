// RailShotTV — Chromatic Command — Scoreboard Overlay Editor
// ScoreboardPage: configurable event scoreboard with live overlay preview
import React, { useState, useEffect, useRef, useCallback } from "react";
import AppSidebar from "@/components/AppSidebar";
import { toast } from "sonner";
import {
  Trophy, Play, Pause, RotateCcw, Eye, EyeOff, Plus, Minus,
  ChevronDown, Save, Upload, Layers, Monitor, AlignCenter,
  AlignLeft, LayoutGrid, Palette, Timer, Users
} from "lucide-react";

// ─── Types ────────────────────────────────────────────────────────────────────
type SportPreset = "generic" | "pool" | "basketball" | "soccer" | "tennis" | "custom";
type LayoutStyle = "lower-third" | "center-banner" | "corner-compact" | "full-width";
type ColorTheme = "dark" | "light" | "team" | "neon" | "minimal";

interface TeamState {
  name: string;
  score: number;
  color: string;
  logo: string; // initials fallback
}

interface ScoreboardState {
  sport: SportPreset;
  layout: LayoutStyle;
  theme: ColorTheme;
  teamA: TeamState;
  teamB: TeamState;
  period: string;
  periodLabel: string;
  timerSeconds: number;
  timerRunning: boolean;
  timerDirection: "up" | "down";
  eventTitle: string;
  visible: boolean;
}

// ─── Sport Presets ────────────────────────────────────────────────────────────
const SPORT_PRESETS: Record<SportPreset, { label: string; periodLabel: string; scoreLabel: string; emoji: string }> = {
  generic:    { label: "Generic",     periodLabel: "Period",  scoreLabel: "Score",  emoji: "🏆" },
  pool:       { label: "Pool / Billiards", periodLabel: "Rack", scoreLabel: "Racks", emoji: "🎱" },
  basketball: { label: "Basketball",  periodLabel: "Quarter", scoreLabel: "Points", emoji: "🏀" },
  soccer:     { label: "Soccer",      periodLabel: "Half",    scoreLabel: "Goals",  emoji: "⚽" },
  tennis:     { label: "Tennis",      periodLabel: "Set",     scoreLabel: "Games",  emoji: "🎾" },
  custom:     { label: "Custom",      periodLabel: "Period",  scoreLabel: "Score",  emoji: "⚡" },
};

const LAYOUT_OPTIONS: { id: LayoutStyle; label: string; icon: React.FC<any> }[] = [
  { id: "lower-third",   label: "Lower Third",    icon: AlignLeft },
  { id: "center-banner", label: "Center Banner",  icon: AlignCenter },
  { id: "corner-compact",label: "Corner Compact", icon: LayoutGrid },
  { id: "full-width",    label: "Full Width",     icon: Monitor },
];

const COLOR_THEMES: { id: ColorTheme; label: string; bg: string; accent: string }[] = [
  { id: "dark",    label: "Dark",    bg: "rgba(10,12,20,0.92)",  accent: "#4F9EFF" },
  { id: "light",   label: "Light",   bg: "rgba(240,244,255,0.95)", accent: "#1E40AF" },
  { id: "team",    label: "Team",    bg: "rgba(20,10,40,0.92)",  accent: "#A855F7" },
  { id: "neon",    label: "Neon",    bg: "rgba(0,8,24,0.95)",    accent: "#22D3EE" },
  { id: "minimal", label: "Minimal", bg: "rgba(15,15,15,0.88)",  accent: "#FF5A2C" },
];

const TEAM_COLORS = ["#FF5A2C","#4F9EFF","#A855F7","#22C55E","#22D3EE","#F59E0B","#EF4444","#EC4899","#FFFFFF","#94A3B8"];

// ─── Overlay Preview Component ────────────────────────────────────────────────
function OverlayPreview({ state }: { state: ScoreboardState }) {
  const theme = COLOR_THEMES.find(t => t.id === state.theme) || COLOR_THEMES[0];
  const preset = SPORT_PRESETS[state.sport];
  const isLight = state.theme === "light";
  const textColor = isLight ? "#0F172A" : "#F1F5F9";
  const subColor = isLight ? "#475569" : "#94A3B8";

  const formatTime = (s: number) => {
    const m = Math.floor(s / 60);
    const sec = s % 60;
    return `${m}:${sec.toString().padStart(2, "0")}`;
  };

  if (!state.visible) {
    return (
      <div style={{
        width: "100%", height: "100%", display: "flex", alignItems: "center", justifyContent: "center",
        background: "repeating-linear-gradient(45deg, #0D1117 0px, #0D1117 10px, #111827 10px, #111827 20px)",
      }}>
        <div style={{ textAlign: "center", color: "#374151" }}>
          <EyeOff size={32} style={{ margin: "0 auto 8px" }} />
          <div style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 12 }}>OVERLAY HIDDEN</div>
        </div>
      </div>
    );
  }

  // Lower Third layout
  if (state.layout === "lower-third") {
    return (
      <div style={{ width: "100%", height: "100%", position: "relative", background: "#0a0f1a", overflow: "hidden" }}>
        {/* Simulated video background */}
        <div style={{
          position: "absolute", inset: 0,
          background: "linear-gradient(135deg, #0f1923 0%, #1a2535 40%, #0d1520 100%)",
        }}>
          <div style={{ position: "absolute", inset: 0, opacity: 0.15,
            backgroundImage: "repeating-linear-gradient(0deg, transparent, transparent 40px, rgba(255,255,255,0.03) 40px, rgba(255,255,255,0.03) 41px), repeating-linear-gradient(90deg, transparent, transparent 40px, rgba(255,255,255,0.03) 40px, rgba(255,255,255,0.03) 41px)"
          }} />
        </div>
        {/* Lower third scoreboard */}
        <div style={{
          position: "absolute", bottom: "12%", left: "50%", transform: "translateX(-50%)",
          width: "80%", minWidth: 320,
        }}>
          {/* Event title bar */}
          {state.eventTitle && (
            <div style={{
              background: theme.accent, color: "#fff",
              fontFamily: "'Bebas Neue', sans-serif", fontSize: 11, letterSpacing: "0.15em",
              padding: "3px 12px", display: "inline-block", marginBottom: 2,
              clipPath: "polygon(0 0, calc(100% - 8px) 0, 100% 100%, 0 100%)",
            }}>{state.eventTitle.toUpperCase()}</div>
          )}
          {/* Main score bar */}
          <div style={{
            background: theme.bg,
            backdropFilter: "blur(12px)",
            border: `1px solid ${theme.accent}33`,
            borderTop: `2px solid ${theme.accent}`,
            display: "flex", alignItems: "stretch",
            borderRadius: "0 0 4px 4px",
            overflow: "hidden",
            boxShadow: `0 4px 24px rgba(0,0,0,0.6), 0 0 0 1px ${theme.accent}22`,
          }}>
            {/* Team A */}
            <div style={{ flex: 1, display: "flex", alignItems: "center", gap: 8, padding: "8px 14px" }}>
              <div style={{
                width: 28, height: 28, borderRadius: 4,
                background: state.teamA.color, display: "flex", alignItems: "center", justifyContent: "center",
                fontFamily: "'Bebas Neue', sans-serif", fontSize: 13, color: "#fff", flexShrink: 0,
              }}>{state.teamA.logo}</div>
              <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 700, fontSize: 14, color: textColor, letterSpacing: "-0.01em" }}>
                {state.teamA.name || "Team A"}
              </span>
            </div>
            {/* Score */}
            <div style={{
              display: "flex", alignItems: "center", gap: 0,
              background: "rgba(0,0,0,0.3)", borderLeft: `1px solid ${theme.accent}22`, borderRight: `1px solid ${theme.accent}22`,
            }}>
              <div style={{ padding: "0 16px", textAlign: "center" }}>
                <div style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 28, color: state.teamA.color, lineHeight: 1 }}>
                  {state.teamA.score}
                </div>
              </div>
              <div style={{ display: "flex", flexDirection: "column", alignItems: "center", padding: "0 8px", gap: 2 }}>
                <div style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 9, color: subColor, letterSpacing: "0.1em" }}>
                  {preset.periodLabel.toUpperCase()} {state.period}
                </div>
                <div style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 11, color: theme.accent }}>
                  {formatTime(state.timerSeconds)}
                </div>
              </div>
              <div style={{ padding: "0 16px", textAlign: "center" }}>
                <div style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 28, color: state.teamB.color, lineHeight: 1 }}>
                  {state.teamB.score}
                </div>
              </div>
            </div>
            {/* Team B */}
            <div style={{ flex: 1, display: "flex", alignItems: "center", gap: 8, padding: "8px 14px", justifyContent: "flex-end" }}>
              <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 700, fontSize: 14, color: textColor, letterSpacing: "-0.01em" }}>
                {state.teamB.name || "Team B"}
              </span>
              <div style={{
                width: 28, height: 28, borderRadius: 4,
                background: state.teamB.color, display: "flex", alignItems: "center", justifyContent: "center",
                fontFamily: "'Bebas Neue', sans-serif", fontSize: 13, color: "#fff", flexShrink: 0,
              }}>{state.teamB.logo}</div>
            </div>
          </div>
        </div>
      </div>
    );
  }

  // Center Banner layout
  if (state.layout === "center-banner") {
    return (
      <div style={{ width: "100%", height: "100%", position: "relative", background: "#0a0f1a", overflow: "hidden", display: "flex", alignItems: "center", justifyContent: "center" }}>
        <div style={{ position: "absolute", inset: 0, background: "linear-gradient(135deg, #0f1923 0%, #1a2535 40%, #0d1520 100%)" }} />
        <div style={{
          position: "relative", zIndex: 2,
          background: theme.bg, backdropFilter: "blur(16px)",
          border: `1px solid ${theme.accent}44`,
          borderRadius: 8, padding: "16px 28px",
          boxShadow: `0 8px 40px rgba(0,0,0,0.7), 0 0 0 1px ${theme.accent}22, 0 0 60px ${theme.accent}11`,
          minWidth: 280, textAlign: "center",
        }}>
          {state.eventTitle && (
            <div style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 11, color: theme.accent, letterSpacing: "0.2em", marginBottom: 10 }}>
              {state.eventTitle.toUpperCase()}
            </div>
          )}
          <div style={{ display: "flex", alignItems: "center", gap: 20, justifyContent: "center" }}>
            <div style={{ textAlign: "center" }}>
              <div style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 700, fontSize: 13, color: textColor, marginBottom: 4 }}>{state.teamA.name || "Team A"}</div>
              <div style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 40, color: state.teamA.color, lineHeight: 1 }}>{state.teamA.score}</div>
            </div>
            <div style={{ textAlign: "center" }}>
              <div style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 10, color: subColor, marginBottom: 4 }}>{preset.periodLabel.toUpperCase()} {state.period}</div>
              <div style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 20, color: theme.accent }}>VS</div>
              <div style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 12, color: theme.accent, marginTop: 4 }}>{formatTime(state.timerSeconds)}</div>
            </div>
            <div style={{ textAlign: "center" }}>
              <div style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 700, fontSize: 13, color: textColor, marginBottom: 4 }}>{state.teamB.name || "Team B"}</div>
              <div style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 40, color: state.teamB.color, lineHeight: 1 }}>{state.teamB.score}</div>
            </div>
          </div>
        </div>
      </div>
    );
  }

  // Corner Compact layout
  if (state.layout === "corner-compact") {
    return (
      <div style={{ width: "100%", height: "100%", position: "relative", background: "#0a0f1a", overflow: "hidden" }}>
        <div style={{ position: "absolute", inset: 0, background: "linear-gradient(135deg, #0f1923 0%, #1a2535 40%, #0d1520 100%)" }} />
        <div style={{
          position: "absolute", top: 12, left: 12, zIndex: 2,
          background: theme.bg, backdropFilter: "blur(12px)",
          border: `1px solid ${theme.accent}44`, borderLeft: `3px solid ${theme.accent}`,
          borderRadius: "0 6px 6px 0", padding: "8px 12px",
          boxShadow: `0 4px 20px rgba(0,0,0,0.6)`,
          minWidth: 120,
        }}>
          <div style={{ display: "flex", flexDirection: "column", gap: 4 }}>
            <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center", gap: 12 }}>
              <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 700, fontSize: 11, color: textColor }}>{state.teamA.name || "A"}</span>
              <span style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 18, color: state.teamA.color, lineHeight: 1 }}>{state.teamA.score}</span>
            </div>
            <div style={{ height: 1, background: `${theme.accent}33` }} />
            <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center", gap: 12 }}>
              <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 700, fontSize: 11, color: textColor }}>{state.teamB.name || "B"}</span>
              <span style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 18, color: state.teamB.color, lineHeight: 1 }}>{state.teamB.score}</span>
            </div>
            <div style={{ display: "flex", justifyContent: "space-between", marginTop: 2 }}>
              <span style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 8, color: subColor }}>{preset.periodLabel.toUpperCase()} {state.period}</span>
              <span style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 8, color: theme.accent }}>{formatTime(state.timerSeconds)}</span>
            </div>
          </div>
        </div>
      </div>
    );
  }

  // Full Width layout
  return (
    <div style={{ width: "100%", height: "100%", position: "relative", background: "#0a0f1a", overflow: "hidden" }}>
      <div style={{ position: "absolute", inset: 0, background: "linear-gradient(135deg, #0f1923 0%, #1a2535 40%, #0d1520 100%)" }} />
      <div style={{
        position: "absolute", top: 0, left: 0, right: 0, zIndex: 2,
        background: theme.bg, backdropFilter: "blur(12px)",
        borderBottom: `2px solid ${theme.accent}`,
        padding: "8px 20px",
        display: "flex", alignItems: "center", justifyContent: "space-between",
      }}>
        <div style={{ display: "flex", alignItems: "center", gap: 10 }}>
          <div style={{ width: 24, height: 24, borderRadius: 3, background: state.teamA.color, display: "flex", alignItems: "center", justifyContent: "center", fontFamily: "'Bebas Neue', sans-serif", fontSize: 11, color: "#fff" }}>{state.teamA.logo}</div>
          <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 700, fontSize: 13, color: textColor }}>{state.teamA.name || "Team A"}</span>
          <span style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 22, color: state.teamA.color, marginLeft: 8 }}>{state.teamA.score}</span>
        </div>
        <div style={{ textAlign: "center" }}>
          {state.eventTitle && <div style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 10, color: theme.accent, letterSpacing: "0.15em" }}>{state.eventTitle.toUpperCase()}</div>}
          <div style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 11, color: subColor }}>{preset.periodLabel.toUpperCase()} {state.period} · {formatTime(state.timerSeconds)}</div>
        </div>
        <div style={{ display: "flex", alignItems: "center", gap: 10 }}>
          <span style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 22, color: state.teamB.color, marginRight: 8 }}>{state.teamB.score}</span>
          <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 700, fontSize: 13, color: textColor }}>{state.teamB.name || "Team B"}</span>
          <div style={{ width: 24, height: 24, borderRadius: 3, background: state.teamB.color, display: "flex", alignItems: "center", justifyContent: "center", fontFamily: "'Bebas Neue', sans-serif", fontSize: 11, color: "#fff" }}>{state.teamB.logo}</div>
        </div>
      </div>
    </div>
  );
}

// ─── Score Button ─────────────────────────────────────────────────────────────
function ScoreBtn({ onClick, children, color }: { onClick: () => void; children: React.ReactNode; color?: string }) {
  return (
    <button onClick={onClick} style={{
      width: 36, height: 36, borderRadius: 6, border: `1px solid ${color || "#4F9EFF"}44`,
      background: `${color || "#4F9EFF"}18`, color: color || "#4F9EFF",
      display: "flex", alignItems: "center", justifyContent: "center",
      cursor: "pointer", transition: "all 0.15s", fontWeight: 700, fontSize: 16,
    }}
      onMouseEnter={e => { (e.currentTarget as HTMLButtonElement).style.background = `${color || "#4F9EFF"}35`; }}
      onMouseLeave={e => { (e.currentTarget as HTMLButtonElement).style.background = `${color || "#4F9EFF"}18`; }}
    >{children}</button>
  );
}

// ─── Section Header ───────────────────────────────────────────────────────────
function SectionHeader({ icon: Icon, label, color = "#4F9EFF" }: { icon: React.FC<any>; label: string; color?: string }) {
  return (
    <div style={{ display: "flex", alignItems: "center", gap: 7, marginBottom: 10 }}>
      <Icon size={13} color={color} />
      <span style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 12, color, letterSpacing: "0.12em" }}>{label}</span>
      <div style={{ flex: 1, height: 1, background: `${color}33` }} />
    </div>
  );
}

// ─── Main Page ────────────────────────────────────────────────────────────────
export default function ScoreboardPage() {
  const [state, setState] = useState<ScoreboardState>({
    sport: "generic",
    layout: "lower-third",
    theme: "dark",
    teamA: { name: "", score: 0, color: "#FF5A2C", logo: "A" },
    teamB: { name: "", score: 0, color: "#4F9EFF", logo: "B" },
    period: "1",
    periodLabel: "Period",
    timerSeconds: 0,
    timerRunning: false,
    timerDirection: "up",
    eventTitle: "",
    visible: true,
  });

  const timerRef = useRef<ReturnType<typeof setInterval> | null>(null);

  // Timer logic
  useEffect(() => {
    if (state.timerRunning) {
      timerRef.current = setInterval(() => {
        setState(prev => ({
          ...prev,
          timerSeconds: prev.timerDirection === "up"
            ? prev.timerSeconds + 1
            : Math.max(0, prev.timerSeconds - 1),
        }));
      }, 1000);
    } else {
      if (timerRef.current) clearInterval(timerRef.current);
    }
    return () => { if (timerRef.current) clearInterval(timerRef.current); };
  }, [state.timerRunning, state.timerDirection]);

  // Update period label when sport changes
  useEffect(() => {
    setState(prev => ({ ...prev, periodLabel: SPORT_PRESETS[prev.sport].periodLabel }));
  }, [state.sport]);

  const update = useCallback((patch: Partial<ScoreboardState>) => setState(prev => ({ ...prev, ...patch })), []);
  const updateTeamA = useCallback((patch: Partial<TeamState>) => setState(prev => ({ ...prev, teamA: { ...prev.teamA, ...patch } })), []);
  const updateTeamB = useCallback((patch: Partial<TeamState>) => setState(prev => ({ ...prev, teamB: { ...prev.teamB, ...patch } })), []);

  const formatTime = (s: number) => `${Math.floor(s / 60)}:${(s % 60).toString().padStart(2, "0")}`;

  const panelStyle: React.CSSProperties = {
    background: "#161B2E", border: "1px solid rgba(255,255,255,0.08)",
    borderRadius: 8, padding: "14px 16px",
  };

  const inputStyle: React.CSSProperties = {
    width: "100%", background: "#0D1117", border: "1px solid rgba(255,255,255,0.1)",
    borderRadius: 6, padding: "7px 10px", color: "#E2E8F0",
    fontFamily: "'DM Sans', sans-serif", fontSize: 13, outline: "none",
    boxSizing: "border-box",
  };

  return (
    <AppSidebar>
      <div style={{ display: "flex", flexDirection: "column", height: "100vh", background: "#0D1117", overflow: "hidden" }}>
        {/* Top bar */}
        <div style={{
          height: 46, minHeight: 46, display: "flex", alignItems: "center", justifyContent: "space-between",
          padding: "0 16px", background: "#111827",
          borderBottom: "1px solid rgba(255,255,255,0.08)",
          borderTop: "2px solid #22C55E",
        }}>
          <div style={{ display: "flex", alignItems: "center", gap: 12 }}>
            <span style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 18, color: "#FF5A2C", letterSpacing: "0.08em" }}>RAILSHOT</span>
            <span style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 18, color: "#E2E8F0", letterSpacing: "0.08em" }}>TV</span>
            <div style={{ width: 1, height: 16, background: "rgba(255,255,255,0.15)", margin: "0 4px" }} />
            <span style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 13, color: "#22C55E", letterSpacing: "0.12em" }}>SCOREBOARD</span>
          </div>
          <div style={{ display: "flex", alignItems: "center", gap: 8 }}>
            {/* Visibility toggle */}
            <button onClick={() => update({ visible: !state.visible })} style={{
              display: "flex", alignItems: "center", gap: 6, padding: "5px 12px",
              background: state.visible ? "rgba(34,197,94,0.15)" : "rgba(107,114,128,0.15)",
              border: `1px solid ${state.visible ? "rgba(34,197,94,0.4)" : "rgba(107,114,128,0.3)"}`,
              borderRadius: 6, color: state.visible ? "#22C55E" : "#6B7280",
              fontFamily: "'DM Sans', sans-serif", fontSize: 12, fontWeight: 600, cursor: "pointer",
            }}>
              {state.visible ? <Eye size={13} /> : <EyeOff size={13} />}
              {state.visible ? "OVERLAY ON" : "OVERLAY OFF"}
            </button>
            <button onClick={() => { toast.success("Scoreboard preset saved"); }} style={{
              display: "flex", alignItems: "center", gap: 6, padding: "5px 12px",
              background: "rgba(79,158,255,0.12)", border: "1px solid rgba(79,158,255,0.3)",
              borderRadius: 6, color: "#4F9EFF", fontFamily: "'DM Sans', sans-serif", fontSize: 12, fontWeight: 600, cursor: "pointer",
            }}>
              <Save size={13} /> SAVE PRESET
            </button>
            <button onClick={() => { toast.info("Load preset — coming in Phase 4C"); }} style={{
              display: "flex", alignItems: "center", gap: 6, padding: "5px 12px",
              background: "rgba(168,85,247,0.12)", border: "1px solid rgba(168,85,247,0.3)",
              borderRadius: 6, color: "#A855F7", fontFamily: "'DM Sans', sans-serif", fontSize: 12, fontWeight: 600, cursor: "pointer",
            }}>
              <Upload size={13} /> LOAD PRESET
            </button>
          </div>
        </div>

        {/* Main layout: left controls | center preview | right style */}
        <div style={{ flex: 1, display: "flex", overflow: "hidden", gap: 0 }}>

          {/* ── Left panel: Score Controls ─────────────────────────────────── */}
          <div style={{ width: 280, minWidth: 280, borderRight: "1px solid rgba(255,255,255,0.07)", overflowY: "auto", padding: 14, display: "flex", flexDirection: "column", gap: 12 }}>

            {/* Sport preset */}
            <div style={panelStyle}>
              <SectionHeader icon={Trophy} label="SPORT PRESET" color="#F59E0B" />
              <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr", gap: 6 }}>
                {(Object.keys(SPORT_PRESETS) as SportPreset[]).map(key => {
                  const p = SPORT_PRESETS[key];
                  const isActive = state.sport === key;
                  return (
                    <button key={key} onClick={() => update({ sport: key })} style={{
                      padding: "7px 8px", borderRadius: 6, cursor: "pointer",
                      background: isActive ? "rgba(245,158,11,0.18)" : "rgba(255,255,255,0.04)",
                      border: `1px solid ${isActive ? "rgba(245,158,11,0.5)" : "rgba(255,255,255,0.08)"}`,
                      color: isActive ? "#F59E0B" : "#9CA3AF",
                      fontFamily: "'DM Sans', sans-serif", fontSize: 11, fontWeight: 600,
                      display: "flex", alignItems: "center", gap: 5,
                      transition: "all 0.15s",
                    }}>
                      <span>{p.emoji}</span> {p.label}
                    </button>
                  );
                })}
              </div>
            </div>

            {/* Team A */}
            <div style={panelStyle}>
              <SectionHeader icon={Users} label="TEAM / PLAYER A" color={state.teamA.color} />
              <div style={{ display: "flex", flexDirection: "column", gap: 8 }}>
                <input
                  style={inputStyle} value={state.teamA.name}
                  onChange={e => updateTeamA({ name: e.target.value })}
                  placeholder="Team A name"
                />
                <input
                  style={{ ...inputStyle, width: 60 }} value={state.teamA.logo}
                  onChange={e => updateTeamA({ logo: e.target.value.slice(0, 2).toUpperCase() })}
                  placeholder="AB" maxLength={2}
                />
                <div style={{ display: "flex", flexWrap: "wrap", gap: 5 }}>
                  {TEAM_COLORS.map(c => (
                    <div key={c} onClick={() => updateTeamA({ color: c })} style={{
                      width: 20, height: 20, borderRadius: 4, background: c, cursor: "pointer",
                      border: state.teamA.color === c ? "2px solid #fff" : "2px solid transparent",
                      transition: "border 0.1s",
                    }} />
                  ))}
                </div>
                {/* Score control */}
                <div style={{ display: "flex", alignItems: "center", gap: 8, marginTop: 4 }}>
                  <ScoreBtn onClick={() => updateTeamA({ score: Math.max(0, state.teamA.score - 1) })} color={state.teamA.color}><Minus size={14} /></ScoreBtn>
                  <div style={{ flex: 1, textAlign: "center", fontFamily: "'Bebas Neue', sans-serif", fontSize: 32, color: state.teamA.color, lineHeight: 1 }}>
                    {state.teamA.score}
                  </div>
                  <ScoreBtn onClick={() => updateTeamA({ score: state.teamA.score + 1 })} color={state.teamA.color}><Plus size={14} /></ScoreBtn>
                </div>
                <button onClick={() => updateTeamA({ score: 0 })} style={{
                  padding: "4px 0", background: "transparent", border: "none",
                  color: "#4B5563", fontFamily: "'DM Sans', sans-serif", fontSize: 11, cursor: "pointer",
                  textDecoration: "underline",
                }}>Reset score</button>
              </div>
            </div>

            {/* Team B */}
            <div style={panelStyle}>
              <SectionHeader icon={Users} label="TEAM / PLAYER B" color={state.teamB.color} />
              <div style={{ display: "flex", flexDirection: "column", gap: 8 }}>
                <input
                  style={inputStyle} value={state.teamB.name}
                  onChange={e => updateTeamB({ name: e.target.value })}
                  placeholder="Team B name"
                />
                <input
                  style={{ ...inputStyle, width: 60 }} value={state.teamB.logo}
                  onChange={e => updateTeamB({ logo: e.target.value.slice(0, 2).toUpperCase() })}
                  placeholder="AB" maxLength={2}
                />
                <div style={{ display: "flex", flexWrap: "wrap", gap: 5 }}>
                  {TEAM_COLORS.map(c => (
                    <div key={c} onClick={() => updateTeamB({ color: c })} style={{
                      width: 20, height: 20, borderRadius: 4, background: c, cursor: "pointer",
                      border: state.teamB.color === c ? "2px solid #fff" : "2px solid transparent",
                      transition: "border 0.1s",
                    }} />
                  ))}
                </div>
                <div style={{ display: "flex", alignItems: "center", gap: 8, marginTop: 4 }}>
                  <ScoreBtn onClick={() => updateTeamB({ score: Math.max(0, state.teamB.score - 1) })} color={state.teamB.color}><Minus size={14} /></ScoreBtn>
                  <div style={{ flex: 1, textAlign: "center", fontFamily: "'Bebas Neue', sans-serif", fontSize: 32, color: state.teamB.color, lineHeight: 1 }}>
                    {state.teamB.score}
                  </div>
                  <ScoreBtn onClick={() => updateTeamB({ score: state.teamB.score + 1 })} color={state.teamB.color}><Plus size={14} /></ScoreBtn>
                </div>
                <button onClick={() => updateTeamB({ score: 0 })} style={{
                  padding: "4px 0", background: "transparent", border: "none",
                  color: "#4B5563", fontFamily: "'DM Sans', sans-serif", fontSize: 11, cursor: "pointer",
                  textDecoration: "underline",
                }}>Reset score</button>
              </div>
            </div>
          </div>

          {/* ── Center: Overlay Preview ─────────────────────────────────────── */}
          <div style={{ flex: 1, display: "flex", flexDirection: "column", overflow: "hidden" }}>
            {/* Preview label */}
            <div style={{
              height: 32, display: "flex", alignItems: "center", justifyContent: "space-between",
              padding: "0 14px", background: "#111827", borderBottom: "1px solid rgba(255,255,255,0.07)",
            }}>
              <span style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 11, color: "#6B7280", letterSpacing: "0.12em" }}>
                OVERLAY PREVIEW — {LAYOUT_OPTIONS.find(l => l.id === state.layout)?.label.toUpperCase()}
              </span>
              <span style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 10, color: "#374151" }}>1920×1080 → SCALED</span>
            </div>
            {/* Preview canvas */}
            <div style={{ flex: 1, padding: 20, display: "flex", alignItems: "center", justifyContent: "center", background: "#0D1117" }}>
              <div style={{
                width: "100%", maxWidth: 640,
                aspectRatio: "16/9",
                borderRadius: 6, overflow: "hidden",
                border: "1px solid rgba(255,255,255,0.1)",
                boxShadow: "0 8px 40px rgba(0,0,0,0.6)",
                position: "relative",
              }}>
                <OverlayPreview state={state} />
              </div>
            </div>
            {/* Timer controls */}
            <div style={{
              height: 56, borderTop: "1px solid rgba(255,255,255,0.07)",
              background: "#111827", display: "flex", alignItems: "center",
              padding: "0 20px", gap: 12,
            }}>
              <Timer size={14} color="#22D3EE" />
              <span style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 22, color: "#22D3EE", minWidth: 60 }}>
                {formatTime(state.timerSeconds)}
              </span>
              <button onClick={() => update({ timerRunning: !state.timerRunning })} style={{
                display: "flex", alignItems: "center", gap: 6, padding: "6px 14px",
                background: state.timerRunning ? "rgba(239,68,68,0.15)" : "rgba(34,197,94,0.15)",
                border: `1px solid ${state.timerRunning ? "rgba(239,68,68,0.4)" : "rgba(34,197,94,0.4)"}`,
                borderRadius: 6, color: state.timerRunning ? "#EF4444" : "#22C55E",
                fontFamily: "'DM Sans', sans-serif", fontSize: 12, fontWeight: 600, cursor: "pointer",
              }}>
                {state.timerRunning ? <><Pause size={13} /> PAUSE</> : <><Play size={13} /> START</>}
              </button>
              <button onClick={() => update({ timerSeconds: 0, timerRunning: false })} style={{
                display: "flex", alignItems: "center", gap: 6, padding: "6px 12px",
                background: "rgba(255,255,255,0.06)", border: "1px solid rgba(255,255,255,0.1)",
                borderRadius: 6, color: "#9CA3AF",
                fontFamily: "'DM Sans', sans-serif", fontSize: 12, fontWeight: 600, cursor: "pointer",
              }}>
                <RotateCcw size={13} /> RESET
              </button>
              <div style={{ display: "flex", alignItems: "center", gap: 6, marginLeft: 8 }}>
                <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, color: "#6B7280" }}>Direction:</span>
                {(["up","down"] as const).map(d => (
                  <button key={d} onClick={() => update({ timerDirection: d })} style={{
                    padding: "4px 10px", borderRadius: 4, cursor: "pointer",
                    background: state.timerDirection === d ? "rgba(34,211,238,0.15)" : "transparent",
                    border: `1px solid ${state.timerDirection === d ? "rgba(34,211,238,0.4)" : "rgba(255,255,255,0.08)"}`,
                    color: state.timerDirection === d ? "#22D3EE" : "#6B7280",
                    fontFamily: "'DM Sans', sans-serif", fontSize: 11, fontWeight: 600,
                  }}>{d === "up" ? "COUNT UP" : "COUNT DOWN"}</button>
                ))}
              </div>
              {/* Period control */}
              <div style={{ display: "flex", alignItems: "center", gap: 6, marginLeft: "auto" }}>
                <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, color: "#6B7280" }}>{SPORT_PRESETS[state.sport].periodLabel}:</span>
                <ScoreBtn onClick={() => update({ period: String(Math.max(1, parseInt(state.period || "1") - 1)) })} color="#A855F7"><Minus size={12} /></ScoreBtn>
                <span style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 20, color: "#A855F7", minWidth: 24, textAlign: "center" }}>{state.period}</span>
                <ScoreBtn onClick={() => update({ period: String(parseInt(state.period || "1") + 1) })} color="#A855F7"><Plus size={12} /></ScoreBtn>
              </div>
            </div>
          </div>

          {/* ── Right panel: Style & Layout ─────────────────────────────────── */}
          <div style={{ width: 240, minWidth: 240, borderLeft: "1px solid rgba(255,255,255,0.07)", overflowY: "auto", padding: 14, display: "flex", flexDirection: "column", gap: 12 }}>

            {/* Event title */}
            <div style={panelStyle}>
              <SectionHeader icon={Layers} label="EVENT INFO" color="#4F9EFF" />
              <div style={{ display: "flex", flexDirection: "column", gap: 8 }}>
                <label style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, color: "#6B7280" }}>Event Title</label>
                <input style={inputStyle} value={state.eventTitle} onChange={e => update({ eventTitle: e.target.value })} placeholder="e.g. CHAMPIONSHIP FINAL" />
              </div>
            </div>

            {/* Layout style */}
            <div style={panelStyle}>
              <SectionHeader icon={Monitor} label="LAYOUT STYLE" color="#22D3EE" />
              <div style={{ display: "flex", flexDirection: "column", gap: 6 }}>
                {LAYOUT_OPTIONS.map(({ id, label, icon: Icon }) => {
                  const isActive = state.layout === id;
                  return (
                    <button key={id} onClick={() => update({ layout: id })} style={{
                      display: "flex", alignItems: "center", gap: 8, padding: "8px 10px",
                      borderRadius: 6, cursor: "pointer",
                      background: isActive ? "rgba(34,211,238,0.12)" : "rgba(255,255,255,0.03)",
                      border: `1px solid ${isActive ? "rgba(34,211,238,0.4)" : "rgba(255,255,255,0.07)"}`,
                      color: isActive ? "#22D3EE" : "#6B7280",
                      fontFamily: "'DM Sans', sans-serif", fontSize: 12, fontWeight: 600,
                      transition: "all 0.15s",
                    }}>
                      <Icon size={13} />
                      {label}
                    </button>
                  );
                })}
              </div>
            </div>

            {/* Color theme */}
            <div style={panelStyle}>
              <SectionHeader icon={Palette} label="COLOR THEME" color="#A855F7" />
              <div style={{ display: "flex", flexDirection: "column", gap: 6 }}>
                {COLOR_THEMES.map(({ id, label, bg, accent }) => {
                  const isActive = state.theme === id;
                  return (
                    <button key={id} onClick={() => update({ theme: id })} style={{
                      display: "flex", alignItems: "center", gap: 8, padding: "7px 10px",
                      borderRadius: 6, cursor: "pointer",
                      background: isActive ? "rgba(168,85,247,0.12)" : "rgba(255,255,255,0.03)",
                      border: `1px solid ${isActive ? "rgba(168,85,247,0.4)" : "rgba(255,255,255,0.07)"}`,
                      color: isActive ? "#A855F7" : "#6B7280",
                      fontFamily: "'DM Sans', sans-serif", fontSize: 12, fontWeight: 600,
                      transition: "all 0.15s",
                    }}>
                      <div style={{ width: 16, height: 16, borderRadius: 3, background: bg, border: `2px solid ${accent}`, flexShrink: 0 }} />
                      {label}
                    </button>
                  );
                })}
              </div>
            </div>

            {/* Quick reset */}
            <div style={panelStyle}>
              <SectionHeader icon={RotateCcw} label="QUICK ACTIONS" color="#EF4444" />
              <div style={{ display: "flex", flexDirection: "column", gap: 6 }}>
                <button onClick={() => { updateTeamA({ score: 0 }); updateTeamB({ score: 0 }); update({ timerSeconds: 0, timerRunning: false, period: "1" }); toast.success("Scoreboard reset"); }} style={{
                  padding: "8px 10px", borderRadius: 6, cursor: "pointer",
                  background: "rgba(239,68,68,0.1)", border: "1px solid rgba(239,68,68,0.3)",
                  color: "#EF4444", fontFamily: "'DM Sans', sans-serif", fontSize: 12, fontWeight: 600,
                }}>Reset All Scores & Timer</button>
                <button onClick={() => { update({ visible: true }); toast.success("Overlay pushed to scene"); }} style={{
                  padding: "8px 10px", borderRadius: 6, cursor: "pointer",
                  background: "rgba(34,197,94,0.1)", border: "1px solid rgba(34,197,94,0.3)",
                  color: "#22C55E", fontFamily: "'DM Sans', sans-serif", fontSize: 12, fontWeight: 600,
                }}>Push to Active Scene</button>
              </div>
            </div>
          </div>
        </div>
      </div>
    </AppSidebar>
  );
}
