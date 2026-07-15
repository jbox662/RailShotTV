// RailShotTV — Chromatic Command — Chat & Audience
import { useState, useRef, useEffect } from "react";
import AppSidebar from "@/components/AppSidebar";
import { Send, Smile, Scissors, Users, Heart, Star, Zap, MessageSquare, Monitor } from "lucide-react";

const MESSAGES = [
  { id: 1, user: "BilliardKing99", msg: "That break shot was insane!", time: "01:23:41", badge: "MOD", badgeColor: "#10B981", color: "#34D399" },
  { id: 2, user: "CueMaster_TX", msg: "What cue is he using?", time: "01:23:44", color: "#60A5FA" },
  { id: 3, user: "PoolShark_Dan", msg: "LETS GO!!!!", time: "01:23:45", color: "#A78BFA" },
  { id: 4, user: "StreamerFan", msg: "donated $25.00", time: "01:23:47", donation: true, color: "#FCD34D" },
  { id: 5, user: "NewSub_Mike", msg: "just subscribed for 3 months!", time: "01:23:49", sub: true, color: "#A78BFA" },
  { id: 6, user: "RailShotFan", msg: "Best stream on the platform rn", time: "01:23:51", color: "#F87171" },
  { id: 7, user: "CueTipQueen", msg: "That masse shot though 🎱", time: "01:23:53", color: "#34D399" },
  { id: 8, user: "BreakAndRun", msg: "GG EZ", time: "01:23:55", color: "#60A5FA" },
  { id: 9, user: "Moderator", msg: "Keep chat clean everyone 🙏", time: "01:23:58", badge: "MOD", badgeColor: "#10B981", color: "#34D399" },
  { id: 10, user: "PoolFan_2026", msg: "First time watching, this is amazing!", time: "01:24:01", color: "#A78BFA" },
];

const ACTIVITY = [
  { type: "follow", user: "NewViewer_42", detail: "just followed", time: "2s", color: "#10B981", icon: Heart },
  { type: "sub", user: "NewSub_Mike", detail: "3-month sub", time: "12s", color: "#8B5CF6", icon: Star },
  { type: "donation", user: "StreamerFan", detail: "$25.00", time: "24s", color: "#F59E0B", icon: Zap },
  { type: "follow", user: "BilliardLover", detail: "just followed", time: "45s", color: "#10B981", icon: Heart },
  { type: "raid", user: "CueMasterTV", detail: "raided with 142 viewers", time: "2m", color: "#3B82F6", icon: Users },
];

