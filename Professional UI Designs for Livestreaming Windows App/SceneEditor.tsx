// RailShotTV — Chromatic Command — Scene Editor + Overlay Source Browser
import { useState, useRef, useCallback, useEffect } from "react";
import AppSidebar from "@/components/AppSidebar";
import { toast } from "sonner";
import {
  Eye, EyeOff, Lock, Unlock, Move, Crop, RotateCcw, AlignCenter, Layers,
  Plus, Trash2, ChevronUp, ChevronDown, Monitor, Camera, Type, Image, Zap,
  ChevronLeft, ChevronRight, Search, Trophy, AlignLeft, Bell, Tag, Tv,
  GripVertical, X, Check, LayoutTemplate, Sparkles
} from "lucide-react";

// ── Overlay Template Library ──────────────────────────────────────────────────
const CATEGORIES = [
  { id: "all",        label: "All",         color: "#8892A4" },
  { id: "scoreboard", label: "Scoreboard",  color: "#FF5A2C" },
  { id: "lowerthird", label: "Lower Thirds",color: "#4F9EFF" },
  { id: "ticker",     label: "Tickers",     color: "#22D3EE" },
  { id: "alert",      label: "Alerts",      color: "#FBBF24" },
  { id: "branding",   label: "Branding",    color: "#A855F7" },
];

