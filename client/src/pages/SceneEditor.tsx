/**
 * Nexus Broadcast — Scene Editor & Sources Panel (Screen 5)
 * Obsidian Studio Dark Theme
 */
import { useState } from "react";
import AppSidebar from "@/components/AppSidebar";
import { Eye, EyeOff, Lock, Unlock, Monitor, Camera, Type, Image, Globe, MoreHorizontal, Plus, Move, Crop, RotateCw, Maximize2, AlignLeft, FlipHorizontal2, FlipVertical2, ChevronDown } from "lucide-react";

const sourcesList = [
  { id: 1, name: "Game Capture", icon: Monitor, visible: true, locked: false },
  { id: 2, name: "Webcam", icon: Camera, visible: true, locked: false, selected: true },
  { id: 3, name: "Text Overlay", icon: Type, visible: true, locked: false },
  { id: 4, name: "Logo Image", icon: Image, visible: true, locked: true },
  { id: 5, name: "Alert Overlay", icon: Globe, visible: false, locked: false },
];

const transitions = [
  { id: "cut", label: "Cut" },
  { id: "fade", label: "Fade" },
  { id: "slide", label: "Slide" },
  { id: "wipe", label: "Wipe" },
  { id: "stinger", label: "Stinger" },
];

const tools = [
  { icon: Move, label: "Move" },
  { icon: Maximize2, label: "Resize" },
  { icon: Crop, label: "Crop" },
  { icon: RotateCw, label: "Rotate" },
  { icon: AlignLeft, label: "Align" },
  { icon: Lock, label: "Lock" },
];

function NumberInput({ label, value }: { label: string; value: string }) {
  return (
    <div>
      <div style={{ fontSize: 9, color: "rgba(255,255,255,0.4)", marginBottom: 3 }}>{label}</div>
      <input
        defaultValue={value}
        className="w-full rounded px-2 py-1 outline-none"
        style={{ background: "#0A0B0F", border: "1px solid rgba(255,255,255,0.1)", color: "#fff", fontSize: 11, fontFamily: "'JetBrains Mono', monospace" }}
      />
    </div>
  );
}

function Toggle({ defaultChecked = false, color = "#3B82F6" }: { defaultChecked?: boolean; color?: string }) {
  const [on, setOn] = useState(defaultChecked);
  return (
    <button onClick={() => setOn(v => !v)} className="rounded-full transition-colors duration-200" style={{ width: 32, height: 18, background: on ? color : "rgba(255,255,255,0.12)", position: "relative", flexShrink: 0 }}>
      <div style={{ position: "absolute", top: 2, left: on ? 16 : 2, width: 14, height: 14, background: "#fff", borderRadius: "50%", transition: "left 0.2s ease" }} />
    </button>
  );
}