export default function ChatPanel() {
  const [input, setInput] = useState("");
  const bottomRef = useRef<HTMLDivElement>(null);
  useEffect(() => { bottomRef.current?.scrollIntoView({ behavior: "smooth" }); }, []);

  return (
    <AppSidebar>
      {/* Top bar */}
      <div className="flex items-center gap-3 px-4 shrink-0" style={{ height: 46, background: "#0D0D15", borderBottom: "1px solid #1E1E2E" }}>
        <div className="flex items-center gap-1 mr-1">
          <span style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 18, color: "#F8F8FF", letterSpacing: "0.06em", lineHeight: 1 }}>RAILSHOT</span>
          <span style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 18, color: "#FF4D1C", letterSpacing: "0.06em", lineHeight: 1 }}>TV</span>
        </div>
        <div className="w-px h-4 mx-1" style={{ background: "#2A2A3A" }} />
        <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 600, fontSize: 11, color: "#606078", letterSpacing: "0.1em", textTransform: "uppercase" }}>Chat & Audience</span>
        <div className="flex items-center gap-1.5">
          {[["T","#9146FF"],["YT","#FF0000"],["FB","#1877F2"]].map(([label, color]) => (
            <span key={label} className="px-1.5 py-0.5 rounded text-xs font-bold" style={{ background: `${color}20`, border: `1px solid ${color}40`, color, fontFamily: "'DM Sans', sans-serif", fontSize: 10 }}>{label}</span>
          ))}
        </div>
        <div className="flex items-center gap-1.5 ml-2">
          <Users size={12} style={{ color: "#3B82F6" }} />
          <span className="mono" style={{ fontSize: 12, color: "#3B82F6", fontWeight: 600 }}>2,853</span>
        </div>
        <div className="flex-1" />
        <div className="flex items-center gap-1.5">
          <div className="live-dot w-1.5 h-1.5 rounded-full" style={{ background: "#FF4D1C" }} />
          <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 700, fontSize: 11, color: "#FF4D1C" }}>LIVE</span>
        </div>
      </div>

      <div className="flex flex-1 overflow-hidden">
        {/* Left — Preview + Quick Actions */}
        <div className="flex flex-col shrink-0" style={{ width: 180, background: "#0D0D15", borderRight: "1px solid #1E1E2E" }}>
          <div className="panel-header-brand px-3 py-1.5">
            <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 600, fontSize: 11, color: "#A0A0B8", letterSpacing: "0.1em", textTransform: "uppercase" }}>Preview</span>
          </div>
          <div className="mx-3 mt-2 rounded flex items-center justify-center" style={{ height: 80, background: "#060608", border: "1px solid #1E1E2E" }}>
            <Monitor size={20} style={{ color: "#2A2A3A" }} />
          </div>
          <div className="flex items-center justify-between px-3 py-1.5">
            <div className="flex items-center gap-1">
              <div className="live-dot w-1.5 h-1.5 rounded-full" style={{ background: "#FF4D1C" }} />
              <span className="mono" style={{ fontSize: 10, color: "#FF4D1C" }}>01:24:02</span>
            </div>
            <span className="mono" style={{ fontSize: 9, color: "#50506A" }}>1080p60</span>
          </div>

          <div className="px-3 py-2" style={{ borderTop: "1px solid #1E1E2E" }}>
            <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, color: "#606078", letterSpacing: "0.08em", textTransform: "uppercase" }}>Quick Actions</span>
            <div className="grid grid-cols-2 gap-1.5 mt-2">
              {[
                { label: "Clip", icon: Scissors, color: "#3B82F6" },
                { label: "Raid", icon: Users, color: "#8B5CF6" },
                { label: "Poll", icon: MessageSquare, color: "#10B981" },
                { label: "Marker", icon: Zap, color: "#F59E0B" },
              ].map(({ label, icon: Icon, color }) => (
                <button key={label} className="flex flex-col items-center gap-1 py-2 rounded transition-all" style={{ background: "#111118", border: "1px solid #1E1E2E" }}>
                  <Icon size={14} style={{ color }} />
                  <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, color: "#A0A0B8" }}>{label}</span>
                </button>
              ))}
            </div>
          </div>

          {/* Commands */}
          <div className="px-3 py-2 mt-auto" style={{ borderTop: "1px solid #1E1E2E" }}>
            <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, color: "#606078", letterSpacing: "0.08em", textTransform: "uppercase" }}>Commands</span>
            <div className="flex flex-wrap gap-1 mt-1.5">
              {["!discord","!socials","!schedule","!rules"].map(cmd => (
                <span key={cmd} className="px-1.5 py-0.5 rounded" style={{ background: "#1A1A24", border: "1px solid #2A2A3A", fontFamily: "'JetBrains Mono', monospace", fontSize: 9, color: "#606078" }}>{cmd}</span>
              ))}
            </div>
          </div>
        </div>

        {/* Center — Chat */}
        <div className="flex flex-col flex-1 overflow-hidden">
          <div className="panel-header-violet px-3 py-1.5 shrink-0">
            <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 600, fontSize: 11, color: "#A0A0B8", letterSpacing: "0.1em", textTransform: "uppercase" }}>Live Chat</span>
          </div>
          <div className="flex-1 overflow-y-auto px-3 py-2" style={{ background: "#0A0A0F" }}>
            {MESSAGES.map(m => (
              <div key={m.id} className={`flex flex-col mb-2 px-2 py-1.5 rounded ${m.donation ? "border" : m.sub ? "border" : ""}`} style={{ background: m.donation ? "#F59E0B0A" : m.sub ? "#8B5CF60A" : "transparent", borderColor: m.donation ? "#F59E0B30" : m.sub ? "#8B5CF630" : "transparent" }}>
                <div className="flex items-center gap-1.5 mb-0.5">
                  <span className="mono" style={{ fontSize: 9, color: "#3A3A50" }}>{m.time}</span>
                  {m.badge && <span className="px-1 py-0 rounded" style={{ background: `${m.badgeColor}20`, border: `1px solid ${m.badgeColor}40`, fontSize: 8, fontWeight: 700, color: m.badgeColor, fontFamily: "'DM Sans', sans-serif", letterSpacing: "0.06em" }}>{m.badge}</span>}
                  <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 12, fontWeight: 600, color: m.color }}>{m.user}</span>
                  {m.donation && <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, color: "#F59E0B", fontWeight: 700 }}>💰 Donated</span>}
                  {m.sub && <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, color: "#8B5CF6", fontWeight: 700 }}>⭐ Subscribed</span>}
                </div>
                <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 13, color: "#D0D0E0" }}>{m.msg}</span>
              </div>
            ))}
            <div ref={bottomRef} />
          </div>
          {/* Input */}
          <div className="flex items-center gap-2 px-3 py-2.5 shrink-0" style={{ background: "#0D0D15", borderTop: "1px solid #1E1E2E" }}>
            <button className="flex items-center justify-center rounded" style={{ width: 28, height: 28, background: "#1A1A24", border: "1px solid #2A2A3A" }}>
              <Smile size={14} style={{ color: "#606078" }} />
            </button>
            <input value={input} onChange={e => setInput(e.target.value)} placeholder="Send a message..." className="flex-1 rounded px-3 outline-none" style={{ height: 32, background: "#111118", border: "1px solid #2A2A3A", fontFamily: "'DM Sans', sans-serif", fontSize: 13, color: "#F8F8FF" }} />
            <button className="flex items-center justify-center rounded transition-all" style={{ width: 32, height: 32, background: "#8B5CF618", border: "1px solid #8B5CF640" }}>
              <Send size={14} style={{ color: "#8B5CF6" }} />
            </button>
          </div>
        </div>

        {/* Right — Activity + Poll */}
        <div className="flex flex-col shrink-0 overflow-y-auto" style={{ width: 220, background: "#0D0D15", borderLeft: "1px solid #1E1E2E" }}>
          <div className="panel-header-violet px-3 py-1.5 shrink-0">
            <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 600, fontSize: 11, color: "#A0A0B8", letterSpacing: "0.1em", textTransform: "uppercase" }}>Activity Feed</span>
          </div>
          <div className="flex flex-col" style={{ borderBottom: "1px solid #1E1E2E" }}>
            {ACTIVITY.map((a, i) => (
              <div key={i} className="flex items-center gap-2 px-3 py-2" style={{ borderBottom: "1px solid #1A1A24" }}>
                <div className="flex items-center justify-center rounded" style={{ width: 24, height: 24, background: `${a.color}18`, border: `1px solid ${a.color}30`, flexShrink: 0 }}>
                  <a.icon size={11} style={{ color: a.color }} />
                </div>
                <div className="flex flex-col flex-1 min-w-0">
                  <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, fontWeight: 600, color: "#F8F8FF", overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }}>{a.user}</span>
                  <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, color: a.color }}>{a.detail}</span>
                </div>
                <span className="mono" style={{ fontSize: 9, color: "#3A3A50", flexShrink: 0 }}>{a.time}</span>
              </div>
            ))}
          </div>

          {/* Poll */}
          <div className="panel-header-emerald px-3 py-1.5 shrink-0">
            <div className="flex items-center justify-between">
              <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 600, fontSize: 11, color: "#A0A0B8", letterSpacing: "0.1em", textTransform: "uppercase" }}>Active Poll</span>
              <span className="px-1.5 py-0.5 rounded" style={{ background: "#10B98118", border: "1px solid #10B98140", fontSize: 9, fontWeight: 700, color: "#10B981", fontFamily: "'DM Sans', sans-serif" }}>LIVE</span>
            </div>
          </div>
          <div className="px-3 py-2.5">
            <p style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 12, fontWeight: 600, color: "#F8F8FF", marginBottom: 8 }}>Which shot next?</p>
            {[{ label: "Bank Shot", pct: 62 }, { label: "Combo Shot", pct: 38 }].map(({ label, pct }) => (
              <div key={label} className="mb-2">
                <div className="flex items-center justify-between mb-1">
                  <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, color: "#A0A0B8" }}>{label}</span>
                  <span className="mono" style={{ fontSize: 11, color: "#10B981" }}>{pct}%</span>
                </div>
                <div className="rounded-full overflow-hidden" style={{ height: 5, background: "#1A1A24" }}>
                  <div style={{ width: `${pct}%`, height: "100%", background: "#10B981", borderRadius: 2, transition: "width 0.5s ease" }} />
                </div>
              </div>
            ))}
            <div className="flex items-center justify-between mt-2">
              <span className="mono" style={{ fontSize: 10, color: "#50506A" }}>1,247 votes</span>
              <span className="mono" style={{ fontSize: 10, color: "#F59E0B" }}>0:42 left</span>
            </div>
          </div>
        </div>
      </div>
    </AppSidebar>
  );
}