const OVERLAY_TEMPLATES = [
  // Scoreboard
  { id: "sb-lower",   cat: "scoreboard", name: "Lower Third Score",   color: "#FF5A2C", icon: Trophy,
    preview: () => (
      <svg viewBox="0 0 160 90" className="w-full h-full">
        <rect x="0" y="60" width="160" height="30" fill="#FF5A2C" opacity="0.9" rx="2"/>
        <rect x="0" y="60" width="80" height="30" fill="#1A2035" opacity="0.95" rx="2"/>
        <text x="8" y="73" fill="white" fontSize="7" fontWeight="bold">TEAM A</text>
        <text x="8" y="83" fill="#FF5A2C" fontSize="5">Player Name</text>
        <text x="68" y="78" fill="white" fontSize="10" fontWeight="bold" textAnchor="middle">3</text>
        <text x="92" y="78" fill="white" fontSize="10" fontWeight="bold" textAnchor="middle">1</text>
        <text x="80" y="78" fill="#8892A4" fontSize="8" textAnchor="middle">–</text>
        <text x="152" y="73" fill="white" fontSize="7" fontWeight="bold" textAnchor="end">TEAM B</text>
        <text x="152" y="83" fill="#FF5A2C" fontSize="5" textAnchor="end">Player Name</text>
      </svg>
    )
  },
  { id: "sb-center",  cat: "scoreboard", name: "Center Banner Score", color: "#FF5A2C", icon: Trophy,
    preview: () => (
      <svg viewBox="0 0 160 90" className="w-full h-full">
        <rect x="30" y="35" width="100" height="20" fill="#1A2035" opacity="0.95" rx="3"/>
        <rect x="30" y="35" width="100" height="4" fill="#FF5A2C" rx="2"/>
        <text x="55" y="50" fill="white" fontSize="8" fontWeight="bold" textAnchor="middle">A  3–1  B</text>
        <text x="80" y="58" fill="#8892A4" fontSize="5" textAnchor="middle">Q2 · 08:42</text>
      </svg>
    )
  },
  { id: "sb-corner",  cat: "scoreboard", name: "Corner Compact",      color: "#FF5A2C", icon: Trophy,
    preview: () => (
      <svg viewBox="0 0 160 90" className="w-full h-full">
        <rect x="4" y="4" width="52" height="28" fill="#1A2035" opacity="0.95" rx="2"/>
        <rect x="4" y="4" width="52" height="3" fill="#FF5A2C"/>
        <text x="10" y="17" fill="white" fontSize="6" fontWeight="bold">A  3</text>
        <text x="10" y="26" fill="white" fontSize="6" fontWeight="bold">B  1</text>
        <text x="46" y="26" fill="#8892A4" fontSize="5" textAnchor="end">Q2</text>
      </svg>
    )
  },
  { id: "sb-full",    cat: "scoreboard", name: "Full Width Score",    color: "#FF5A2C", icon: Trophy,
    preview: () => (
      <svg viewBox="0 0 160 90" className="w-full h-full">
        <rect x="0" y="0" width="160" height="18" fill="#1A2035" opacity="0.97"/>
        <rect x="0" y="0" width="160" height="2" fill="#FF5A2C"/>
        <text x="12" y="12" fill="white" fontSize="7" fontWeight="bold">TEAM A</text>
        <text x="148" y="12" fill="white" fontSize="7" fontWeight="bold" textAnchor="end">TEAM B</text>
        <text x="80" y="13" fill="white" fontSize="9" fontWeight="bold" textAnchor="middle">3 – 1</text>
        <text x="80" y="8" fill="#8892A4" fontSize="4" textAnchor="middle">PREVIEW</text>
      </svg>
    )
  },
  // Lower Thirds
  { id: "lt-player",  cat: "lowerthird", name: "Player Name / Title", color: "#4F9EFF", icon: AlignLeft,
    preview: () => (
      <svg viewBox="0 0 160 90" className="w-full h-full">
        <rect x="0" y="62" width="110" height="24" fill="#1A2035" opacity="0.95"/>
        <rect x="0" y="62" width="4" height="24" fill="#4F9EFF"/>
        <text x="12" y="73" fill="white" fontSize="8" fontWeight="bold">PLAYER NAME</text>
        <text x="12" y="82" fill="#4F9EFF" fontSize="6">Title / Role</text>
      </svg>
    )
  },
  { id: "lt-commentator", cat: "lowerthird", name: "Commentator ID",  color: "#4F9EFF", icon: AlignLeft,
    preview: () => (
      <svg viewBox="0 0 160 90" className="w-full h-full">
        <rect x="0" y="62" width="120" height="24" fill="#4F9EFF" opacity="0.92"/>
        <text x="8" y="73" fill="white" fontSize="8" fontWeight="bold">COMMENTATOR</text>
        <text x="8" y="82" fill="#1A2035" fontSize="6" fontWeight="bold">Live Commentary</text>
      </svg>
    )
  },
  { id: "lt-sponsor",  cat: "lowerthird", name: "Sponsor Mention",    color: "#4F9EFF", icon: AlignLeft,
    preview: () => (
      <svg viewBox="0 0 160 90" className="w-full h-full">
        <rect x="20" y="65" width="120" height="20" fill="#1A2035" opacity="0.9" rx="2"/>
        <rect x="20" y="65" width="120" height="2" fill="#4F9EFF"/>
        <text x="80" y="76" fill="#8892A4" fontSize="5" textAnchor="middle">PRESENTED BY</text>
        <text x="80" y="83" fill="white" fontSize="7" fontWeight="bold" textAnchor="middle">SPONSOR NAME</text>
      </svg>
    )
  },
  { id: "lt-intro",    cat: "lowerthird", name: "Event Intro",         color: "#4F9EFF", icon: AlignLeft,
    preview: () => (
      <svg viewBox="0 0 160 90" className="w-full h-full">
        <rect x="0" y="55" width="160" height="35" fill="#1A2035" opacity="0.95"/>
        <rect x="0" y="55" width="160" height="3" fill="#4F9EFF"/>
        <text x="80" y="68" fill="white" fontSize="9" fontWeight="bold" textAnchor="middle">EVENT TITLE</text>
        <text x="80" y="78" fill="#4F9EFF" fontSize="6" textAnchor="middle">Venue · Date · Time</text>
        <text x="80" y="86" fill="#8892A4" fontSize="5" textAnchor="middle">Presented by RailShotTV</text>
      </svg>
    )
  },
  // Tickers
  { id: "tk-news",     cat: "ticker",    name: "Breaking News Ticker", color: "#22D3EE", icon: Tv,
    preview: () => (
      <svg viewBox="0 0 160 90" className="w-full h-full">
        <rect x="0" y="78" width="160" height="12" fill="#1A2035" opacity="0.97"/>
        <rect x="0" y="78" width="38" height="12" fill="#22D3EE"/>
        <text x="4" y="87" fill="#1A2035" fontSize="6" fontWeight="bold">BREAKING</text>
        <text x="42" y="87" fill="white" fontSize="6">Latest event updates scroll here continuously...</text>
      </svg>
    )
  },
  { id: "tk-score",    cat: "ticker",    name: "Score Ticker",         color: "#22D3EE", icon: Tv,
    preview: () => (
      <svg viewBox="0 0 160 90" className="w-full h-full">
        <rect x="0" y="78" width="160" height="12" fill="#0D1220" opacity="0.97"/>
        <rect x="0" y="78" width="28" height="12" fill="#FF5A2C"/>
        <text x="4" y="87" fill="white" fontSize="6" fontWeight="bold">SCORES</text>
        <text x="32" y="87" fill="#22D3EE" fontSize="6">A 3–1 B  ·  C 2–0 D  ·  E 1–1 F</text>
      </svg>
    )
  },
  { id: "tk-social",   cat: "ticker",    name: "Social Feed Ticker",   color: "#22D3EE", icon: Tv,
    preview: () => (
      <svg viewBox="0 0 160 90" className="w-full h-full">
        <rect x="0" y="78" width="160" height="12" fill="#1A2035" opacity="0.97"/>
        <rect x="0" y="78" width="22" height="12" fill="#A855F7"/>
        <text x="3" y="87" fill="white" fontSize="5" fontWeight="bold">CHAT</text>
        <text x="26" y="87" fill="#E2E8F0" fontSize="6">@viewer: Great match! Keep it up! 🔥</text>
      </svg>
    )
  },
  // Alerts
  { id: "al-follow",   cat: "alert",     name: "Follow Alert",         color: "#FBBF24", icon: Bell,
    preview: () => (
      <svg viewBox="0 0 160 90" className="w-full h-full">
        <rect x="40" y="28" width="80" height="34" fill="#1A2035" opacity="0.97" rx="4"/>
        <rect x="40" y="28" width="80" height="3" fill="#FBBF24" rx="2"/>
        <text x="80" y="44" fill="#FBBF24" fontSize="7" fontWeight="bold" textAnchor="middle">NEW FOLLOWER</text>
        <text x="80" y="54" fill="white" fontSize="6" textAnchor="middle">username just followed!</text>
      </svg>
    )
  },
  { id: "al-donation", cat: "alert",     name: "Donation Alert",       color: "#FBBF24", icon: Bell,
    preview: () => (
      <svg viewBox="0 0 160 90" className="w-full h-full">
        <rect x="30" y="25" width="100" height="40" fill="#1A2035" opacity="0.97" rx="4"/>
        <rect x="30" y="25" width="100" height="3" fill="#22C55E" rx="2"/>
        <text x="80" y="40" fill="#22C55E" fontSize="7" fontWeight="bold" textAnchor="middle">DONATION</text>
        <text x="80" y="50" fill="white" fontSize="8" fontWeight="bold" textAnchor="middle">$25.00</text>
        <text x="80" y="60" fill="#8892A4" fontSize="5" textAnchor="middle">from username</text>
      </svg>
    )
  },
  { id: "al-sub",      cat: "alert",     name: "Subscription Alert",   color: "#FBBF24", icon: Bell,
    preview: () => (
      <svg viewBox="0 0 160 90" className="w-full h-full">
        <rect x="25" y="22" width="110" height="46" fill="#A855F7" opacity="0.92" rx="4"/>
        <text x="80" y="38" fill="white" fontSize="7" fontWeight="bold" textAnchor="middle">NEW SUBSCRIBER</text>
        <text x="80" y="50" fill="white" fontSize="8" fontWeight="bold" textAnchor="middle">username</text>
        <text x="80" y="61" fill="#E9D5FF" fontSize="5" textAnchor="middle">Tier 1 · Thank you! 🎉</text>
      </svg>
    )
  },
  // Branding
  { id: "br-sponsor",  cat: "branding",  name: "Sponsor Logo Bar",     color: "#A855F7", icon: Tag,
    preview: () => (
      <svg viewBox="0 0 160 90" className="w-full h-full">
        <rect x="0" y="0" width="160" height="14" fill="#1A2035" opacity="0.95"/>
        <text x="8" y="10" fill="#8892A4" fontSize="5">SPONSORS</text>
        <rect x="40" y="3" width="24" height="8" fill="#303D5A" rx="1"/>
        <rect x="68" y="3" width="24" height="8" fill="#303D5A" rx="1"/>
        <rect x="96" y="3" width="24" height="8" fill="#303D5A" rx="1"/>
        <text x="52" y="9" fill="#8892A4" fontSize="5" textAnchor="middle">LOGO</text>
        <text x="80" y="9" fill="#8892A4" fontSize="5" textAnchor="middle">LOGO</text>
        <text x="108" y="9" fill="#8892A4" fontSize="5" textAnchor="middle">LOGO</text>
      </svg>
    )
  },
  { id: "br-watermark",cat: "branding",  name: "Watermark / Bug",      color: "#A855F7", icon: Tag,
    preview: () => (
      <svg viewBox="0 0 160 90" className="w-full h-full">
        <rect x="124" y="6" width="30" height="14" fill="#1A2035" opacity="0.8" rx="2"/>
        <text x="139" y="16" fill="#A855F7" fontSize="6" fontWeight="bold" textAnchor="middle">RAIL</text>
        <circle cx="139" cy="11" r="3" fill="none" stroke="#A855F7" strokeWidth="0.8"/>
      </svg>
    )
  },
];

