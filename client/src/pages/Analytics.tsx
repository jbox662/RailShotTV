/**
 * Nexus Broadcast — Analytics & Stream Health (Screen 4)
 * Obsidian Studio Dark Theme
 */
import { useState, useEffect } from "react";
import AppSidebar from "@/components/AppSidebar";
import { AreaChart, Area, LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer, PieChart, Pie, Cell, BarChart, Bar } from "recharts";
import { Users, Clock, UserPlus, DollarSign, Download, RefreshCw } from "lucide-react";

const viewerData = [
  { time: "12:00", viewers: 420 }, { time: "12:15", viewers: 680 }, { time: "12:30", viewers: 1100 },
  { time: "12:45", viewers: 1580 }, { time: "13:00", viewers: 2200 }, { time: "13:15", viewers: 2847 },
  { time: "13:30", viewers: 3152 }, { time: "13:45", viewers: 2900 }, { time: "14:00", viewers: 2650 },
];

const healthData = Array.from({ length: 20 }, (_, i) => ({
  t: i,
  bitrate: 5800 + Math.random() * 400,
  cpu: 25 + Math.random() * 8,
  gpu: 38 + Math.random() * 10,
  fps: 59 + Math.random() * 2,
  dropped: Math.random() * 0.02,
}));

const platformData = [
  { name: "Twitch", value: 54, color: "#9146FF" },
  { name: "YouTube", value: 31, color: "#FF0000" },
  { name: "Facebook", value: 15, color: "#1877F2" },
];

const countryData = [
  { country: "United States", pct: 32 },
  { country: "Germany", pct: 12 },
  { country: "Brazil", pct: 9 },
  { country: "United Kingdom", pct: 7 },
  { country: "Canada", pct: 5 },
];

const sessions = [
  { date: "Jul 13, 2026 · 2:00 PM", duration: "3h 12m", peak: 3152, avg: 2187, revenue: "$184.50", score: "Excellent", scoreColor: "#22C55E" },
  { date: "Jul 12, 2026 · 2:15 PM", duration: "2h 45m", peak: 2876, avg: 1984, revenue: "$162.30", score: "Excellent", scoreColor: "#22C55E" },
  { date: "Jul 11, 2026 · 1:30 PM", duration: "3h 05m", peak: 2541, avg: 1763, revenue: "$142.80", score: "Good", scoreColor: "#3B82F6" },
  { date: "Jul 10, 2026 · 3:00 PM", duration: "2h 20m", peak: 2103, avg: 1412, revenue: "$121.40", score: "Good", scoreColor: "#3B82F6" },
  { date: "Jul 9, 2026 · 2:10 PM", duration: "2h 10m", peak: 1892, avg: 1238, revenue: "$98.70", score: "Fair", scoreColor: "#F59E0B" },
];

const kpis = [
  { label: "PEAK VIEWERS", value: "3,152", sub: "+18.7% vs yesterday", icon: Users, color: "#3B82F6" },
  { label: "AVG. WATCH TIME", value: "18m 42s", sub: "+12.4% vs yesterday", icon: Clock, color: "#06B6D4" },
  { label: "NEW FOLLOWERS", value: "+247", sub: "+29.3% vs yesterday", icon: UserPlus, color: "#22C55E" },
  { label: "TOTAL REVENUE", value: "$184.50", sub: "+15.2% vs yesterday", icon: DollarSign, color: "#F59E0B" },
];

const CustomTooltip = ({ active, payload, label }: any) => {
  if (active && payload && payload.length) {
    return (
      <div className="rounded px-2 py-1.5" style={{ background: "#1A1D2B", border: "1px solid rgba(255,255,255,0.15)", fontSize: 11 }}>
        <div style={{ color: "rgba(255,255,255,0.5)" }}>{label}</div>
        <div style={{ color: "#3B82F6", fontWeight: 600, fontFamily: "'JetBrains Mono', monospace" }}>{payload[0]?.value?.toLocaleString()} viewers</div>
      </div>
    );
  }
  return null;
};