export default function SceneEditor() {
  const [selectedSource, setSelectedSource] = useState(2);
  const [activeTool, setActiveTool] = useState(0);
  const [activeTransition, setActiveTransition] = useState("cut");
  const [sources, setSources] = useState(sourcesList);

  const toggleVisibility = (id: number) => {
    setSources(s => s.map(src => src.id === id ? { ...src, visible: !src.visible } : src));
  };

  return (
    <AppSidebar>
      <div className="flex flex-col h-full overflow-hidden" style={{ fontFamily: "'Inter', sans-serif" }}>
        {/* Top bar */}
        <div className="flex items-center px-4 border-b shrink-0" style={{ borderColor: "rgba(255,255,255,0.07)", minHeight: 46, background: "#0D0E12" }}>
          <span style={{ fontFamily: "'Space Grotesk', sans-serif", fontWeight: 700, fontSize: 12, color: "#fff", letterSpacing: "0.1em" }}>SCENE EDITOR</span>
          <div className="mx-3 w-px h-4" style={{ background: "rgba(255,255,255,0.1)" }} />
          <div className="flex items-center gap-1.5 rounded px-2 py-0.5" style={{ background: "rgba(59,130,246,0.1)", border: "1px solid rgba(59,130,246,0.25)" }}>
            <span style={{ fontSize: 10, color: "#3B82F6", fontFamily: "'JetBrains Mono', monospace" }}>My Broadcast Project</span>
          </div>
          <div className="ml-auto flex items-center gap-3" style={{ fontSize: 10, color: "rgba(255,255,255,0.3)", fontFamily: "'JetBrains Mono', monospace" }}>
            <div className="flex items-center gap-1.5">
              <div className="w-1.5 h-1.5 rounded-full animate-pulse" style={{ background: "#EF4444" }} />
              <span>LIVE 00:00:00</span>
            </div>
            <span style={{ color: "rgba(255,255,255,0.12)" }}>|</span>
            <span>REC 00:00:00</span>
            <span style={{ color: "rgba(255,255,255,0.12)" }}>|</span>
            <span>CPU <span style={{ color: "#22C55E" }}>2.1%</span></span>
            <span style={{ color: "rgba(255,255,255,0.12)" }}>|</span>
            <span><span style={{ color: "#3B82F6" }}>60.00</span> FPS</span>
            <span style={{ color: "rgba(255,255,255,0.12)" }}>|</span>
            <span><span style={{ color: "#06B6D4" }}>5120</span> KB/s</span>
          </div>
        </div>

        {/* Main area */}
        <div className="flex flex-1 overflow-hidden">
          {/* Canvas + Tools */}
          <div className="flex flex-col flex-1 overflow-hidden">
            {/* Canvas */}
            <div className="flex-1 relative overflow-hidden p-3" style={{ background: "#0A0B0F" }}>
              {/* Checkerboard border */}
              <div className="absolute inset-3 rounded" style={{ border: "1px solid rgba(255,255,255,0.1)" }}>
                {/* Canvas content */}
                <div className="w-full h-full relative overflow-hidden rounded" style={{ background: "#111318" }}>
                  {/* Background placeholder */}
                  <div className="absolute inset-0 flex items-center justify-center" style={{ background: "linear-gradient(135deg, #0D1117 0%, #161B22 50%, #0D1117 100%)" }}>
                    <div className="text-center opacity-20">
                      <Monitor size={48} color="#fff" />
                      <div style={{ color: "#fff", fontSize: 12, marginTop: 8, fontFamily: "'JetBrains Mono', monospace" }}>Game Capture Layer</div>
                    </div>
                  </div>
                  {/* Canvas grid */}
                  <svg className="absolute inset-0 w-full h-full pointer-events-none" style={{ opacity: 0.04 }}>
                    <defs>
                      <pattern id="canvasgrid" width="60" height="60" patternUnits="userSpaceOnUse">
                        <path d="M 60 0 L 0 0 0 60" fill="none" stroke="rgba(255,255,255,1)" strokeWidth="0.5" />
                      </pattern>
                    </defs>
                    <rect width="100%" height="100%" fill="url(#canvasgrid)" />
                  </svg>
                  {/* Crosshair center */}
                  <div className="absolute inset-0 pointer-events-none flex items-center justify-center">
                    <div style={{ width: 20, height: 1, background: "rgba(59,130,246,0.25)" }} />
                    <div style={{ position: "absolute", width: 1, height: 20, background: "rgba(59,130,246,0.25)" }} />
                  </div>
                  {/* Canvas metadata strip */}
                  <div className="absolute bottom-0 left-0 right-0 flex items-center justify-between px-2 py-1" style={{ background: "rgba(0,0,0,0.6)", borderTop: "1px solid rgba(255,255,255,0.06)" }}>
                    <span style={{ fontSize: 9, color: "rgba(255,255,255,0.35)", fontFamily: "'JetBrains Mono', monospace" }}>1920×1080 · 16:9 · 60fps</span>
                    <span style={{ fontSize: 9, color: "rgba(59,130,246,0.7)", fontFamily: "'JetBrains Mono', monospace" }}>SCENE 1 · 5 SOURCES</span>
                    <span style={{ fontSize: 9, color: "rgba(255,255,255,0.35)", fontFamily: "'JetBrains Mono', monospace" }}>SAFE AREA: ON</span>
                  </div>

                  {/* Webcam source (selected) */}
                  <div
                    className="absolute"
                    style={{
                      left: "5%", bottom: "10%", width: "28%", height: "38%",
                      border: "2px solid #3B82F6",
                      boxShadow: "0 0 0 1px rgba(59,130,246,0.3)",
                      background: "#1A1D2B",
                    }}
                  >
                    <div className="w-full h-full flex items-center justify-center" style={{ background: "linear-gradient(135deg, #1a1d2b, #0e1117)" }}>
                      <Camera size={20} color="rgba(59,130,246,0.4)" />
                    </div>
                    {/* Resize handles */}
                    {[["top-0 left-0 -translate-x-1/2 -translate-y-1/2", ""], ["top-0 right-0 translate-x-1/2 -translate-y-1/2", ""], ["bottom-0 left-0 -translate-x-1/2 translate-y-1/2", ""], ["bottom-0 right-0 translate-x-1/2 translate-y-1/2", ""], ["top-0 left-1/2 -translate-x-1/2 -translate-y-1/2", ""], ["bottom-0 left-1/2 -translate-x-1/2 translate-y-1/2", ""], ["top-1/2 left-0 -translate-x-1/2 -translate-y-1/2", ""], ["top-1/2 right-0 translate-x-1/2 -translate-y-1/2", ""]].map(([pos], i) => (
                      <div key={i} className={`absolute w-2 h-2 rounded-sm ${pos}`} style={{ background: "#fff", border: "1px solid #3B82F6" }} />
                    ))}
                  </div>

                  {/* Text overlay */}
                  <div className="absolute top-4 left-1/2 -translate-x-1/2" style={{ border: "1px dashed rgba(255,255,255,0.2)", padding: "4px 12px" }}>
                    <span style={{ fontFamily: "'Space Grotesk', sans-serif", fontWeight: 700, fontSize: 16, color: "rgba(255,255,255,0.7)", letterSpacing: "0.04em" }}>LIVE BROADCAST</span>
                  </div>

                  {/* Logo */}
                  <div className="absolute top-3 right-3" style={{ border: "1px dashed rgba(255,255,255,0.15)", padding: 6 }}>
                    <div className="w-12 h-12 rounded flex items-center justify-center" style={{ background: "rgba(59,130,246,0.2)" }}>
                      <Monitor size={18} color="#3B82F6" />
                    </div>
                  </div>

                  {/* Safe area corners */}
                  {[["top-2 left-2", "border-t border-l"], ["top-2 right-2", "border-t border-r"], ["bottom-2 left-2", "border-b border-l"], ["bottom-2 right-2", "border-b border-r"]].map(([pos, borders], i) => (
                    <div key={i} className={`absolute ${pos} w-5 h-5 ${borders}`} style={{ borderColor: "rgba(59,130,246,0.4)" }} />
                  ))}
                </div>
              </div>

              {/* Tool sidebar */}
              <div className="absolute right-4 top-1/2 -translate-y-1/2 flex flex-col gap-1">
                {tools.map((tool, i) => (
                  <button
                    key={i}
                    onClick={() => setActiveTool(i)}
                    className="rounded flex items-center justify-center transition-all duration-150"
                    style={{
                      width: 32, height: 32,
                      background: activeTool === i ? "#3B82F6" : "rgba(255,255,255,0.06)",
                      border: `1px solid ${activeTool === i ? "#3B82F6" : "rgba(255,255,255,0.1)"}`,
                    }}
                  >
                    <tool.icon size={14} color={activeTool === i ? "#fff" : "rgba(255,255,255,0.5)"} />
                  </button>
                ))}
              </div>
            </div>

            {/* Transitions */}
            <div className="shrink-0 px-3 py-2 border-t" style={{ borderColor: "rgba(255,255,255,0.07)", background: "#111318" }}>
              <div className="flex items-center gap-4">
                <span style={{ fontSize: 10, fontWeight: 600, color: "rgba(255,255,255,0.4)", letterSpacing: "0.1em", whiteSpace: "nowrap" }}>TRANSITIONS</span>
                <div className="flex gap-2">
                  {transitions.map(t => (
                    <button
                      key={t.id}
                      onClick={() => setActiveTransition(t.id)}
                      className="rounded px-3 py-1.5 transition-all"
                      style={{
                        background: activeTransition === t.id ? "rgba(59,130,246,0.2)" : "#0E0F14",
                        border: `1px solid ${activeTransition === t.id ? "#3B82F6" : "rgba(255,255,255,0.08)"}`,
                        color: activeTransition === t.id ? "#3B82F6" : "rgba(255,255,255,0.4)",
                        fontSize: 11, fontWeight: activeTransition === t.id ? 600 : 400,
                      }}
                    >
                      {t.label}
                    </button>
                  ))}
                </div>
                <div className="flex items-center gap-2 ml-auto">
                  <span style={{ fontSize: 10, color: "rgba(255,255,255,0.4)" }}>Duration</span>
                  <input defaultValue="300" className="rounded px-2 py-1 outline-none" style={{ width: 60, background: "#0E0F14", border: "1px solid rgba(255,255,255,0.1)", color: "#fff", fontSize: 11, fontFamily: "'JetBrains Mono', monospace" }} />
                  <span style={{ fontSize: 10, color: "rgba(255,255,255,0.4)" }}>ms</span>
                  <button className="rounded px-3 py-1.5 text-xs font-semibold" style={{ background: "#3B82F6", color: "#fff", fontSize: 11 }}>Apply Transition</button>
                </div>
              </div>
            </div>
          </div>

          {/* Right: Sources + Properties */}
          <div className="flex flex-col shrink-0 overflow-hidden" style={{ width: 320, borderLeft: "1px solid rgba(255,255,255,0.07)" }}>
            {/* Sources */}
            <div className="shrink-0" style={{ borderBottom: "1px solid rgba(255,255,255,0.07)" }}>
              <div className="flex items-center justify-between px-3 py-2" style={{ borderBottom: "1px solid rgba(255,255,255,0.06)" }}>
                <span style={{ fontSize: 10, fontWeight: 600, color: "rgba(255,255,255,0.4)", letterSpacing: "0.1em" }}>SOURCES</span>
                <div className="flex gap-1">
                  <button className="rounded p-1 hover:bg-white/5"><Plus size={12} color="rgba(255,255,255,0.5)" /></button>
                  <button className="rounded p-1 hover:bg-white/5"><MoreHorizontal size={12} color="rgba(255,255,255,0.5)" /></button>
                </div>
              </div>
              {sources.map(src => (
                <div
                  key={src.id}
                  onClick={() => setSelectedSource(src.id)}
                  className="flex items-center gap-2 px-3 py-2 cursor-pointer transition-all"
                  style={{
                    background: selectedSource === src.id ? "#1A1D2B" : "transparent",
                    borderLeft: `3px solid ${selectedSource === src.id ? "#3B82F6" : "transparent"}`,
                    paddingLeft: selectedSource === src.id ? 9 : 12,
                  }}
                >
                  <div className="w-4 h-4 flex items-center justify-center cursor-grab" style={{ color: "rgba(255,255,255,0.2)" }}>⠿</div>
                  <button onClick={e => { e.stopPropagation(); toggleVisibility(src.id); }}>
                    {src.visible ? <Eye size={13} color="rgba(255,255,255,0.5)" /> : <EyeOff size={13} color="rgba(255,255,255,0.25)" />}
                  </button>
                  {src.locked ? <Lock size={12} color="rgba(255,255,255,0.3)" /> : <Unlock size={12} color="rgba(255,255,255,0.2)" />}
                  <src.icon size={13} color={selectedSource === src.id ? "#3B82F6" : "rgba(255,255,255,0.5)"} />
                  <span style={{ flex: 1, fontSize: 12, color: selectedSource === src.id ? "#fff" : "rgba(255,255,255,0.6)", fontWeight: selectedSource === src.id ? 500 : 400 }}>{src.name}</span>
                  <button className="rounded p-0.5 hover:bg-white/5"><MoreHorizontal size={12} color="rgba(255,255,255,0.3)" /></button>
                </div>
              ))}
            </div>

            {/* Source Properties */}
            <div className="flex-1 overflow-y-auto">
              <div className="px-3 py-2" style={{ borderBottom: "1px solid rgba(255,255,255,0.06)" }}>
                <span style={{ fontSize: 10, fontWeight: 600, color: "rgba(255,255,255,0.4)", letterSpacing: "0.1em" }}>SOURCE PROPERTIES: WEBCAM</span>
              </div>
              <div className="p-3 space-y-3">
                {/* Position & Size */}
                <div className="grid grid-cols-2 gap-2">
                  <div>
                    <div style={{ fontSize: 9, color: "rgba(255,255,255,0.4)", marginBottom: 4 }}>POSITION</div>
                    <div className="grid grid-cols-2 gap-1">
                      <NumberInput label="X" value="80.0" />
                      <NumberInput label="Y" value="720.0" />
                    </div>
                  </div>
                  <div>
                    <div style={{ fontSize: 9, color: "rgba(255,255,255,0.4)", marginBottom: 4 }}>SIZE</div>
                    <div className="grid grid-cols-2 gap-1">
                      <NumberInput label="W" value="640.0" />
                      <NumberInput label="H" value="360.0" />
                    </div>
                  </div>
                </div>

                {/* Rotation & Opacity */}
                <div className="grid grid-cols-2 gap-2">
                  <div>
                    <div style={{ fontSize: 9, color: "rgba(255,255,255,0.4)", marginBottom: 4 }}>ROTATION</div>
                    <NumberInput label="" value="0.0°" />
                  </div>
                  <div>
                    <div style={{ fontSize: 9, color: "rgba(255,255,255,0.4)", marginBottom: 4 }}>OPACITY</div>
                    <div className="flex items-center gap-2">
                      <div className="flex-1 rounded-full" style={{ height: 4, background: "rgba(255,255,255,0.08)" }}>
                        <div style={{ width: "100%", height: "100%", background: "#3B82F6", borderRadius: 9999 }} />
                      </div>
                      <span style={{ fontSize: 10, color: "#fff", fontFamily: "'JetBrains Mono', monospace" }}>100%</span>
                    </div>
                  </div>
                </div>

                {/* Chroma Key */}
                <div className="rounded p-2.5" style={{ background: "#0E0F14", border: "1px solid rgba(255,255,255,0.07)" }}>
                  <div className="flex items-center justify-between mb-2">
                    <span style={{ fontSize: 11, fontWeight: 600, color: "#fff" }}>Chroma Key</span>
                    <Toggle defaultChecked={true} color="#22C55E" />
                  </div>
                  <div className="grid grid-cols-2 gap-2">
                    <div>
                      <div style={{ fontSize: 9, color: "rgba(255,255,255,0.4)", marginBottom: 3 }}>Color</div>
                      <div className="flex items-center gap-1.5">
                        <div className="w-5 h-5 rounded" style={{ background: "#22C55E", border: "1px solid rgba(255,255,255,0.2)" }} />
                        <span style={{ fontSize: 10, color: "#22C55E", fontFamily: "'JetBrains Mono', monospace" }}>#22C55E</span>
                      </div>
                    </div>
                    <div>
                      <div style={{ fontSize: 9, color: "rgba(255,255,255,0.4)", marginBottom: 3 }}>Similarity</div>
                      <div className="flex items-center gap-1">
                        <div className="flex-1 rounded-full" style={{ height: 3, background: "rgba(255,255,255,0.08)" }}>
                          <div style={{ width: "50%", height: "100%", background: "#3B82F6", borderRadius: 9999 }} />
                        </div>
                        <span style={{ fontSize: 9, color: "rgba(255,255,255,0.5)", fontFamily: "'JetBrains Mono', monospace" }}>50%</span>
                      </div>
                    </div>
                  </div>
                </div>

                {/* Flip */}
                <div>
                  <div style={{ fontSize: 9, color: "rgba(255,255,255,0.4)", marginBottom: 6 }}>FLIP</div>
                  <div className="flex gap-2">
                    <button className="flex items-center gap-1.5 rounded px-3 py-1.5 text-xs" style={{ background: "#0E0F14", border: "1px solid rgba(255,255,255,0.1)", color: "rgba(255,255,255,0.6)" }}>
                      <FlipHorizontal2 size={12} /> Flip Horizontal
                    </button>
                    <button className="flex items-center gap-1.5 rounded px-3 py-1.5 text-xs" style={{ background: "#0E0F14", border: "1px solid rgba(255,255,255,0.1)", color: "rgba(255,255,255,0.6)" }}>
                      <FlipVertical2 size={12} /> Flip Vertical
                    </button>
                  </div>
                </div>

                {/* Crop */}
                <div>
                  <div style={{ fontSize: 9, color: "rgba(255,255,255,0.4)", marginBottom: 6 }}>CROP (px)</div>
                  <div className="grid grid-cols-2 gap-2">
                    {["Top", "Bottom", "Left", "Right"].map(side => (
                      <NumberInput key={side} label={side} value="0" />
                    ))}
                  </div>
                  <button className="w-full mt-2 rounded py-1.5 text-xs" style={{ background: "rgba(255,255,255,0.04)", border: "1px solid rgba(255,255,255,0.08)", color: "rgba(255,255,255,0.5)" }}>Reset Crop</button>
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>
    </AppSidebar>
  );
}
