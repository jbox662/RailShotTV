// RailShotTV — Chromatic Command — Dashboard
// Colors: Brand=#FF5A2C, Blue=#4F9EFF, Violet=#A855F7, Emerald=#22C55E, Cyan=#22D3EE, Amber=#FBBF24
import { useState, useEffect, useRef } from "react";
import AppSidebar from "@/components/AppSidebar";
import { Wifi, Users, Clock, Activity, Cpu, Monitor, ChevronLeft, ChevronRight, LayoutGrid, List, Plus, Square, Mic, Music, Bell, Volume2 } from "lucide-react";

const SCENES = [
  { id: 1, name: "Main View", live: true },
  { id: 2, name: "Wide Angle" },
  { id: 3, name: "Replay" },
  { id: 4, name: "Interview" },
  { id: 5, name: "Scoreboard" },
  { id: 6, name: "End Screen" },
];

const CHANNELS = [
  { name: "Host Mic", sub: "XLR Input 1", icon: Mic, color: "#A855F7", levels: [0.6,0.7,0.5,0.8,0.6,0.4,0.7,0.5,0.6,0.8,0.5,0.7,0.6,0.4,0.5,0.7,0.8,0.6,0.5,0.7], db: "-3.2" },
  { name: "Commentator", sub: "USB Audio", icon: Mic, color: "#4F9EFF", levels: [0.4,0.5,0.3,0.6,0.4,0.5,0.3,0.7,0.5,0.4,0.6,0.3,0.5,0.4,0.6,0.5,0.3,0.4,0.5,0.6], db: "-10.4" },
  { name: "Music", sub: "Spotify", icon: Music, color: "#22C55E", levels: [0.2,0.3,0.2,0.3,0.2,0.3,0.2,0.3,0.2,0.3,0.2,0.3,0.2,0.3,0.2,0.3,0.2,0.3,0.2,0.3], db: "-18.1" },
  { name: "Alert SFX", sub: "Stream Elements", icon: Bell, color: "#FBBF24", levels: [0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1], db: "-28.0" },
];

function VUMeter({ levels, color }: { levels: number[]; color: string }) {
  const [tick, setTick] = useState(0);
  useEffect(() => { const t = setInterval(() => setTick(p => p + 1), 80); return () => clearInterval(t); }, []);
  return (
    <div className="flex items-end gap-px" style={{ height: 28 }}>
      {levels.map((l, i) => {
        const jitter = (Math.sin(tick * 0.7 + i * 1.3) * 0.15 + Math.cos(tick * 0.4 + i * 0.8) * 0.1);
        const h = Math.max(2, Math.round((l + jitter) * 28));
        const pct = (l + jitter);
        const c = pct > 0.85 ? "#EF4444" : pct > 0.65 ? "#FBBF24" : color;
        return <div key={i} className="vu-bar" style={{ height: h, background: c, opacity: 0.9 }} />;
      })}
    </div>
  );
}

function BitrateSparkline() {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const dataRef = useRef<number[]>(Array(40).fill(8600));
  useEffect(() => {
    const interval = setInterval(() => {
      const last = dataRef.current[dataRef.current.length - 1];
      const next = Math.max(7000, Math.min(10000, last + (Math.random() - 0.48) * 300));
      dataRef.current = [...dataRef.current.slice(1), next];
      const canvas = canvasRef.current; if (!canvas) return;
      const ctx = canvas.getContext("2d"); if (!ctx) return;
      const { width: w, height: h } = canvas;
      ctx.clearRect(0, 0, w, h);
      const data = dataRef.current;
      const min = 6000, max = 11000;
      const pts = data.map((v, i) => ({ x: (i / (data.length - 1)) * w, y: h - ((v - min) / (max - min)) * h * 0.85 - 2 }));
      const grad = ctx.createLinearGradient(0, 0, 0, h);
      grad.addColorStop(0, "rgba(59,130,246,0.4)");
      grad.addColorStop(1, "rgba(59,130,246,0.02)");
      ctx.beginPath();
      ctx.moveTo(pts[0].x, h);
      ctx.lineTo(pts[0].x, pts[0].y);
      pts.forEach(p => ctx.lineTo(p.x, p.y));
      ctx.lineTo(pts[pts.length-1].x, h);
      ctx.closePath();
      ctx.fillStyle = grad;
      ctx.fill();
      ctx.beginPath();
      ctx.moveTo(pts[0].x, pts[0].y);
      pts.forEach(p => ctx.lineTo(p.x, p.y));
      ctx.strokeStyle = "#4F9EFF";
      ctx.lineWidth = 1.5;
      ctx.stroke();
    }, 200);
    return () => clearInterval(interval);
  }, []);
  return <canvas ref={canvasRef} width={200} height={44} style={{ width: "100%", height: 44 }} />;
}

