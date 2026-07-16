// RailShotTV — Chromatic Command — Analytics
import { useState, useEffect, useRef } from "react";
import AppSidebar from "@/components/AppSidebar";
import { TrendingUp, Users, Clock, DollarSign, Activity, Download } from "lucide-react";
import { AreaChart, Area, XAxis, YAxis, Tooltip, ResponsiveContainer, LineChart, Line } from "recharts";

const generateViewerData = () => [] as { time: string; viewers: number }[];

const HEALTH_DATA: { t: number; bitrate: number; cpu: number; gpu: number; fps: number }[] = [];

const SESSIONS: { date: string; duration: string; peak: number; avg: number; revenue: string; quality: number }[] = [];

const AUDIENCE: { platform: string; pct: number; color: string }[] = [];

const COUNTRIES: { name: string; pct: number }[] = [];

export default function Analytics() {
  const [viewerData] = useState(generateViewerData);
  const [range, setRange] = useState("6h");

  return (
    <AppSidebar>
      {/* Top bar */}
      <div className="flex items-center gap-3 px-4 shrink-0" style={{ height: 46, background: "#141619", borderBottom: "1px solid #2A2D35" }}>
        <div className="flex items-center gap-1 mr-1">
          <span style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 18, color: "#E0E2E8", letterSpacing: "0.06em", lineHeight: 1 }}>RAILSHOT</span>
          <span style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 18, color: "#FF5A2C", letterSpacing: "0.06em", lineHeight: 1 }}>TV</span>
        </div>
        <div className="w-px h-4 mx-1" style={{ background: "#3A3D45" }} />
        <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 600, fontSize: 11, color: "#8892A4", letterSpacing: "0.1em", textTransform: "uppercase" }}>Analytics</span>
        <div className="flex-1" />
        {["5m","15m","1h","3h","6h","12h"].map(r => (
          <button key={r} onClick={() => setRange(r)} className="px-2 py-0.5 rounded text-xs transition-all" style={{ background: range === r ? "#22D3EE18" : "transparent", border: range === r ? "1px solid #22D3EE40" : "1px solid transparent", color: range === r ? "#22D3EE" : "#606878", fontFamily: "'DM Sans', sans-serif", fontSize: 11 }}>{r}</button>
        ))}
        <button className="flex items-center gap-1.5 px-2.5 py-1 rounded" style={{ background: "#141619", border: "1px solid #3A3D45" }}>
          <Download size={12} style={{ color: "#8892A4" }} />
          <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, color: "#8892A4" }}>Export</span>
        </button>
      </div>

      <div className="flex-1 overflow-y-auto px-4 py-3" style={{ background: "#0F1114" }}>
        {/* KPI cards */}
        <div className="grid grid-cols-4 gap-3 mb-4">
          {[
            { label: "Peak Viewers", value: "—", sub: "No data yet", color: "#4F9EFF", icon: Users },
            { label: "Avg Watch Time", value: "—", sub: "No data yet", color: "#A855F7", icon: Clock },
            { label: "New Followers", value: "—", sub: "No data yet", color: "#22C55E", icon: TrendingUp },
            { label: "Total Revenue", value: "—", sub: "No data yet", color: "#FBBF24", icon: DollarSign },
          ].map(({ label, value, sub, color, icon: Icon }) => (
            <div key={label} className="rounded p-3" style={{ background: "#1A1D22", border: "1px solid #2A2D35" }}>
              <div className="flex items-center justify-between mb-1.5">
                <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, color: "#8892A4", letterSpacing: "0.08em", textTransform: "uppercase" }}>{label}</span>
                <Icon size={14} style={{ color }} />
              </div>
              <div style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 28, color: "#E0E2E8", lineHeight: 1, letterSpacing: "0.04em" }}>{value}</div>
              <div style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, color, marginTop: 4 }}>{sub}</div>
            </div>
          ))}
        </div>

        {/* Main viewer chart */}
        <div className="rounded mb-4" style={{ background: "#1A1D22", border: "1px solid #2A2D35" }}>
          <div className="flex items-center justify-between px-4 py-2.5 panel-header-cyan rounded-t">
            <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 600, fontSize: 11, color: "#C0C2C8", letterSpacing: "0.1em", textTransform: "uppercase" }}>Viewer Count</span>
            <div className="flex items-center gap-1.5 px-2 py-0.5 rounded" style={{ background: "#22D3EE18", border: "1px solid #22D3EE40" }}>
              <div className="w-1.5 h-1.5 rounded-full" style={{ background: "#606878" }} />
              <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, color: "#606878", fontWeight: 600 }}>NO DATA</span>
            </div>
          </div>
          <div className="px-2 pb-3 pt-1">
            <ResponsiveContainer width="100%" height={140}>
              <AreaChart data={viewerData} margin={{ top: 4, right: 4, left: -20, bottom: 0 }}>
                <defs>
                  <linearGradient id="viewerGrad" x1="0" y1="0" x2="0" y2="1">
                    <stop offset="5%" stopColor="#22D3EE" stopOpacity={0.3}/>
                    <stop offset="95%" stopColor="#22D3EE" stopOpacity={0.02}/>
                  </linearGradient>
                </defs>
                <XAxis dataKey="time" tick={{ fill: "#606878", fontSize: 9, fontFamily: "'JetBrains Mono', monospace" }} tickLine={false} axisLine={false} interval={3} />
                <YAxis tick={{ fill: "#606878", fontSize: 9, fontFamily: "'JetBrains Mono', monospace" }} tickLine={false} axisLine={false} />
                <Tooltip contentStyle={{ background: "#141619", border: "1px solid #3A3D45", borderRadius: 4, fontSize: 11, fontFamily: "'DM Sans', sans-serif", color: "#E0E2E8" }} />
                <Area type="monotone" dataKey="viewers" stroke="#22D3EE" strokeWidth={2} fill="url(#viewerGrad)" />
              </AreaChart>
            </ResponsiveContainer>
          </div>
        </div>

        {/* Bottom row */}
        <div className="grid grid-cols-3 gap-3 mb-4">
          {/* Stream health */}
          <div className="col-span-2 rounded" style={{ background: "#1A1D22", border: "1px solid #2A2D35" }}>
            <div className="px-4 py-2.5 panel-header-emerald rounded-t">
              <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 600, fontSize: 11, color: "#C0C2C8", letterSpacing: "0.1em", textTransform: "uppercase" }}>Stream Health</span>
            </div>
            <div className="px-3 pb-3 pt-1">
              <ResponsiveContainer width="100%" height={100}>
                <LineChart data={HEALTH_DATA} margin={{ top: 4, right: 4, left: -20, bottom: 0 }}>
                  <XAxis dataKey="t" hide />
                  <YAxis hide />
                  <Tooltip contentStyle={{ background: "#141619", border: "1px solid #3A3D45", borderRadius: 4, fontSize: 10, fontFamily: "'DM Sans', sans-serif", color: "#E0E2E8" }} />
                  <Line type="monotone" dataKey="bitrate" stroke="#4F9EFF" strokeWidth={1.5} dot={false} name="Bitrate" />
                  <Line type="monotone" dataKey="cpu" stroke="#22C55E" strokeWidth={1.5} dot={false} name="CPU %" />
                  <Line type="monotone" dataKey="gpu" stroke="#22D3EE" strokeWidth={1.5} dot={false} name="GPU %" />
                  <Line type="monotone" dataKey="fps" stroke="#FBBF24" strokeWidth={1.5} dot={false} name="FPS" />
                </LineChart>
              </ResponsiveContainer>
              <div className="flex gap-4 mt-1">
                {[["Bitrate","#4F9EFF"],["CPU","#22C55E"],["GPU","#22D3EE"],["FPS","#FBBF24"]].map(([l,c]) => (
                  <div key={l} className="flex items-center gap-1">
                    <div className="w-3 h-0.5 rounded" style={{ background: c }} />
                    <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, color: "#8892A4" }}>{l}</span>
                  </div>
                ))}
              </div>
            </div>
          </div>

          {/* Audience breakdown */}
          <div className="rounded" style={{ background: "#1A1D22", border: "1px solid #2A2D35" }}>
            <div className="px-4 py-2.5 panel-header-violet rounded-t">
              <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 600, fontSize: 11, color: "#C0C2C8", letterSpacing: "0.1em", textTransform: "uppercase" }}>Audience</span>
            </div>
            <div className="px-3 py-2">
              {AUDIENCE.map(({ platform, pct, color }) => (
                <div key={platform} className="flex items-center gap-2 mb-2">
                  <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, color: "#C0C2C8", width: 64 }}>{platform}</span>
                  <div className="flex-1 rounded-full overflow-hidden" style={{ height: 5, background: "#141619" }}>
                    <div style={{ width: `${pct}%`, height: "100%", background: color, borderRadius: 2 }} />
                  </div>
                  <span className="mono" style={{ fontSize: 11, color, width: 32, textAlign: "right" }}>{pct}%</span>
                </div>
              ))}
              <div className="mt-2 pt-2" style={{ borderTop: "1px solid #2A2D35" }}>
                <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, color: "#8892A4", letterSpacing: "0.08em", textTransform: "uppercase" }}>Top Countries</span>
                {COUNTRIES.map(({ name, pct }) => (
                  <div key={name} className="flex items-center gap-2 mt-1.5">
                    <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, color: "#C0C2C8", flex: 1 }}>{name}</span>
                    <div className="rounded-full overflow-hidden" style={{ width: 48, height: 3, background: "#141619" }}>
                      <div style={{ width: `${pct}%`, height: "100%", background: "#22D3EE", borderRadius: 2 }} />
                    </div>
                    <span className="mono" style={{ fontSize: 10, color: "#22D3EE", width: 28, textAlign: "right" }}>{pct}%</span>
                  </div>
                ))}
              </div>
            </div>
          </div>
        </div>

        {/* Sessions table */}
        <div className="rounded" style={{ background: "#1A1D22", border: "1px solid #2A2D35" }}>
          <div className="px-4 py-2.5 panel-header-brand rounded-t">
            <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 600, fontSize: 11, color: "#C0C2C8", letterSpacing: "0.1em", textTransform: "uppercase" }}>Recent Sessions</span>
          </div>
          <table className="w-full">
            <thead>
              <tr style={{ borderBottom: "1px solid #2A2D35" }}>
                {["Date","Duration","Peak","Avg","Revenue","Quality"].map(h => (
                  <th key={h} className="px-4 py-2 text-left" style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, color: "#606878", fontWeight: 600, letterSpacing: "0.08em", textTransform: "uppercase" }}>{h}</th>
                ))}
              </tr>
            </thead>
            <tbody>
              {SESSIONS.map((s, i) => (
                <tr key={i} style={{ borderBottom: "1px solid #141619" }}>
                  <td className="px-4 py-2.5" style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 12, color: "#C0C2C8" }}>{s.date}</td>
                  <td className="px-4 py-2.5 mono" style={{ fontSize: 12, color: "#E0E2E8" }}>{s.duration}</td>
                  <td className="px-4 py-2.5 mono" style={{ fontSize: 12, color: "#22D3EE" }}>{s.peak.toLocaleString()}</td>
                  <td className="px-4 py-2.5 mono" style={{ fontSize: 12, color: "#C0C2C8" }}>{s.avg.toLocaleString()}</td>
                  <td className="px-4 py-2.5 mono" style={{ fontSize: 12, color: "#22C55E" }}>{s.revenue}</td>
                  <td className="px-4 py-2.5">
                    <span className="px-2 py-0.5 rounded text-xs mono" style={{ background: s.quality >= 98 ? "#22C55E18" : "#FBBF2418", border: `1px solid ${s.quality >= 98 ? "#22C55E40" : "#FBBF2440"}`, color: s.quality >= 98 ? "#22C55E" : "#FBBF24" }}>{s.quality}%</span>
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
