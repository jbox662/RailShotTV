/**
 * Nexus Broadcast — Chat & Audience Panel (Screen 3)
 * Obsidian Studio Dark Theme
 */
import { useState, useRef, useEffect } from "react";
import AppSidebar from "@/components/AppSidebar";
import { Scissors, Users, Tv, BarChart2, Flag, Send, Smile, Shield, Ban, Clock, Trash2, MessageSquare, Heart, Star, DollarSign, Zap } from "lucide-react";

const initialMessages = [
  { id: 1, user: "PixelNinja", badge: "mod", color: "#9146FF", text: "Let's gooooo! 🔥🔥🔥", time: "12:34:11", type: "chat" },
  { id: 2, user: "ModMaven", badge: "sub", color: "#22C55E", text: "Keep the vibes positive! 💙", time: "12:34:15", type: "chat" },
  { id: 3, user: "GameKnight", badge: null, color: "#3B82F6", text: "That was INSANE! 😱", time: "12:34:18", type: "chat" },
  { id: 4, user: "StreamerFan", badge: null, color: "#F59E0B", text: "donated $25.00 — Keep it up!", time: "12:34:22", type: "donation", amount: "$25.00" },
  { id: 5, user: "LunaBytes", badge: null, color: "#EC4899", text: "GG! 🎮 💜", time: "12:34:26", type: "chat" },
  { id: 6, user: "NewSub", badge: null, color: "#9146FF", text: "just subscribed for 3 months!", time: "12:34:30", type: "sub" },
  { id: 7, user: "CoolCat", badge: "sub", color: "#06B6D4", text: "What a play! 💯", time: "12:34:33", type: "chat" },
  { id: 8, user: "EchoStorm", badge: null, color: "#8B5CF6", text: "First time catching a stream live! 👋", time: "12:34:37", type: "chat" },
  { id: 9, user: "ViperX", badge: "sub", color: "#EF4444", text: "The comeback is real!", time: "12:34:41", type: "chat" },
  { id: 10, user: "OGViewer", badge: null, color: "#F59E0B", text: "Been here since day 1! ❤️", time: "12:34:45", type: "chat" },
];

const activityFeed = [
  { type: "follow", icon: Heart, color: "#EC4899", user: "GalaxyWave", text: "is now following!", time: "12:34:50" },
  { type: "sub", icon: Star, color: "#9146FF", user: "NewSub", text: "subscribed at Tier 1.", time: "12:34:30" },
  { type: "donation", icon: DollarSign, color: "#F59E0B", user: "StreamerFan", text: "donated $25.00", time: "12:34:22" },
  { type: "raid", icon: Zap, color: "#3B82F6", user: "NinjaSquad", text: "raided with 126 viewers!", time: "12:33:58" },
];

const chatCommands = [
  { cmd: "!discord", desc: "Discord Server" },
  { cmd: "!socials", desc: "Social Links" },
  { cmd: "!schedule", desc: "Stream Schedule" },
  { cmd: "!rules", desc: "Chat Rules" },
  { cmd: "!support", desc: "Support the Stream" },
];