// ── Source list ───────────────────────────────────────────────────────────────
// Sources start empty — populated when OBS sources are added
type SourceItem = { id: number; name: string; type: string; icon: React.ElementType; color: string; visible: boolean; locked: boolean; selected?: boolean };
const INIT_SOURCES: SourceItem[] = [];
const TRANSITIONS = ["Cut","Fade","Slide","Wipe","Stinger"];

// ── Source type catalogue for the Add Source modal ───────────────────────────
const SOURCE_TYPES = [
  { type: "display",    label: "Display Capture", icon: Monitor,   color: "#4F9EFF" },
  { type: "camera",     label: "Camera / Webcam",  icon: Camera,    color: "#22C55E" },
  { type: "text",       label: "Text",             icon: Type,      color: "#A855F7" },
  { type: "image",      label: "Image",            icon: Image,     color: "#FBBF24" },
  { type: "browser",    label: "Browser Source",   icon: Tv,        color: "#22D3EE" },
  { type: "alert",      label: "Alert / Stinger",  icon: Bell,      color: "#FF5A2C" },
  { type: "scoreboard", label: "Scoreboard",       icon: Trophy,    color: "#FF5A2C" },
  { type: "lowerthird", label: "Lower Third",      icon: AlignLeft, color: "#4F9EFF" },
];