export default function Dashboard() {
  const [activeScene, setActiveScene] = useState(1);
  const [tc, setTc] = useState("01:23:49");
  const [viewers, setViewers] = useState(2853);
  const [bitrate, setBitrate] = useState(8642);

  useEffect(() => {
    const t = setInterval(() => {
      setViewers(v => Math.max(2800, v + Math.floor((Math.random() - 0.48) * 5)));
      setBitrate(v => Math.max(7000, Math.min(10000, v + Math.floor((Math.random() - 0.48) * 120))));
    }, 1200);
    return () => clearInterval(t);
  }, []);

  useEffect(() => {
    let secs = 83 * 60 + 49;
    const t = setInterval(() => {
      secs++;
      const h = String(Math.floor(secs / 3600)).padStart(2, "0");
      const m = String(Math.floor((secs % 3600) / 60)).padStart(2, "0");
      const s = String(secs % 60).padStart(2, "0");
      setTc(`${h}:${m}:${s}`);
    }, 1000);
    return () => clearInterval(t);
  }, []);

  return (
    <AppSidebar>
      {/* Live top border */}
      <div className="live-top-border" />

      {/* Top bar */}
      <div className="flex items-center gap-3 px-4 shrink-0" style={{ height: 46, background: "#1A2035", borderBottom: "1px solid #2A3350" }}>
        {/* RailShotTV wordmark */}
        <div className="flex items-center gap-1 mr-1">
          <span style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 18, color: "#F8F8FF", letterSpacing: "0.06em", lineHeight: 1 }}>RAILSHOT</span>
          <span style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 18, color: "#FF5A2C", letterSpacing: "0.06em", lineHeight: 1 }}>TV</span>
        </div>
        <div className="w-px h-4 mx-1" style={{ background: "#303D5A" }} />
        <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 600, fontSize: 11, color: "#8892A4", letterSpacing: "0.1em", textTransform: "uppercase" }}>Dashboard</span>
        <div className="flex items-center gap-1.5 px-2 py-1 rounded" style={{ background: "#FF5A2C18", border: "1px solid #FF5A2C40" }}>
          <div className="live-dot w-1.5 h-1.5 rounded-full" style={{ background: "#FF5A2C" }} />
          <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 700, fontSize: 11, color: "#FF5A2C", letterSpacing: "0.06em" }}>LIVE</span>
        </div>
        <span style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 11, color: "#FF6B35" }}>YT</span>
        <span className="mono" style={{ fontSize: 12, color: "#22D3EE" }}>{tc}</span>
        <span className="mono" style={{ fontSize: 11, color: "#50506A" }}>1920×1080</span>
        <span className="mono" style={{ fontSize: 11, color: "#50506A" }}>60fps</span>
        <span className="mono" style={{ fontSize: 11, color: "#4F9EFF" }}>H.264</span>
        <div className="flex-1" />
        <div className="flex items-center gap-1.5">
          <div className="w-1.5 h-1.5 rounded-full" style={{ background: "#22C55E" }} />
          <span style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 10, color: "#22C55E" }}>SIG OK</span>
        </div>
        <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, color: "#50506A" }}>Default Profile</span>
      </div>

      {/* Body */}
      <div className="flex flex-1 overflow-hidden">
        {/* Center */}
        <div className="flex flex-col flex-1 overflow-hidden">
          {/* Program output */}
          <div className="flex-1 flex flex-col overflow-hidden" style={{ minHeight: 0 }}>
            <div className="flex items-center justify-between px-3 py-1.5 panel-header-brand shrink-0">
              <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 600, fontSize: 11, color: "#A0A0B8", letterSpacing: "0.1em", textTransform: "uppercase" }}>Program Output</span>
              <div className="flex items-center gap-1.5">
                <div className="live-dot w-1.5 h-1.5 rounded-full" style={{ background: "#FF5A2C" }} />
                <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 700, fontSize: 10, color: "#FF5A2C", letterSpacing: "0.08em" }}>LIVE</span>
              </div>
            </div>
            <div className="flex-1 relative overflow-hidden" style={{ background: "#161B2E", minHeight: 0 }}>
              {/* Corner brackets */}
              {[["top-2 left-2","border-t-2 border-l-2"],["top-2 right-2","border-t-2 border-r-2"],["bottom-2 left-2","border-b-2 border-l-2"],["bottom-2 right-2","border-b-2 border-r-2"]].map(([pos, bdr], i) => (
                <div key={i} className={`absolute ${pos} ${bdr} w-4 h-4`} style={{ borderColor: "#4F9EFF40" }} />
              ))}
            {/* Empty canvas placeholder */}
            <div className="absolute inset-0 flex flex-col items-center justify-center gap-2">
              <Monitor size={28} style={{ color: "#303D5A" }} />
              <span className="mono" style={{ fontSize: 11, color: "#303D5A" }}>1920 × 1080 · 60fps · H.264</span>
            </div>
            {/* Broadcast canvas overlays */}
            <svg className="absolute inset-0 w-full h-full pointer-events-none" style={{ opacity: 0.07 }}>
              {/* Dynamic accent lines */}
              <line x1="0" y1="100%" x2="40%" y2="0" stroke="#FF5A2C" strokeWidth="1"/>
              <line x1="100%" y1="0" x2="60%" y2="100%" stroke="#FF5A2C" strokeWidth="1"/>
              {/* Corner bracket accents */}
              <circle cx="0" cy="0" r="18" fill="none" stroke="#FF5A2C" strokeWidth="1.5"/>
              <circle cx="100%" cy="0" r="18" fill="none" stroke="#FF5A2C" strokeWidth="1.5"/>
              <circle cx="0" cy="100%" r="18" fill="none" stroke="#FF5A2C" strokeWidth="1.5"/>
              <circle cx="100%" cy="100%" r="18" fill="none" stroke="#FF5A2C" strokeWidth="1.5"/>
              {/* Center spot */}
              <circle cx="50%" cy="50%" r="4" fill="#FF5A2C"/>
              <circle cx="50%" cy="50%" r="20" fill="none" stroke="#FF5A2C" strokeWidth="0.8"/>
              {/* Safe area guides */}
              <rect x="8" y="8" width="calc(100% - 16px)" height="calc(100% - 16px)" fill="none" stroke="#4F9EFF" strokeWidth="0.8" strokeDasharray="4 8"/>
            </svg>
              {/* Timecode strip */}
              <div className="absolute bottom-0 left-0 right-0 flex items-center gap-4 px-3 py-1.5" style={{ background: "rgba(0,0,0,0.75)", borderTop: "1px solid #2A3350" }}>
                <span style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 10, color: "#50506A", letterSpacing: "0.06em" }}>TC</span>
                <span className="mono" style={{ fontSize: 11, color: "#22D3EE" }}>{tc}</span>
                <span style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 10, color: "#50506A" }}>SCENE</span>
                <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, color: "#F8F8FF" }}>
                  {SCENES.find(s => s.id === activeScene)?.name ?? "Scene 1"}
                </span>
                <span style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 10, color: "#22C55E" }}>SRC ACTIVE</span>
                <div className="flex-1" />
                <span style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 10, color: "#50506A" }}>VIEWERS</span>
                <span className="mono" style={{ fontSize: 11, color: "#4F9EFF" }}>{viewers.toLocaleString()}</span>
                <span style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 10, color: "#50506A" }}>BITRATE</span>
                <span className="mono" style={{ fontSize: 11, color: "#FF5A2C" }}>{bitrate.toLocaleString()} kbps</span>
              </div>
            </div>
          </div>

          {/* Scenes */}
          <div className="shrink-0" style={{ borderTop: "1px solid #2A3350" }}>
            <div className="flex items-center justify-between px-3 py-1.5 panel-header-blue">
              <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 600, fontSize: 11, color: "#A0A0B8", letterSpacing: "0.1em", textTransform: "uppercase" }}>Scenes</span>
              <div className="flex items-center gap-1">
                {[ChevronLeft, ChevronRight].map((Icon, i) => (
                  <button key={i} className="flex items-center justify-center rounded transition-colors" style={{ width: 22, height: 22, background: "#1A1A24", border: "1px solid #303D5A" }}>
                    <Icon size={12} style={{ color: "#8892A4" }} />
                  </button>
                ))}
                <div className="w-px h-4 mx-1" style={{ background: "#303D5A" }} />
                {[LayoutGrid, List].map((Icon, i) => (
                  <button key={i} className="flex items-center justify-center rounded transition-colors" style={{ width: 22, height: 22, background: "#1A1A24", border: "1px solid #303D5A" }}>
                    <Icon size={12} style={{ color: "#8892A4" }} />
                  </button>
                ))}
                <button className="flex items-center justify-center rounded transition-colors ml-1" style={{ width: 22, height: 22, background: "#FF5A2C18", border: "1px solid #FF5A2C40" }}>
                  <Plus size={12} style={{ color: "#FF5A2C" }} />
                </button>
              </div>
            </div>
            <div className="flex gap-2 px-3 py-2 overflow-x-auto" style={{ background: "#1A2035" }}>
              {SCENES.map(scene => (
                <button
                  key={scene.id}
                  onClick={() => setActiveScene(scene.id)}
                  className="flex flex-col items-center gap-1 rounded shrink-0 transition-all duration-150"
                  style={{
                    width: 100, padding: "8px 6px",
                    background: activeScene === scene.id ? "#4F9EFF18" : "#1E2640",
                    border: activeScene === scene.id ? "1px solid #4F9EFF50" : "1px solid #2A3350",
                    boxShadow: activeScene === scene.id ? "0 0 14px rgba(59,130,246,0.2)" : "none",
                  }}
                >
                  <div className="w-full rounded flex items-center justify-center relative" style={{ height: 52, background: "#161B2E", border: "1px solid #2A3350" }}>
                    <Monitor size={16} style={{ color: activeScene === scene.id ? "#4F9EFF" : "#303D5A" }} />
                    {scene.live && activeScene === scene.id && (
                      <div className="absolute top-1 right-1 flex items-center gap-0.5 px-1 rounded" style={{ background: "#FF5A2C", fontSize: 7, fontWeight: 700, color: "#fff", fontFamily: "'DM Sans', sans-serif", letterSpacing: "0.06em" }}>
                        <div className="live-dot w-1 h-1 rounded-full bg-white" />
                        LIVE
                      </div>
                    )}
                  </div>
                  <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, fontWeight: activeScene === scene.id ? 600 : 400, color: activeScene === scene.id ? "#F8F8FF" : "#606078", whiteSpace: "nowrap" }}>
                    {scene.name}
                  </span>
                </button>
              ))}
            </div>
          </div>

          {/* Audio mixer */}
          <div className="shrink-0" style={{ borderTop: "1px solid #2A3350" }}>
            <div className="flex items-center justify-between px-3 py-1.5 panel-header-violet">
              <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 600, fontSize: 11, color: "#A0A0B8", letterSpacing: "0.1em", textTransform: "uppercase" }}>Audio Mixer</span>
              <div className="flex items-center gap-2">
                <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, color: "#8892A4" }}>MASTER</span>
                <Volume2 size={12} style={{ color: "#A855F7" }} />
                <div className="rounded-full overflow-hidden" style={{ width: 80, height: 3, background: "#1A1A24" }}>
                  <div style={{ width: "75%", height: "100%", background: "linear-gradient(90deg, #A855F7, #A78BFA)" }} />
                </div>
                <span className="mono" style={{ fontSize: 10, color: "#A855F7" }}>-6.0 dB</span>
              </div>
            </div>
            <div className="flex gap-0 overflow-x-auto" style={{ background: "#1A2035" }}>
              {CHANNELS.map((ch, idx) => (
                <div key={idx} className="flex flex-col gap-1 px-3 py-2 shrink-0" style={{ minWidth: 130, borderRight: "1px solid #2A3350" }}>
                  <div className="flex items-center gap-1.5">
                    <ch.icon size={12} style={{ color: ch.color }} />
                    <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 12, fontWeight: 600, color: "#F8F8FF" }}>{ch.name}</span>
                  </div>
                  <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, color: "#50506A" }}>{ch.sub}</span>
                  <VUMeter levels={ch.levels} color={ch.color} />
                  <div className="flex items-center justify-between mt-0.5">
                    <span className="mono" style={{ fontSize: 10, color: ch.color }}>{ch.db} dB</span>
                    <div className="flex gap-1">
                      {["S","M"].map(l => (
                        <button key={l} className="flex items-center justify-center rounded" style={{ width: 18, height: 18, background: "#1A1A24", border: "1px solid #303D5A", fontSize: 9, fontWeight: 700, color: "#8892A4", fontFamily: "'DM Sans', sans-serif" }}>
                          {l}
                        </button>
                      ))}
                    </div>
                  </div>
                </div>
              ))}
            </div>
          </div>
        </div>

        {/* Right panel */}
        <div className="flex flex-col shrink-0 overflow-y-auto" style={{ width: 240, background: "#1A2035", borderLeft: "1px solid #2A3350" }}>
          {/* Stream status */}
          <div className="panel-header-brand px-3 py-1.5 shrink-0">
            <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 600, fontSize: 11, color: "#A0A0B8", letterSpacing: "0.1em", textTransform: "uppercase" }}>Stream Status</span>
          </div>

          {/* Live indicator */}
          <div className="flex items-center gap-2 px-3 py-2.5" style={{ borderBottom: "1px solid #2A3350" }}>
            <div className="live-dot w-2 h-2 rounded-full" style={{ background: "#FF5A2C" }} />
            <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 700, fontSize: 13, color: "#FF5A2C" }}>LIVE</span>
            <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 12, color: "#A0A0B8" }}>on YouTube</span>
          </div>

          {/* Bitrate */}
          <div className="px-3 py-2.5" style={{ borderBottom: "1px solid #2A3350" }}>
            <div className="flex items-center justify-between mb-1.5">
              <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, color: "#8892A4", letterSpacing: "0.08em", textTransform: "uppercase" }}>Bitrate</span>
              <span style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 20, color: "#4F9EFF", letterSpacing: "0.04em" }}>{bitrate.toLocaleString()} <span style={{ fontSize: 12, color: "#8892A4", fontFamily: "'JetBrains Mono', monospace" }}>kbps</span></span>
            </div>
            <BitrateSparkline />
          </div>

          {/* Viewers + Uptime */}
          <div className="grid grid-cols-2 gap-0" style={{ borderBottom: "1px solid #2A3350" }}>
            {[
              { label: "Viewers", value: viewers.toLocaleString(), sub: "Peak: 3,152", color: "#4F9EFF", icon: Users },
              { label: "Uptime", value: tc, sub: "Session Live", color: "#22D3EE", icon: Clock },
            ].map(({ label, value, sub, color, icon: Icon }) => (
              <div key={label} className="flex flex-col gap-0.5 px-3 py-2.5" style={{ borderRight: label === "Viewers" ? "1px solid #2A3350" : "none" }}>
                <div className="flex items-center gap-1">
                  <Icon size={11} style={{ color }} />
                  <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, color: "#8892A4", letterSpacing: "0.06em", textTransform: "uppercase" }}>{label}</span>
                </div>
                <span className="mono" style={{ fontSize: label === "Uptime" ? 14 : 22, fontWeight: 700, color: "#F8F8FF", fontFamily: label === "Uptime" ? "'JetBrains Mono', monospace" : "'Bebas Neue', sans-serif", letterSpacing: label === "Uptime" ? "0.02em" : "0.04em" }}>{value}</span>
                <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, color }}>{sub}</span>
              </div>
            ))}
          </div>

          {/* Stream health */}
          <div className="px-3 py-2.5" style={{ borderBottom: "1px solid #2A3350" }}>
            <div className="flex items-center justify-between mb-2">
              <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, color: "#8892A4", letterSpacing: "0.08em", textTransform: "uppercase" }}>Stream Health</span>
              <Activity size={11} style={{ color: "#22C55E" }} />
            </div>
            {[
              { label: "CPU", pct: 28, color: "#22C55E" },
              { label: "GPU", pct: 42, color: "#22C55E" },
              { label: "Network", pct: 15, color: "#22C55E", text: "Excellent" },
            ].map(({ label, pct, color, text }) => (
              <div key={label} className="flex items-center gap-2 mb-1.5">
                <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, color: "#A0A0B8", width: 52 }}>{label}</span>
                <div className="flex-1 rounded-full overflow-hidden" style={{ height: 4, background: "#1A1A24" }}>
                  <div style={{ width: `${pct}%`, height: "100%", background: color, borderRadius: 2, transition: "width 0.5s ease" }} />
                </div>
                <span className="mono" style={{ fontSize: 10, color, width: text ? 56 : 28, textAlign: "right" }}>{text ?? `${pct}%`}</span>
              </div>
            ))}
          </div>

          {/* End stream button */}
          <div className="px-3 py-3 mt-auto">
            <button
              className="w-full flex items-center justify-center gap-2 rounded font-bold transition-all duration-150"
              style={{ height: 44, background: "linear-gradient(135deg, #FF5A2C 0%, #FF6B35 100%)", boxShadow: "0 0 20px rgba(255,77,28,0.35)", color: "#fff", fontFamily: "'DM Sans', sans-serif", fontSize: 13, letterSpacing: "0.06em" }}
            >
              <Square size={13} fill="white" />
              END STREAM
            </button>
          </div>
        </div>
      </div>
    </AppSidebar>
  );
}
