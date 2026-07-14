/**
 * Nexus Broadcast — Main Dashboard (Screen 1)
 * Obsidian Studio Dark Theme
 * Program output preview + Scene switcher + Audio mixer + Stream status
 */
import { useState, useEffect } from "react";
import AppSidebar from "@/components/AppSidebar";
import {
  Radio, Wifi, Cpu, Zap, Globe, Users, Clock, Activity,
  ChevronLeft, ChevronRight, LayoutGrid, List, Plus,
  MoreHorizontal, Volume2, VolumeX, Mic, Monitor, Music, Bell,
  Square, Settings2
} from "lucide-react";

const scenes = [
  { id: 1, name: "Scene 1", active: true },
  { id: 2, name: "Scene 2", active: false },
  { id: 3, name: "Scene 3", active: false },
  { id: 4, name: "Starting Soon", active: false },
  { id: 5, name: "Be Right Back", active: false },
  { id: 6, name: "End Screen", active: false },
];

const audioChannels = [
  { name: "Mic", sub: "Shure SM7B", icon: Mic, level: 72, db: "-3.2" },
  { name: "Desktop", sub: "System Audio", icon: Monitor, level: 45, db: "-10.4" },
  { name: "Music", sub: "Spotify", icon: Music, level: 30, db: "-14.7" },
  { name: "Alert", sub: "Stream Elements", icon: Bell, level: 55, db: "-6.8" },
];

function VUMeter({ level }: { level: number }) {
  const bars = 20;
  return (
    <div className="flex gap-px items-end" style={{ height: 16 }}>
      {Array.from({ length: bars }).map((_, i) => {
        const threshold = (i / bars) * 100;
        const active = threshold < level;
        const color = i < 14 ? "#22C55E" : i < 17 ? "#F59E0B" : "#EF4444";
        return (
          <div
            key={i}
            style={{
              width: 3,
              height: 4 + (i % 3),
              background: active ? color : "rgba(255,255,255,0.1)",
              borderRadius: 1,
              transition: "background 0.05s",
            }}
          />
        );
      })}
    </div>
  );
}

function HealthBar({ value, color }: { value: number; color: string }) {
  return (
    <div className="flex-1 rounded-full overflow-hidden" style={{ height: 4, background: "rgba(255,255,255,0.08)" }}>
      <div style={{ width: `${value}%`, height: "100%", background: color, borderRadius: 9999, transition: "width 0.5s ease" }} />
    </div>
  );
}

