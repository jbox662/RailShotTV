// RailShotTV — Chromatic Command — Unified Broadcast Control Surface (OBS-style single screen)
// Colors: Brand=#FF5A2C, Blue=#4F9EFF, Violet=#A855F7, Emerald=#22C55E, Cyan=#22D3EE, Amber=#FBBF24
import { useState, useEffect, useRef, useCallback, useMemo, memo } from "react";
import AppSidebar from "@/components/AppSidebar";
import GoLiveModal from "@/components/GoLiveModal";
import { useScenes } from "@/contexts/SceneContext";
import { toast } from "sonner";
import {
  Plus, Trash2, Eye, EyeOff, Lock, Unlock, ChevronUp, ChevronDown,
  Mic, Music, Volume2, Square, Layers, Monitor, Camera, Globe, Type,
  Image as ImageIcon, Bell, Trophy, AlignLeft, X, Search, Sparkles,
  LayoutTemplate, Activity, Cpu, Users, Clock, Settings,
} from "lucide-react";
import { ContextMenu, ContextMenuTrigger, ContextMenuContent, ContextMenuItem, ContextMenuSeparator, ContextMenuSub, ContextMenuSubTrigger, ContextMenuSubContent, ContextMenuLabel } from "@/components/ui/context-menu";
import { InputSettingsDrawer, DEFAULT_INPUT_SETTINGS } from "@/components/InputSettingsDrawer";
import type { InputSettings } from "@/components/InputSettingsDrawer";


// ── Canvas source transform ───────────────────────────────────────────────────
type CanvasTransform = { x: number; y: number; w: number; h: number };
const DEFAULT_TRANSFORMS: Record<string, CanvasTransform> = {
  camera:     { x: 0.05, y: 0.05, w: 0.4,  h: 0.4  },
  display:    { x: 0,    y: 0,    w: 1,    h: 1    },
  browser:    { x: 0.1,  y: 0.1,  w: 0.8,  h: 0.8  },
  image:      { x: 0.05, y: 0.05, w: 0.3,  h: 0.3  },
  text:       { x: 0.1,  y: 0.7,  w: 0.5,  h: 0.12 },
  alert:      { x: 0.2,  y: 0.05, w: 0.6,  h: 0.25 },
  scoreboard: { x: 0.05, y: 0.05, w: 0.9,  h: 0.9  },
  lowerthird: { x: 0.0,  y: 0.72, w: 1.0,  h: 0.2  },
};
// ── Program Canvas ────────────────────────────────────────────────────────────
const ProgramCanvas = memo(function ProgramCanvas({
  sources, selectedId, transforms, onSelect, onTransformChange,
}: {
  sources: SourceItem[];
  selectedId: number | null;
  transforms: Record<number, CanvasTransform>;
  onSelect: (id: number | null) => void;
  onTransformChange: (id: number, t: CanvasTransform) => void;
}) {
  const containerRef = useRef<HTMLDivElement>(null);
  const dragRef = useRef<{ id: number; startX: number; startY: number; origX: number; origY: number } | null>(null);
  const resizeRef = useRef<{ id: number; handle: string; startX: number; startY: number; orig: CanvasTransform } | null>(null);

  const getTransform = (s: SourceItem): CanvasTransform =>
    transforms[s.id] ?? DEFAULT_TRANSFORMS[s.type] ?? { x: 0.1, y: 0.1, w: 0.5, h: 0.5 };

  const normPos = (e: React.MouseEvent | MouseEvent) => {
    const el = containerRef.current;
    if (!el) return { nx: 0, ny: 0 };
    const r = el.getBoundingClientRect();
    return { nx: (e.clientX - r.left) / r.width, ny: (e.clientY - r.top) / r.height };
  };

  // Mouse move / up on window
  useEffect(() => {
    const onMove = (e: MouseEvent) => {
      if (dragRef.current) {
        const { id, startX, startY, origX, origY } = dragRef.current;
        const el = containerRef.current; if (!el) return;
        const r = el.getBoundingClientRect();
        const dx = (e.clientX - startX) / r.width;
        const dy = (e.clientY - startY) / r.height;
        const prev = transforms[id] ?? DEFAULT_TRANSFORMS["browser"];
        onTransformChange(id, { ...prev, x: Math.max(0, Math.min(1 - prev.w, origX + dx)), y: Math.max(0, Math.min(1 - prev.h, origY + dy)) });
      }
      if (resizeRef.current) {
        const { id, handle, startX, startY, orig } = resizeRef.current;
        const el = containerRef.current; if (!el) return;
        const r = el.getBoundingClientRect();
        const dx = (e.clientX - startX) / r.width;
        const dy = (e.clientY - startY) / r.height;
        let { x, y, w, h } = orig;
        const MIN = 0.05;
        if (handle.includes("e")) w = Math.max(MIN, Math.min(1 - x, orig.w + dx));
        if (handle.includes("s")) h = Math.max(MIN, Math.min(1 - y, orig.h + dy));
        if (handle.includes("w")) { const nw = Math.max(MIN, orig.w - dx); x = orig.x + orig.w - nw; w = nw; }
        if (handle.includes("n")) { const nh = Math.max(MIN, orig.h - dy); y = orig.y + orig.h - nh; h = nh; }
        onTransformChange(id, { x, y, w, h });
      }
    };
    const onUp = () => { dragRef.current = null; resizeRef.current = null; };
    window.addEventListener("mousemove", onMove);
    window.addEventListener("mouseup", onUp);
    return () => { window.removeEventListener("mousemove", onMove); window.removeEventListener("mouseup", onUp); };
  }, [transforms, onTransformChange]);

  const HANDLES = ["n","s","e","w","nw","ne","sw","se"];
  const handleCursor: Record<string, string> = { n:"ns-resize", s:"ns-resize", e:"ew-resize", w:"ew-resize", nw:"nwse-resize", se:"nwse-resize", ne:"nesw-resize", sw:"nesw-resize" };
  const handlePos = (h: string, w: number, ht: number): React.CSSProperties => {
    const mid = "50%"; const edge = -4;
    const top    = h.includes("n") ? edge : h.includes("s") ? "calc(100% - 4px)" : mid;
    const left   = h.includes("w") ? edge : h.includes("e") ? "calc(100% - 4px)" : mid;
    return { position: "absolute", top, left, transform: "translate(-50%,-50%)", width: 8, height: 8, borderRadius: 2, background: "#4F9EFF", border: "1px solid #fff", cursor: handleCursor[h], zIndex: 10 };
  };

  const visibleSources = sources.filter(s => s.visible);

  return (
    <div
      ref={containerRef}
      style={{ position: "absolute", inset: 0, background: "#000", overflow: "hidden" }}
      onClick={e => { if (e.target === containerRef.current) onSelect(null); }}
    >
      {visibleSources.length === 0 && (
        <div style={{ position: "absolute", inset: 0, display: "flex", flexDirection: "column", alignItems: "center", justifyContent: "center", gap: 10, pointerEvents: "none" }}>
          <svg width="48" height="44" viewBox="0 0 48 44" fill="none" style={{ opacity: 0.18 }}>
            <circle cx="24" cy="8" r="7" stroke="#4F9EFF" strokeWidth="1.5"/>
            <circle cx="14" cy="22" r="7" stroke="#4F9EFF" strokeWidth="1.5"/>
            <circle cx="34" cy="22" r="7" stroke="#4F9EFF" strokeWidth="1.5"/>
            <circle cx="4" cy="36" r="7" stroke="#4F9EFF" strokeWidth="1.5"/>
            <circle cx="24" cy="36" r="7" stroke="#FF5A2C" strokeWidth="1.5"/>
            <circle cx="44" cy="36" r="7" stroke="#4F9EFF" strokeWidth="1.5"/>
          </svg>
          <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, color: "#3A4A6A", letterSpacing: "0.06em", textTransform: "uppercase" }}>Rack this scene</span>
          <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, color: "#2A3550" }}>Add sources to build your layout</span>
        </div>
      )}
      {visibleSources.map(src => {
        const t = getTransform(src);
        const isSelected = src.id === selectedId;
        const url = src.settings?.url as string | undefined;
        return (
          <div
            key={src.id}
            style={{
              position: "absolute",
              left: `${t.x * 100}%`, top: `${t.y * 100}%`,
              width: `${t.w * 100}%`, height: `${t.h * 100}%`,
              border: isSelected ? `2px solid #4F9EFF` : "none",
              boxSizing: "border-box",
              cursor: src.locked ? "not-allowed" : "move",
              userSelect: "none",
              zIndex: isSelected ? 20 : 1,
            }}
            onMouseDown={e => {
              if (src.locked) return;
              e.stopPropagation();
              onSelect(src.id);
              const { nx, ny } = normPos(e);
              dragRef.current = { id: src.id, startX: e.clientX, startY: e.clientY, origX: t.x, origY: t.y };
            }}
          >
            {/* Source content */}
            {src.type === "browser" && url ? (
              <iframe
                src={url}
                style={{ width: "100%", height: "100%", border: "none", display: "block", pointerEvents: "none" }}
                title={src.name}
                sandbox="allow-scripts allow-same-origin allow-forms"
              />
            ) : src.type === "camera" ? (
              <div style={{ width: "100%", height: "100%", background: "#0A1A0A", display: "flex", flexDirection: "column", alignItems: "center", justifyContent: "center", gap: 4 }}>
                <src.icon size={24} style={{ color: src.color, opacity: 0.5 }} />
                <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, color: src.color, opacity: 0.7 }}>{src.name}</span>
              </div>
            ) : src.type === "image" && (src.settings?.src as string) ? (
              <img src={src.settings.src as string} alt={src.name} style={{ width: "100%", height: "100%", objectFit: "contain" }} />
            ) : src.type === "text" ? (
              <div style={{ width: "100%", height: "100%", display: "flex", alignItems: "center", justifyContent: "center", padding: "0 8px" }}>
                <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: Math.max(10, (src.settings?.fontSize as number ?? 48) * 0.4), color: "#F8F8FF", fontWeight: src.settings?.bold ? 700 : 400, textShadow: "0 2px 8px rgba(0,0,0,0.8)" }}>
                  {(src.settings?.text as string) || src.name}
                </span>
              </div>
            ) : (
              <div style={{ width: "100%", height: "100%", background: src.color + "18", display: "flex", flexDirection: "column", alignItems: "center", justifyContent: "center", gap: 4 }}>
                <src.icon size={20} style={{ color: src.color, opacity: 0.6 }} />
                <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, color: src.color, opacity: 0.8 }}>{src.name}</span>
              </div>
            )}

            {/* Resize handles */}
            {isSelected && !src.locked && HANDLES.map(h => (
              <div
                key={h}
                style={handlePos(h, t.w, t.h)}
                onMouseDown={e => {
                  e.stopPropagation();
                  resizeRef.current = { id: src.id, handle: h, startX: e.clientX, startY: e.clientY, orig: { ...t } };
                }}
              />
            ))}
          </div>
        );
      })}
    </div>
  );
});