export default function Analytics() {
  return (
    <AppSidebar>
      <div className="flex flex-col h-full overflow-hidden" style={{ fontFamily: "'Inter', sans-serif" }}>
        {/* Top bar */}
        <div className="flex items-center px-4 border-b shrink-0" style={{ borderColor: "rgba(255,255,255,0.07)", minHeight: 46, background: "#0D0E12" }}>
          <span style={{ fontFamily: "'Space Grotesk', sans-serif", fontWeight: 700, fontSize: 12, color: "#fff", letterSpacing: "0.1em" }}>ANALYTICS</span>
          <div className="mx-3 w-px h-4" style={{ background: "rgba(255,255,255,0.1)" }} />
          <span style={{ fontSize: 10, color: "rgba(255,255,255,0.3)", fontFamily: "'JetBrains Mono', monospace" }}>STREAM HEALTH MONITOR</span>
          <div className="ml-auto flex items-center gap-2">
            <button className="flex items-center gap-1.5 rounded px-3 py-1" style={{ background: "#111318", border: "1px solid rgba(255,255,255,0.1)", color: "rgba(255,255,255,0.5)", fontSize: 10, fontFamily: "'JetBrains Mono', monospace" }}>
              <Clock size={10} /> TODAY
            </button>
            <button className="flex items-center gap-1.5 rounded px-3 py-1" style={{ background: "#111318", border: "1px solid rgba(255,255,255,0.1)", color: "rgba(255,255,255,0.5)", fontSize: 10, fontFamily: "'JetBrains Mono', monospace" }}>
              <Download size={10} /> EXPORT
            </button>
            <button className="rounded p-1.5" style={{ background: "#111318", border: "1px solid rgba(255,255,255,0.1)" }}>
              <RefreshCw size={11} color="rgba(255,255,255,0.4)" />
            </button>
          </div>
        </div>

        <div className="flex-1 overflow-y-auto p-3 space-y-3">
          {/* KPI Cards */}
          <div className="grid grid-cols-4 gap-3">
            {kpis.map(kpi => (
              <div key={kpi.label} className="rounded-lg p-3" style={{ background: "#111318", border: `1px solid ${kpi.color}30` }}>
                <div className="flex items-center justify-between mb-1">
                  <span style={{ fontSize: 9, fontWeight: 600, color: "rgba(255,255,255,0.4)", letterSpacing: "0.1em" }}>{kpi.label}</span>
                  <kpi.icon size={13} color={kpi.color} />
                </div>
                <div style={{ fontSize: 22, fontWeight: 700, color: "#fff", fontFamily: "'Space Grotesk', sans-serif" }}>{kpi.value}</div>
                <div style={{ fontSize: 10, color: kpi.color, marginTop: 2 }}>{kpi.sub}</div>
              </div>
            ))}
          </div>

          {/* Viewer Chart */}
          <div className="rounded-lg p-3" style={{ background: "#111318", border: "1px solid rgba(255,255,255,0.08)" }}>
            <div className="flex items-center justify-between mb-3">
              <span style={{ fontSize: 11, fontWeight: 600, color: "rgba(255,255,255,0.6)" }}>Viewer Count Over Time</span>
              <div className="flex items-center gap-2 mr-2">
                <div className="w-1.5 h-1.5 rounded-full animate-pulse" style={{ background: "#EF4444" }} />
                <span style={{ fontSize: 9, color: "#EF4444", fontFamily: "'JetBrains Mono', monospace", fontWeight: 700, letterSpacing: "0.08em" }}>LIVE DATA</span>
              </div>
              <div className="flex gap-1">
                {["5m", "15m", "1h", "3h", "6h", "12h"].map(t => (
                  <button key={t} className="rounded px-2 py-0.5 text-xs" style={{ background: t === "3h" ? "#3B82F6" : "rgba(255,255,255,0.05)", color: t === "3h" ? "#fff" : "rgba(255,255,255,0.4)", fontSize: 10 }}>{t}</button>
                ))}
              </div>
            </div>
            <div className="flex items-center gap-4 mb-2" style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 9, color: "rgba(255,255,255,0.3)" }}>
              <span>PEAK: <span style={{ color: "#3B82F6", fontWeight: 600 }}>3,152</span></span>
              <span>AVG: <span style={{ color: "rgba(255,255,255,0.6)" }}>2,187</span></span>
              <span>CURRENT: <span style={{ color: "#22C55E", fontWeight: 600 }}>2,847</span></span>
              <span style={{ marginLeft: "auto" }}>RANGE: <span style={{ color: "rgba(255,255,255,0.5)" }}>12:00 — 14:00</span></span>
            </div>
            <ResponsiveContainer width="100%" height={140}>
              <AreaChart data={viewerData}>
                <defs>
                  <linearGradient id="viewerGrad" x1="0" y1="0" x2="0" y2="1">
                    <stop offset="5%" stopColor="#3B82F6" stopOpacity={0.3} />
                    <stop offset="95%" stopColor="#3B82F6" stopOpacity={0} />
                  </linearGradient>
                </defs>
                <CartesianGrid strokeDasharray="3 3" stroke="rgba(255,255,255,0.04)" />
                <XAxis dataKey="time" tick={{ fill: "rgba(255,255,255,0.3)", fontSize: 10 }} axisLine={false} tickLine={false} />
                <YAxis tick={{ fill: "rgba(255,255,255,0.3)", fontSize: 10 }} axisLine={false} tickLine={false} />
                <Tooltip content={<CustomTooltip />} />
                <Area type="monotone" dataKey="viewers" stroke="#3B82F6" strokeWidth={2} fill="url(#viewerGrad)" dot={false} />
              </AreaChart>
            </ResponsiveContainer>
          </div>

          {/* Health + Audience */}
          <div className="grid grid-cols-2 gap-3">
            {/* Stream Health */}
            <div className="rounded-lg p-3" style={{ background: "#111318", border: "1px solid rgba(255,255,255,0.08)" }}>
              <div className="flex items-center justify-between mb-3">
                <span style={{ fontSize: 11, fontWeight: 600, color: "rgba(255,255,255,0.6)" }}>Stream Health</span>
                <div className="flex items-center gap-1.5">
                  <div className="w-1.5 h-1.5 rounded-full" style={{ background: "#22C55E" }} />
                  <span style={{ fontSize: 10, color: "#22C55E" }}>Excellent</span>
                </div>
              </div>
              {[
                { key: "bitrate", label: "Bitrate", color: "#3B82F6", format: (v: number) => `${Math.round(v / 100) * 100} kbps` },
                { key: "cpu", label: "CPU Usage", color: "#22C55E", format: (v: number) => `${v.toFixed(0)}%` },
                { key: "gpu", label: "GPU Usage", color: "#06B6D4", format: (v: number) => `${v.toFixed(0)}%` },
                { key: "fps", label: "Frame Rate", color: "#fff", format: (v: number) => `${v.toFixed(0)} fps` },
                { key: "dropped", label: "Dropped Frames", color: "#EF4444", format: (v: number) => `${(v * 100).toFixed(2)}%` },
              ].map(metric => (
                <div key={metric.key} className="mb-1.5">
                  <div className="flex items-center justify-between mb-0.5">
                    <div className="flex items-center gap-1.5">
                      <div className="w-2 h-2 rounded-full" style={{ background: metric.color }} />
                      <span style={{ fontSize: 10, color: "rgba(255,255,255,0.5)" }}>{metric.label}</span>
                    </div>
                    <span style={{ fontSize: 10, color: metric.color, fontFamily: "'JetBrains Mono', monospace" }}>
                      {metric.format(healthData[healthData.length - 1][metric.key as keyof typeof healthData[0]] as number)}
                    </span>
                  </div>
                  <ResponsiveContainer width="100%" height={28}>
                    <LineChart data={healthData}>
                      <Line type="monotone" dataKey={metric.key} stroke={metric.color} strokeWidth={1.5} dot={false} />
                    </LineChart>
                  </ResponsiveContainer>
                </div>
              ))}
            </div>

            {/* Audience Breakdown */}
            <div className="rounded-lg p-3" style={{ background: "#111318", border: "1px solid rgba(255,255,255,0.08)" }}>
              <div className="flex items-center justify-between mb-3">
                <span style={{ fontSize: 11, fontWeight: 600, color: "rgba(255,255,255,0.6)" }}>Audience Breakdown</span>
              </div>
              <div className="flex gap-4">
                <PieChart width={120} height={120}>
                  <Pie data={platformData} cx={55} cy={55} innerRadius={35} outerRadius={55} dataKey="value" strokeWidth={0}>
                    {platformData.map((entry, i) => <Cell key={i} fill={entry.color} />)}
                  </Pie>
                </PieChart>
                <div className="flex flex-col justify-center gap-2">
                  {platformData.map(p => (
                    <div key={p.name} className="flex items-center gap-2">
                      <div className="w-2 h-2 rounded-full" style={{ background: p.color }} />
                      <span style={{ fontSize: 11, color: "rgba(255,255,255,0.6)" }}>{p.name}</span>
                      <span style={{ fontSize: 11, color: "#fff", fontWeight: 600, fontFamily: "'JetBrains Mono', monospace" }}>{p.value}%</span>
                    </div>
                  ))}
                </div>
              </div>
              <div style={{ fontSize: 10, fontWeight: 600, color: "rgba(255,255,255,0.4)", letterSpacing: "0.08em", marginTop: 12, marginBottom: 8 }}>TOP COUNTRIES</div>
              {countryData.map(c => (
                <div key={c.country} className="flex items-center gap-2 mb-1.5">
                  <span style={{ fontSize: 10, color: "rgba(255,255,255,0.5)", width: 110, flexShrink: 0 }}>{c.country}</span>
                  <div className="flex-1 rounded-full overflow-hidden" style={{ height: 4, background: "rgba(255,255,255,0.08)" }}>
                    <div style={{ width: `${(c.pct / 32) * 100}%`, height: "100%", background: "#3B82F6", borderRadius: 9999 }} />
                  </div>
                  <span style={{ fontSize: 10, color: "rgba(255,255,255,0.5)", fontFamily: "'JetBrains Mono', monospace", width: 28, textAlign: "right" }}>{c.pct}%</span>
                </div>
              ))}
            </div>
          </div>

          {/* Session History */}
          <div className="rounded-lg overflow-hidden" style={{ background: "#111318", border: "1px solid rgba(255,255,255,0.08)" }}>
            <div className="px-4 py-2.5" style={{ borderBottom: "1px solid rgba(255,255,255,0.06)" }}>
              <span style={{ fontSize: 11, fontWeight: 600, color: "rgba(255,255,255,0.6)" }}>Recent Stream Sessions</span>
            </div>
            <table className="w-full">
              <thead>
                <tr style={{ borderBottom: "1px solid rgba(255,255,255,0.05)" }}>
                  {["Date", "Duration", "Peak Viewers", "Avg Viewers", "Revenue", "Quality Score"].map(h => (
                    <th key={h} className="px-4 py-2 text-left" style={{ fontSize: 10, fontWeight: 600, color: "rgba(255,255,255,0.35)", letterSpacing: "0.06em" }}>{h}</th>
                  ))}
                </tr>
              </thead>
              <tbody>
                {sessions.map((s, i) => (
                  <tr key={i} className="transition-colors hover:bg-white/[0.02]" style={{ borderBottom: "1px solid rgba(255,255,255,0.04)" }}>
                    <td className="px-4 py-2.5" style={{ fontSize: 11, color: "rgba(255,255,255,0.6)" }}>{s.date}</td>
                    <td className="px-4 py-2.5" style={{ fontSize: 11, color: "rgba(255,255,255,0.6)", fontFamily: "'JetBrains Mono', monospace" }}>{s.duration}</td>
                    <td className="px-4 py-2.5" style={{ fontSize: 11, color: "#3B82F6", fontWeight: 600, fontFamily: "'JetBrains Mono', monospace" }}>{s.peak.toLocaleString()}</td>
                    <td className="px-4 py-2.5" style={{ fontSize: 11, color: "rgba(255,255,255,0.6)", fontFamily: "'JetBrains Mono', monospace" }}>{s.avg.toLocaleString()}</td>
                    <td className="px-4 py-2.5" style={{ fontSize: 11, color: "#22C55E", fontWeight: 600, fontFamily: "'JetBrains Mono', monospace" }}>{s.revenue}</td>
                    <td className="px-4 py-2.5">
                      <span className="rounded px-2 py-0.5" style={{ fontSize: 10, fontWeight: 600, background: `${s.scoreColor}22`, color: s.scoreColor, border: `1px solid ${s.scoreColor}44` }}>{s.score}</span>
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        </div>
      </div>
    </AppSidebar>
  );
}
