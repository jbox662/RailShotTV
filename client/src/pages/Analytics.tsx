// RailShotTV — Chromatic Command — Analytics
import { useState, useEffect, useRef } from "react";
import AppSidebar from "@/components/AppSidebar";
import { TrendingUp, Users, Clock, DollarSign, Activity, Download } from "lucide-react";
import { AreaChart, Area, XAxis, YAxis, Tooltip, ResponsiveContainer, LineChart, Line } from "recharts";

const generateViewerData = () => Array.from({ length: 24 }, (_, i) => ({
  time: `${String(i).padStart(2,"0")}:00`,
  viewers: Math.floor(800 + Math.random() * 2400 + Math.sin(i * 0.5) * 600),
}));

const HEALTH_DATA = Array.from({ length: 30 }, (_, i) => ({
  t: i,
  bitrate: 8000 + Math.sin(i * 0.4) * 800 + Math.random() * 400,
  cpu: 22 + Math.sin(i * 0.3) * 8 + Math.random() * 5,
  gpu: 38 + Math.cos(i * 0.35) * 10 + Math.random() * 5,
  fps: 59 + Math.random() * 1.5,
}));

const SESSIONS = [
  { date: "Jul 14, 2026", duration: "2h 18m", peak: 3152, avg: 2341, revenue: "$184.50", quality: 98 },
  { date: "Jul 13, 2026", duration: "1h 52m", peak: 2890, avg: 2105, revenue: "$142.00", quality: 97 },
  { date: "Jul 12, 2026", duration: "3h 04m", peak: 4210, avg: 3180, revenue: "$267.80", quality: 99 },
  { date: "Jul 11, 2026", duration: "1h 37m", peak: 1980, avg: 1540, revenue: "$98.20", quality: 95 },
  { date: "Jul 10, 2026", duration: "2h 45m", peak: 3640, avg: 2780, revenue: "$221.40", quality: 98 },
];

const AUDIENCE = [
  { platform: "Twitch", pct: 54, color: "#8B5CF6" },
  { platform: "YouTube", pct: 31, color: "#EF4444" },
  { platform: "Facebook", pct: 15, color: "#3B82F6" },
];

const COUNTRIES = [
  { name: "United States", pct: 42 },
  { name: "Canada", pct: 18 },
  { name: "United Kingdom", pct: 12 },
  { name: "Germany", pct: 8 },
  { name: "Australia", pct: 6 },
];