// ── Source type catalogue ─────────────────────────────────────────────────────
const SOURCE_TYPES = [
  { type: "display",    label: "Display Capture", icon: Monitor,   color: "#4F9EFF" },
  { type: "camera",     label: "Camera / Webcam", icon: Camera,    color: "#22C55E" },
  { type: "browser",    label: "Browser Source",  icon: Globe,     color: "#22D3EE" },
  { type: "text",       label: "Text (GDI+)",     icon: Type,      color: "#A855F7" },
  { type: "image",      label: "Image",           icon: ImageIcon, color: "#FBBF24" },
  { type: "alert",      label: "Alert / Stinger", icon: Bell,      color: "#FF5A2C" },
  { type: "scoreboard", label: "Scoreboard",      icon: Trophy,    color: "#FF5A2C" },
  { type: "lowerthird", label: "Lower Third",     icon: AlignLeft, color: "#4F9EFF" },
];

// ── Overlay template catalogue ────────────────────────────────────────────────
const OVERLAY_TEMPLATES = [
  { id: "sb-pool",   name: "Billiards Scoreboard", cat: "scoreboard", icon: Trophy,    color: "#FF5A2C" },
  { id: "sb-bball",  name: "Basketball Board",     cat: "scoreboard", icon: Trophy,    color: "#FBBF24" },
  { id: "lt-player", name: "Player Lower Third",   cat: "lowerthird", icon: AlignLeft, color: "#4F9EFF" },
  { id: "lt-team",   name: "Team Lower Third",     cat: "lowerthird", icon: AlignLeft, color: "#4F9EFF" },
  { id: "ticker",    name: "Score Ticker",         cat: "browser",    icon: Globe,     color: "#22D3EE" },
  { id: "alert-sub", name: "Sub Alert",            cat: "alert",      icon: Bell,      color: "#FF5A2C" },
  { id: "logo",      name: "Logo Overlay",         cat: "image",      icon: ImageIcon, color: "#FBBF24" },
  { id: "cam-frame", name: "Camera Frame",         cat: "camera",     icon: Camera,    color: "#22C55E" },
];

// ── Vertical VU Meter (OBS-style, two channels L/R) ─────────────────────────
function VUMeterVertical({ color, active, volume }: { color: string; active: boolean; volume: number }) {
  const [levels, setLevels] = useState([0, 0]);
  const [peaks, setPeaks]   = useState([0, 0]);
  const peakHoldRef = useRef([0, 0]);
  useEffect(() => {
    if (!active) { setLevels([0, 0]); setPeaks([0, 0]); peakHoldRef.current = [0, 0]; return; }
    const t = setInterval(() => {
      const vol = volume / 100;
      const newLevels = [0, 1].map(i => {
        const base = 0.55 + Math.random() * 0.35;
        return Math.min(1, base * vol + (Math.random() - 0.5) * 0.08);
      });
      setLevels(newLevels);
      setPeaks(prev => prev.map((p, i) => {
        if (newLevels[i] > p) { peakHoldRef.current[i] = 60; return newLevels[i]; }
        peakHoldRef.current[i] = Math.max(0, peakHoldRef.current[i] - 1);
        return peakHoldRef.current[i] > 0 ? p : Math.max(0, p - 0.01);
      }));
    }, 60);
    return () => clearInterval(t);
  }, [active, volume]);

  const DB_MARKS = [0, -6, -12, -18, -24, -30, -42, -54];
  const meterH = 80;
  const dbToY = (db: number) => (1 - (db + 60) / 60) * meterH;

  const levelColor = (pct: number) =>
    pct > 0.9 ? "#EF4444" : pct > 0.75 ? "#FBBF24" : pct > 0.5 ? "#84CC16" : color;

  return (
    <div className="flex gap-1 items-end" style={{ height: meterH, position: "relative" }}>
      {[0, 1].map(ch => (
        <div key={ch} style={{ width: 8, height: meterH, background: "#0A0E1A", borderRadius: 2, position: "relative", overflow: "hidden", border: "1px solid rgba(255,255,255,0.06)" }}>
          {/* Filled level bar */}
          <div style={{
            position: "absolute", bottom: 0, left: 0, right: 0,
            height: `${levels[ch] * 100}%`,
            background: `linear-gradient(to top, ${color} 0%, #84CC16 60%, #FBBF24 80%, #EF4444 100%)`,
            transition: "height 0.05s linear",
            opacity: active ? 1 : 0.15,
          }} />
          {/* Peak hold indicator */}
          {active && peaks[ch] > 0 && (
            <div style={{
              position: "absolute", left: 0, right: 0, height: 2,
              bottom: `${peaks[ch] * 100}%`,
              background: peaks[ch] > 0.9 ? "#EF4444" : "#fff",
              opacity: 0.9,
            }} />
          )}
          {/* dB grid lines */}
          {DB_MARKS.map(db => (
            <div key={db} style={{
              position: "absolute", left: 0, right: 0, height: 1,
              bottom: `${((db + 60) / 60) * 100}%`,
              background: "rgba(255,255,255,0.08)",
            }} />
          ))}
        </div>
      ))}
    </div>
  );
}

// ── Bitrate sparkline ─────────────────────────────────────────────────────────
function BitrateSparkline({ active }: { active: boolean }) {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const dataRef   = useRef<number[]>(Array(40).fill(0));
  useEffect(() => {
    const interval = setInterval(() => {
      const last = dataRef.current[dataRef.current.length - 1];
      const next = active ? Math.max(7000, Math.min(10000, last + (Math.random() - 0.48) * 300)) : 0;
      dataRef.current = [...dataRef.current.slice(1), next];
      const canvas = canvasRef.current; if (!canvas) return;
      const ctx = canvas.getContext("2d"); if (!ctx) return;
      ctx.clearRect(0, 0, canvas.width, canvas.height);
      const pts = dataRef.current.map((v, i) => ({
        x: (i / 39) * canvas.width,
        y: canvas.height - (v / 12000) * canvas.height,
      }));
      ctx.beginPath();
      pts.forEach((p, i) => (i === 0 ? ctx.moveTo(p.x, p.y) : ctx.lineTo(p.x, p.y)));
      ctx.strokeStyle = active ? "#4F9EFF" : "#303D5A";
      ctx.lineWidth = 1.5;
      ctx.stroke();
    }, 200);
    return () => clearInterval(interval);
  }, [active]);
  return <canvas ref={canvasRef} width={180} height={36} style={{ width: "100%", height: 36 }} />;
}

// ── Add Source Modal ──────────────────────────────────────────────────────────
function AddSourceModal({ onAdd, onClose }: {
  onAdd: (type: string, name: string, icon: React.ElementType, color: string) => void;
  onClose: () => void;
}) {
  const [name, setName]       = useState(SOURCE_TYPES[0].label);
  const [selected, setSelected] = useState(SOURCE_TYPES[0]);
  useEffect(() => { setName(selected.label); }, [selected]);
  useEffect(() => {
    const h = (e: KeyboardEvent) => { if (e.key === "Escape") onClose(); };
    window.addEventListener("keydown", h);
    return () => window.removeEventListener("keydown", h);
  }, [onClose]);
  return (
    <div className="fixed inset-0 flex items-center justify-center"
      style={{ background: "rgba(0,0,0,0.75)", zIndex: 9999 }}
      onClick={e => { if (e.target === e.currentTarget) onClose(); }}>
      <div style={{ background: "linear-gradient(135deg, #161E30 0%, #111826 100%)", border: "1px solid rgba(79,158,255,0.2)", borderRadius: 12, width: 420, boxShadow: "0 32px 80px rgba(0,0,0,0.8), 0 0 0 1px rgba(79,158,255,0.08), 0 0 60px rgba(79,158,255,0.06)" }}>
        <div className="flex items-center justify-between px-5 py-3.5" style={{ borderBottom: "1px solid rgba(79,158,255,0.15)", background: "linear-gradient(90deg, rgba(79,158,255,0.06) 0%, transparent 100%)" }}>
          <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 700, fontSize: 14, color: "#F8F8FF" }}>Add Source</span>
          <button onClick={onClose} style={{ background: "none", border: "none", cursor: "pointer", color: "#606078" }}><X size={16} /></button>
        </div>
        <div className="grid grid-cols-4 gap-2 p-4">
          {SOURCE_TYPES.map(st => (
            <button key={st.type} onClick={() => setSelected(st)}
              style={{ display: "flex", flexDirection: "column", alignItems: "center", gap: 6, padding: "10px 6px", borderRadius: 7, cursor: "pointer",
                border: `1px solid ${selected.type === st.type ? st.color + "90" : "rgba(255,255,255,0.07)"}`,
                background: selected.type === st.type ? st.color + "22" : "rgba(255,255,255,0.03)", transition: "all 0.15s", boxShadow: selected.type === st.type ? `0 0 16px ${st.color}30, inset 0 1px 0 rgba(255,255,255,0.1)` : "0 1px 4px rgba(0,0,0,0.3)" }}>
              <st.icon size={20} style={{ color: selected.type === st.type ? st.color : "#606078" }} />
              <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, color: selected.type === st.type ? "#F8F8FF" : "#606078", textAlign: "center", lineHeight: 1.2 }}>{st.label}</span>
            </button>
          ))}
        </div>
        <div className="px-4 pb-4">
          <label style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, color: "#8892A4", display: "block", marginBottom: 6, letterSpacing: "0.06em", textTransform: "uppercase" }}>Source Name</label>
          <input autoFocus value={name} onChange={e => setName(e.target.value)}
            onKeyDown={e => { if (e.key === "Enter" && name.trim()) { onAdd(selected.type, name.trim(), selected.icon, selected.color); onClose(); } }}
            style={{ width: "100%", background: "#141928", border: "1px solid #303D5A", borderRadius: 6, color: "#F8F8FF", fontFamily: "'DM Sans', sans-serif", fontSize: 13, padding: "8px 12px", outline: "none", boxSizing: "border-box" as const }} />
        </div>
        <div className="flex justify-end gap-2 px-4 pb-4">
          <button onClick={onClose} style={{ padding: "7px 16px", borderRadius: 6, border: "1px solid #303D5A", background: "none", color: "#8892A4", fontFamily: "'DM Sans', sans-serif", fontSize: 12, cursor: "pointer" }}>Cancel</button>
          <button onClick={() => { if (name.trim()) { onAdd(selected.type, name.trim(), selected.icon, selected.color); onClose(); } }}
            style={{ padding: "7px 16px", borderRadius: 6, border: "none", background: "linear-gradient(135deg, #4F9EFF 0%, #7C3AED 100%)", color: "#fff", fontFamily: "'DM Sans', sans-serif", fontSize: 12, fontWeight: 700, cursor: "pointer", boxShadow: "0 0 20px rgba(79,158,255,0.4), 0 2px 8px rgba(124,58,237,0.3), inset 0 1px 0 rgba(255,255,255,0.2)" }}>
            Add Source
          </button>
        </div>
      </div>
    </div>
  );
}