export default function ChatPanel() {
  const [messages, setMessages] = useState(initialMessages);
  const [input, setInput] = useState("");
  const bottomRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    bottomRef.current?.scrollIntoView({ behavior: "smooth" });
  }, [messages]);

  const sendMessage = () => {
    if (!input.trim()) return;
    setMessages(m => [...m, { id: Date.now(), user: "You", badge: "mod", color: "#3B82F6", text: input, time: new Date().toLocaleTimeString("en-US", { hour: "2-digit", minute: "2-digit", second: "2-digit", hour12: false }), type: "chat" }]);
    setInput("");
  };

  return (
    <AppSidebar>
      <div className="flex h-full overflow-hidden" style={{ fontFamily: "'Inter', sans-serif" }}>
        {/* Left: Preview + Quick Actions */}
        <div className="flex flex-col gap-3 p-3 shrink-0" style={{ width: 220, borderRight: "1px solid rgba(255,255,255,0.07)" }}>
          {/* Mini preview */}
          <div className="rounded-lg overflow-hidden" style={{ background: "#111318", border: "1px solid rgba(255,255,255,0.08)" }}>
            <div className="flex items-center justify-between px-2 py-1.5" style={{ borderBottom: "1px solid rgba(255,255,255,0.06)" }}>
              <span style={{ fontSize: 9, fontWeight: 600, color: "rgba(255,255,255,0.4)", letterSpacing: "0.1em" }}>PROGRAM PREVIEW</span>
              <div className="flex items-center gap-1">
                <div className="w-1.5 h-1.5 rounded-full animate-pulse" style={{ background: "#EF4444" }} />
                <span style={{ fontSize: 9, fontWeight: 700, color: "#EF4444" }}>LIVE</span>
              </div>
            </div>
            <div className="relative" style={{ aspectRatio: "16/9", background: "#0A0B0F" }}>
              <div className="absolute inset-0 flex items-center justify-center">
                <Tv size={24} color="rgba(255,255,255,0.1)" />
              </div>
              <div className="absolute bottom-1 left-1 right-1 flex items-center justify-between">
                <span style={{ fontSize: 8, color: "rgba(255,255,255,0.4)", fontFamily: "'JetBrains Mono', monospace" }}>02:35:47</span>
                <span style={{ fontSize: 8, color: "rgba(255,255,255,0.4)", fontFamily: "'JetBrains Mono', monospace" }}>1080p60</span>
              </div>
            </div>
          </div>

          {/* Quick Actions */}
          <div className="rounded-lg p-2" style={{ background: "#111318", border: "1px solid rgba(255,255,255,0.08)" }}>
            <div className="flex items-center justify-between mb-2">
              <span style={{ fontSize: 9, fontWeight: 600, color: "rgba(255,255,255,0.4)", letterSpacing: "0.1em" }}>QUICK ACTIONS</span>
            </div>
            <div className="grid grid-cols-2 gap-1.5">
              {[
                { label: "Clip", icon: Scissors },
                { label: "Raid", icon: Zap },
                { label: "Host", icon: Users },
                { label: "Poll", icon: BarChart2 },
              ].map(a => (
                <button key={a.label} className="rounded flex flex-col items-center gap-1 py-2.5 transition-all duration-150 hover:bg-white/5" style={{ background: "#0E0F14", border: "1px solid rgba(255,255,255,0.08)" }}>
                  <a.icon size={16} color="#3B82F6" />
                  <span style={{ fontSize: 10, color: "rgba(255,255,255,0.6)" }}>{a.label}</span>
                </button>
              ))}
            </div>
            <button className="w-full mt-1.5 rounded flex flex-col items-center gap-1 py-2 transition-all hover:bg-white/5" style={{ background: "#0E0F14", border: "1px solid rgba(255,255,255,0.08)" }}>
              <Flag size={14} color="#3B82F6" />
              <span style={{ fontSize: 10, color: "rgba(255,255,255,0.6)" }}>Marker</span>
            </button>
          </div>
        </div>

        {/* Center: Chat */}
        <div className="flex flex-col flex-1 overflow-hidden">
          {/* Header */}
          <div className="flex items-center gap-3 px-4 border-b shrink-0" style={{ borderColor: "rgba(255,255,255,0.07)", minHeight: 46, background: "#0D0E12" }}>
            <span style={{ fontFamily: "'Space Grotesk', sans-serif", fontWeight: 700, fontSize: 12, color: "#fff", letterSpacing: "0.1em" }}>LIVE CHAT</span>
            <div className="w-px h-4" style={{ background: "rgba(255,255,255,0.08)" }} />
            <div className="flex gap-1.5">
              {[{ label: "T", color: "#9146FF" }, { label: "Y", color: "#FF0000" }, { label: "F", color: "#1877F2" }].map(p => (
                <div key={p.label} className="w-5 h-5 rounded flex items-center justify-center" style={{ background: p.color, fontSize: 9, fontWeight: 700, color: "#fff" }}>{p.label}</div>
              ))}
            </div>
            <div className="flex items-center gap-1 ml-1 rounded px-2 py-0.5" style={{ background: "rgba(255,255,255,0.06)", border: "1px solid rgba(255,255,255,0.1)" }}>
              <Users size={11} color="rgba(255,255,255,0.5)" />
              <span style={{ fontSize: 11, color: "rgba(255,255,255,0.7)", fontFamily: "'JetBrains Mono', monospace" }}>2,847</span>
            </div>
          </div>

          {/* Messages */}
          <div className="flex-1 overflow-y-auto px-3 py-2 space-y-1">
            {messages.map(msg => (
              <div key={msg.id} className={`rounded px-2 py-1.5 ${msg.type !== "chat" ? "border" : ""}`}
                style={{
                  background: msg.type === "donation" ? "rgba(245,158,11,0.1)" : msg.type === "sub" ? "rgba(145,70,255,0.1)" : "transparent",
                  borderColor: msg.type === "donation" ? "rgba(245,158,11,0.25)" : msg.type === "sub" ? "rgba(145,70,255,0.25)" : "transparent",
                }}>
                <div className="flex items-baseline gap-1.5 flex-wrap">
                  <span style={{ fontSize: 10, color: "rgba(255,255,255,0.3)", fontFamily: "'JetBrains Mono', monospace" }}>{msg.time}</span>
                 {msg.badge && (
                    <span className="rounded px-1" style={{ fontSize: 8, fontWeight: 700, background: msg.badge === "mod" ? "rgba(34,197,94,0.2)" : "rgba(145,70,255,0.2)", color: msg.badge === "mod" ? "#22C55E" : "#9146FF", border: `1px solid ${msg.badge === "mod" ? "rgba(34,197,94,0.3)" : "rgba(145,70,255,0.3)"}` }}>
                      {msg.badge.toUpperCase()}
                    </span>
                  )}
                  <span style={{ fontSize: 12, fontWeight: 600, color: msg.color }}>{msg.user}:</span>
                  <span style={{ fontSize: 12, color: msg.type !== "chat" ? "#fff" : "rgba(255,255,255,0.8)" }}>{msg.text}</span>
                </div>
              </div>
            ))}
            <div ref={bottomRef} />
          </div>

          {/* Input */}
          <div className="px-3 py-2 border-t shrink-0" style={{ borderColor: "rgba(255,255,255,0.07)" }}>
            <div className="flex gap-2">
              <input
                value={input}
                onChange={e => setInput(e.target.value)}
                onKeyDown={e => e.key === "Enter" && sendMessage()}
                placeholder="Send a message..."
                className="flex-1 rounded px-3 py-2 outline-none"
                style={{ background: "#111318", border: "1px solid rgba(255,255,255,0.1)", color: "#fff", fontSize: 12 }}
              />
              <button onClick={() => {}} className="rounded px-2 py-2" style={{ background: "rgba(255,255,255,0.06)", border: "1px solid rgba(255,255,255,0.1)" }}>
                <Smile size={14} color="rgba(255,255,255,0.5)" />
              </button>
              <button onClick={sendMessage} className="rounded px-3 py-2 font-semibold" style={{ background: "#3B82F6", color: "#fff", fontSize: 12 }}>
                <Send size={14} />
              </button>
            </div>
            <div className="flex gap-2 mt-2">
              {[Shield, Ban, Users, MessageSquare, Clock, Trash2].map((Icon, i) => (
                <button key={i} className="rounded p-1.5 transition-colors hover:bg-white/5" style={{ background: "rgba(255,255,255,0.04)", border: "1px solid rgba(255,255,255,0.07)" }}>
                  <Icon size={12} color="rgba(255,255,255,0.4)" />
                </button>
              ))}
            </div>
          </div>
        </div>

        {/* Right: Activity + Polls + Commands */}
        <div className="flex flex-col gap-3 p-3 shrink-0 overflow-y-auto" style={{ width: 260, borderLeft: "1px solid rgba(255,255,255,0.07)" }}>
          {/* Activity Feed */}
          <div className="rounded-lg overflow-hidden" style={{ background: "#111318", border: "1px solid rgba(255,255,255,0.08)" }}>
            <div className="flex items-center justify-between px-3 py-2" style={{ borderBottom: "1px solid rgba(255,255,255,0.06)" }}>
              <span style={{ fontSize: 10, fontWeight: 600, color: "rgba(255,255,255,0.4)", letterSpacing: "0.1em" }}>ACTIVITY FEED</span>
            </div>
            <div className="p-2 space-y-2">
              {activityFeed.map((item, i) => (
                <div key={i} className="flex items-start gap-2">
                  <div className="w-7 h-7 rounded flex items-center justify-center shrink-0" style={{ background: `${item.color}22` }}>
                    <item.icon size={13} color={item.color} />
                  </div>
                  <div className="flex-1 min-w-0">
                    <div style={{ fontSize: 11 }}>
                      <span style={{ fontWeight: 600, color: item.color }}>{item.user}</span>
                      <span style={{ color: "rgba(255,255,255,0.6)" }}> {item.text}</span>
                    </div>
                    <div style={{ fontSize: 9, color: "rgba(255,255,255,0.3)", fontFamily: "'JetBrains Mono', monospace", marginTop: 1 }}>{item.time}</div>
                  </div>
                </div>
              ))}
            </div>
          </div>

          {/* Poll */}
          <div className="rounded-lg overflow-hidden" style={{ background: "#111318", border: "1px solid rgba(255,255,255,0.08)" }}>
            <div className="flex items-center justify-between px-3 py-2" style={{ borderBottom: "1px solid rgba(255,255,255,0.06)" }}>
              <span style={{ fontSize: 10, fontWeight: 600, color: "rgba(255,255,255,0.4)", letterSpacing: "0.1em" }}>POLLS & PREDICTIONS</span>
              <div className="rounded px-1.5 py-0.5" style={{ background: "rgba(34,197,94,0.15)", border: "1px solid rgba(34,197,94,0.3)", fontSize: 9, fontWeight: 700, color: "#22C55E" }}>ACTIVE</div>
            </div>
            <div className="p-3">
              <div style={{ fontSize: 12, fontWeight: 500, color: "#fff", marginBottom: 10 }}>Which game should we play next?</div>
              {[{ label: "Cyberpunk 2077", pct: 68, votes: 1940 }, { label: "Elden Ring", pct: 32, votes: 907 }].map(opt => (
                <div key={opt.label} className="mb-2">
                  <div className="flex justify-between mb-1">
                    <span style={{ fontSize: 11, color: "rgba(255,255,255,0.7)" }}>{opt.label}</span>
                    <span style={{ fontSize: 11, color: "#3B82F6", fontFamily: "'JetBrains Mono', monospace" }}>{opt.pct}%</span>
                  </div>
                  <div className="rounded-full overflow-hidden" style={{ height: 5, background: "rgba(255,255,255,0.08)" }}>
                    <div style={{ width: `${opt.pct}%`, height: "100%", background: "#3B82F6", borderRadius: 9999 }} />
                  </div>
                </div>
              ))}
              <div style={{ fontSize: 10, color: "rgba(255,255,255,0.35)", marginTop: 8 }}>Total Votes: 2,847 · Ends in: <span style={{ color: "#06B6D4", fontFamily: "'JetBrains Mono', monospace" }}>03:25</span></div>
            </div>
          </div>

          {/* Commands */}
          <div className="rounded-lg overflow-hidden" style={{ background: "#111318", border: "1px solid rgba(255,255,255,0.08)" }}>
            <div className="px-3 py-2" style={{ borderBottom: "1px solid rgba(255,255,255,0.06)" }}>
              <span style={{ fontSize: 10, fontWeight: 600, color: "rgba(255,255,255,0.4)", letterSpacing: "0.1em" }}>CHAT COMMANDS</span>
            </div>
            <div className="p-2 grid grid-cols-2 gap-1.5">
              {chatCommands.map(c => (
                <button key={c.cmd} className="rounded px-2 py-1.5 text-left transition-colors hover:bg-white/5" style={{ background: "#0E0F14", border: "1px solid rgba(255,255,255,0.08)" }}>
                  <div style={{ fontSize: 11, fontWeight: 600, color: "#3B82F6", fontFamily: "'JetBrains Mono', monospace" }}>{c.cmd}</div>
                  <div style={{ fontSize: 9, color: "rgba(255,255,255,0.4)", marginTop: 1 }}>{c.desc}</div>
                </button>
              ))}
            </div>
          </div>
        </div>
      </div>
    </AppSidebar>
  );
}