export default function Analytics() {
  const [viewerData] = useState(generateViewerData);
  const [range, setRange] = useState("6h");

  return (
    <AppSidebar>
      {/* Top bar */}
      <div className="flex items-center gap-3 px-4 shrink-0" style={{ height: 46, background: "#0D0D15", borderBottom: "1px solid #1E1E2E" }}>
        <div className="flex items-center gap-1 mr-1">
          <span style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 18, color: "#F8F8FF", letterSpacing: "0.06em", lineHeight: 1 }}>RAILSHOT</span>
          <span style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 18, color: "#FF4D1C", letterSpacing: "0.06em", lineHeight: 1 }}>TV</span>
        </div>
        <div className="w-px h-4 mx-1" style={{ background: "#2A2A3A" }} />
        <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 600, fontSize: 11, color: "#606078", letterSpacing: "0.1em", textTransform: "uppercase" }}>Analytics</span>
        <div className="flex-1" />
        {["5m","15m","1h","3h","6h","12h"].map(r => (
          <button key={r} onClick={() => setRange(r)} className="px-2 py-0.5 rounded text-xs transition-all" style={{ background: range === r ? "#06B6D418" : "transparent", border: range === r ? "1px solid #06B6D440" : "1px solid transparent", color: range === r ? "#06B6D4" : "#50506A", fontFamily: "'DM Sans', sans-serif", fontSize: 11 }}>{r}</button>
        ))}
        <button className="flex items-center gap-1.5 px-2.5 py-1 rounded" style={{ background: "#1A1A24", border: "1px solid #2A2A3A" }}>
          <Download size={12} style={{ color: "#606078" }} />
          <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, color: "#606078" }}>Export</span>
        </button>
      </div>

      <div className="flex-1 overflow-y-auto px-4 py-3" style={{ background: "#0A0A0F" }}>
        {/* KPI cards */}
        <div className="grid grid-cols-4 gap-3 mb-4">
          {[
            { label: "Peak Viewers", value: "3,152", sub: "+18% vs last", color: "#3B82F6", icon: Users },
            { label: "Avg Watch Time", value: "18m 42s", sub: "+4m 12s vs last", color: "#8B5CF6", icon: Clock },
            { label: "New Followers", value: "+247", sub: "This session", color: "#10B981", icon: TrendingUp },
            { label: "Total Revenue", value: "$184.50", sub: "Subs + Donations", color: "#F59E0B", icon: DollarSign },
          ].map(({ label, value, sub, color, icon: Icon }) => (
            <div key={label} className="rounded p-3" style={{ background: "#111118", border: "1px solid #1E1E2E" }}>
              <div className="flex items-center justify-between mb-1.5">
                <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, color: "#606078", letterSpacing: "0.08em", textTransform: "uppercase" }}>{label}</span>
                <Icon size={14} style={{ color }} />
              </div>
              <div style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 28, color: "#F8F8FF", lineHeight: 1, letterSpacing: "0.04em" }}>{value}</div>
              <div style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, color, marginTop: 4 }}>{sub}</div>
            </div>
          ))}
        </div>

        {/* Main viewer chart */}
        <div className="rounded mb-4" style={{ background: "#111118", border: "1px solid #1E1E2E" }}>
          <div className="flex items-center justify-between px-4 py-2.5 panel-header-cyan rounded-t">
            <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 600, fontSize: 11, color: "#A0A0B8", letterSpacing: "0.1em", textTransform: "uppercase" }}>Viewer Count</span>
            <div className="flex items-center gap-1.5 px-2 py-0.5 rounded" style={{ background: "#06B6D418", border: "1px solid #06B6D440" }}>
              <div className="live-dot w-1.5 h-1.5 rounded-full" style={{ background: "#06B6D4" }} />
              <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, color: "#06B6D4", fontWeight: 600 }}>LIVE DATA</span>
            </div>
          </div>
          <div className="px-2 pb-3 pt-1">
            <ResponsiveContainer width="100%" height={140}>
              <AreaChart data={viewerData} margin={{ top: 4, right: 4, left: -20, bottom: 0 }}>
                <defs>
                  <linearGradient id="viewerGrad" x1="0" y1="0" x2="0" y2="1">
                    <stop offset="5%" stopColor="#06B6D4" stopOpacity={0.3}/>
                    <stop offset="95%" stopColor="#06B6D4" stopOpacity={0.02}/>
                  </linearGradient>
                </defs>
                <XAxis dataKey="time" tick={{ fill: "#50506A", fontSize: 9, fontFamily: "'JetBrains Mono', monospace" }} tickLine={false} axisLine={false} interval={3} />
                <YAxis tick={{ fill: "#50506A", fontSize: 9, fontFamily: "'JetBrains Mono', monospace" }} tickLine={false} axisLine={false} />
                <Tooltip contentStyle={{ background: "#1A1A24", border: "1px solid #2A2A3A", borderRadius: 4, fontSize: 11, fontFamily: "'DM Sans', sans-serif", color: "#F8F8FF" }} />
                <Area type="monotone" dataKey="viewers" stroke="#06B6D4" strokeWidth={2} fill="url(#viewerGrad)" />
              </AreaChart>
            </ResponsiveContainer>
          </div>
        </div>

        {/* Bottom row */}
        <div className="grid grid-cols-3 gap-3 mb-4">
          {/* Stream health */}
          <div className="col-span-2 rounded" style={{ background: "#111118", border: "1px solid #1E1E2E" }}>
            <div className="px-4 py-2.5 panel-header-emerald rounded-t">
              <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 600, fontSize: 11, color: "#A0A0B8", letterSpacing: "0.1em", textTransform: "uppercase" }}>Stream Health</span>
            </div>
            <div className="px-3 pb-3 pt-1">
              <ResponsiveContainer width="100%" height={100}>
                <LineChart data={HEALTH_DATA} margin={{ top: 4, right: 4, left: -20, bottom: 0 }}>
                  <XAxis dataKey="t" hide />
                  <YAxis hide />
                  <Tooltip contentStyle={{ background: "#1A1A24", border: "1px solid #2A2A3A", borderRadius: 4, fontSize: 10, fontFamily: "'DM Sans', sans-serif", color: "#F8F8FF" }} />
                  <Line type="monotone" dataKey="bitrate" stroke="#3B82F6" strokeWidth={1.5} dot={false} name="Bitrate" />
                  <Line type="monotone" dataKey="cpu" stroke="#10B981" strokeWidth={1.5} dot={false} name="CPU %" />
                  <Line type="monotone" dataKey="gpu" stroke="#06B6D4" strokeWidth={1.5} dot={false} name="GPU %" />
                  <Line type="monotone" dataKey="fps" stroke="#F59E0B" strokeWidth={1.5} dot={false} name="FPS" />
                </LineChart>
              </ResponsiveContainer>
              <div className="flex gap-4 mt-1">
                {[["Bitrate","#3B82F6"],["CPU","#10B981"],["GPU","#06B6D4"],["FPS","#F59E0B"]].map(([l,c]) => (
                  <div key={l} className="flex items-center gap-1">
                    <div className="w-3 h-0.5 rounded" style={{ background: c }} />
                    <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, color: "#606078" }}>{l}</span>
                  </div>
                ))}
              </div>
            </div>
          </div>

          {/* Audience breakdown */}
          <div className="rounded" style={{ background: "#111118", border: "1px solid #1E1E2E" }}>
            <div className="px-4 py-2.5 panel-header-violet rounded-t">
              <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 600, fontSize: 11, color: "#A0A0B8", letterSpacing: "0.1em", textTransform: "uppercase" }}>Audience</span>
            </div>
            <div className="px-3 py-2">
              {AUDIENCE.map(({ platform, pct, color }) => (
                <div key={platform} className="flex items-center gap-2 mb-2">
                  <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, color: "#A0A0B8", width: 64 }}>{platform}</span>
                  <div className="flex-1 rounded-full overflow-hidden" style={{ height: 5, background: "#1A1A24" }}>
                    <div style={{ width: `${pct}%`, height: "100%", background: color, borderRadius: 2 }} />
                  </div>
                  <span className="mono" style={{ fontSize: 11, color, width: 32, textAlign: "right" }}>{pct}%</span>
                </div>
              ))}
              <div className="mt-2 pt-2" style={{ borderTop: "1px solid #1E1E2E" }}>
                <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, color: "#606078", letterSpacing: "0.08em", textTransform: "uppercase" }}>Top Countries</span>
                {COUNTRIES.map(({ name, pct }) => (
                  <div key={name} className="flex items-center gap-2 mt-1.5">
                    <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, color: "#A0A0B8", flex: 1 }}>{name}</span>
                    <div className="rounded-full overflow-hidden" style={{ width: 48, height: 3, background: "#1A1A24" }}>
                      <div style={{ width: `${pct}%`, height: "100%", background: "#06B6D4", borderRadius: 2 }} />
                    </div>
                    <span className="mono" style={{ fontSize: 10, color: "#06B6D4", width: 28, textAlign: "right" }}>{pct}%</span>
                  </div>
                ))}
              </div>
            </div>
          </div>
        </div>

        {/* Sessions table */}
        <div className="rounded" style={{ background: "#111118", border: "1px solid #1E1E2E" }}>
          <div className="px-4 py-2.5 panel-header-brand rounded-t">
            <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 600, fontSize: 11, color: "#A0A0B8", letterSpacing: "0.1em", textTransform: "uppercase" }}>Recent Sessions</span>
          </div>
          <table className="w-full">
            <thead>
              <tr style={{ borderBottom: "1px solid #1E1E2E" }}>
                {["Date","Duration","Peak","Avg","Revenue","Quality"].map(h => (
                  <th key={h} className="px-4 py-2 text-left" style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, color: "#50506A", fontWeight: 600, letterSpacing: "0.08em", textTransform: "uppercase" }}>{h}</th>
                ))}
              </tr>
            </thead>
            <tbody>
              {SESSIONS.map((s, i) => (
                <tr key={i} style={{ borderBottom: "1px solid #1A1A24" }}>
                  <td className="px-4 py-2.5" style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 12, color: "#A0A0B8" }}>{s.date}</td>
                  <td className="px-4 py-2.5 mono" style={{ fontSize: 12, color: "#F8F8FF" }}>{s.duration}</td>
                  <td className="px-4 py-2.5 mono" style={{ fontSize: 12, color: "#06B6D4" }}>{s.peak.toLocaleString()}</td>
                  <td className="px-4 py-2.5 mono" style={{ fontSize: 12, color: "#A0A0B8" }}>{s.avg.toLocaleString()}</td>
                  <td className="px-4 py-2.5 mono" style={{ fontSize: 12, color: "#10B981" }}>{s.revenue}</td>
                  <td className="px-4 py-2.5">
                    <span className="px-2 py-0.5 rounded text-xs mono" style={{ background: s.quality >= 98 ? "#10B98118" : "#F59E0B18", border: `1px solid ${s.quality >= 98 ? "#10B98140" : "#F59E0B40"}`, color: s.quality >= 98 ? "#10B981" : "#F59E0B" }}>{s.quality}%</span>
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      </div>
    </AppSidebar>
  );
}
