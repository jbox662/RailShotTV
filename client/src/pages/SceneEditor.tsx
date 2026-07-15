// RailShotTV — Chromatic Command — Scene Editor
import { useState } from "react";
import AppSidebar from "@/components/AppSidebar";
import { Eye, EyeOff, Lock, Unlock, Move, Crop, RotateCcw, AlignCenter, Layers, Plus, Trash2, ChevronUp, ChevronDown, Monitor, Camera, Type, Image, Zap } from "lucide-react";

const SOURCES = [
  { id: 1, name: "Game Capture", type: "capture", icon: Monitor, color: "#3B82F6", visible: true, locked: false },
  { id: 2, name: "Webcam", type: "camera", icon: Camera, color: "#8B5CF6", visible: true, locked: false, selected: true },
  { id: 3, name: "Score Overlay", type: "text", icon: Type, color: "#FF4D1C", visible: true, locked: false },
  { id: 4, name: "RailShotTV Logo", type: "image", icon: Image, color: "#10B981", visible: true, locked: true },
  { id: 5, name: "Alert Overlay", type: "alert", icon: Zap, color: "#F59E0B", visible: false, locked: false },
];

const TRANSITIONS = ["Cut","Fade","Slide","Wipe","Stinger"];

export default function SceneEditor() {
  const [selectedSource, setSelectedSource] = useState(2);
  const [activeTrans, setActiveTrans] = useState("Cut");

  return (
    <AppSidebar>
      {/* Top bar */}
      <div className="flex items-center gap-3 px-4 shrink-0" style={{ height: 46, background: "#0D0D15", borderBottom: "1px solid #1E1E2E" }}>
        <div className="flex items-center gap-1 mr-1">
          <span style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 18, color: "#F8F8FF", letterSpacing: "0.06em", lineHeight: 1 }}>RAILSHOT</span>
          <span style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 18, color: "#FF4D1C", letterSpacing: "0.06em", lineHeight: 1 }}>TV</span>
        </div>
        <div className="w-px h-4 mx-1" style={{ background: "#2A2A3A" }} />
        <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 600, fontSize: 11, color: "#606078", letterSpacing: "0.1em", textTransform: "uppercase" }}>Scene Editor</span>
        <div className="px-2 py-0.5 rounded" style={{ background: "#3B82F618", border: "1px solid #3B82F640" }}>
          <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, color: "#3B82F6", fontWeight: 600 }}>Table 1 — Main</span>
        </div>
        <div className="flex-1" />
        <span className="mono" style={{ fontSize: 11, color: "#50506A" }}>1920×1080</span>
        <span className="mono" style={{ fontSize: 11, color: "#50506A" }}>60fps</span>
        <div className="flex items-center gap-1.5">
          <div className="live-dot w-1.5 h-1.5 rounded-full" style={{ background: "#FF4D1C" }} />
          <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 700, fontSize: 11, color: "#FF4D1C" }}>LIVE</span>
        </div>
      </div>

      <div className="flex flex-1 overflow-hidden">
        {/* Tool rail */}
        <div className="flex flex-col items-center gap-1 py-2 px-1 shrink-0" style={{ width: 40, background: "#0D0D15", borderRight: "1px solid #1E1E2E" }}>
          {[Move, Crop, RotateCcw, AlignCenter, Layers].map((Icon, i) => (
            <button key={i} className="flex items-center justify-center rounded transition-colors" style={{ width: 28, height: 28, background: i === 0 ? "#3B82F618" : "transparent", border: i === 0 ? "1px solid #3B82F640" : "1px solid transparent" }}>
              <Icon size={14} style={{ color: i === 0 ? "#3B82F6" : "#50506A" }} />
            </button>
          ))}
        </div>

        {/* Canvas area */}
        <div className="flex flex-col flex-1 overflow-hidden">
          <div className="flex-1 relative overflow-hidden" style={{ background: "#060608" }}>
            {/* Grid overlay */}
            <svg className="absolute inset-0 w-full h-full" style={{ opacity: 0.06 }}>
              <defs>
                <pattern id="grid" width="40" height="40" patternUnits="userSpaceOnUse">
                  <path d="M 40 0 L 0 0 0 40" fill="none" stroke="#3B82F6" strokeWidth="0.5"/>
                </pattern>
              </defs>
              <rect width="100%" height="100%" fill="url(#grid)" />
            </svg>
            {/* Corner brackets */}
            {[["top-3 left-3","border-t-2 border-l-2"],["top-3 right-3","border-t-2 border-r-2"],["bottom-10 left-3","border-b-2 border-l-2"],["bottom-10 right-3","border-b-2 border-r-2"]].map(([pos, bdr], i) => (
              <div key={i} className={`absolute ${pos} ${bdr} w-5 h-5`} style={{ borderColor: "#3B82F660" }} />
            ))}
            {/* Center crosshair */}
            <div className="absolute inset-0 flex items-center justify-center pointer-events-none">
              <div style={{ width: 1, height: 40, background: "#3B82F620" }} />
            </div>
            <div className="absolute inset-0 flex items-center justify-center pointer-events-none">
              <div style={{ width: 40, height: 1, background: "#3B82F620" }} />
            </div>
            {/* Selected source indicator */}
            <div className="absolute" style={{ top: "25%", left: "30%", width: "40%", height: "40%", border: "1.5px dashed #8B5CF6", borderRadius: 2 }}>
              {/* Handles */}
              {[["-4px","-4px"],["calc(50% - 4px)","-4px"],["calc(100% - 4px)","-4px"],["-4px","calc(50% - 4px)"],["calc(100% - 4px)","calc(50% - 4px)"],["-4px","calc(100% - 4px)"],["calc(50% - 4px)","calc(100% - 4px)"],["calc(100% - 4px)","calc(100% - 4px)"]].map(([l, t], i) => (
                <div key={i} className="absolute" style={{ left: l, top: t, width: 8, height: 8, background: "#8B5CF6", border: "1px solid #0A0A0F", borderRadius: 1 }} />
              ))}
              <div className="absolute inset-0 flex items-center justify-center">
                <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, color: "#8B5CF680" }}>Webcam</span>
              </div>
            </div>
            {/* Canvas status bar */}
            <div className="absolute bottom-0 left-0 right-0 flex items-center gap-4 px-3 py-1.5" style={{ background: "rgba(0,0,0,0.8)", borderTop: "1px solid #1E1E2E" }}>
              <span className="mono" style={{ fontSize: 10, color: "#50506A" }}>1920×1080</span>
              <span className="mono" style={{ fontSize: 10, color: "#50506A" }}>16:9</span>
              <span className="mono" style={{ fontSize: 10, color: "#50506A" }}>60fps</span>
              <div className="flex-1" />
              <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, color: "#606078" }}>SAFE AREA</span>
              <div className="w-6 h-3 rounded-full relative cursor-pointer" style={{ background: "#1A1A24", border: "1px solid #2A2A3A" }}>
                <div className="absolute top-0.5 left-0.5 w-2 h-2 rounded-full" style={{ background: "#606078" }} />
              </div>
            </div>
          </div>

          {/* Transitions bar */}
          <div className="flex items-center gap-2 px-3 py-2 shrink-0" style={{ background: "#0D0D15", borderTop: "1px solid #1E1E2E" }}>
            <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, color: "#606078", letterSpacing: "0.08em", textTransform: "uppercase", width: 80 }}>Transition</span>
            <div className="flex gap-1">
              {TRANSITIONS.map(t => (
                <button key={t} onClick={() => setActiveTrans(t)} className="px-2.5 py-1 rounded text-xs font-medium transition-all" style={{ background: activeTrans === t ? "#3B82F618" : "#111118", border: activeTrans === t ? "1px solid #3B82F650" : "1px solid #1E1E2E", color: activeTrans === t ? "#3B82F6" : "#606078", fontFamily: "'DM Sans', sans-serif", fontSize: 11 }}>
                  {t}
                </button>
              ))}
            </div>
            <div className="flex items-center gap-1.5 ml-2">
              <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, color: "#606078" }}>Duration</span>
              <div className="flex items-center rounded px-2" style={{ height: 24, background: "#111118", border: "1px solid #2A2A3A" }}>
                <span className="mono" style={{ fontSize: 11, color: "#3B82F6" }}>300ms</span>
              </div>
            </div>
            <button className="ml-auto px-3 py-1 rounded text-xs font-bold" style={{ background: "#3B82F618", border: "1px solid #3B82F640", color: "#3B82F6", fontFamily: "'DM Sans', sans-serif", fontSize: 11 }}>
              APPLY
            </button>
          </div>
        </div>

        {/* Right — Sources + Properties */}
        <div className="flex flex-col shrink-0 overflow-y-auto" style={{ width: 240, background: "#0D0D15", borderLeft: "1px solid #1E1E2E" }}>
          {/* Sources */}
          <div className="panel-header-blue px-3 py-1.5 flex items-center justify-between shrink-0">
            <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 600, fontSize: 11, color: "#A0A0B8", letterSpacing: "0.1em", textTransform: "uppercase" }}>Sources</span>
            <div className="flex gap-1">
              {[Plus, Trash2].map((Icon, i) => (
                <button key={i} className="flex items-center justify-center rounded" style={{ width: 20, height: 20, background: "#1A1A24", border: "1px solid #2A2A3A" }}>
                  <Icon size={11} style={{ color: i === 0 ? "#3B82F6" : "#606078" }} />
                </button>
              ))}
            </div>
          </div>
          <div className="flex flex-col" style={{ borderBottom: "1px solid #1E1E2E" }}>
            {SOURCES.map(src => (
              <div key={src.id} onClick={() => setSelectedSource(src.id)} className="flex items-center gap-2 px-3 py-2 cursor-pointer transition-colors" style={{ background: selectedSource === src.id ? `${src.color}12` : "transparent", borderLeft: selectedSource === src.id ? `2px solid ${src.color}` : "2px solid transparent" }}>
                <div className="flex items-center gap-1">
                  <button className="opacity-50 hover:opacity-100" onClick={e => e.stopPropagation()}>
                    {src.visible ? <Eye size={11} style={{ color: "#606078" }} /> : <EyeOff size={11} style={{ color: "#3A3A50" }} />}
                  </button>
                  <button className="opacity-50 hover:opacity-100" onClick={e => e.stopPropagation()}>
                    {src.locked ? <Lock size={11} style={{ color: "#F59E0B" }} /> : <Unlock size={11} style={{ color: "#606078" }} />}
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
          <div className="panel-header-violet px-3 py-1.5 shrink-0">
            <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 600, fontSize: 11, color: "#A0A0B8", letterSpacing: "0.1em", textTransform: "uppercase" }}>Properties — Webcam</span>
          </div>
          <div className="flex flex-col gap-0 px-3 py-2">
            {[["Position X","640"],["Position Y","270"],["Width","640"],["Height","360"],["Rotation","0°"],["Opacity","100%"]].map(([label, val]) => (
              <div key={label} className="flex items-center justify-between py-1.5" style={{ borderBottom: "1px solid #1A1A24" }}>
                <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, color: "#606078" }}>{label}</span>
                <div className="flex items-center rounded px-2" style={{ height: 22, background: "#111118", border: "1px solid #2A2A3A", minWidth: 64 }}>
                  <span className="mono" style={{ fontSize: 11, color: "#8B5CF6" }}>{val}</span>
                </div>
              </div>
            ))}
            {/* Chroma key */}
            <div className="flex items-center justify-between py-2 mt-1" style={{ borderTop: "1px solid #2A2A3A" }}>
              <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, color: "#A0A0B8", fontWeight: 600 }}>Chroma Key</span>
              <div className="flex items-center gap-2">
                <div className="w-4 h-4 rounded-sm" style={{ background: "#22C55E", border: "1px solid #2A2A3A" }} />
                <div className="w-8 h-4 rounded-full relative" style={{ background: "#10B981" }}>
                  <div className="absolute top-0.5 right-0.5 w-3 h-3 rounded-full bg-white" />
                </div>
              </div>
            </div>
            <div className="flex gap-2 mt-1">
              {["Flip H","Flip V","Reset"].map(l => (
                <button key={l} className="flex-1 py-1 rounded text-xs" style={{ background: "#111118", border: "1px solid #2A2A3A", color: "#606078", fontFamily: "'DM Sans', sans-serif", fontSize: 10 }}>{l}</button>
              ))}
            </div>
          </div>
        </div>
      </div>
    </AppSidebar>
  );
}