// ── Properties panel ──────────────────────────────────────────────────────────
type SourceItem = ReturnType<typeof useScenes>["scenes"][0]["sources"][0];
function PropertiesPanel({ sourceId, sources, onUpdateSettings, onUpdateTransform }: {
  sourceId: number | null;
  sources: SourceItem[];
  onUpdateSettings: (k: string, v: string | number | boolean) => void;
  onUpdateTransform: (k: string, v: number | boolean) => void;
}) {
  const src = sources.find(s => s.id === sourceId) ?? null;
  if (!src) return (
    <div className="flex flex-col items-center justify-center flex-1 gap-2" style={{ color: "#50506A" }}>
      <Layers size={20} />
      <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11 }}>Select a source</span>
    </div>
  );
  const setting = (k: string, def: string | number | boolean = "") => (src.settings as Record<string, unknown>)?.[k] ?? def;
  const numIn = (val: number, onChange: (v: number) => void, min?: number, max?: number) => (
    <input type="number" value={val} min={min} max={max} onChange={e => onChange(Number(e.target.value))}
      style={{ width: 58, height: 20, background: "rgba(168,85,247,0.08)", border: "1px solid rgba(168,85,247,0.3)", borderRadius: 4, color: "#C084FC", fontFamily: "'JetBrains Mono', monospace", fontSize: 10, padding: "0 5px", outline: "none", boxShadow: "inset 0 1px 3px rgba(0,0,0,0.3)" }} />
  );
  const txtIn = (val: string, onChange: (v: string) => void, placeholder = "", width = 120) => (
    <input type="text" value={val} onChange={e => onChange(e.target.value)} placeholder={placeholder}
      style={{ width, height: 20, background: "rgba(79,158,255,0.06)", border: "1px solid rgba(79,158,255,0.2)", borderRadius: 4, color: "#A8C0E0", fontFamily: "'DM Sans', sans-serif", fontSize: 10, padding: "0 5px", outline: "none", boxShadow: "inset 0 1px 3px rgba(0,0,0,0.3)" }} />
  );
  const tog = (val: boolean, onChange: (v: boolean) => void) => (
    <div onClick={() => onChange(!val)} style={{ width: 28, height: 16, borderRadius: 8, background: val ? "linear-gradient(135deg, #FF5A2C, #FF8C42)" : "rgba(255,255,255,0.05)", border: `1px solid ${val ? "rgba(255,90,44,0.7)" : "rgba(255,255,255,0.1)"}`, boxShadow: val ? "0 0 10px rgba(255,90,44,0.4)" : "none", position: "relative", cursor: "pointer", transition: "background 0.15s" }}>
      <div style={{ position: "absolute", top: 2, left: val ? 12 : 2, width: 10, height: 10, borderRadius: "50%", background: "#fff", transition: "left 0.15s" }} />
    </div>
  );
  const selIn = (val: string, onChange: (v: string) => void, options: string[]) => (
    <select value={val} onChange={e => onChange(e.target.value)} style={{ height: 20, background: "rgba(79,158,255,0.06)", border: "1px solid rgba(79,158,255,0.2)", borderRadius: 4, color: "#A8C0E0", fontFamily: "'DM Sans', sans-serif", fontSize: 10, padding: "0 4px", outline: "none" }}>
      {options.map(o => <option key={o}>{o}</option>)}
    </select>
  );
  const row = (label: string, input: React.ReactNode) => (
    <div key={label} className="flex items-center justify-between py-1" style={{ borderBottom: "1px solid #1A1A24" }}>
      <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, color: "#8892A4" }}>{label}</span>
      {input}
    </div>
  );
  const t = (src as SourceItem & { _transform?: Record<string, number | boolean> })._transform ?? { x: 0, y: 0, w: 1920, h: 1080, rot: 0, opacity: 100 };
  return (
    <div className="flex flex-col overflow-y-auto flex-1" style={{ scrollbarWidth: "thin", scrollbarColor: "#303D5A transparent" }}>
      <div className="px-2 pt-2 pb-1">
        <div style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 9, fontWeight: 700, color: "#7B8FBF", letterSpacing: "0.12em", textTransform: "uppercase", marginBottom: 3 }}>Transform</div>
        {row("X", numIn(t.x as number, v => onUpdateTransform("x", v)))}
        {row("Y", numIn(t.y as number, v => onUpdateTransform("y", v)))}
        {row("W", numIn(t.w as number, v => onUpdateTransform("w", v), 1))}
        {row("H", numIn(t.h as number, v => onUpdateTransform("h", v), 1))}
        {row("Rot°", numIn(t.rot as number, v => onUpdateTransform("rot", v), -360, 360))}
        {row("Opacity", numIn(t.opacity as number, v => onUpdateTransform("opacity", v), 0, 100))}
        <div className="flex gap-1 mt-1.5 mb-1">
          {["Flip H","Flip V","Reset"].map(l => (
            <button key={l} onClick={() => onUpdateTransform(l === "Reset" ? "__reset" : l === "Flip H" ? "flipH" : "flipV", true)}
              style={{ flex: 1, padding: "3px 0", borderRadius: 4, border: "1px solid rgba(79,158,255,0.25)", background: "linear-gradient(135deg, #1A2540 0%, #141E30 100%)", color: "#7BAAD0", fontFamily: "'DM Sans', sans-serif", fontSize: 9, cursor: "pointer", boxShadow: "0 1px 4px rgba(0,0,0,0.3), inset 0 1px 0 rgba(79,158,255,0.08)" }}>{l}</button>
          ))}
        </div>
      </div>
      <div className="px-2 pb-2" style={{ borderTop: "1px solid #2A3350" }}>
        <div style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 9, fontWeight: 700, color: "#7B8FBF", letterSpacing: "0.12em", textTransform: "uppercase", marginBottom: 3, marginTop: 6 }}>Settings</div>
        {src.type === "camera" && (<>
          {row("Device", selIn(setting("device","Default Camera") as string, v => onUpdateSettings("device", v), ["Default Camera","USB Camera 1","USB Camera 2","IP Camera (RTSP)","NDI Source"]))}
          {row("RTSP URL", txtIn(setting("rtsp","") as string, v => onUpdateSettings("rtsp", v), "rtsp://..."))}
          {row("Flip H", tog(setting("flipCam",false) as boolean, v => onUpdateSettings("flipCam", v)))}
        </>)}
        {src.type === "browser" && (<>
          {row("URL", txtIn(setting("url","") as string, v => onUpdateSettings("url", v), "https://...", 130))}
          {row("W px", numIn(setting("bw",1920) as number, v => onUpdateSettings("bw", v), 1))}
          {row("H px", numIn(setting("bh",1080) as number, v => onUpdateSettings("bh", v), 1))}
          {row("Auto-reload", tog(setting("refresh",false) as boolean, v => onUpdateSettings("refresh", v)))}
        </>)}
        {src.type === "image" && (<>
          {row("Path / URL", txtIn(setting("src","") as string, v => onUpdateSettings("src", v), "/path/to/img", 130))}
        </>)}
        {src.type === "text" && (<>
          {row("Content", txtIn(setting("text","") as string, v => onUpdateSettings("text", v), "Enter text...", 130))}
          {row("Size", numIn(setting("fontSize",48) as number, v => onUpdateSettings("fontSize", v), 8, 400))}
          {row("Bold", tog(setting("bold",false) as boolean, v => onUpdateSettings("bold", v)))}
        </>)}
        {src.type === "display" && (<>
          {row("Monitor", selIn(setting("monitor","Primary Monitor") as string, v => onUpdateSettings("monitor", v), ["Primary Monitor","Secondary Monitor","Monitor 3"]))}
          {row("Cursor", tog(setting("cursor",true) as boolean, v => onUpdateSettings("cursor", v)))}
        </>)}
        {src.type === "alert" && (<>
          {row("Alert URL", txtIn(setting("alertUrl","") as string, v => onUpdateSettings("alertUrl", v), "https://...", 130))}
        </>)}
        {src.type === "scoreboard" && (<>
          {row("Sport", selIn(setting("sport","Billiards") as string, v => onUpdateSettings("sport", v), ["Billiards","Basketball","Football","Soccer","Tennis"]))}
        </>)}
        {src.type === "lowerthird" && (<>
          {row("Title", txtIn(setting("title","") as string, v => onUpdateSettings("title", v), "Player Name", 120))}
          {row("Subtitle", txtIn(setting("subtitle","") as string, v => onUpdateSettings("subtitle", v), "Team / Role", 120))}
          {row("Duration s", numIn(setting("duration",5) as number, v => onUpdateSettings("duration", v), 1, 60))}
        </>)}
        {!["camera","browser","image","text","display","alert","scoreboard","lowerthird","overlay"].includes(src.type) && (
          <div style={{ color: "#50506A", fontFamily: "'DM Sans', sans-serif", fontSize: 10, paddingTop: 6 }}>No settings for this type.</div>
        )}
      </div>
    </div>
  );
}