// ── Add Source Modal ──────────────────────────────────────────────────────────
function AddSourceModal({ onAdd, onClose }: {
  onAdd: (type: string, name: string, icon: React.ElementType, color: string) => void;
  onClose: () => void;
}) {
  const [name, setName] = useState(SOURCE_TYPES[0].label);
  const [selected, setSelected] = useState(SOURCE_TYPES[0]);
  useEffect(() => { setName(selected.label); }, [selected]);
  useEffect(() => {
    const handler = (e: KeyboardEvent) => { if (e.key === "Escape") onClose(); };
    window.addEventListener("keydown", handler);
    return () => window.removeEventListener("keydown", handler);
  }, [onClose]);
  return (
    <div className="fixed inset-0 flex items-center justify-center" style={{ background: "rgba(0,0,0,0.7)", zIndex: 9999 }}
      onClick={e => { if (e.target === e.currentTarget) onClose(); }}>
      <div style={{ background: "#1E2640", border: "1px solid #2A3350", borderRadius: 10, width: 420, boxShadow: "0 24px 64px rgba(0,0,0,0.6)" }}>
        <div className="flex items-center justify-between px-5 py-3.5" style={{ borderBottom: "1px solid #2A3350" }}>
          <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 700, fontSize: 14, color: "#F8F8FF" }}>Add Source</span>
          <button onClick={onClose} style={{ background: "none", border: "none", cursor: "pointer", color: "#606078", display: "flex" }}><X size={16} /></button>
        </div>
        <div className="grid grid-cols-4 gap-2 p-4">
          {SOURCE_TYPES.map(st => (
            <button key={st.type} onClick={() => setSelected(st)}
              style={{ display: "flex", flexDirection: "column", alignItems: "center", gap: 6, padding: "10px 6px", borderRadius: 7, cursor: "pointer",
                border: `1px solid ${selected.type === st.type ? st.color + "80" : "#2A3350"}`,
                background: selected.type === st.type ? st.color + "18" : "#141928", transition: "all 0.15s" }}>
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
            style={{ padding: "7px 16px", borderRadius: 6, border: "none", background: "linear-gradient(135deg, #4F9EFF, #7C3AED)", color: "#fff", fontFamily: "'DM Sans', sans-serif", fontSize: 12, fontWeight: 700, cursor: "pointer" }}>
            Add Source
          </button>
        </div>
      </div>
    </div>
  );
}

// ── Dropped overlay indicator on canvas ──────────────────────────────────────
interface CanvasOverlay { id: string; name: string; color: string; x: number; y: number }

export default function SceneEditor() {
  const [selectedSource, setSelectedSource] = useState<number | null>(null);
  const [activeTrans, setActiveTrans]       = useState("Cut");
  const [sources, setSources]               = useState(INIT_SOURCES);
  const [browserOpen, setBrowserOpen]       = useState(true);
  const [activeCategory, setActiveCategory] = useState("all");
  const [searchQuery, setSearchQuery]       = useState("");
  const [dragging, setDragging]             = useState<string | null>(null);
  const [canvasOverlays, setCanvasOverlays] = useState<CanvasOverlay[]>([]);
  const [dropFlash, setDropFlash]           = useState(false);
  const [addedToast, setAddedToast]         = useState<string | null>(null);
  const canvasRef = useRef<HTMLDivElement>(null);
  let nextId = useRef(100);
  const [showAddSource, setShowAddSource]   = useState(false);
  const [transforms, setTransforms] = useState<Record<number, { x: number; y: number; w: number; h: number; rot: number; opacity: number; flipH: boolean; flipV: boolean }>>({});

  const getTransform = (id: number) => transforms[id] ?? { x: 640, y: 270, w: 640, h: 360, rot: 0, opacity: 100, flipH: false, flipV: false };
  const setTransform = (id: number, patch: Partial<ReturnType<typeof getTransform>>) =>
    setTransforms(prev => ({ ...prev, [id]: { ...getTransform(id), ...patch } }));

  const handleAddSource = useCallback((type: string, name: string, icon: React.ElementType, color: string) => {
    const newId = nextId.current++;
    setSources(prev => [...prev, { id: newId, name, type, icon, color, visible: true, locked: false }]);
    setSelectedSource(newId);
    toast.success(`Added "${name}"`);
  }, []);

  const handleDeleteSource = useCallback(() => {
    if (selectedSource === null) { toast.info("Select a source first"); return; }
    setSources(prev => prev.filter(s => s.id !== selectedSource));
    setCanvasOverlays(prev => prev.filter(o => o.id !== String(selectedSource)));
    setSelectedSource(null);
  }, [selectedSource]);

  const handleToggleVisible = useCallback((id: number) => {
    setSources(prev => prev.map(s => s.id === id ? { ...s, visible: !s.visible } : s));
  }, []);

  const handleToggleLock = useCallback((id: number) => {
    setSources(prev => prev.map(s => s.id === id ? { ...s, locked: !s.locked } : s));
  }, []);

  const handleMoveSource = useCallback((id: number, dir: "up" | "down") => {
    setSources(prev => {
      const idx = prev.findIndex(s => s.id === id);
      if (idx < 0) return prev;
      const next = [...prev];
      const swapIdx = dir === "up" ? idx - 1 : idx + 1;
      if (swapIdx < 0 || swapIdx >= next.length) return prev;
      [next[idx], next[swapIdx]] = [next[swapIdx], next[idx]];
      return next;
    });
  }, []);

  const filteredTemplates = OVERLAY_TEMPLATES.filter(t => {
    const catMatch = activeCategory === "all" || t.cat === activeCategory;
    const searchMatch = !searchQuery || t.name.toLowerCase().includes(searchQuery.toLowerCase());
    return catMatch && searchMatch;
  });

  const handleDragStart = useCallback((e: React.DragEvent, templateId: string) => {
    e.dataTransfer.setData("overlayTemplateId", templateId);
    setDragging(templateId);
  }, []);

  const handleDragEnd = useCallback(() => setDragging(null), []);

  const handleCanvasDrop = useCallback((e: React.DragEvent) => {
    e.preventDefault();
    const templateId = e.dataTransfer.getData("overlayTemplateId");
    const template = OVERLAY_TEMPLATES.find(t => t.id === templateId);
    if (!template || !canvasRef.current) return;
    const rect = canvasRef.current.getBoundingClientRect();
    const x = ((e.clientX - rect.left) / rect.width) * 100;
    const y = ((e.clientY - rect.top) / rect.height) * 100;
    const newId = nextId.current++;
    setCanvasOverlays(prev => [...prev, { id: String(newId), name: template.name, color: template.color, x, y }]);
    // Add to sources list
    const IconComp = template.icon;
    setSources(prev => [...prev, { id: newId, name: template.name, type: "overlay", icon: IconComp, color: template.color, visible: true, locked: false }]);
    setSelectedSource(newId);
    setDropFlash(true);
    setAddedToast(template.name);
    setTimeout(() => setDropFlash(false), 600);
    setTimeout(() => setAddedToast(null), 2500);
    setDragging(null);
  }, []);

  const handleAddTemplate = useCallback((template: typeof OVERLAY_TEMPLATES[0]) => {
    const newId = nextId.current++;
    setCanvasOverlays(prev => [...prev, { id: String(newId), name: template.name, color: template.color, x: 10 + Math.random() * 30, y: 10 + Math.random() * 30 }]);
    setSources(prev => [...prev, { id: newId, name: template.name, type: "overlay", icon: template.icon, color: template.color, visible: true, locked: false }]);
    setSelectedSource(newId);
    setAddedToast(template.name);
    setTimeout(() => setAddedToast(null), 2500);
  }, []);

  return (
    <AppSidebar>
      {/* Top bar */}
      <div className="flex items-center gap-3 px-4 shrink-0" style={{ height: 46, background: "#1A2035", borderBottom: "1px solid #2A3350" }}>
        <div className="flex items-center gap-1 mr-1">
          <span style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 18, color: "#F8F8FF", letterSpacing: "0.06em", lineHeight: 1 }}>RAILSHOT</span>
          <span style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 18, color: "#FF5A2C", letterSpacing: "0.06em", lineHeight: 1 }}>TV</span>
        </div>
        <div className="w-px h-4 mx-1" style={{ background: "#303D5A" }} />
        <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 600, fontSize: 11, color: "#8892A4", letterSpacing: "0.1em", textTransform: "uppercase" }}>Scene Editor</span>
        <div className="px-2 py-0.5 rounded" style={{ background: "#4F9EFF18", border: "1px solid #4F9EFF40" }}>
          <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, color: "#4F9EFF", fontWeight: 600 }}>Event — Main</span>
        </div>
        <div className="flex-1" />
        {/* Overlay browser toggle */}
        <button onClick={() => setBrowserOpen(v => !v)} className="flex items-center gap-1.5 px-2.5 py-1 rounded transition-all" style={{ background: browserOpen ? "#4F9EFF18" : "transparent", border: `1px solid ${browserOpen ? "#4F9EFF40" : "#303D5A"}`, color: browserOpen ? "#4F9EFF" : "#8892A4" }}>
          <LayoutTemplate size={13} />
          <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, fontWeight: 600 }}>Overlays</span>
        </button>
        <span className="mono" style={{ fontSize: 11, color: "#50506A" }}>1920×1080</span>
        <span className="mono" style={{ fontSize: 11, color: "#50506A" }}>60fps</span>
        <div className="flex items-center gap-1.5">
          <div className="w-1.5 h-1.5 rounded-full" style={{ background: "#50506A" }} />
          <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 700, fontSize: 11, color: "#50506A" }}>OFFLINE</span>
        </div>
      </div>

      <div className="flex flex-1 overflow-hidden">

        {/* ── Overlay Source Browser (left panel) ── */}
        {browserOpen && (
          <div className="flex flex-col shrink-0 overflow-hidden" style={{ width: 220, background: "#141928", borderRight: "1px solid #2A3350" }}>
            {/* Browser header */}
            <div className="flex items-center justify-between px-3 py-2 shrink-0" style={{ borderBottom: "1px solid #2A3350", background: "#1A2035" }}>
              <div className="flex items-center gap-1.5">
                <Sparkles size={13} style={{ color: "#A855F7" }} />
                <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 700, fontSize: 11, color: "#F8F8FF", letterSpacing: "0.06em", textTransform: "uppercase" }}>Overlay Library</span>
              </div>
              <button onClick={() => setBrowserOpen(false)} className="flex items-center justify-center rounded" style={{ width: 18, height: 18, background: "#1E2640", border: "1px solid #303D5A" }}>
                <X size={10} style={{ color: "#606078" }} />
              </button>
            </div>

            {/* Search */}
            <div className="px-2 py-2 shrink-0" style={{ borderBottom: "1px solid #1E2640" }}>
              <div className="flex items-center gap-1.5 px-2 rounded" style={{ height: 26, background: "#1E2640", border: "1px solid #303D5A" }}>
                <Search size={11} style={{ color: "#606078" }} />
                <input
                  value={searchQuery}
                  onChange={e => setSearchQuery(e.target.value)}
                  placeholder="Search overlays…"
                  style={{ flex: 1, background: "transparent", border: "none", outline: "none", fontFamily: "'DM Sans', sans-serif", fontSize: 11, color: "#A0A0B8" }}
                />
              </div>
            </div>

            {/* Category pills */}
            <div className="flex flex-wrap gap-1 px-2 py-2 shrink-0" style={{ borderBottom: "1px solid #1E2640" }}>
              {CATEGORIES.map(cat => (
                <button
                  key={cat.id}
                  onClick={() => setActiveCategory(cat.id)}
                  className="px-2 py-0.5 rounded-full transition-all"
                  style={{
                    fontFamily: "'DM Sans', sans-serif", fontSize: 10, fontWeight: 600,
                    background: activeCategory === cat.id ? `${cat.color}22` : "#1E2640",
                    border: `1px solid ${activeCategory === cat.id ? cat.color + "60" : "#303D5A"}`,
                    color: activeCategory === cat.id ? cat.color : "#606078"
                  }}
                >
                  {cat.label}
                </button>
              ))}
            </div>

            {/* Template grid */}
            <div className="flex-1 overflow-y-auto px-2 py-2" style={{ scrollbarWidth: "thin", scrollbarColor: "#303D5A transparent" }}>
              <div className="grid grid-cols-2 gap-2">
                {filteredTemplates.map(template => (
                  <div
                    key={template.id}
                    draggable
                    onDragStart={e => handleDragStart(e, template.id)}
                    onDragEnd={handleDragEnd}
                    className="flex flex-col rounded overflow-hidden cursor-grab active:cursor-grabbing transition-all"
                    style={{
                      background: dragging === template.id ? `${template.color}20` : "#1E2640",
                      border: `1px solid ${dragging === template.id ? template.color + "60" : "#303D5A"}`,
                      transform: dragging === template.id ? "scale(0.97)" : "scale(1)",
                      opacity: dragging === template.id ? 0.7 : 1,
                    }}
                    title={`Drag to canvas or double-click to add: ${template.name}`}
                    onDoubleClick={() => handleAddTemplate(template)}
                  >
                    {/* Preview thumbnail */}
                    <div className="relative" style={{ aspectRatio: "16/9", background: "#0D1220" }}>
                      <template.preview />
                      {/* Drag hint */}
                      <div className="absolute top-1 right-1 opacity-0 hover:opacity-100 transition-opacity">
                        <GripVertical size={10} style={{ color: "#8892A4" }} />
                      </div>
                      {/* Category dot */}
                      <div className="absolute bottom-1 left-1 w-1.5 h-1.5 rounded-full" style={{ background: template.color }} />
                    </div>
                    {/* Name */}
                    <div className="px-1.5 py-1">
                      <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 9, color: "#A0A0B8", lineHeight: 1.2, display: "block" }}>{template.name}</span>
                    </div>
                  </div>
                ))}
              </div>
              {filteredTemplates.length === 0 && (
                <div className="flex flex-col items-center justify-center py-8 gap-2">
                  <LayoutTemplate size={24} style={{ color: "#303D5A" }} />
                  <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, color: "#50506A" }}>No overlays found</span>
                </div>
              )}
              <div className="mt-3 px-1">
                <p style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 9, color: "#50506A", lineHeight: 1.4 }}>
                  Drag to canvas or double-click to add at default position.
                </p>
              </div>
            </div>
          </div>
        )}

        {/* ── Tool rail ── */}
        <div className="flex flex-col items-center gap-1 py-2 px-1 shrink-0" style={{ width: 40, background: "#1A2035", borderRight: "1px solid #2A3350" }}>
          {[Move, Crop, RotateCcw, AlignCenter, Layers].map((Icon, i) => (
            <button key={i} className="flex items-center justify-center rounded transition-colors" style={{ width: 28, height: 28, background: i === 0 ? "#4F9EFF18" : "transparent", border: i === 0 ? "1px solid #4F9EFF40" : "1px solid transparent" }}>
              <Icon size={14} style={{ color: i === 0 ? "#4F9EFF" : "#50506A" }} />
            </button>
          ))}
          {/* Toggle browser from tool rail */}
          <div className="mt-auto">
            <button onClick={() => setBrowserOpen(v => !v)} className="flex items-center justify-center rounded transition-colors" style={{ width: 28, height: 28, background: browserOpen ? "#A855F718" : "transparent", border: browserOpen ? "1px solid #A855F740" : "1px solid transparent" }}>
              <LayoutTemplate size={14} style={{ color: browserOpen ? "#A855F7" : "#50506A" }} />
            </button>
          </div>
        </div>

        {/* ── Canvas area ── */}
        <div className="flex flex-col flex-1 overflow-hidden">
          <div
            ref={canvasRef}
            className="flex-1 relative overflow-hidden"
            style={{ background: "#060608" }}
            onDragOver={e => e.preventDefault()}
            onDrop={handleCanvasDrop}
          >
            {/* Grid overlay */}
            <svg className="absolute inset-0 w-full h-full pointer-events-none" style={{ opacity: 0.06 }}>
              <defs>
                <pattern id="grid" width="40" height="40" patternUnits="userSpaceOnUse">
                  <path d="M 40 0 L 0 0 0 40" fill="none" stroke="#4F9EFF" strokeWidth="0.5"/>
                </pattern>
              </defs>
              <rect width="100%" height="100%" fill="url(#grid)"/>
            </svg>
            {/* Crosshair */}
            <svg className="absolute inset-0 w-full h-full pointer-events-none" style={{ opacity: 0.08 }}>
              <line x1="50%" y1="0" x2="50%" y2="100%" stroke="#4F9EFF" strokeWidth="0.5" strokeDasharray="4 4"/>
              <line x1="0" y1="50%" x2="100%" y2="50%" stroke="#4F9EFF" strokeWidth="0.5" strokeDasharray="4 4"/>
            </svg>
            {/* Corner brackets */}
            {[["4px","4px","tl"],["calc(100%-4px)","4px","tr"],["4px","calc(100%-4px)","bl"],["calc(100%-4px)","calc(100%-4px)","br"]].map(([x,y,pos]) => (
              <svg key={pos as string} className="absolute pointer-events-none" style={{ left: pos.includes("r") ? "auto" : x, right: pos.includes("r") ? "4px" : "auto", top: pos.includes("b") ? "auto" : y, bottom: pos.includes("b") ? "4px" : "auto", width: 16, height: 16, opacity: 0.4 }}>
                {pos === "tl" && <><line x1="0" y1="0" x2="12" y2="0" stroke="#4F9EFF" strokeWidth="1.5"/><line x1="0" y1="0" x2="0" y2="12" stroke="#4F9EFF" strokeWidth="1.5"/></>}
                {pos === "tr" && <><line x1="16" y1="0" x2="4" y2="0" stroke="#4F9EFF" strokeWidth="1.5"/><line x1="16" y1="0" x2="16" y2="12" stroke="#4F9EFF" strokeWidth="1.5"/></>}
                {pos === "bl" && <><line x1="0" y1="16" x2="12" y2="16" stroke="#4F9EFF" strokeWidth="1.5"/><line x1="0" y1="16" x2="0" y2="4" stroke="#4F9EFF" strokeWidth="1.5"/></>}
                {pos === "br" && <><line x1="16" y1="16" x2="4" y2="16" stroke="#4F9EFF" strokeWidth="1.5"/><line x1="16" y1="16" x2="16" y2="4" stroke="#4F9EFF" strokeWidth="1.5"/></>}
              </svg>
            ))}

            {/* Drop zone highlight when dragging */}
            {dragging && (
              <div className="absolute inset-0 pointer-events-none" style={{ border: "2px dashed #A855F760", background: "#A855F708", zIndex: 10 }}>
                <div className="absolute inset-0 flex items-center justify-center">
                  <div className="flex flex-col items-center gap-2 px-6 py-4 rounded-lg" style={{ background: "#1A2035CC", border: "1px solid #A855F740" }}>
                    <LayoutTemplate size={28} style={{ color: "#A855F7" }} />
                    <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 700, fontSize: 13, color: "#F8F8FF" }}>Drop to add overlay</span>
                  </div>
                </div>
              </div>
            )}

            {/* Drop flash */}
            {dropFlash && (
              <div className="absolute inset-0 pointer-events-none" style={{ background: "#22C55E18", zIndex: 20, transition: "opacity 0.3s" }} />
            )}

            {/* Placed overlays on canvas */}
            {canvasOverlays.map(ov => (
              <div
                key={ov.id}
                className="absolute flex items-center gap-1 px-2 py-1 rounded"
                style={{
                  left: `${Math.min(Math.max(ov.x, 5), 80)}%`,
                  top: `${Math.min(Math.max(ov.y, 5), 80)}%`,
                  background: `${ov.color}22`,
                  border: `1px solid ${ov.color}60`,
                  zIndex: 5,
                  cursor: "move",
                  maxWidth: "40%"
                }}
              >
                <div className="w-1.5 h-1.5 rounded-full shrink-0" style={{ background: ov.color }} />
                <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, color: "#F8F8FF", whiteSpace: "nowrap", overflow: "hidden", textOverflow: "ellipsis" }}>{ov.name}</span>
              </div>
            ))}

            {/* Empty state */}
            {canvasOverlays.length === 0 && !dragging && (
              <div className="absolute inset-0 flex flex-col items-center justify-center gap-2 pointer-events-none" style={{ opacity: 0.35 }}>
                <Monitor size={32} style={{ color: "#4F9EFF" }} />
                <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 12, color: "#8892A4" }}>1920 × 1080 · 60fps · H.264</span>
                <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, color: "#50506A" }}>Drag overlays from the library to place them here</span>
              </div>
            )}

            {/* Added toast */}
            {addedToast && (
              <div className="absolute bottom-4 left-1/2 -translate-x-1/2 flex items-center gap-2 px-3 py-2 rounded-lg" style={{ background: "#1A2035", border: "1px solid #22C55E60", zIndex: 30 }}>
                <Check size={13} style={{ color: "#22C55E" }} />
                <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 12, color: "#F8F8FF", fontWeight: 600 }}>{addedToast} added to scene</span>
              </div>
            )}
          </div>

          {/* Transitions bar */}
          <div className="flex items-center gap-2 px-3 shrink-0" style={{ height: 44, background: "#1A2035", borderTop: "1px solid #2A3350" }}>
            <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 600, fontSize: 10, color: "#8892A4", letterSpacing: "0.1em", textTransform: "uppercase", marginRight: 4 }}>Transition</span>
            {TRANSITIONS.map(t => (
              <button key={t} onClick={() => setActiveTrans(t)} className="px-3 py-1 rounded transition-all" style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, fontWeight: activeTrans === t ? 700 : 400, background: activeTrans === t ? "#4F9EFF18" : "#1E2640", border: `1px solid ${activeTrans === t ? "#4F9EFF60" : "#303D5A"}`, color: activeTrans === t ? "#4F9EFF" : "#606078" }}>
                {t}
              </button>
            ))}
            <div className="flex-1" />
            <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, color: "#50506A" }}>Duration</span>
            <div className="flex items-center rounded px-2" style={{ height: 24, background: "#1E2640", border: "1px solid #303D5A" }}>
              <span className="mono" style={{ fontSize: 11, color: "#4F9EFF" }}>300ms</span>
            </div>
          </div>
        </div>

        {/* ── Right: Sources + Properties ── */}
        <div className="flex flex-col shrink-0 overflow-y-auto" style={{ width: 240, background: "#1A2035", borderLeft: "1px solid #2A3350" }}>
          {/* Sources header */}
          <div className="px-3 py-1.5 flex items-center justify-between shrink-0" style={{ borderBottom: "1px solid #2A3350", background: "#141928" }}>
            <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 600, fontSize: 11, color: "#A0A0B8", letterSpacing: "0.1em", textTransform: "uppercase" }}>Sources</span>
            <div className="flex gap-1">
              {[Plus, Trash2].map((Icon, i) => (
                <button key={i} className="flex items-center justify-center rounded" style={{ width: 20, height: 20, background: "#1A1A24", border: "1px solid #303D5A" }}>
                  <Icon size={11} style={{ color: i === 0 ? "#4F9EFF" : "#606078" }} />
                </button>
              ))}
            </div>
          </div>
          <div className="flex flex-col" style={{ borderBottom: "1px solid #2A3350" }}>
            {sources.map(src => (
              <div key={src.id} onClick={() => setSelectedSource(src.id)} className="flex items-center gap-2 px-3 py-2 cursor-pointer transition-colors" style={{ background: selectedSource === src.id ? `${src.color}12` : "transparent", borderLeft: selectedSource === src.id ? `2px solid ${src.color}` : "2px solid transparent" }}>
                <div className="flex items-center gap-1">
                  <button className="opacity-50 hover:opacity-100" onClick={e => e.stopPropagation()}>
                    {src.visible ? <Eye size={11} style={{ color: "#8892A4" }} /> : <EyeOff size={11} style={{ color: "#3A3A50" }} />}
                  </button>
                  <button className="opacity-50 hover:opacity-100" onClick={e => e.stopPropagation()}>
                    {src.locked ? <Lock size={11} style={{ color: "#FBBF24" }} /> : <Unlock size={11} style={{ color: "#8892A4" }} />}
                  </button>
                </div>
                <src.icon size={13} style={{ color: src.color }} />
                <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 12, color: selectedSource === src.id ? "#F8F8FF" : "#A0A0B8", fontWeight: selectedSource === src.id ? 600 : 400 }}>{src.name}</span>
                <div className="ml-auto flex gap-0.5">
                  {[ChevronUp, ChevronDown].map((Icon, i) => (
                    <button key={i} onClick={e => e.stopPropagation()} className="flex items-center justify-center rounded" style={{ width: 16, height: 16 }}>
                      <Icon size={10} style={{ color: "#3A3A50" }} />
                    </button>
                  ))}
                </div>
              </div>
            ))}
          </div>

          {/* Properties */}
          <div className="px-3 py-1.5 shrink-0" style={{ borderBottom: "1px solid #2A3350", background: "#141928" }}>
            <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 600, fontSize: 11, color: "#A0A0B8", letterSpacing: "0.1em", textTransform: "uppercase" }}>
              Properties — {sources.find(s => s.id === selectedSource)?.name ?? "—"}
            </span>
          </div>
          <div className="flex flex-col gap-0 px-3 py-2">
            {[["Position X","640"],["Position Y","270"],["Width","640"],["Height","360"],["Rotation","0°"],["Opacity","100%"]].map(([label, val]) => (
              <div key={label} className="flex items-center justify-between py-1.5" style={{ borderBottom: "1px solid #1A1A24" }}>
                <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, color: "#8892A4" }}>{label}</span>
                <div className="flex items-center rounded px-2" style={{ height: 22, background: "#1E2640", border: "1px solid #303D5A", minWidth: 64 }}>
                  <span className="mono" style={{ fontSize: 11, color: "#A855F7" }}>{val}</span>
                </div>
              </div>
            ))}
            <div className="flex items-center justify-between py-2 mt-1" style={{ borderTop: "1px solid #303D5A" }}>
              <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, color: "#A0A0B8", fontWeight: 600 }}>Chroma Key</span>
              <div className="flex items-center gap-2">
                <div className="w-4 h-4 rounded-sm" style={{ background: "#22C55E", border: "1px solid #303D5A" }} />
                <div className="w-8 h-4 rounded-full relative" style={{ background: "#22C55E" }}>
                  <div className="absolute top-0.5 right-0.5 w-3 h-3 rounded-full bg-white" />
                </div>
              </div>
            </div>
            {selectedSource !== null && (
              <div className="flex gap-2 mt-1">
                {[
                  { label: "Flip H", action: () => setTransform(selectedSource!, { flipH: !getTransform(selectedSource!).flipH }) },
                  { label: "Flip V", action: () => setTransform(selectedSource!, { flipV: !getTransform(selectedSource!).flipV }) },
                  { label: "Reset",  action: () => setTransforms(prev => { const n = { ...prev }; delete n[selectedSource!]; return n; }) },
                ].map(({ label, action }) => (
                  <button key={label} onClick={action} className="flex-1 py-1 rounded transition-colors hover:bg-white/5"
                    style={{ background: "#1E2640", border: "1px solid #303D5A", color: "#8892A4", fontFamily: "'DM Sans', sans-serif", fontSize: 10, cursor: "pointer" }}>
                    {label}
                  </button>
                ))}
              </div>
            )}
          </div>
        </div>

      </div>
      {showAddSource && <AddSourceModal onAdd={handleAddSource} onClose={() => setShowAddSource(false)} />}
    </AppSidebar>
  );
}
