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
  LayoutTemplate, Activity, Cpu, Users, Clock,
} from "lucide-react";
import { ContextMenu, ContextMenuTrigger, ContextMenuContent, ContextMenuItem, ContextMenuSeparator } from "@/components/ui/context-menu";


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

  // Overlay browser
  const [overlayBrowserOpen, setOverlayBrowserOpen] = useState(false);
  const [overlaySearch, setOverlaySearch]           = useState("");

  // Audio mixer
  const [channelState, setChannelState] = useState<Record<number, { muted: boolean; solo: boolean; volume: number }>>({});

  // Active scene
  const activeScene  = useMemo(() => scenes.find(s => s.id === activeSceneId) ?? null, [scenes, activeSceneId]);
  const sources      = activeScene?.sources ?? [];
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
      <div style={{ display: "flex", flexDirection: "column", height: "100vh", overflow: "hidden", background: "#080E1A" }}>

        {/* ── Top bar ── */}
        <div className="flex items-center gap-3 px-3 shrink-0" style={{ height: 44, background: "linear-gradient(90deg, #0A1020 0%, #111828 50%, #0A1020 100%)", borderBottom: "1px solid rgba(79,158,255,0.22)", boxShadow: "0 1px 0 rgba(79,158,255,0.12), 0 4px 24px rgba(0,0,0,0.5)" }}>
          <div className="flex items-center gap-1">
            <span style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 17, color: "#F8F8FF", letterSpacing: "0.06em" }}>RAILSHOT</span>
            <span style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 17, color: "#FF5A2C", letterSpacing: "0.06em" }}>TV</span>
          </div>
          <div className="w-px h-4" style={{ background: "#303D5A" }} />
          <span style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 13, color: isLive ? "#4F9EFF" : "#50506A", letterSpacing: "0.04em" }}>{tc}</span>
          <span style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 10, color: "#50506A" }}>1920×1080</span>
          <span style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 10, color: "#50506A" }}>60fps</span>
          <span style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 10, color: "#50506A" }}>H.264</span>
          <div className="flex-1" />
          <div className="flex items-center gap-1.5 px-2 py-1 rounded" style={{ background: isLive ? "#FF5A2C18" : "#1E2640", border: `1px solid ${isLive ? "#FF5A2C40" : "#303D5A"}` }}>
            <div className="w-1.5 h-1.5 rounded-full" style={{ background: isLive ? "#FF5A2C" : "#50506A" }} />
            <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 700, fontSize: 11, color: isLive ? "#FF5A2C" : "#50506A" }}>
              {isLive ? `LIVE · ${livePlatform.toUpperCase()}` : "OFFLINE"}
            </span>
          </div>
          {isLive && <span style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 11, color: "#4F9EFF" }}>{Math.round(bitrate / 100) / 10} Mbps</span>}
          {isLive && (
            <div className="flex items-center gap-1">
              <Users size={11} style={{ color: "#22C55E" }} />
              <span style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 11, color: "#22C55E" }}>{viewers}</span>
            </div>
          )}
          {!isLive ? (
            <button onClick={() => setShowGoLive(true)}
              style={{ height: 30, padding: "0 16px", background: "linear-gradient(135deg, #FF5A2C 0%, #FF8C42 100%)", boxShadow: "0 0 20px rgba(255,90,44,0.55), 0 2px 8px rgba(255,90,44,0.3), inset 0 1px 0 rgba(255,255,255,0.2)", color: "#fff", fontFamily: "'DM Sans', sans-serif", fontSize: 12, fontWeight: 700, letterSpacing: "0.06em", border: "none", borderRadius: 6, cursor: "pointer", transition: "transform 0.12s, box-shadow 0.12s" }}>
              ● GO LIVE
            </button>
          ) : (
            <button onClick={() => { setIsLive(false); setLivePlatform(""); setTc("00:00:00"); setViewers(0); setBitrate(0); }}
              style={{ height: 30, padding: "0 16px", background: "#7F1D1D", border: "1px solid #EF444440", color: "#FCA5A5", fontFamily: "'DM Sans', sans-serif", fontSize: 12, fontWeight: 700, letterSpacing: "0.06em", borderRadius: 6, cursor: "pointer" }}>
              ■ END STREAM
            </button>
          )}
        </div>

        {/* ── Main 3-column layout ── */}
        <div className="flex flex-1 overflow-hidden">

          {/* ── LEFT: Scenes + Sources ── */}
          <div className="flex flex-col shrink-0 overflow-hidden" style={{ width: 220, background: "linear-gradient(180deg, #0C1220 0%, #090E1A 100%)", borderRight: "1px solid rgba(79,158,255,0.18)" }}>

            {/* Scenes section */}
            <div className="flex flex-col shrink-0" style={{ height: "40%", borderBottom: "1px solid rgba(255,255,255,0.06)" }}>
              <div className="flex items-center justify-between px-2 py-1.5 shrink-0" style={{ background: "linear-gradient(90deg, rgba(255,90,44,0.18) 0%, rgba(255,90,44,0.04) 100%)", borderBottom: "1px solid rgba(255,90,44,0.25)" }}>
                <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 700, fontSize: 10, color: "#FF8C60", letterSpacing: "0.12em", textTransform: "uppercase" }}>Scenes</span>
                  <button onClick={() => { handleAddScene(); }}
                  style={{ background: "rgba(79,158,255,0.12)", border: "1px solid rgba(79,158,255,0.3)", borderRadius: 4, cursor: "pointer", color: "#4F9EFF", display: "flex", padding: "2px 4px", boxShadow: "0 0 8px rgba(79,158,255,0.2)" }}>
                  <Plus size={12} />
                </button>
              </div>
              <div className="flex flex-col overflow-y-auto flex-1" style={{ scrollbarWidth: "thin", scrollbarColor: "#303D5A transparent" }}>
                {scenes.length === 0 ? (
                  <div className="flex flex-col items-center justify-center flex-1 gap-1.5 p-3">
                    <Monitor size={16} style={{ color: "#2A3550" }} />
                    <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, textAlign: "center", color: "#3A4A6A", letterSpacing: "0.04em" }}>No scenes yet</span>
                    <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 9, textAlign: "center", color: "#2A3550" }}>Press + to rack your first</span>
                  </div>
                ) : scenes.map(scene => (
                  <ContextMenu key={scene.id}>
                    <ContextMenuTrigger>
                      <div
                        onClick={() => { setActiveSceneId(scene.id); setSelectedSourceId(null); }}
                        onDoubleClick={() => { setRenamingSceneId(scene.id); setRenameValue(scene.name); }}
                        className="flex items-center gap-2 px-2 py-1.5 cursor-pointer"
                        style={{ background: scene.id === activeSceneId ? "linear-gradient(90deg, rgba(255,90,44,0.22) 0%, rgba(255,90,44,0.06) 100%)" : "transparent", borderLeft: `2px solid ${scene.id === activeSceneId ? "#FF6A3C" : "transparent"}`, boxShadow: scene.id === activeSceneId ? "inset 0 0 24px rgba(255,90,44,0.08)" : "none" }}>
                        <Monitor size={12} style={{ color: scene.id === activeSceneId ? "#FF5A2C" : "#606078", flexShrink: 0 }} />
                        {renamingSceneId === scene.id ? (
                          <input autoFocus value={renameValue} onChange={e => setRenameValue(e.target.value)}
                            onBlur={() => { if (renameValue.trim()) renameScene(scene.id, renameValue.trim()); setRenamingSceneId(null); }}
                            onKeyDown={e => { if (e.key === "Enter") { if (renameValue.trim()) renameScene(scene.id, renameValue.trim()); setRenamingSceneId(null); } if (e.key === "Escape") setRenamingSceneId(null); }}
                            onClick={e => e.stopPropagation()}
                            style={{ flex: 1, background: "#0F1520", border: "1px solid #4F9EFF", borderRadius: 3, color: "#F8F8FF", fontFamily: "'DM Sans', sans-serif", fontSize: 12, padding: "1px 4px", outline: "none" }} />
                        ) : (
                          <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 12, color: scene.id === activeSceneId ? "#F8F8FF" : "#8892A4", flex: 1, overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }}>{scene.name}</span>
                        )}
                      </div>
                    </ContextMenuTrigger>
                    <ContextMenuContent style={{ background: "linear-gradient(135deg, #161E30 0%, #111826 100%)", border: "1px solid rgba(79,158,255,0.2)", boxShadow: "0 8px 32px rgba(0,0,0,0.6), 0 0 0 1px rgba(79,158,255,0.06)" }}>
                      <ContextMenuItem onClick={() => { setRenamingSceneId(scene.id); setRenameValue(scene.name); }} style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 12 }}>Rename</ContextMenuItem>
                      <ContextMenuItem onClick={() => duplicateScene(scene)} style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 12 }}>Duplicate</ContextMenuItem>
                      <ContextMenuSeparator />
                      <ContextMenuItem onClick={() => { deleteScene(scene.id); if (activeSceneId === scene.id) setSelectedSourceId(null); }} style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 12, color: "#EF4444" }}>Delete</ContextMenuItem>
                    </ContextMenuContent>
                  </ContextMenu>
                ))}
              </div>
            </div>

            {/* Sources section */}
            <div className="flex flex-col flex-1 overflow-hidden">
              <div className="flex items-center justify-between px-2 py-1.5 shrink-0" style={{ background: "linear-gradient(90deg, rgba(79,158,255,0.18) 0%, rgba(79,158,255,0.04) 100%)", borderBottom: "1px solid rgba(79,158,255,0.25)" }}>
                <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 700, fontSize: 10, color: "#6BAAFF", letterSpacing: "0.12em", textTransform: "uppercase" }}>Sources</span>
                <div className="flex items-center gap-1">
                  <button onClick={() => setOverlayBrowserOpen(v => !v)} title="Overlay Library"
                    style={{ background: overlayBrowserOpen ? "#A855F718" : "none", border: overlayBrowserOpen ? "1px solid #A855F740" : "1px solid transparent", borderRadius: 3, cursor: "pointer", color: overlayBrowserOpen ? "#A855F7" : "#606078", display: "flex", padding: 2 }}>
                    <LayoutTemplate size={12} />
                  </button>
                  <button onClick={() => setShowAddSource(true)} disabled={activeSceneId === null}
                    style={{ background: activeSceneId === null ? "transparent" : "rgba(79,158,255,0.12)", border: `1px solid ${activeSceneId === null ? "transparent" : "rgba(79,158,255,0.3)"}`, borderRadius: 4, cursor: activeSceneId === null ? "not-allowed" : "pointer", color: activeSceneId === null ? "#303D5A" : "#4F9EFF", display: "flex", padding: "2px 4px", boxShadow: activeSceneId === null ? "none" : "0 0 8px rgba(79,158,255,0.2)" }}>
                    <Plus size={12} />
                  </button>
                  <button onClick={handleDeleteSource} disabled={selectedSourceId === null}
                    style={{ background: "none", border: "none", cursor: selectedSourceId === null ? "not-allowed" : "pointer", color: selectedSourceId === null ? "#303D5A" : "#EF4444", display: "flex" }}>
                    <Trash2 size={13} />
                  </button>
                </div>
              </div>
              {/* Overlay browser */}
              {overlayBrowserOpen && (
                <div className="shrink-0 overflow-y-auto" style={{ maxHeight: 180, borderBottom: "1px solid #2A3350", background: "#0F1520" }}>
                  <div className="flex items-center gap-1 px-2 py-1.5" style={{ borderBottom: "1px solid #1E2640" }}>
                    <Search size={10} style={{ color: "#606078" }} />
                    <input value={overlaySearch} onChange={e => setOverlaySearch(e.target.value)} placeholder="Search overlays…"
                      style={{ flex: 1, background: "none", border: "none", outline: "none", color: "#A0A0B8", fontFamily: "'DM Sans', sans-serif", fontSize: 11 }} />
                  </div>
                  {filteredOverlays.map(tmpl => (
                    <div key={tmpl.id}
                      onClick={() => handleAddSource(tmpl.cat, tmpl.name, tmpl.icon, tmpl.color)}
                      className="flex items-center gap-2 px-2 py-1.5 cursor-pointer"
                      style={{ borderBottom: "1px solid #1A1A24" }}
                      onMouseEnter={e => (e.currentTarget as HTMLDivElement).style.background = "#1E2640"}
                      onMouseLeave={e => (e.currentTarget as HTMLDivElement).style.background = "transparent"}>
                      <tmpl.icon size={12} style={{ color: tmpl.color, flexShrink: 0 }} />
                      <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, color: "#A0A0B8" }}>{tmpl.name}</span>
                      <Sparkles size={9} style={{ color: "#A855F7", marginLeft: "auto", flexShrink: 0 }} />
                    </div>
                  ))}
                </div>
              )}
              {/* Source list */}
              <div className="flex flex-col overflow-y-auto flex-1" style={{ scrollbarWidth: "thin", scrollbarColor: "#303D5A transparent" }}>
                {activeSceneId === null ? (
                  <div className="flex flex-col items-center justify-center flex-1 gap-1.5 p-3">
                    <Layers size={14} style={{ color: "#2A3550" }} />
                    <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, textAlign: "center", color: "#3A4A6A" }}>Select a scene</span>
                  </div>
                ) : sources.length === 0 ? (
                  <div className="flex flex-col items-center justify-center flex-1 gap-1.5 p-3">
                    <Layers size={14} style={{ color: "#2A3550" }} />
                    <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, textAlign: "center", color: "#3A4A6A" }}>Scene is empty</span>
                    <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 9, textAlign: "center", color: "#2A3550" }}>Press + to add a source</span>
                  </div>
                ) : sources.map((src, idx) => {
                  const isSelected = src.id === selectedSourceId;
                  return (
                    <ContextMenu key={src.id}>
                      <ContextMenuTrigger asChild>
                        <div
                          onClick={() => setSelectedSourceId(src.id)}
                          className="flex items-center gap-1.5 px-2 py-1.5 cursor-pointer"
                          style={{ background: isSelected ? `linear-gradient(90deg, ${src.color}28 0%, ${src.color}08 100%)` : "transparent", borderLeft: `2px solid ${isSelected ? src.color : "transparent"}`, boxShadow: isSelected ? `inset 0 0 20px ${src.color}12` : "none" }}>
                          <src.icon size={11} style={{ color: src.color, flexShrink: 0 }} />
                          {renamingSourceId === src.id ? (
                            <input autoFocus value={renameSourceValue} onChange={e => setRenameSourceValue(e.target.value)}
                              onBlur={() => handleRenameSource(src.id, renameSourceValue)}
                              onKeyDown={e => { if (e.key === "Enter") handleRenameSource(src.id, renameSourceValue); if (e.key === "Escape") setRenamingSourceId(null); }}
                              onClick={e => e.stopPropagation()}
                              style={{ flex: 1, background: "#0F1520", border: "1px solid #4F9EFF", borderRadius: 3, color: "#F8F8FF", fontFamily: "'DM Sans', sans-serif", fontSize: 11, padding: "1px 4px", outline: "none" }} />
                          ) : (
                            <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, color: isSelected ? "#F8F8FF" : "#8892A4", flex: 1, overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }}>{src.name}</span>
                          )}
                          <div className="flex items-center gap-0.5 shrink-0">
                            <button onClick={e => { e.stopPropagation(); if (activeSceneId) updateSource(activeSceneId, src.id, { visible: !src.visible }); }}
                              style={{ background: "none", border: "none", cursor: "pointer", color: src.visible ? "#8892A4" : "#303D5A", display: "flex", padding: 1 }}>
                              {src.visible ? <Eye size={10} /> : <EyeOff size={10} />}
                            </button>
                            <button onClick={e => { e.stopPropagation(); if (activeSceneId) updateSource(activeSceneId, src.id, { locked: !src.locked }); }}
                              style={{ background: "none", border: "none", cursor: "pointer", color: src.locked ? "#FBBF24" : "#303D5A", display: "flex", padding: 1 }}>
                              {src.locked ? <Lock size={10} /> : <Unlock size={10} />}
                            </button>
                            <button onClick={e => { e.stopPropagation(); if (activeSceneId && idx > 0) moveSource(activeSceneId, src.id, "up"); }}
                              style={{ background: "none", border: "none", cursor: "pointer", color: "#606078", display: "flex", padding: 1 }}>
                              <ChevronUp size={10} />
                            </button>
                            <button onClick={e => { e.stopPropagation(); if (activeSceneId && idx < sources.length - 1) moveSource(activeSceneId, src.id, "down"); }}
                              style={{ background: "none", border: "none", cursor: "pointer", color: "#606078", display: "flex", padding: 1 }}>
                              <ChevronDown size={10} />
                            </button>
                          </div>
                        </div>
                      </ContextMenuTrigger>
                      <ContextMenuContent style={{ background: "linear-gradient(135deg, #161E30 0%, #111826 100%)", border: "1px solid rgba(79,158,255,0.2)", boxShadow: "0 8px 32px rgba(0,0,0,0.6), 0 0 0 1px rgba(79,158,255,0.06)" }}>
                        <ContextMenuItem onClick={() => { setSelectedSourceId(src.id); setRenamingSourceId(src.id); setRenameSourceValue(src.name); }} style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 12 }}>Rename</ContextMenuItem>
                        <ContextMenuItem onClick={() => handleDuplicateSource(src)} style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 12 }}>Duplicate</ContextMenuItem>
                        <ContextMenuSeparator />
                        <ContextMenuItem onClick={() => { if (activeSceneId) { removeSource(activeSceneId, src.id); if (selectedSourceId === src.id) setSelectedSourceId(null); } }} style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 12, color: "#EF4444" }}>Delete</ContextMenuItem>
                      </ContextMenuContent>
                    </ContextMenu>
                  );
                })}
              </div>
            </div>
          </div>

          {/* ── CENTER: Program canvas ── */}
          <div className="flex flex-col flex-1 overflow-hidden">
            <div className="flex items-center px-2 py-1.5 shrink-0" style={{ borderBottom: "1px solid rgba(255,90,44,0.25)", background: "linear-gradient(90deg, rgba(255,90,44,0.12) 0%, #0D1525 60%, #131C30 100%)" }}>
              <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 700, fontSize: 10, color: "#FF8C60", letterSpacing: "0.12em", textTransform: "uppercase" }}>Program Output</span>
              <div className="flex-1" />
              <div className="flex items-center gap-1.5">
                <div className="w-1.5 h-1.5 rounded-full" style={{ background: isLive ? "#FF5A2C" : "#50506A" }} />
                <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 700, fontSize: 10, color: isLive ? "#FF5A2C" : "#50506A" }}>{isLive ? "LIVE" : "OFFLINE"}</span>
              </div>
            </div>
            <div className="flex-1 flex items-center justify-center overflow-hidden" style={{ background: "radial-gradient(ellipse at center, #0A1018 30%, #040810 100%)", position: "relative" }}>
              <div style={{ position: "relative", width: "100%", maxWidth: "calc((100vh - 44px - 32px - 36px - 80px) * 16/9)", aspectRatio: "16/9", background: "#000", border: "1px solid rgba(79,158,255,0.2)", borderRadius: 4, overflow: "hidden", boxShadow: "0 0 40px rgba(0,0,0,0.8), 0 0 1px rgba(79,158,255,0.3)" }}>
                {activeSceneId === null ? (
                  <div className="flex flex-col items-center justify-center w-full h-full gap-3" style={{ position: "absolute", inset: 0 }}>
                    <svg width="56" height="56" viewBox="0 0 56 56" fill="none" style={{ opacity: 0.22 }}>
                      <circle cx="28" cy="28" r="22" stroke="#4F9EFF" strokeWidth="1.5"/>
                      <circle cx="28" cy="28" r="14" stroke="#4F9EFF" strokeWidth="1"/>
                      <line x1="6" y1="28" x2="50" y2="28" stroke="#FF5A2C" strokeWidth="1.5" strokeDasharray="3 2"/>
                      <circle cx="28" cy="28" r="4" fill="#4F9EFF" opacity="0.5"/>
                    </svg>
                    <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, color: "#3A4A6A", letterSpacing: "0.08em", textTransform: "uppercase" }}>Build a rack-ready scene</span>
                    <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, color: "#2A3550" }}>Create a scene in the panel to begin</span>
                  </div>
                ) : (
                  <ProgramCanvas
                    sources={sources}
                    selectedId={selectedSourceId}
                    transforms={canvasTransforms}
                    onSelect={setSelectedSourceId}
                    onTransformChange={handleCanvasTransformChange}
                  />
                )}
                {/* Corner markers */}
                {(["top-left","top-right","bottom-left","bottom-right"] as const).map(pos => (
                  <div key={pos} style={{ position: "absolute", width: 16, height: 16, pointerEvents: "none",
                    top: pos.includes("top") ? 4 : "auto", bottom: pos.includes("bottom") ? 4 : "auto",
                    left: pos.includes("left") ? 4 : "auto", right: pos.includes("right") ? 4 : "auto",
                    borderTop: pos.includes("top") ? "2px solid rgba(79,158,255,0.4)" : "none",
                    borderBottom: pos.includes("bottom") ? "2px solid rgba(79,158,255,0.4)" : "none",
                    borderLeft: pos.includes("left") ? "2px solid rgba(79,158,255,0.4)" : "none",
                    borderRight: pos.includes("right") ? "2px solid rgba(79,158,255,0.4)" : "none",
                  }} />
                ))}
                {/* Title-safe area (80% inset) */}
                <div style={{ position: "absolute", inset: "10%", border: "1px dashed rgba(79,158,255,0.07)", borderRadius: 2, pointerEvents: "none" }} />
                {/* Center crosshair */}
                <div style={{ position: "absolute", top: "50%", left: "50%", transform: "translate(-50%,-50%)", width: 20, height: 20, pointerEvents: "none" }}>
                  <div style={{ position: "absolute", top: "50%", left: 0, right: 0, height: 1, background: "rgba(79,158,255,0.1)", transform: "translateY(-50%)" }} />
                  <div style={{ position: "absolute", left: "50%", top: 0, bottom: 0, width: 1, background: "rgba(79,158,255,0.1)", transform: "translateX(-50%)" }} />
                </div>
              </div>
            </div>
            {/* Transitions strip */}
            <div className="flex items-center gap-2 px-3 shrink-0" style={{ height: 40, background: "linear-gradient(90deg, #0A1020 0%, #0D1525 100%)", borderTop: "1px solid rgba(79,158,255,0.2)" }}>
              <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, color: "#4F6080", letterSpacing: "0.1em", textTransform: "uppercase", fontWeight: 700 }}>Transition</span>
              {["Cut","Fade","Slide","Wipe"].map(t => (
                <button key={t}
                  onClick={() => setActiveTransition(t)}
                  style={{ padding: "2px 10px", borderRadius: 4,
                    border: `1px solid ${activeTransition === t ? "rgba(79,158,255,0.6)" : "rgba(255,255,255,0.08)"}`,
                    background: activeTransition === t ? "linear-gradient(135deg, rgba(79,158,255,0.25) 0%, rgba(79,158,255,0.1) 100%)" : "rgba(255,255,255,0.03)",
                    color: activeTransition === t ? "#7BBFFF" : "#606880",
                    fontFamily: "'DM Sans', sans-serif", fontSize: 10, cursor: "pointer", fontWeight: activeTransition === t ? 700 : 400,
                    boxShadow: activeTransition === t ? "0 0 12px rgba(79,158,255,0.3), inset 0 1px 0 rgba(79,158,255,0.2)" : "none",
                    transition: "all 0.15s" }}>{t}</button>
              ))}
              <div className="flex-1" />
              <span style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 10, color: "#50506A" }}>SCENE</span>
              <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, color: "#F8F8FF" }}>{activeScene?.name ?? "—"}</span>
            </div>
          </div>

          {/* ── RIGHT: Properties + Stream Status ── */}
          <div className="flex flex-col shrink-0 overflow-hidden" style={{ width: 240, background: "linear-gradient(180deg, #0C1220 0%, #090E1A 100%)", borderLeft: "1px solid rgba(79,158,255,0.18)" }}>
            {/* Properties */}
            <div className="flex flex-col overflow-hidden" style={{ flex: 1, borderBottom: "1px solid #2A3350" }}>
              <div className="flex items-center px-2 py-1.5 shrink-0" style={{ background: "linear-gradient(90deg, rgba(168,85,247,0.18) 0%, rgba(168,85,247,0.04) 100%)", borderBottom: "1px solid rgba(168,85,247,0.25)" }}>
                <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 700, fontSize: 10, color: "#C084FC", letterSpacing: "0.12em", textTransform: "uppercase" }}>
                  {selectedSource ? `Props — ${selectedSource.name}` : "Properties"}
                </span>
              </div>
              <PropertiesPanel
                sourceId={selectedSourceId}
                sources={sources}
                onUpdateSettings={handleUpdateSettings}
                onUpdateTransform={handleUpdateTransform}
              />
            </div>
            {/* Stream Status */}
            <div className="flex flex-col shrink-0">
              <div className="flex items-center px-2 py-1.5 shrink-0" style={{ background: "linear-gradient(90deg, rgba(34,197,94,0.18) 0%, rgba(34,197,94,0.04) 100%)", borderBottom: "1px solid rgba(34,197,94,0.25)" }}>
                <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 700, fontSize: 10, color: "#4ADE80", letterSpacing: "0.12em", textTransform: "uppercase" }}>Stream Status</span>
              </div>
              <div className="px-3 py-2 flex flex-col gap-2">
                <div className="flex items-center gap-2">
                  <div className="w-2 h-2 rounded-full" style={{ background: isLive ? "#FF5A2C" : "#50506A" }} />
                  <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 700, fontSize: 12, color: isLive ? "#FF5A2C" : "#50506A" }}>
                    {isLive ? `LIVE · ${livePlatform.toUpperCase()}` : "OFFLINE"}
                  </span>
                </div>
                <div className="flex items-center justify-between">
                  <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, color: "#606078", textTransform: "uppercase", letterSpacing: "0.08em" }}>Bitrate</span>
                  <span style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 12, color: "#4F9EFF" }}>{isLive ? `${Math.round(bitrate / 100) / 10} Mbps` : "—"}</span>
                </div>
                <BitrateSparkline active={isLive} />
                <div className="grid grid-cols-2 gap-2">
                  {[
                    { icon: Users,    label: "Viewers", val: isLive ? String(viewers) : "—", color: "#22C55E" },
                    { icon: Clock,    label: "Uptime",  val: isLive ? tc : "—",              color: "#4F9EFF" },
                    { icon: Cpu,      label: "CPU",     val: isLive ? "12%" : "—",           color: "#A855F7" },
                    { icon: Activity, label: "Health",  val: isLive ? "Good" : "—",          color: "#22C55E" },
                  ].map(({ icon: Icon, label, val, color }) => (
                    <div key={label} className="flex flex-col gap-0.5 px-2 py-1.5 rounded" style={{ background: "linear-gradient(135deg, #131C2C 0%, #0C1420 100%)", border: "1px solid rgba(255,255,255,0.1)", boxShadow: "0 4px 12px rgba(0,0,0,0.5), inset 0 1px 0 rgba(255,255,255,0.06)" }}>
                      <div className="flex items-center gap-1">
                        <Icon size={9} style={{ color }} />
                        <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 9, color: "#606078", textTransform: "uppercase", letterSpacing: "0.08em" }}>{label}</span>
                      </div>
                      <span style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 11, color }}>{val}</span>
                    </div>
                  ))}
                </div>
              </div>
            </div>
          </div>
        </div>

        {/* ── BOTTOM: Audio Mixer ── */}
        <div className="flex flex-col shrink-0" style={{ borderTop: "1px solid rgba(168,85,247,0.25)", background: "linear-gradient(90deg, #0A1020 0%, #0D1525 100%)" }}>
          <div className="flex items-center justify-between px-3 py-1.5 shrink-0" style={{ background: "linear-gradient(90deg, rgba(168,85,247,0.18) 0%, rgba(168,85,247,0.04) 100%)", borderBottom: "1px solid rgba(168,85,247,0.25)" }}>
            <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 700, fontSize: 10, color: "#C084FC", letterSpacing: "0.12em", textTransform: "uppercase" }}>Audio Mixer</span>
            <div className="flex items-center gap-3">
              <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 9, color: "#50506A" }}>{audioChannels.length} ch</span>
              <Volume2 size={11} style={{ color: "#A855F7" }} />
            </div>
          </div>
          {/* dB scale + channel strips */}
          <div className="flex overflow-x-auto" style={{ height: 120 }}>
            {/* dB scale ruler */}
            <div className="shrink-0 flex flex-col justify-between py-1 pr-1" style={{ width: 28, paddingTop: 4, paddingBottom: 22 }}>
              {[0, -6, -12, -18, -24, -30].map(db => (
                <span key={db} style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 8, color: db === 0 ? "#EF4444" : db >= -6 ? "#FBBF24" : "#50506A", textAlign: "right", lineHeight: 1 }}>{db}</span>
              ))}
            </div>
            {/* Channel strips */}
            {audioChannels.map(ch => {
              const cs = channelState[ch.id] ?? { muted: false, solo: false, volume: 75 };
              const isMic = ch.type === "mic" || ch.type === "camera";
              const isDesk = ch.type === "desktop";
              return (
                <div key={ch.id} className="flex flex-col items-center gap-1 px-2 py-1 shrink-0"
                  style={{ minWidth: 72, borderRight: "1px solid rgba(255,255,255,0.05)", opacity: cs.muted ? 0.4 : 1, transition: "opacity 0.15s", background: "rgba(255,255,255,0.015)" }}>
                  {/* VU meters */}
                  <VUMeterVertical color={ch.color} active={isLive && !cs.muted} volume={cs.volume} />
                  {/* Volume fader */}
                  <div className="flex items-center gap-1 w-full">
                    <input type="range" min={0} max={100} value={cs.volume}
                      onChange={e => setChannelState(p => ({ ...p, [ch.id]: { ...cs, volume: Number(e.target.value) } }))}
                      style={{ flex: 1, accentColor: ch.color, cursor: "pointer", height: 3 }} />
                    <span style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 8, color: "#606078", width: 22, textAlign: "right" }}>
                      {cs.volume === 100 ? "0dB" : cs.volume > 0 ? `${Math.round((cs.volume / 100 - 1) * 60)}` : "−∞"}
                    </span>
                  </div>
                  {/* Channel name + controls */}
                  <div className="flex items-center justify-between w-full">
                    <div className="flex items-center gap-0.5 overflow-hidden">
                      {isMic ? <Mic size={8} style={{ color: ch.color, flexShrink: 0 }} /> : isDesk ? <Monitor size={8} style={{ color: ch.color, flexShrink: 0 }} /> : <Music size={8} style={{ color: ch.color, flexShrink: 0 }} />}
                      <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 9, fontWeight: 600, color: "#A0A8C0", overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap", maxWidth: 42 }}>{ch.name}</span>
                    </div>
                    <div className="flex gap-0.5">
                      <button onClick={() => setChannelState(p => ({ ...p, [ch.id]: { ...cs, solo: !cs.solo } }))}
                        style={{ width: 14, height: 14, borderRadius: 2, border: `1px solid ${cs.solo ? "#FBBF24" : "#303D5A"}`, background: cs.solo ? "#FBBF24" : "#0F1520", color: cs.solo ? "#0F1520" : "#606078", fontFamily: "'DM Sans', sans-serif", fontSize: 8, fontWeight: 700, cursor: "pointer", display: "flex", alignItems: "center", justifyContent: "center" }}>S</button>
                      <button onClick={() => setChannelState(p => ({ ...p, [ch.id]: { ...cs, muted: !cs.muted } }))}
                        style={{ width: 14, height: 14, borderRadius: 2, border: `1px solid ${cs.muted ? "#EF4444" : "#303D5A"}`, background: cs.muted ? "#EF4444" : "#0F1520", color: cs.muted ? "#fff" : "#606078", fontFamily: "'DM Sans', sans-serif", fontSize: 8, fontWeight: 700, cursor: "pointer", display: "flex", alignItems: "center", justifyContent: "center" }}>M</button>
                    </div>
                  </div>
                </div>
              );
            })}
          </div>
        </div>
      </div>

      {/* Modals */}
      {showAddSource && <AddSourceModal onAdd={handleAddSource} onClose={() => setShowAddSource(false)} />}
      <GoLiveModal open={showGoLive} onClose={() => setShowGoLive(false)}
        onGoLive={(cfg: { platform: string }) => { setIsLive(true); setLivePlatform(cfg.platform); setShowGoLive(false); }} />
    </AppSidebar>
  );
}