export default function Dashboard() {
  const [activeScene, setActiveScene] = useState(1);
  const [uptime, setUptime] = useState(5027);
  const [bitrate, setBitrate] = useState(8450);
  const [viewers, setViewers] = useState(2847);
  const [bitrateHistory, setBitrateHistory] = useState<number[]>(Array.from({ length: 30 }, () => 7000 + Math.random() * 2000));
  const [frame, setFrame] = useState(0);

  useEffect(() => {
    const interval = setInterval(() => {
      setUptime(u => u + 1);
      setBitrate(b => Math.max(6000, Math.min(10000, b + (Math.random() - 0.5) * 400)));
      setViewers(v => Math.max(2700, Math.min(3200, v + Math.round((Math.random() - 0.48) * 10))));
      setBitrateHistory(h => [...h.slice(1), 7000 + Math.random() * 2000]);
      setFrame(f => (f + 1) % 60);
    }, 1000);
    return () => clearInterval(interval);
  }, []);

  const formatTime = (s: number) => {
    const h = Math.floor(s / 3600).toString().padStart(2, "0");
    const m = Math.floor((s % 3600) / 60).toString().padStart(2, "0");
    const sec = (s % 60).toString().padStart(2, "0");
    return `${h}:${m}:${sec}`;
  };

  const maxBitrate = Math.max(...bitrateHistory);
  const svgPoints = bitrateHistory.map((v, i) => `${(i / (bitrateHistory.length - 1)) * 100},${100 - (v / maxBitrate) * 80}`).join(" ");

  return (
    <AppSidebar>
      <div className="flex flex-col h-full overflow-hidden" style={{ fontFamily: "'Inter', sans-serif" }}>
        {/* Top bar */}
        <div className="flex items-center px-4 border-b shrink-0" style={{ borderColor: "rgba(255,255,255,0.07)", minHeight: 46, background: "#0D0E12" }}>
          <span style={{ fontFamily: "'Space Grotesk', sans-serif", fontWeight: 700, fontSize: 12, color: "#fff", letterSpacing: "0.1em" }}>DASHBOARD</span>
          <div className="mx-3 w-px h-4" style={{ background: "rgba(255,255,255,0.1)" }} />
          <div className="flex items-center gap-1.5 rounded px-2 py-0.5" style={{ background: "rgba(239,68,68,0.15)", border: "1px solid rgba(239,68,68,0.35)" }}>
            <div className="w-1.5 h-1.5 rounded-full animate-pulse" style={{ background: "#EF4444" }} />
            <span style={{ fontSize: 10, fontWeight: 700, color: "#EF4444", letterSpacing: "0.1em" }}>LIVE</span>
          </div>
          <div className="ml-2 flex items-center gap-2" style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 10 }}>
            <span style={{ color: "rgba(255,255,255,0.3)" }}>YT</span>
            <span style={{ color: "rgba(255,255,255,0.12)" }}>|</span>
            <span style={{ color: "#06B6D4" }}>{formatTime(uptime)}</span>
            <span style={{ color: "rgba(255,255,255,0.12)" }}>|</span>
            <span style={{ color: "rgba(255,255,255,0.35)" }}>1920×1080</span>
            <span style={{ color: "rgba(255,255,255,0.12)" }}>|</span>
            <span style={{ color: "rgba(255,255,255,0.35)" }}>60fps</span>
            <span style={{ color: "rgba(255,255,255,0.12)" }}>|</span>
            <span style={{ color: "#22C55E" }}>H.264</span>
          </div>
          <div className="ml-auto flex items-center gap-3">
            <div className="flex items-center gap-1.5">
              <div className="w-1.5 h-1.5 rounded-full" style={{ background: "#22C55E" }} />
              <span style={{ fontSize: 9, color: "rgba(255,255,255,0.35)", fontFamily: "'JetBrains Mono', monospace", letterSpacing: "0.06em" }}>SIG OK</span>
            </div>
            <div className="w-px h-4" style={{ background: "rgba(255,255,255,0.08)" }} />
            <span style={{ fontSize: 10, color: "rgba(255,255,255,0.3)" }}>Default Profile</span>
            <Settings2 size={13} color="rgba(255,255,255,0.3)" className="cursor-pointer" />
          </div>
        </div>

        {/* Main content */}
        <div className="flex flex-1 overflow-hidden">
          {/* Center: Preview + Scenes + Audio */}
          <div className="flex-1 flex flex-col overflow-hidden p-3 gap-3">
            {/* Program Output */}
            <div className="rounded-lg overflow-hidden shrink-0" style={{ background: "#111318", border: "1px solid rgba(255,255,255,0.08)" }}>
              <div className="flex items-center justify-between px-3 py-2" style={{ borderBottom: "1px solid rgba(255,255,255,0.06)" }}>
                <span style={{ fontSize: 10, fontWeight: 600, color: "rgba(255,255,255,0.5)", letterSpacing: "0.1em" }}>PROGRAM OUTPUT</span>
                <div className="flex items-center gap-1.5">
                  <div className="w-1.5 h-1.5 rounded-full animate-pulse" style={{ background: "#EF4444" }} />
                  <span style={{ fontSize: 10, fontWeight: 700, color: "#EF4444", letterSpacing: "0.08em" }}>LIVE</span>
                </div>
              </div>
              <div className="relative" style={{ aspectRatio: "16/9", background: "#0A0B0F" }}>
                {/* Grid overlay */}
                <svg className="absolute inset-0 w-full h-full" style={{ opacity: 0.06 }}>
                  <defs>
                    <pattern id="grid" width="40" height="40" patternUnits="userSpaceOnUse">
                      <path d="M 40 0 L 0 0 0 40" fill="none" stroke="rgba(255,255,255,0.5)" strokeWidth="0.5" />
                    </pattern>
                  </defs>
                  <rect width="100%" height="100%" fill="url(#grid)" />
                </svg>
                <div className="absolute inset-0 flex items-center justify-center">
                  <div className="text-center">
                    <Monitor size={32} color="rgba(255,255,255,0.15)" />
                    <div style={{ fontSize: 11, color: "rgba(255,255,255,0.2)", marginTop: 6, fontFamily: "'JetBrains Mono', monospace" }}>1920 × 1080 · 60fps · H.264</div>
                  </div>
                </div>
                {/* Corner markers */}
                {[["top-2 left-2", "border-t border-l"], ["top-2 right-2", "border-t border-r"], ["bottom-2 left-2", "border-b border-l"], ["bottom-2 right-2", "border-b border-r"]].map(([pos, borders], i) => (
                  <div key={i} className={`absolute ${pos} w-4 h-4 ${borders}`} style={{ borderColor: "rgba(59,130,246,0.6)" }} />
                ))}
              </div>
              {/* Instrumentation strip */}
              <div className="flex items-center justify-between px-3 py-1.5" style={{ borderTop: "1px solid rgba(255,255,255,0.05)", background: "#0A0B0F" }}>
                <div className="flex items-center gap-3" style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 9 }}>
                  <span style={{ color: "rgba(255,255,255,0.25)" }}>TC</span>
                  <span style={{ color: "#06B6D4" }}>{formatTime(uptime)}:{String(frame).padStart(2, "0")}</span>
                  <span style={{ color: "rgba(255,255,255,0.1)" }}>|</span>
                  <span style={{ color: "rgba(255,255,255,0.25)" }}>SCENE</span>
                  <span style={{ color: "#fff" }}>Scene {activeScene}</span>
                  <span style={{ color: "rgba(255,255,255,0.1)" }}>|</span>
                  <span style={{ color: "#22C55E" }}>SRC ACTIVE</span>
                </div>
                <div className="flex items-center gap-3" style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 9 }}>
                  <span style={{ color: "rgba(255,255,255,0.25)" }}>VIEWERS</span>
                  <span style={{ color: "#3B82F6", fontWeight: 600 }}>{viewers.toLocaleString()}</span>
                  <span style={{ color: "rgba(255,255,255,0.1)" }}>|</span>
                  <span style={{ color: "rgba(255,255,255,0.25)" }}>BITRATE</span>
                  <span style={{ color: Math.round(bitrate) > 7000 ? "#22C55E" : "#F59E0B", fontWeight: 600 }}>{Math.round(bitrate / 100) * 100} kbps</span>
                </div>
              </div>
            </div>

            {/* Scenes */}
            <div className="rounded-lg shrink-0" style={{ background: "#111318", border: "1px solid rgba(255,255,255,0.08)" }}>
              <div className="flex items-center justify-between px-3 py-2" style={{ borderBottom: "1px solid rgba(255,255,255,0.06)" }}>
                <span style={{ fontSize: 10, fontWeight: 600, color: "rgba(255,255,255,0.5)", letterSpacing: "0.1em" }}>SCENES</span>
                <div className="flex items-center gap-1">
                  <button className="p-1 rounded hover:bg-white/5 transition-colors"><ChevronLeft size={13} color="rgba(255,255,255,0.4)" /></button>
                  <button className="p-1 rounded hover:bg-white/5 transition-colors"><ChevronRight size={13} color="rgba(255,255,255,0.4)" /></button>
                  <div className="w-px h-3 mx-1" style={{ background: "rgba(255,255,255,0.1)" }} />
                  <button className="p-1 rounded hover:bg-white/5 transition-colors"><LayoutGrid size={13} color="rgba(255,255,255,0.4)" /></button>
                  <button className="p-1 rounded hover:bg-white/5 transition-colors"><List size={13} color="rgba(255,255,255,0.4)" /></button>
                  <button className="p-1 rounded hover:bg-white/5 transition-colors"><Plus size={13} color="rgba(255,255,255,0.4)" /></button>
                </div>
              </div>
              <div className="flex gap-2 p-2 overflow-x-auto">
                {scenes.map((scene) => (
                  <button
                    key={scene.id}
                    onClick={() => setActiveScene(scene.id)}
                    className="shrink-0 rounded overflow-hidden transition-all duration-150"
                    style={{
                      width: 110, height: 70,
                      background: "#0A0B0F",
                      border: `2px solid ${activeScene === scene.id ? "#3B82F6" : "rgba(255,255,255,0.08)"}`,
                      boxShadow: activeScene === scene.id ? "0 0 12px rgba(59,130,246,0.3)" : "none",
                    }}
                  >
                    <div className="w-full h-full flex flex-col items-center justify-center gap-1 relative">
                      <Monitor size={16} color={activeScene === scene.id ? "#3B82F6" : "rgba(255,255,255,0.2)"} />
                      <span style={{ fontSize: 10, color: activeScene === scene.id ? "#fff" : "rgba(255,255,255,0.4)", fontWeight: activeScene === scene.id ? 600 : 400 }}>
                        {scene.name}
                      </span>
                      {activeScene === scene.id && (
                        <div className="absolute top-1 right-1 rounded px-1" style={{ background: "#EF4444", fontSize: 8, fontWeight: 700, color: "#fff", letterSpacing: "0.06em" }}>LIVE</div>
                      )}
                    </div>
                  </button>
                ))}
              </div>
            </div>

            {/* Audio Mixer */}
            <div className="rounded-lg flex-1 min-h-0" style={{ background: "#111318", border: "1px solid rgba(255,255,255,0.08)" }}>
              <div className="flex items-center justify-between px-3 py-2" style={{ borderBottom: "1px solid rgba(255,255,255,0.06)" }}>
                <span style={{ fontSize: 10, fontWeight: 600, color: "rgba(255,255,255,0.5)", letterSpacing: "0.1em" }}>AUDIO MIXER</span>
                <div className="flex items-center gap-2">
                  <span style={{ fontSize: 10, color: "rgba(255,255,255,0.4)" }}>MASTER</span>
                  <Volume2 size={12} color="rgba(255,255,255,0.4)" />
                  <div className="rounded-full" style={{ width: 80, height: 4, background: "rgba(255,255,255,0.1)", position: "relative" }}>
                    <div style={{ width: "70%", height: "100%", background: "#3B82F6", borderRadius: 9999 }} />
                  </div>
                  <span style={{ fontSize: 10, color: "rgba(255,255,255,0.5)", fontFamily: "'JetBrains Mono', monospace" }}>-6.0 dB</span>
                  <Settings2 size={12} color="rgba(255,255,255,0.3)" className="cursor-pointer" />
                </div>
              </div>
              <div className="grid grid-cols-4 gap-2 p-2">
                {audioChannels.map((ch) => (
                  <div key={ch.name} className="rounded p-2" style={{ background: "#0E0F14", border: "1px solid rgba(255,255,255,0.06)" }}>
                    <div className="flex items-center gap-1.5 mb-2">
                      <ch.icon size={12} color="#3B82F6" />
                      <div>
                        <div style={{ fontSize: 11, fontWeight: 600, color: "#fff" }}>{ch.name}</div>
                        <div style={{ fontSize: 9, color: "rgba(255,255,255,0.35)" }}>{ch.sub}</div>
                      </div>
                    </div>
                    <VUMeter level={ch.level} />
                    <div className="mt-1.5 rounded-full" style={{ height: 3, background: "rgba(255,255,255,0.08)" }}>
                      <div style={{ width: `${ch.level}%`, height: "100%", background: "#3B82F6", borderRadius: 9999 }} />
                    </div>
                    <div className="flex items-center justify-between mt-1.5">
                      <span style={{ fontSize: 9, color: "rgba(255,255,255,0.4)", fontFamily: "'JetBrains Mono', monospace" }}>{ch.db} dB</span>
                      <div className="flex gap-1">
                        <button className="rounded px-1 py-0.5 text-xs" style={{ background: "rgba(255,255,255,0.06)", color: "rgba(255,255,255,0.5)", fontSize: 9, fontWeight: 600 }}>S</button>
                        <button className="rounded px-1 py-0.5 text-xs" style={{ background: "rgba(255,255,255,0.06)", color: "rgba(255,255,255,0.5)", fontSize: 9, fontWeight: 600 }}>M</button>
                      </div>
                    </div>
                  </div>
                ))}
              </div>
            </div>
          </div>

          {/* Right: Stream Status */}
          <div className="flex flex-col gap-3 p-3 shrink-0 overflow-y-auto" style={{ width: 240, borderLeft: "1px solid rgba(255,255,255,0.07)" }}>
            <div style={{ fontSize: 10, fontWeight: 600, color: "rgba(255,255,255,0.4)", letterSpacing: "0.1em" }}>STREAM STATUS</div>

            {/* Platform */}
            <div className="rounded-lg p-2.5 flex items-center gap-2" style={{ background: "#111318", border: "1px solid rgba(255,255,255,0.08)" }}>
              <div className="w-2 h-2 rounded-full animate-pulse" style={{ background: "#EF4444" }} />
              <span style={{ fontSize: 12, fontWeight: 600, color: "#EF4444" }}>LIVE</span>
              <span style={{ fontSize: 11, color: "rgba(255,255,255,0.6)" }}>on YouTube</span>
            </div>

            {/* Bitrate */}
            <div className="rounded-lg p-3" style={{ background: "#111318", border: "1px solid rgba(255,255,255,0.08)" }}>
              <div className="flex items-center justify-between mb-1">
                <span style={{ fontSize: 10, color: "rgba(255,255,255,0.4)", letterSpacing: "0.08em" }}>BITRATE</span>
                <span style={{ fontSize: 13, fontWeight: 700, color: "#3B82F6", fontFamily: "'JetBrains Mono', monospace" }}>{Math.round(bitrate).toLocaleString()} kbps</span>
              </div>
              <svg viewBox="0 0 100 100" preserveAspectRatio="none" style={{ width: "100%", height: 40 }}>
                <defs>
                  <linearGradient id="bitrateGrad" x1="0" y1="0" x2="0" y2="1">
                    <stop offset="0%" stopColor="#3B82F6" stopOpacity="0.4" />
                    <stop offset="100%" stopColor="#3B82F6" stopOpacity="0" />
                  </linearGradient>
                </defs>
                <polyline points={svgPoints} fill="none" stroke="#3B82F6" strokeWidth="1.5" />
                <polygon points={`0,100 ${svgPoints} 100,100`} fill="url(#bitrateGrad)" />
              </svg>
            </div>

            {/* Viewers + Uptime */}
            <div className="grid grid-cols-2 gap-2">
              {[
                { label: "VIEWERS", value: viewers.toLocaleString(), sub: "Peak: 3,152", icon: Users, color: "#3B82F6" },
                { label: "UPTIME", value: formatTime(uptime), sub: "Session Live", icon: Clock, color: "#06B6D4" },
              ].map((stat) => (
                <div key={stat.label} className="rounded-lg p-2.5" style={{ background: "#111318", border: "1px solid rgba(255,255,255,0.08)" }}>
                  <div className="flex items-center gap-1 mb-1">
                    <stat.icon size={10} color={stat.color} />
                    <span style={{ fontSize: 9, color: "rgba(255,255,255,0.4)", letterSpacing: "0.08em" }}>{stat.label}</span>
                  </div>
                  <div style={{ fontSize: 15, fontWeight: 700, color: "#fff", fontFamily: "'JetBrains Mono', monospace" }}>{stat.value}</div>
                  <div style={{ fontSize: 9, color: stat.color, marginTop: 2 }}>{stat.sub}</div>
                </div>
              ))}
            </div>

            {/* Stream Health */}
            <div className="rounded-lg p-3" style={{ background: "#111318", border: "1px solid rgba(255,255,255,0.08)" }}>
              <div className="flex items-center justify-between mb-2">
                <span style={{ fontSize: 10, color: "rgba(255,255,255,0.4)", letterSpacing: "0.08em" }}>STREAM HEALTH</span>
                <Activity size={11} color="#22C55E" />
              </div>
              {[
                { label: "CPU", value: 28, color: "#22C55E" },
                { label: "GPU", value: 42, color: "#22C55E" },
                { label: "Network", value: 90, color: "#22C55E", text: "Excellent" },
              ].map((item) => (
                <div key={item.label} className="flex items-center gap-2 mb-2">
                  <span style={{ fontSize: 10, color: "rgba(255,255,255,0.5)", width: 50 }}>{item.label}</span>
                  <HealthBar value={item.value} color={item.color} />
                  <span style={{ fontSize: 10, color: item.color, fontFamily: "'JetBrains Mono', monospace", width: 40, textAlign: "right" }}>
                    {item.text ?? `${item.value}%`}
                  </span>
                </div>
              ))}
            </div>

            {/* End Stream */}
            <button
              className="w-full rounded-lg py-3 flex items-center justify-center gap-2 font-semibold transition-all duration-150 active:scale-95"
              style={{ background: "#EF4444", color: "#fff", fontSize: 13, fontFamily: "'Space Grotesk', sans-serif", fontWeight: 700, letterSpacing: "0.04em", boxShadow: "0 4px 16px rgba(239,68,68,0.3)" }}
            >
              <Square size={14} fill="#fff" />
              END STREAM
            </button>
          </div>
        </div>
      </div>
    </AppSidebar>
  );
}