// ── Main unified broadcast control surface ────────────────────────────────────
export default function Dashboard() {
  const {
    scenes, activeSceneId, setActiveSceneId,
    addScene, renameScene, deleteScene, duplicateScene,
    addSource, removeSource, updateSource, moveSource, updateSourceSettings,
  } = useScenes();
  // SceneContext addScene returns void and auto-activates; we track the new id via scenes length change
  const handleAddScene = useCallback((name?: string) => {
    addScene();
    // SceneContext auto-activates the new scene; we just clear source selection
    setSelectedSourceId(null);
  }, [addScene]);

  // Stream state
  const [isLive, setIsLive]             = useState(false);
  const [livePlatform, setLivePlatform] = useState("");
  const [showGoLive, setShowGoLive]     = useState(false);
  const [tc, setTc]                     = useState("00:00:00");
  const [bitrate, setBitrate]           = useState(0);
  const [viewers, setViewers]           = useState(0);

  // Scene rename
  const [renamingSceneId, setRenamingSceneId] = useState<number | null>(null);
  const [renameValue, setRenameValue]         = useState("");

  // Source state
  const [selectedSourceId, setSelectedSourceId] = useState<number | null>(null);
  const [showAddSource, setShowAddSource]       = useState(false);
  const [sourceTransforms, setSourceTransforms] = useState<Record<number, Record<string, number | boolean>>>({});
  const [activeTransition, setActiveTransition] = useState<string>("Cut");
  const [transitionSpeed, setTransitionSpeed] = useState(500);
  const [programSceneId, setProgramSceneId] = useState<number | null>(null);
  const [previewSceneId, setPreviewSceneId] = useState<number | null>(null);
  const [isTransitioning, setIsTransitioning] = useState(false);
  const [transitionPhase, setTransitionPhase] = useState<"idle"|"out"|"in">("idle");
  const [renamingSourceId, setRenamingSourceId] = useState<number | null>(null);
  const [renameSourceValue, setRenameSourceValue] = useState("");
  const [canvasTransforms, setCanvasTransforms] = useState<Record<number, CanvasTransform>>(() => {
    try { const s = localStorage.getItem("railshot_canvas_transforms_v1"); return s ? JSON.parse(s) : {}; } catch { return {}; }
  });
  useEffect(() => {
    try { localStorage.setItem("railshot_canvas_transforms_v1", JSON.stringify(canvasTransforms)); } catch {}
  }, [canvasTransforms]);
  const handleCanvasTransformChange = useCallback((id: number, t: CanvasTransform) => {
    setCanvasTransforms(p => ({ ...p, [id]: t }));
  }, []);
  // GO transition handler
  const handleGo = useCallback((targetSceneId: number) => {
    if (isTransitioning) return;
    if (activeTransition === "Cut") {
      // Instant cut — no animation
      setProgramSceneId(targetSceneId);
      setActiveSceneId(targetSceneId);
      setSelectedSourceId(null);
      return;
    }
    setIsTransitioning(true);
    setTransitionPhase("out");
    const halfDuration = activeTransition === "FTB" ? transitionSpeed : transitionSpeed / 2;
    setTimeout(() => {
      setProgramSceneId(targetSceneId);
      setActiveSceneId(targetSceneId);
      setSelectedSourceId(null);
      setTransitionPhase("in");
      setTimeout(() => {
        setIsTransitioning(false);
        setTransitionPhase("idle");
      }, halfDuration);
    }, halfDuration);
  }, [isTransitioning, activeTransition, transitionSpeed, setActiveSceneId]);

  // Overlay browser
  const [overlayBrowserOpen, setOverlayBrowserOpen] = useState(false);
  const [overlaySearch, setOverlaySearch]           = useState("");

  // Audio mixer
  const [channelState, setChannelState] = useState<Record<number, { muted: boolean; solo: boolean; volume: number }>>({});
  const [audioMixerOpen, setAudioMixerOpen] = useState(false);
  // Input settings drawer
  const [inputSettingsSceneId, setInputSettingsSceneId] = useState<number | null>(null);
  const [inputSettingsMap, setInputSettingsMap] = useState<Record<number, InputSettings>>({});
  const inputSettingsScene = useMemo(() => scenes.find(s => s.id === inputSettingsSceneId) ?? null, [scenes, inputSettingsSceneId]);

  const handleOpenInputSettings = useCallback((sceneId: number) => {
    setInputSettingsSceneId(sceneId);
  }, []);

  const handleSaveInputSettings = useCallback((sceneId: number, settings: InputSettings) => {
    setInputSettingsMap(prev => ({ ...prev, [sceneId]: settings }));
    // Apply the name change to the scene itself
    if (settings.name.trim()) {
      renameScene(sceneId, settings.name.trim());
    }
    toast.success("Input settings saved");
  }, [renameScene]);

  // Fullscreen state and handler
  const [isFullscreen, setIsFullscreen] = useState(false);
  const [inputsPaused, setInputsPaused] = useState(false);
  const [activeColorFilter, setActiveColorFilter] = useState<string | null>(null);
  const handleFullscreen = useCallback(() => {
    if (!document.fullscreenElement) {
      document.documentElement.requestFullscreen().catch(() => toast.error("Fullscreen not available"));
    } else {
      document.exitFullscreen();
    }
  }, []);
  useEffect(() => {
    const handler = () => setIsFullscreen(!!document.fullscreenElement);
    document.addEventListener("fullscreenchange", handler);
    return () => document.removeEventListener("fullscreenchange", handler);
  }, []);
  // Keyboard shortcuts
  useEffect(() => {
    const handler = (e: KeyboardEvent) => {
      // Ignore if typing in an input/textarea
      const tag = (e.target as HTMLElement)?.tagName?.toLowerCase();
      if (tag === "input" || tag === "textarea" || tag === "select") return;
      // Enter or Space → GO (trigger transition on previewSceneId)
      if ((e.key === "Enter" || e.key === " ") && !e.repeat) {
        e.preventDefault();
        if (previewSceneId !== null) {
          handleGo(previewSceneId);
          toast.success(`${activeTransition} → ${scenes.find(s => s.id === previewSceneId)?.name ?? "scene"}`);
        } else {
          toast.error("No scene in Preview — click a tile first");
        }
        return;
      }
      // Number keys 1–8 → set that scene as Preview
      if (e.key >= "1" && e.key <= "8" && !e.ctrlKey && !e.metaKey && !e.altKey) {
        const idx = parseInt(e.key) - 1;
        const sc = scenes[idx];
        if (sc) {
          setPreviewSceneId(sc.id);
          setActiveSceneId(sc.id);
          setSelectedSourceId(null);
          toast.info(`Preview → ${sc.name}`);
        }
        return;
      }
      // F → fullscreen toggle
      if (e.key === "f" || e.key === "F") {
        handleFullscreen();
        return;
      }
    };
    window.addEventListener("keydown", handler);
    return () => window.removeEventListener("keydown", handler);
  }, [previewSceneId, handleGo, activeTransition, scenes, setActiveSceneId, handleFullscreen]);

  // Active scene — memoize sources so ProgramCanvas React.memo can bail out on timecode ticks
  const activeScene  = useMemo(() => scenes.find(s => s.id === activeSceneId) ?? null, [scenes, activeSceneId]);
  const EMPTY_SOURCES: typeof activeScene extends null ? never[] : NonNullable<typeof activeScene>["sources"] = useMemo(() => [] as any, []);
  const sources      = useMemo(() => activeScene?.sources ?? EMPTY_SOURCES, [activeScene, EMPTY_SOURCES]);
  // Memoize preview/program sources separately so their canvases don't re-render on timecode ticks
  const previewScene  = useMemo(() => scenes.find(s => s.id === previewSceneId) ?? null, [scenes, previewSceneId]);
  const programScene  = useMemo(() => scenes.find(s => s.id === programSceneId) ?? null, [scenes, programSceneId]);
  const previewSources = useMemo(() => previewScene?.sources ?? EMPTY_SOURCES, [previewScene, EMPTY_SOURCES]);
  const programSources = useMemo(() => programScene?.sources ?? EMPTY_SOURCES, [programScene, EMPTY_SOURCES]);
  const selectedSource = sources.find(s => s.id === selectedSourceId) ?? null;

  // Timecode
  useEffect(() => {
    if (!isLive) return;
    let secs = 0;
    const t = setInterval(() => {
      secs++;
      const h = String(Math.floor(secs / 3600)).padStart(2, "0");
      const m = String(Math.floor((secs % 3600) / 60)).padStart(2, "0");
      const s = String(secs % 60).padStart(2, "0");
      setTc(`${h}:${m}:${s}`);
      setBitrate(prev => Math.max(7000, Math.min(10000, prev + (Math.random() - 0.48) * 300)));
      if (Math.random() < 0.05) setViewers(v => v + Math.floor(Math.random() * 3));
    }, 1000);
    return () => clearInterval(t);
  }, [isLive]);

  // Handlers
  const handleAddSource = useCallback((type: string, name: string, icon: React.ElementType, color: string) => {
    if (activeSceneId === null) { toast.error("Create a scene first"); return; }
    const id = addSource(activeSceneId, { name, type, icon, iconKey: type, color, visible: true, locked: false, settings: {} });
    setSelectedSourceId(id);
    toast.success(`Added "${name}"`);
  }, [activeSceneId, addSource]);

  const handleDeleteSource = useCallback(() => {
    if (activeSceneId === null || selectedSourceId === null) return;
    removeSource(activeSceneId, selectedSourceId);
    setSelectedSourceId(null);
  }, [activeSceneId, selectedSourceId, removeSource]);

  const handleUpdateSettings = useCallback((k: string, v: string | number | boolean) => {
    if (activeSceneId === null || selectedSourceId === null) return;
    updateSourceSettings(activeSceneId, selectedSourceId, k, v);
  }, [activeSceneId, selectedSourceId, updateSourceSettings]);

  const handleUpdateTransform = useCallback((k: string, v: number | boolean) => {
    if (selectedSourceId === null) return;
    if (k === "__reset") { setSourceTransforms(p => { const n = { ...p }; delete n[selectedSourceId]; return n; }); return; }
    setSourceTransforms(p => ({ ...p, [selectedSourceId]: { ...(p[selectedSourceId] ?? {}), [k]: v } }));
  }, [selectedSourceId]);

  const handleRenameSource = useCallback((id: number, newName: string) => {
    if (!activeSceneId || !newName.trim()) { setRenamingSourceId(null); return; }
    updateSource(activeSceneId, id, { name: newName.trim() });
    setRenamingSourceId(null);
  }, [activeSceneId, updateSource]);

  const handleDuplicateSource = useCallback((src: SourceItem) => {
    if (!activeSceneId) return;
    const newId = addSource(activeSceneId, { ...src, settings: { ...src.settings } });
    setSelectedSourceId(newId);
    toast.success(`Duplicated "${src.name}"`);
  }, [activeSceneId, addSource]);

  // Global audio channels always present (like OBS Desktop Audio + Mic/Aux)
  const GLOBAL_AUDIO_CHANNELS = useMemo(() => [
    { id: -1, name: "Desktop Audio", type: "desktop", color: "#4F9EFF", icon: "desktop" },
    { id: -2, name: "Mic/Aux",       type: "mic",     color: "#22C55E", icon: "mic"     },
  ], []);

  const sceneAudioChannels = useMemo(() =>
    sources.filter(s => ["camera","display"].includes(s.type)).map(s => ({ id: s.id, name: s.name, type: s.type, color: s.color, icon: s.type })),
    [sources]
  );

  const audioChannels = useMemo(() => [...GLOBAL_AUDIO_CHANNELS, ...sceneAudioChannels], [GLOBAL_AUDIO_CHANNELS, sceneAudioChannels]);

  const filteredOverlays = useMemo(() =>
    OVERLAY_TEMPLATES.filter(t => t.name.toLowerCase().includes(overlaySearch.toLowerCase())),
    [overlaySearch]
  );

  return (
    <AppSidebar>
      <div style={{ display: "flex", flexDirection: "column", height: "100vh", overflow: "hidden", background: "#0B0D0F", fontFamily: "'DM Sans', sans-serif" }}>

        {/* ── TOP MENU BAR (vMix-style) ── */}
        <div style={{ display: "flex", alignItems: "center", gap: 2, padding: "0 8px", height: 36, background: "linear-gradient(180deg, #1A1D22 0%, #141619 100%)", borderBottom: "1px solid #2A2D35", boxShadow: "0 1px 0 rgba(0,0,0,0.5)", flexShrink: 0 }}>
          {/* Brand */}
          <div style={{ display: "flex", alignItems: "center", gap: 0, marginRight: 12 }}>
            <span style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 16, color: "#F0F0F0", letterSpacing: "0.04em" }}>RAILSHOT</span>
            <span style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 16, color: "#FF5A2C", letterSpacing: "0.04em" }}>TV</span>
          </div>
          {/* Menu buttons */}
          {["Preset","New","Open","Save","Save As","Last"].map(item => (
            <button key={item} onClick={() => toast.info(`${item} — coming soon`)}
              style={{ padding: "3px 10px", background: item === "Preset" ? "linear-gradient(180deg,#2A2D35,#1E2128)" : "linear-gradient(180deg,#1E2128,#16181E)", border: "1px solid #3A3D45", borderRadius: 3, color: "#C8CAD0", fontSize: 11, cursor: "pointer", fontFamily: "'DM Sans',sans-serif", fontWeight: 500, boxShadow: "0 1px 3px rgba(0,0,0,0.4), inset 0 1px 0 rgba(255,255,255,0.06)" }}>
              {item}
            </button>
          ))}
          <div style={{ flex: 1 }} />
          {/* Resolution selector */}
          <button onClick={handleFullscreen}
            style={{ padding: "3px 10px", background: isFullscreen ? "linear-gradient(180deg,#2A3A2A,#1E281E)" : "linear-gradient(180deg,#1E2128,#16181E)", border: `1px solid ${isFullscreen ? "#22C55E60" : "#3A3D45"}`, borderRadius: 3, color: isFullscreen ? "#22C55E" : "#C8CAD0", fontSize: 11, cursor: "pointer", fontFamily: "'DM Sans',sans-serif", boxShadow: "0 1px 3px rgba(0,0,0,0.4)" }}>
            {isFullscreen ? "Exit Fullscreen" : "Fullscreen"}
          </button>
          <div style={{ width: 1, height: 20, background: "#3A3D45", margin: "0 6px" }} />
          {/* Status */}
          <span style={{ fontFamily: "'JetBrains Mono',monospace", fontSize: 10, color: "#606878" }}>1080p29.97</span>
          <span style={{ fontFamily: "'JetBrains Mono',monospace", fontSize: 10, color: "#606878", marginLeft: 8 }}>EX FPS: 30</span>
          <span style={{ fontFamily: "'JetBrains Mono',monospace", fontSize: 10, color: "#606878", marginLeft: 8 }}>CPU: {isLive ? "12%" : "3%"}</span>
          <div style={{ width: 1, height: 20, background: "#3A3D45", margin: "0 6px" }} />
          {/* Pause Inputs */}
          <button onClick={() => { setInputsPaused(v => !v); toast.info(inputsPaused ? "Inputs resumed" : "Inputs paused"); }}
            style={{ padding: "3px 10px", background: inputsPaused ? "linear-gradient(180deg,#3A2A1A,#281E14)" : "linear-gradient(180deg,#1E2128,#16181E)", border: `1px solid ${inputsPaused ? "#F9731660" : "#3A3D45"}`, borderRadius: 3, color: inputsPaused ? "#F97316" : "#C8CAD0", fontSize: 11, cursor: "pointer", fontFamily: "'DM Sans',sans-serif", boxShadow: "0 1px 3px rgba(0,0,0,0.4)" }}>
            {inputsPaused ? "▐▐ Paused" : "Pause Inputs"}
          </button>
          <button onClick={() => toast.info("Basic mode — simplified layout coming soon")}
            style={{ padding: "3px 10px", background: "linear-gradient(180deg,#1E2128,#16181E)", border: "1px solid #3A3D45", borderRadius: 3, color: "#C8CAD0", fontSize: 11, cursor: "pointer", fontFamily: "'DM Sans',sans-serif", boxShadow: "0 1px 3px rgba(0,0,0,0.4)" }}>
            Basic
          </button>
          <button onClick={() => { window.location.href = "/settings"; }}
            style={{ padding: "3px 10px", background: "linear-gradient(180deg,#1E2128,#16181E)", border: "1px solid #3A3D45", borderRadius: 3, color: "#C8CAD0", fontSize: 11, cursor: "pointer", fontFamily: "'DM Sans',sans-serif", boxShadow: "0 1px 3px rgba(0,0,0,0.4)" }}>
            Settings
          </button>
        </div>

        {/* ── MAIN AREA: Preview | Transitions | Program ── */}
        <div style={{ display: "flex", flex: 1, overflow: "hidden", minHeight: 0 }}>

          {/* ── PREVIEW monitor (left) ── */}
          <div style={{ flex: 1, display: "flex", flexDirection: "column", overflow: "hidden", background: "#0D0F12", borderRight: "1px solid #2A2D35" }}>
            {/* Preview label bar */}
            <div style={{ display: "flex", alignItems: "center", justifyContent: "space-between", padding: "3px 10px", background: "linear-gradient(180deg,#1A1D22,#141619)", borderBottom: "1px solid #2A2D35", flexShrink: 0 }}>
              <span style={{ fontFamily: "'DM Sans',sans-serif", fontWeight: 700, fontSize: 11, color: "#22C55E", letterSpacing: "0.06em" }}>PREVIEW</span>
              <div style={{ display: "flex", alignItems: "center", gap: 6 }}>
                <span style={{ fontFamily: "'JetBrains Mono',monospace", fontSize: 10, color: "#A0A8B8" }}>{scenes.find(s => s.id === previewSceneId)?.name ?? "No Scene"}</span>
                {previewSceneId !== null && (
                  <button onClick={() => { setPreviewSceneId(null); setSelectedSourceId(null); }} title="Clear Preview"
                    style={{ padding: "1px 7px", background: "none", border: "1px solid #22C55E40", borderRadius: 2, color: "#22C55E70", fontSize: 9, fontWeight: 700, cursor: "pointer", fontFamily: "'DM Sans',sans-serif", letterSpacing: "0.06em" }}>
                    CLEAR
                  </button>
                )}
              </div>
            </div>
            {/* Preview canvas */}
            <div style={{ flex: 1, display: "flex", alignItems: "center", justifyContent: "center", background: "#080A0D", position: "relative", overflow: "hidden" }}>
              <div style={{ position: "relative", width: "100%", maxWidth: "100%", aspectRatio: "16/9", background: "#000", border: "2px solid #22C55E30", overflow: "hidden" }}>
                {previewSceneId === null ? (
                  <div style={{ position: "absolute", inset: 0, display: "flex", flexDirection: "column", alignItems: "center", justifyContent: "center", gap: 8 }}>
                    <Monitor size={32} style={{ color: "#2A3550", opacity: 0.4 }} />
                    <span style={{ fontSize: 11, color: "#3A4560", letterSpacing: "0.06em", textTransform: "uppercase" }}>No Preview</span>
                  </div>
                ) : (
                    <ProgramCanvas
                      sources={previewSources}
                      selectedId={selectedSourceId}
                      transforms={canvasTransforms}
                      onSelect={setSelectedSourceId}
                      onTransformChange={handleCanvasTransformChange}
                    />
                )}
                {/* PREVIEW badge */}
                <div style={{ position: "absolute", top: 6, left: 6, padding: "1px 6px", background: "#22C55E", borderRadius: 2, fontFamily: "'DM Sans',sans-serif", fontWeight: 700, fontSize: 9, color: "#000", letterSpacing: "0.08em", pointerEvents: "none" }}>PREVIEW</div>
              </div>
            </div>
          </div>

          {/* ── CENTER: Transition controls ── */}
          <div style={{ width: 100, display: "flex", flexDirection: "column", alignItems: "stretch", background: "linear-gradient(180deg,#141619 0%,#0F1114 100%)", borderLeft: "1px solid #2A2D35", borderRight: "1px solid #2A2D35", flexShrink: 0, overflow: "hidden" }}>
            {/* Quick Play */}
            <button onClick={() => { if (activeScene) { toast.success(`Quick Play: ${activeScene.name}`); } else { toast.error("No scene selected"); } }}
              style={{ margin: "8px 6px 4px", padding: "6px 0", background: "linear-gradient(180deg,#2A2D35,#1E2128)", border: "1px solid #4A4D55", borderRadius: 3, color: "#D0D2D8", fontSize: 11, fontWeight: 600, cursor: "pointer", fontFamily: "'DM Sans',sans-serif", boxShadow: "0 2px 6px rgba(0,0,0,0.5), inset 0 1px 0 rgba(255,255,255,0.08)" }}>
              Quick Play
            </button>
            {/* Transition buttons */}
            {[
              { label: "Cut",      color: "#E0E2E8" },
              { label: "Fade",     color: "#E0E2E8" },
              { label: "Merge",    color: "#E0E2E8" },
              { label: "Wipe",     color: "#E0E2E8" },
              { label: "CubeZoom",color: "#E0E2E8" },
              { label: "FTB",      color: "#E0E2E8" },
            ].map(({ label, color }) => (
              <div key={label} style={{ display: "flex", alignItems: "stretch", margin: "2px 6px", gap: 2 }}>
                <button
                  onClick={() => { setActiveTransition(label); toast.success(`Transition: ${label}`); }}
                  style={{ flex: 1, padding: "7px 0", background: activeTransition === label ? "linear-gradient(180deg,#3A6AFF,#2A50CC)" : "linear-gradient(180deg,#2A2D35,#1E2128)", border: `1px solid ${activeTransition === label ? "#5A8AFF" : "#3A3D45"}`, borderRadius: "3px 0 0 3px", color: activeTransition === label ? "#fff" : "#C0C2C8", fontSize: 11, fontWeight: activeTransition === label ? 700 : 500, cursor: "pointer", fontFamily: "'DM Sans',sans-serif", boxShadow: activeTransition === label ? "0 0 12px rgba(58,106,255,0.4), inset 0 1px 0 rgba(255,255,255,0.15)" : "0 1px 4px rgba(0,0,0,0.4), inset 0 1px 0 rgba(255,255,255,0.05)", transition: "all 0.12s" }}>
                  {label}
                </button>
                <button onClick={() => toast.info(`${label} settings`)}
                  style={{ width: 18, padding: 0, background: "linear-gradient(180deg,#222530,#181B22)", border: "1px solid #3A3D45", borderLeft: "none", borderRadius: "0 3px 3px 0", color: "#606878", fontSize: 10, cursor: "pointer", display: "flex", alignItems: "center", justifyContent: "center", boxShadow: "0 1px 4px rgba(0,0,0,0.4)" }}>
                  ▾
                </button>
              </div>
            ))}
            {/* Scene number buttons */}
            <div style={{ margin: "6px 6px 2px", display: "grid", gridTemplateColumns: "repeat(4,1fr)", gap: 2 }}>
              {[1,2,3,4,5,6,7,8].map(n => (
                <button key={n}
                  onClick={() => { const sc = scenes[n-1]; if (sc) { setActiveSceneId(sc.id); setSelectedSourceId(null); } }}
                  style={{ padding: "4px 0", background: scenes[n-1]?.id === activeSceneId ? "linear-gradient(180deg,#3A6AFF,#2A50CC)" : "linear-gradient(180deg,#2A2D35,#1E2128)", border: `1px solid ${scenes[n-1]?.id === activeSceneId ? "#5A8AFF" : "#3A3D45"}`, borderRadius: 3, color: scenes[n-1]?.id === activeSceneId ? "#fff" : scenes[n-1] ? "#C0C2C8" : "#404450", fontSize: 11, fontWeight: 700, cursor: scenes[n-1] ? "pointer" : "default", fontFamily: "'JetBrains Mono',monospace", boxShadow: scenes[n-1]?.id === activeSceneId ? "0 0 8px rgba(58,106,255,0.5)" : "0 1px 3px rgba(0,0,0,0.4)" }}>
                  {n}
                </button>
              ))}
            </div>
            {/* Transition speed fader */}
            <div style={{ margin: "4px 6px", display: "flex", flexDirection: "column", gap: 3 }}>
              <span style={{ fontFamily: "'DM Sans',sans-serif", fontSize: 9, color: "#606878", textAlign: "center" }}>Speed</span>
              <input type="range" min={100} max={2000} value={transitionSpeed} onChange={e => setTransitionSpeed(Number(e.target.value))}
                style={{ width: "100%", accentColor: "#3A6AFF", cursor: "pointer", height: 3 }} />
            </div>
          </div>

          {/* ── PROGRAM monitor (right) ── */}
          <div style={{ flex: 1, display: "flex", flexDirection: "column", overflow: "hidden", background: "#0D0F12" }}>
            {/* Program label bar */}
            <div style={{ display: "flex", alignItems: "center", justifyContent: "space-between", padding: "3px 10px", background: "linear-gradient(180deg,#1A1D22,#141619)", borderBottom: "1px solid #2A2D35", flexShrink: 0 }}>
              <span style={{ fontFamily: "'DM Sans',sans-serif", fontWeight: 700, fontSize: 11, color: isLive ? "#FF5A2C" : "#FF5A2C80", letterSpacing: "0.06em" }}>PROGRAM</span>
              <div style={{ display: "flex", alignItems: "center", gap: 6 }}>
                {isLive && <span style={{ fontFamily: "'JetBrains Mono',monospace", fontSize: 10, color: "#FF5A2C" }}>● LIVE</span>}
                <span style={{ fontFamily: "'JetBrains Mono',monospace", fontSize: 10, color: "#A0A8B8" }}>{scenes.find(s => s.id === programSceneId)?.name ?? "No Scene"}</span>
                <span style={{ fontFamily: "'JetBrains Mono',monospace", fontSize: 10, color: "#606878" }}>{tc}</span>
                {programSceneId !== null && (
                  <button onClick={() => setProgramSceneId(null)} title="Clear Program output"
                    style={{ padding: "1px 7px", background: "none", border: "1px solid #FF5A2C40", borderRadius: 2, color: "#FF5A2C70", fontSize: 9, fontWeight: 700, cursor: "pointer", fontFamily: "'DM Sans',sans-serif", letterSpacing: "0.06em" }}>
                    CLEAR
                  </button>
                )}
              </div>
            </div>
            {/* Program canvas */}
            <div style={{ flex: 1, display: "flex", alignItems: "center", justifyContent: "center", background: "#080A0D", position: "relative", overflow: "hidden" }}>
              <div style={{ position: "relative", width: "100%", maxWidth: "100%", aspectRatio: "16/9", background: "#000", border: `2px solid ${isLive ? "#FF5A2C50" : "#FF5A2C20"}`, overflow: "hidden", boxShadow: isLive ? "0 0 30px rgba(255,90,44,0.15)" : "none", transition: "box-shadow 0.3s, border-color 0.3s" }}>
                {programSceneId === null ? (
                  <div style={{ position: "absolute", inset: 0, display: "flex", flexDirection: "column", alignItems: "center", justifyContent: "center", gap: 8 }}>
                    <Monitor size={32} style={{ color: "#2A3550", opacity: 0.4 }} />
                    <span style={{ fontSize: 11, color: "#3A4560", letterSpacing: "0.06em", textTransform: "uppercase" }}>No Output</span>
                  </div>
                ) : (() => {
                  const transStyle: React.CSSProperties = transitionPhase === "out"
                    ? activeTransition === "Fade" || activeTransition === "FTB"
                      ? { opacity: 0, transition: `opacity ${transitionSpeed/2}ms ease-in-out` }
                      : activeTransition === "Slide"
                      ? { transform: "translateX(-100%)", transition: `transform ${transitionSpeed/2}ms ease-in-out` }
                      : activeTransition === "Wipe"
                      ? { clipPath: "inset(0 100% 0 0)", transition: `clip-path ${transitionSpeed/2}ms ease-in-out` }
                      : activeTransition === "CubeZoom"
                      ? { transform: "scale(0.8)", opacity: 0, transition: `all ${transitionSpeed/2}ms ease-in-out` }
                      : {}
                    : transitionPhase === "in"
                    ? activeTransition === "Fade" || activeTransition === "FTB"
                      ? { opacity: 1, transition: `opacity ${transitionSpeed/2}ms ease-in-out` }
                      : activeTransition === "Slide"
                      ? { transform: "translateX(0)", transition: `transform ${transitionSpeed/2}ms ease-in-out`, animation: `slideInRight ${transitionSpeed/2}ms ease-out` }
                      : activeTransition === "Wipe"
                      ? { clipPath: "inset(0 0% 0 0)", transition: `clip-path ${transitionSpeed/2}ms ease-in-out` }
                      : activeTransition === "CubeZoom"
                      ? { transform: "scale(1)", opacity: 1, transition: `all ${transitionSpeed/2}ms ease-in-out` }
                      : {}
                    : {};
                  return (
                    <div style={{ position: "absolute", inset: 0, ...transStyle }}>
                      <ProgramCanvas
                        sources={programSources}
                        selectedId={null}
                        transforms={canvasTransforms}
                        onSelect={() => {}}
                        onTransformChange={handleCanvasTransformChange}
                      />
                    </div>
                  );
                })()}
                {/* ON AIR badge */}
                {isLive && <div style={{ position: "absolute", top: 6, left: 6, padding: "1px 6px", background: "#FF5A2C", borderRadius: 2, fontFamily: "'DM Sans',sans-serif", fontWeight: 700, fontSize: 9, color: "#fff", letterSpacing: "0.08em", pointerEvents: "none", animation: "pulse 1.5s infinite" }}>ON AIR</div>}
                {/* Corner markers */}
                {(["tl","tr","bl","br"] as const).map(c => (
                  <div key={c} style={{ position: "absolute", width: 14, height: 14, pointerEvents: "none",
                    top: c[0]==="t" ? 4 : "auto", bottom: c[0]==="b" ? 4 : "auto",
                    left: c[1]==="l" ? 4 : "auto", right: c[1]==="r" ? 4 : "auto",
                    borderTop: c[0]==="t" ? `1px solid ${isLive ? "rgba(255,90,44,0.5)" : "rgba(79,158,255,0.3)"}` : "none",
                    borderBottom: c[0]==="b" ? `1px solid ${isLive ? "rgba(255,90,44,0.5)" : "rgba(79,158,255,0.3)"}` : "none",
                    borderLeft: c[1]==="l" ? `1px solid ${isLive ? "rgba(255,90,44,0.5)" : "rgba(79,158,255,0.3)"}` : "none",
                    borderRight: c[1]==="r" ? `1px solid ${isLive ? "rgba(255,90,44,0.5)" : "rgba(79,158,255,0.3)"}` : "none",
                  }} />
                ))}
              </div>
            </div>
          </div>
        </div>

        {/* ── INPUT TILES ROW ── */}
        <div style={{ display: "flex", alignItems: "stretch", background: "#0D0F12", borderTop: "1px solid #2A2D35", flexShrink: 0, minHeight: 0, overflow: "hidden" }}>
          {/* Color filter dots (vMix-style) */}
          <div style={{ display: "flex", alignItems: "center", gap: 4, padding: "0 8px", borderRight: "1px solid #2A2D35", flexShrink: 0 }}>
            {["#EF4444","#F97316","#EAB308","#22C55E","#3B82F6","#A855F7"].map(c => (
              <div key={c} onClick={() => setActiveColorFilter(activeColorFilter === c ? null : c)} style={{ width: 12, height: 12, borderRadius: "50%", background: c, cursor: "pointer", boxShadow: activeColorFilter === c ? `0 0 10px ${c}, 0 0 20px ${c}80` : `0 0 6px ${c}60`, outline: activeColorFilter === c ? `2px solid ${c}` : "none", outlineOffset: 2, transition: "all 0.15s" }} />
            ))}
            <Search size={12} style={{ color: "#606878", cursor: "pointer", marginLeft: 2 }} />
          </div>
          {/* Input tiles */}
          <div style={{ display: "flex", flex: 1, overflow: "hidden", gap: 0 }}>
            {scenes.length === 0 ? (
              <div style={{ display: "flex", alignItems: "center", justifyContent: "center", flex: 1, padding: "12px 20px" }}>
                <span style={{ fontSize: 10, color: "#404450", letterSpacing: "0.06em" }}>No inputs — click Add Input to begin</span>
              </div>
            ) : scenes.map((scene, idx) => {
              const isPreview = scene.id === previewSceneId;
              const isProgram = scene.id === programSceneId;
              return (
               <div key={scene.id} style={{ display: "flex", flexDirection: "column", minWidth: 160, maxWidth: 200, flex: "1 1 160px", borderRight: "1px solid #2A2D35", position: "relative", background: isProgram ? "#1A0A0A" : isPreview ? "#0A150A" : "#0D0F12", outline: isProgram ? "2px solid #FF5A2C60" : isPreview ? "2px solid #22C55E60" : "none", outlineOffset: -2 }}>
                 {/* Tile header */}
                 <div style={{ display: "flex", alignItems: "center", justifyContent: "space-between", padding: "2px 6px", background: isProgram ? "linear-gradient(90deg,#FF5A2C,#CC3A14)" : isPreview ? "linear-gradient(90deg,#22C55E,#16A34A)" : "linear-gradient(180deg,#1E2128,#181B22)", borderBottom: "1px solid #2A2D35", flexShrink: 0 }}>
                   <span style={{ fontFamily: "'DM Sans',sans-serif", fontWeight: 700, fontSize: 10, color: isProgram || isPreview ? "#000" : "#C0C2C8", overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap", maxWidth: 110 }}>
                     {idx + 1} {scene.name}
                   </span>
                    <div style={{ display: "flex", alignItems: "center", gap: 2, flexShrink: 0 }}>
                      {/* Gear icon → opens Input Settings drawer */}
                      <button
                        onClick={e => { e.stopPropagation(); handleOpenInputSettings(scene.id); }}
                        title="Input Settings"
                        style={{ background: "none", border: "none", color: isProgram || isPreview ? "#00000080" : "#4F9EFF80", cursor: "pointer", display: "flex", padding: 1, transition: "color 0.15s" }}
                        onMouseEnter={e => (e.currentTarget.style.color = isProgram || isPreview ? "#000" : "#4F9EFF")}
                        onMouseLeave={e => (e.currentTarget.style.color = isProgram || isPreview ? "#00000080" : "#4F9EFF80")}
                      >
                        <Settings size={10} />
                      </button>
                      {/* Delete button */}
                      <button onClick={() => { const sc = scenes.find(s => s.id !== scene.id); if (sc) setActiveSceneId(sc.id); deleteScene(scene.id); }}
                        style={{ background: "none", border: "none", color: isProgram ? "#00000080" : "#606878", cursor: "pointer", display: "flex", padding: 1 }}>
                        <X size={10} />
                      </button>
                    </div>
                 </div>
                 {/* Tile preview — right-click for context menu */}
                  <ContextMenu>
                    <ContextMenuTrigger asChild>
                      <div style={{ flex: 1, background: "#000", minHeight: 60, maxHeight: 80, display: "flex", alignItems: "center", justifyContent: "center", position: "relative", cursor: "pointer" }}
                        onClick={() => { setPreviewSceneId(scene.id); setActiveSceneId(scene.id); setSelectedSourceId(null); }}>
                        {scene.sources.length === 0 ? (
                          <span style={{ fontSize: 9, color: "#303540", letterSpacing: "0.04em" }}>Blank</span>
                        ) : (
                          <div style={{ display: "flex", flexWrap: "wrap", gap: 2, padding: 4, alignItems: "center", justifyContent: "center" }}>
                            {scene.sources.slice(0,3).map(src => (
                              <div key={src.id} style={{ display: "flex", alignItems: "center", gap: 2 }}>
                                <src.icon size={10} style={{ color: src.color }} />
                                <span style={{ fontSize: 8, color: "#808898" }}>{src.name}</span>
                              </div>
                            ))}
                            {scene.sources.length > 3 && <span style={{ fontSize: 8, color: "#606878" }}>+{scene.sources.length - 3}</span>}
                          </div>
                        )}
                      </div>
                    </ContextMenuTrigger>
                    <ContextMenuContent style={{ minWidth: 180, background: "#1A1D22", border: "1px solid #3A3D45", borderRadius: 4, padding: "4px 0", boxShadow: "0 8px 32px rgba(0,0,0,0.7)" }}>
                      <ContextMenuLabel style={{ padding: "3px 10px", fontSize: 9, color: "#606878", fontFamily: "'DM Sans',sans-serif", letterSpacing: "0.08em", textTransform: "uppercase" }}>{scene.name}</ContextMenuLabel>
                      <ContextMenuSeparator style={{ height: 1, background: "#2A2D35", margin: "2px 0" }} />
                      {/* Preview / Program quick actions */}
                      <ContextMenuItem onSelect={() => { setPreviewSceneId(scene.id); setActiveSceneId(scene.id); setSelectedSourceId(null); }}
                        style={{ padding: "5px 10px", fontSize: 11, color: "#22C55E", fontFamily: "'DM Sans',sans-serif", cursor: "pointer" }}>
                        → Set as Preview
                      </ContextMenuItem>
                      <ContextMenuSub>
                        <ContextMenuSubTrigger style={{ padding: "5px 10px", fontSize: 11, color: "#D0D2D8", fontFamily: "'DM Sans',sans-serif", cursor: "pointer" }}>
                          Send to Program with…
                        </ContextMenuSubTrigger>
                        <ContextMenuSubContent style={{ minWidth: 140, background: "#1A1D22", border: "1px solid #3A3D45", borderRadius: 4, padding: "4px 0", boxShadow: "0 8px 32px rgba(0,0,0,0.7)" }}>
                          {["Cut","Fade","Merge","Wipe","CubeZoom","FTB"].map(t => (
                            <ContextMenuItem key={t} onSelect={() => {
                              const prev = activeTransition;
                              setActiveTransition(t);
                              handleGo(scene.id);
                              toast.success(`${t} → ${scene.name}`);
                              setTimeout(() => setActiveTransition(prev), 100);
                            }}
                              style={{ padding: "5px 10px", fontSize: 11, color: "#D0D2D8", fontFamily: "'DM Sans',sans-serif", cursor: "pointer" }}>
                              {t}
                            </ContextMenuItem>
                          ))}
                        </ContextMenuSubContent>
                      </ContextMenuSub>
                      <ContextMenuSeparator style={{ height: 1, background: "#2A2D35", margin: "2px 0" }} />
                      {/* Settings */}
                      <ContextMenuItem onSelect={() => handleOpenInputSettings(scene.id)}
                        style={{ padding: "5px 10px", fontSize: 11, color: "#4F9EFF", fontFamily: "'DM Sans',sans-serif", cursor: "pointer" }}>
                        ⚙ Input Settings…
                      </ContextMenuItem>
                      <ContextMenuItem onSelect={() => duplicateScene(scene)}
                        style={{ padding: "5px 10px", fontSize: 11, color: "#D0D2D8", fontFamily: "'DM Sans',sans-serif", cursor: "pointer" }}>
                        Duplicate
                      </ContextMenuItem>
                      <ContextMenuSeparator style={{ height: 1, background: "#2A2D35", margin: "2px 0" }} />
                      <ContextMenuItem onSelect={() => { const sc = scenes.find(s => s.id !== scene.id); if (sc) setActiveSceneId(sc.id); deleteScene(scene.id); }}
                        style={{ padding: "5px 10px", fontSize: 11, color: "#EF4444", fontFamily: "'DM Sans',sans-serif", cursor: "pointer" }}>
                        Delete Input
                      </ContextMenuItem>
                    </ContextMenuContent>
                  </ContextMenu>
                 {/* Tile controls */}
                  <div style={{ display: "flex", alignItems: "center", gap: 2, padding: "3px 4px", borderTop: "1px solid #2A2D35", background: "#0D0F12", flexShrink: 0 }}>
                    {[1,2,3,4].map(n => (
                      <button key={n} style={{ width: 16, height: 16, padding: 0, background: "linear-gradient(180deg,#2A2D35,#1E2128)", border: "1px solid #3A3D45", borderRadius: 2, color: "#808898", fontSize: 9, fontWeight: 700, cursor: "pointer", fontFamily: "'JetBrains Mono',monospace" }}>{n}</button>
                    ))}
                    <button onClick={() => { handleGo(scene.id); toast.success(`${activeTransition} → ${scene.name}`); }}
                      disabled={isTransitioning}
                      style={{ padding: "2px 6px", background: isTransitioning ? "linear-gradient(180deg,#1A3A1A,#122A12)" : "linear-gradient(180deg,#22C55E,#16A34A)", border: "1px solid #22C55E80", borderRadius: 2, color: isTransitioning ? "#22C55E60" : "#000", fontSize: 9, fontWeight: 700, cursor: isTransitioning ? "not-allowed" : "pointer", fontFamily: "'DM Sans',sans-serif", boxShadow: isTransitioning ? "none" : "0 0 8px rgba(34,197,94,0.3)", marginLeft: "auto", transition: "all 0.15s" }}>
                      {isTransitioning ? "..." : "GO"}
                    </button>
                    <button onClick={() => { setProgramSceneId(scene.id); setActiveSceneId(scene.id); setSelectedSourceId(null); toast.success(`Cut → ${scene.name}`); }}
                      style={{ padding: "2px 6px", background: "linear-gradient(180deg,#2A2D35,#1E2128)", border: "1px solid #3A3D45", borderRadius: 2, color: "#C0C2C8", fontSize: 9, fontWeight: 700, cursor: "pointer", fontFamily: "'DM Sans',sans-serif" }}>
                      Cut
                    </button>
                  </div>
                </div>
              );
            })}
          </div>
          {/* Audio Mixer — slides in horizontally from the right inside the tiles row */}
          <div style={{
            display: "flex", overflow: "hidden", flexShrink: 0,
            maxWidth: audioMixerOpen ? 320 : 0,
            transition: "max-width 0.25s cubic-bezier(0.23,1,0.32,1)",
            borderLeft: audioMixerOpen ? "1px solid #2A2D35" : "none",
            background: "#0D0F12",
          }}>
            {/* OUTPUTS label */}
            <div style={{ width: 20, display: "flex", alignItems: "center", justifyContent: "center", background: "#1A3AFF15", borderRight: "1px solid #2A2D35", flexShrink: 0 }}>
              <span style={{ fontFamily: "'DM Sans',sans-serif", fontSize: 8, fontWeight: 700, color: "#3A6AFF", letterSpacing: "0.1em", writingMode: "vertical-rl", transform: "rotate(180deg)", textTransform: "uppercase" }}>OUTPUTS</span>
            </div>
            {/* Master fader */}
            <div style={{ width: 60, display: "flex", flexDirection: "column", alignItems: "center", gap: 3, padding: "6px 4px", borderRight: "1px solid #2A2D35", flexShrink: 0 }}>
              <div style={{ padding: "1px 6px", background: "linear-gradient(180deg,#22C55E,#16A34A)", borderRadius: 2, fontFamily: "'DM Sans',sans-serif", fontWeight: 700, fontSize: 9, color: "#000", letterSpacing: "0.06em", whiteSpace: "nowrap" }}>Master</div>
              <button style={{ width: 22, height: 22, background: "linear-gradient(180deg,#2A2D35,#1E2128)", border: "1px solid #4A4D55", borderRadius: 3, color: "#808898", cursor: "pointer", display: "flex", alignItems: "center", justifyContent: "center", flexShrink: 0 }}>
                <Cpu size={9} />
              </button>
              <div style={{ flex: 1, display: "flex", alignItems: "center", justifyContent: "center", width: "100%", minHeight: 0 }}>
                <div style={{ width: 6, height: "100%", background: "#1E2128", border: "1px solid #3A3D45", borderRadius: 3, position: "relative" }}>
                  <div style={{ position: "absolute", bottom: "30%", left: -7, right: -7, height: 12, background: "linear-gradient(180deg,#4A4D55,#2A2D35)", border: "1px solid #5A5D65", borderRadius: 2, cursor: "pointer" }} />
                </div>
              </div>
              <div style={{ display: "flex", gap: 3, flexShrink: 0 }}>
                <button style={{ width: 18, height: 18, background: "linear-gradient(180deg,#22C55E,#16A34A)", border: "1px solid #22C55E80", borderRadius: 2, color: "#000", cursor: "pointer", display: "flex", alignItems: "center", justifyContent: "center" }}><Volume2 size={9} /></button>
                <button style={{ width: 18, height: 18, background: "linear-gradient(180deg,#2A2D35,#1E2128)", border: "1px solid #4A4D55", borderRadius: 2, color: "#808898", cursor: "pointer", display: "flex", alignItems: "center", justifyContent: "center" }}><Mic size={9} /></button>
              </div>
            </div>
            {/* Channel strips */}
            <div style={{ display: "flex", overflowX: "auto", minWidth: 0 }}>
              {audioChannels.map(ch => {
                const cs = channelState[ch.id] ?? { muted: false, solo: false, volume: 75 };
                return (
                  <div key={ch.id} style={{ minWidth: 52, display: "flex", flexDirection: "column", alignItems: "center", gap: 2, padding: "5px 3px", borderRight: "1px solid #2A2D35", opacity: cs.muted ? 0.4 : 1, transition: "opacity 0.15s", flexShrink: 0 }}>
                    <VUMeterVertical color={ch.color} active={isLive && !cs.muted} volume={cs.volume} />
                    <input type="range" min={0} max={100} value={cs.volume}
                      onChange={e => setChannelState(p => ({ ...p, [ch.id]: { ...cs, volume: Number(e.target.value) } }))}
                      style={{ width: "100%", accentColor: ch.color, cursor: "pointer", height: 3 }} />
                    <span style={{ fontFamily: "'DM Sans',sans-serif", fontSize: 8, color: "#808898", overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap", maxWidth: 46, textAlign: "center" }}>{ch.name}</span>
                    <div style={{ display: "flex", gap: 2 }}>
                      <button onClick={() => setChannelState(p => ({ ...p, [ch.id]: { ...cs, solo: !cs.solo } }))}
                        style={{ width: 15, height: 15, borderRadius: 2, border: `1px solid ${cs.solo ? "#FBBF24" : "#3A3D45"}`, background: cs.solo ? "#FBBF24" : "#1E2128", color: cs.solo ? "#000" : "#606878", fontSize: 8, fontWeight: 700, cursor: "pointer", display: "flex", alignItems: "center", justifyContent: "center" }}>S</button>
                      <button onClick={() => setChannelState(p => ({ ...p, [ch.id]: { ...cs, muted: !cs.muted } }))}
                        style={{ width: 15, height: 15, borderRadius: 2, border: `1px solid ${cs.muted ? "#EF4444" : "#3A3D45"}`, background: cs.muted ? "#EF4444" : "#1E2128", color: cs.muted ? "#fff" : "#606878", fontSize: 8, fontWeight: 700, cursor: "pointer", display: "flex", alignItems: "center", justifyContent: "center" }}>M</button>
                    </div>
                  </div>
                );
              })}
            </div>
            {/* INPUTS label */}
            <div style={{ width: 20, display: "flex", alignItems: "center", justifyContent: "center", background: "#1A3AFF15", borderLeft: "1px solid #2A2D35", flexShrink: 0 }}>
              <span style={{ fontFamily: "'DM Sans',sans-serif", fontSize: 8, fontWeight: 700, color: "#3A6AFF", letterSpacing: "0.1em", writingMode: "vertical-rl", textTransform: "uppercase" }}>INPUTS</span>
            </div>
          </div>
        </div>

        {/* ── BOTTOM ACTION TOOLBAR ── */}
        <div style={{ display: "flex", alignItems: "center", gap: 2, padding: "4px 8px", background: "linear-gradient(180deg,#141619,#0F1114)", borderTop: "1px solid #2A2D35", flexShrink: 0 }}>
          {/* Add Input */}
          <div style={{ display: "flex", alignItems: "stretch", marginRight: 4 }}>
            <button onClick={() => setShowAddSource(true)}
              style={{ padding: "5px 12px", background: "linear-gradient(180deg,#2A2D35,#1E2128)", border: "1px solid #4A4D55", borderRadius: "3px 0 0 3px", color: "#D0D2D8", fontSize: 11, fontWeight: 600, cursor: "pointer", fontFamily: "'DM Sans',sans-serif", boxShadow: "0 2px 6px rgba(0,0,0,0.5), inset 0 1px 0 rgba(255,255,255,0.08)" }}>
              Add Input
            </button>
            <button style={{ width: 18, padding: 0, background: "linear-gradient(180deg,#222530,#181B22)", border: "1px solid #4A4D55", borderLeft: "none", borderRadius: "0 3px 3px 0", color: "#808898", fontSize: 10, cursor: "pointer", display: "flex", alignItems: "center", justifyContent: "center" }}>▾</button>
          </div>
          {/* Settings */}
          <button style={{ width: 28, height: 28, padding: 0, background: "linear-gradient(180deg,#2A2D35,#1E2128)", border: "1px solid #4A4D55", borderRadius: 3, color: "#808898", cursor: "pointer", display: "flex", alignItems: "center", justifyContent: "center", boxShadow: "0 1px 4px rgba(0,0,0,0.4)" }}>
            <Cpu size={13} />
          </button>
          {/* Record */}
          <div style={{ display: "flex", alignItems: "stretch", marginLeft: 4 }}>
            <button onClick={() => toast.info("Record — coming soon")}
              style={{ padding: "5px 12px", background: "linear-gradient(180deg,#2A2D35,#1E2128)", border: "1px solid #4A4D55", borderRadius: "3px 0 0 3px", color: "#D0D2D8", fontSize: 11, fontWeight: 600, cursor: "pointer", fontFamily: "'DM Sans',sans-serif", boxShadow: "0 2px 6px rgba(0,0,0,0.5), inset 0 1px 0 rgba(255,255,255,0.08)" }}>
              Record
            </button>
            <button style={{ width: 18, padding: 0, background: "linear-gradient(180deg,#222530,#181B22)", border: "1px solid #4A4D55", borderLeft: "none", borderRadius: "0 3px 3px 0", color: "#808898", fontSize: 10, cursor: "pointer", display: "flex", alignItems: "center", justifyContent: "center" }}>▾</button>
          </div>
          {/* External */}
          <div style={{ display: "flex", alignItems: "stretch", marginLeft: 4 }}>
            <button onClick={() => toast.info("External output — coming soon")}
              style={{ padding: "5px 12px", background: "linear-gradient(180deg,#2A2D35,#1E2128)", border: "1px solid #4A4D55", borderRadius: "3px 0 0 3px", color: "#D0D2D8", fontSize: 11, fontWeight: 600, cursor: "pointer", fontFamily: "'DM Sans',sans-serif", boxShadow: "0 2px 6px rgba(0,0,0,0.5), inset 0 1px 0 rgba(255,255,255,0.08)" }}>
              External
            </button>
            <button style={{ width: 18, padding: 0, background: "linear-gradient(180deg,#222530,#181B22)", border: "1px solid #4A4D55", borderLeft: "none", borderRadius: "0 3px 3px 0", color: "#808898", fontSize: 10, cursor: "pointer", display: "flex", alignItems: "center", justifyContent: "center" }}>▾</button>
          </div>
          {/* Stream */}
          <div style={{ display: "flex", alignItems: "stretch", marginLeft: 4 }}>
            {!isLive ? (
              <>
                <button onClick={() => setShowGoLive(true)}
                  style={{ padding: "5px 12px", background: "linear-gradient(180deg,#FF5A2C,#CC3A18)", border: "1px solid #FF5A2C80", borderRadius: "3px 0 0 3px", color: "#fff", fontSize: 11, fontWeight: 700, cursor: "pointer", fontFamily: "'DM Sans',sans-serif", boxShadow: "0 0 16px rgba(255,90,44,0.4), 0 2px 6px rgba(0,0,0,0.5), inset 0 1px 0 rgba(255,255,255,0.15)" }}>
                  ● Stream
                </button>
                <button style={{ width: 18, padding: 0, background: "linear-gradient(180deg,#CC3A18,#AA2A10)", border: "1px solid #FF5A2C80", borderLeft: "none", borderRadius: "0 3px 3px 0", color: "#fff", fontSize: 10, cursor: "pointer", display: "flex", alignItems: "center", justifyContent: "center" }}>▾</button>
              </>
            ) : (
              <button onClick={() => { setIsLive(false); setLivePlatform(""); setTc("00:00:00"); setViewers(0); setBitrate(0); }}
                style={{ padding: "5px 12px", background: "linear-gradient(180deg,#7F1D1D,#5A1010)", border: "1px solid #EF444440", borderRadius: 3, color: "#FCA5A5", fontSize: 11, fontWeight: 700, cursor: "pointer", fontFamily: "'DM Sans',sans-serif" }}>
                ■ End Stream
              </button>
            )}
          </div>
          {/* MultiCorder */}
          <div style={{ display: "flex", alignItems: "stretch", marginLeft: 4 }}>
            <button onClick={() => toast.info("MultiCorder — coming soon")}
              style={{ padding: "5px 12px", background: "linear-gradient(180deg,#2A2D35,#1E2128)", border: "1px solid #4A4D55", borderRadius: "3px 0 0 3px", color: "#D0D2D8", fontSize: 11, fontWeight: 600, cursor: "pointer", fontFamily: "'DM Sans',sans-serif", boxShadow: "0 2px 6px rgba(0,0,0,0.5), inset 0 1px 0 rgba(255,255,255,0.08)" }}>
              MultiCorder
            </button>
            <button style={{ width: 18, padding: 0, background: "linear-gradient(180deg,#222530,#181B22)", border: "1px solid #4A4D55", borderLeft: "none", borderRadius: "0 3px 3px 0", color: "#808898", fontSize: 10, cursor: "pointer", display: "flex", alignItems: "center", justifyContent: "center" }}>▾</button>
          </div>
          {/* PlayList */}
          <button onClick={() => toast.info("PlayList — coming soon")}
            style={{ marginLeft: 4, padding: "5px 12px", background: "linear-gradient(180deg,#2A2D35,#1E2128)", border: "1px solid #4A4D55", borderRadius: 3, color: "#D0D2D8", fontSize: 11, fontWeight: 600, cursor: "pointer", fontFamily: "'DM Sans',sans-serif", boxShadow: "0 2px 6px rgba(0,0,0,0.5), inset 0 1px 0 rgba(255,255,255,0.08)" }}>
            PlayList
          </button>
          <div style={{ flex: 1 }} />
          {/* Overlay / view mode icons */}
          <button onClick={() => toast.info("Overlay — coming soon")}
            style={{ padding: "5px 12px", background: "linear-gradient(180deg,#2A2D35,#1E2128)", border: "1px solid #4A4D55", borderRadius: 3, color: "#D0D2D8", fontSize: 11, fontWeight: 600, cursor: "pointer", fontFamily: "'DM Sans',sans-serif", boxShadow: "0 2px 6px rgba(0,0,0,0.5)" }}>
            Overlay
          </button>
          {[<Square size={12} />, <Activity size={12} />, <LayoutTemplate size={12} />, <Cpu size={12} />, <Lock size={12} />].map((icon, i) => (
            <button key={i} style={{ width: 28, height: 28, padding: 0, background: "linear-gradient(180deg,#1E2128,#16181E)", border: "1px solid #3A3D45", borderRadius: 3, color: "#606878", cursor: "pointer", display: "flex", alignItems: "center", justifyContent: "center", marginLeft: 2 }}>
              {icon}
            </button>
          ))}
          {/* Audio Mixer toggle */}
          <button onClick={() => setAudioMixerOpen(v => !v)}
            style={{ marginLeft: 8, padding: "5px 12px", background: audioMixerOpen ? "linear-gradient(180deg,#1A3AFF,#1230CC)" : "linear-gradient(180deg,#1E2128,#16181E)", border: `1px solid ${audioMixerOpen ? "#3A6AFF" : "#3A3D45"}`, borderRadius: 3, color: audioMixerOpen ? "#fff" : "#C0C2C8", fontSize: 11, fontWeight: 600, cursor: "pointer", fontFamily: "'DM Sans',sans-serif", boxShadow: audioMixerOpen ? "0 0 10px rgba(58,106,255,0.4), inset 0 1px 0 rgba(255,255,255,0.15)" : "0 2px 6px rgba(0,0,0,0.5)", display: "flex", alignItems: "center", gap: 5, transition: "all 0.15s" }}>
            <Volume2 size={12} />
            Audio Mixer
          </button>
          {/* Status bar */}
          <div style={{ marginLeft: 8, display: "flex", alignItems: "center", gap: 8, padding: "2px 8px", background: "#0A0C0F", border: "1px solid #2A2D35", borderRadius: 3 }}>
            <span style={{ fontFamily: "'JetBrains Mono',monospace", fontSize: 9, color: "#22C55E" }}>1080p29.97</span>
            <span style={{ fontFamily: "'JetBrains Mono',monospace", fontSize: 9, color: "#606878" }}>EX FPS: 30</span>
            <span style={{ fontFamily: "'JetBrains Mono',monospace", fontSize: 9, color: "#606878" }}>Render: 1ms</span>
            <span style={{ fontFamily: "'JetBrains Mono',monospace", fontSize: 9, color: "#606878" }}>GPU: 2%</span>
            <span style={{ fontFamily: "'JetBrains Mono',monospace", fontSize: 9, color: "#606878" }}>CPU: {isLive ? "12%" : "3%"}</span>
            <span style={{ fontFamily: "'JetBrains Mono',monospace", fontSize: 9, color: isLive ? "#FF5A2C" : "#606878" }}>Total: {isLive ? "63%" : "8%"}</span>
          </div>
        </div>
      </div>

      {/* Modals */}
      {showAddSource && <AddSourceModal onAdd={handleAddSource} onClose={() => setShowAddSource(false)} />}
      <GoLiveModal open={showGoLive} onClose={() => setShowGoLive(false)}
        onGoLive={(cfg: { platform: string }) => { setIsLive(true); setLivePlatform(cfg.platform); setShowGoLive(false); }} />
      {/* Input Settings Drawer */}
      <InputSettingsDrawer
        scene={inputSettingsScene}
        open={inputSettingsSceneId !== null}
        onClose={() => setInputSettingsSceneId(null)}
        onSave={handleSaveInputSettings}
        initialSettings={inputSettingsSceneId !== null ? inputSettingsMap[inputSettingsSceneId] : undefined}
      />
    </AppSidebar>
  );
}
