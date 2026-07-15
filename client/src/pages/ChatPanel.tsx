import { useState, useRef, useEffect, useCallback } from "react";
import AppSidebar from "@/components/AppSidebar";
import {
  MessageSquare, Users, Zap, Heart, Star, Monitor, Scissors,
  Wifi, WifiOff, Settings, Send, Trash2, Clock, Lock, Smile,
  ChevronDown, AlertCircle, CheckCircle2, RefreshCw, Shield,
  Gift, TrendingUp, Volume2, VolumeX, Pin, X
} from "lucide-react";
import { toast } from "sonner";

// ─── Types ────────────────────────────────────────────────────────────────────
type Platform = "twitch" | "youtube" | "facebook";
type ConnStatus = "disconnected" | "connecting" | "connected" | "error";

interface ChatMessage {
  id: number;
  platform: Platform;
  user: string;
  color: string;
  msg: string;
  time: string;
  badge?: string;
  badgeColor?: string;
  isSub?: boolean;
  isDonation?: boolean;
  donationAmount?: string;
  isMod?: boolean;
  isPinned?: boolean;
  isHighlight?: boolean;
}

interface ActivityEvent {
  id: number;
  type: "follow" | "sub" | "resub" | "giftsub" | "donation" | "raid" | "superchat";
  platform: Platform;
  user: string;
  detail: string;
  amount?: string;
  time: string;
  color: string;
}

interface PlatformConn {
  status: ConnStatus;
  channel: string;
  viewers: number;
  apiKey?: string;
}

// ─── Constants ────────────────────────────────────────────────────────────────
const PLATFORM_META: Record<Platform, { label: string; color: string; bg: string }> = {
  twitch:   { label: "Twitch",   color: "#9146FF", bg: "#9146FF20" },
  youtube:  { label: "YouTube",  color: "#FF0000", bg: "#FF000020" },
  facebook: { label: "Facebook", color: "#1877F2", bg: "#1877F220" },
};

const DEMO_MESSAGES: ChatMessage[] = [
  { id: 1,  platform: "twitch",  user: "StreamKing99",   color: "#9146FF", msg: "LET'S GOOO!! 🔥🔥🔥", time: "01:23:41" },
  { id: 2,  platform: "youtube", user: "EventWatcher",   color: "#FF0000", msg: "Amazing production quality!", time: "01:23:43" },
  { id: 3,  platform: "twitch",  user: "ProViewer",      color: "#4F9EFF", msg: "This setup is insane PogChamp", time: "01:23:45" },
  { id: 4,  platform: "youtube", user: "StreamerFan",    color: "#FBBF24", msg: "Super Chat $25 — Keep it up!", time: "01:23:47", isDonation: true, donationAmount: "$25.00", isHighlight: true },
  { id: 5,  platform: "twitch",  user: "NewSub_Mike",    color: "#A78BFA", msg: "just subscribed for 3 months! 🎉", time: "01:23:49", isSub: true, isHighlight: true },
  { id: 6,  platform: "twitch",  user: "RailShotFan",    color: "#F87171", msg: "Best stream on the platform rn", time: "01:23:51" },
  { id: 7,  platform: "youtube", user: "EventQueen",     color: "#34D399", msg: "That angle though 🎯", time: "01:23:53" },
  { id: 8,  platform: "twitch",  user: "LiveAndRun",     color: "#60A5FA", msg: "GG EZ", time: "01:23:55" },
  { id: 9,  platform: "twitch",  user: "Moderator_Dan",  color: "#34D399", msg: "Keep chat clean everyone 🙏", time: "01:23:58", isMod: true, badge: "MOD", badgeColor: "#22C55E" },
  { id: 10, platform: "youtube", user: "EventFan_2026",  color: "#A78BFA", msg: "First time watching, this is amazing!", time: "01:24:01" },
  { id: 11, platform: "twitch",  user: "GiftSub_Hero",   color: "#F59E0B", msg: "gifted 5 subs to the community! 🎁", time: "01:24:04", isSub: true, isHighlight: true },
  { id: 12, platform: "twitch",  user: "ChatGuru",       color: "#818CF8", msg: "Clip that!! ClipIt", time: "01:24:06" },
  { id: 13, platform: "youtube", user: "SuperFan_YT",    color: "#FF6B6B", msg: "Super Chat $10 — Love this stream!", time: "01:24:09", isDonation: true, donationAmount: "$10.00", isHighlight: true },
  { id: 14, platform: "twitch",  user: "NightOwl_TV",    color: "#22D3EE", msg: "Been watching for 2 hours straight 👀", time: "01:24:11" },
  { id: 15, platform: "facebook",user: "FB_Viewer_Joe",  color: "#1877F2", msg: "Watching from Facebook Live! 🙌", time: "01:24:13" },
];

const DEMO_ACTIVITY: ActivityEvent[] = [
  { id: 1, type: "follow",   platform: "twitch",  user: "NewViewer_42",    detail: "just followed",          time: "2s",  color: "#22C55E" },
  { id: 2, type: "sub",      platform: "twitch",  user: "NewSub_Mike",     detail: "3-month sub",            time: "12s", color: "#A855F7" },
  { id: 3, type: "superchat",platform: "youtube", user: "StreamerFan",     detail: "Super Chat",  amount: "$25.00", time: "24s", color: "#FBBF24" },
  { id: 4, type: "giftsub",  platform: "twitch",  user: "GiftSub_Hero",    detail: "gifted 5 subs",          time: "42s", color: "#F59E0B" },
  { id: 5, type: "raid",     platform: "twitch",  user: "StreamMasterTV",  detail: "raided with 142 viewers",time: "2m",  color: "#4F9EFF" },
  { id: 6, type: "follow",   platform: "youtube", user: "EventLover_YT",   detail: "subscribed",             time: "3m",  color: "#FF0000" },
];

const ACTIVITY_ICONS: Record<string, typeof Heart> = {
  follow: Heart, sub: Star, resub: Star, giftsub: Gift,
  donation: Zap, raid: Users, superchat: Zap,
};

// ─── Connection Status Badge ──────────────────────────────────────────────────
function ConnBadge({ status }: { status: ConnStatus }) {
  const map = {
    connected:    { color: "#22C55E", label: "Connected",    icon: CheckCircle2 },
    connecting:   { color: "#FBBF24", label: "Connecting…",  icon: RefreshCw },
    disconnected: { color: "#50506A", label: "Disconnected", icon: WifiOff },
    error:        { color: "#FF5A2C", label: "Error",        icon: AlertCircle },
  };
  const m = map[status];
  const Icon = m.icon;
  return (
    <span className="flex items-center gap-1" style={{ color: m.color }}>
      <Icon size={10} className={status === "connecting" ? "animate-spin" : ""} />
      <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, fontWeight: 600 }}>{m.label}</span>
    </span>
  );
}

// ─── Platform Connect Panel ───────────────────────────────────────────────────
function PlatformConnectPanel({
  platform, conn, onConnect, onDisconnect, onChannelChange
}: {
  platform: Platform;
  conn: PlatformConn;
  onConnect: (p: Platform) => void;
  onDisconnect: (p: Platform) => void;
  onChannelChange: (p: Platform, v: string) => void;
}) {
  const meta = PLATFORM_META[platform];
  const [expanded, setExpanded] = useState(false);
  const isConnected = conn.status === "connected";

  return (
    <div className="rounded overflow-hidden" style={{ border: `1px solid ${isConnected ? meta.color + "40" : "#2A3350"}`, background: "#111827" }}>
      {/* Header row */}
      <div
        className="flex items-center gap-2 px-3 py-2 cursor-pointer"
        style={{ background: isConnected ? meta.bg : "transparent" }}
        onClick={() => setExpanded(e => !e)}
      >
        <span className="font-bold text-xs" style={{ color: meta.color, fontFamily: "'DM Sans', sans-serif", minWidth: 28 }}>
          {platform === "youtube" ? "YT" : platform === "facebook" ? "FB" : "TW"}
        </span>
        <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 12, color: "#C0C8D8", fontWeight: 600 }}>{meta.label}</span>
        {isConnected && (
          <span className="flex items-center gap-1 ml-1">
            <Users size={10} style={{ color: meta.color }} />
            <span className="mono" style={{ fontSize: 10, color: meta.color }}>{conn.viewers.toLocaleString()}</span>
          </span>
        )}
        <div className="flex-1" />
        <ConnBadge status={conn.status} />
        <ChevronDown size={12} style={{ color: "#50506A", transform: expanded ? "rotate(180deg)" : "none", transition: "transform 0.2s" }} />
      </div>

      {/* Expanded config */}
      {expanded && (
        <div className="px-3 pb-3 pt-1" style={{ borderTop: "1px solid #2A3350" }}>
          <div className="flex flex-col gap-2">
            <div>
              <label style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, color: "#8892A4", letterSpacing: "0.08em", textTransform: "uppercase" }}>
                {platform === "twitch" ? "Channel Name" : platform === "youtube" ? "Channel / Live Chat ID" : "Page ID"}
              </label>
              <input
                value={conn.channel}
                onChange={e => onChannelChange(platform, e.target.value)}
                placeholder={platform === "twitch" ? "your_channel" : platform === "youtube" ? "UC... or @handle" : "page_id"}
                className="w-full mt-1 px-2 py-1.5 rounded text-xs"
                style={{ background: "#0D1220", border: "1px solid #2A3350", color: "#C0C8D8", fontFamily: "'JetBrains Mono', monospace", outline: "none" }}
              />
            </div>
            {platform !== "twitch" && (
              <div>
                <label style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, color: "#8892A4", letterSpacing: "0.08em", textTransform: "uppercase" }}>
                  API Key
                </label>
                <input
                  type="password"
                  placeholder="••••••••••••••••"
                  className="w-full mt-1 px-2 py-1.5 rounded text-xs"
                  style={{ background: "#0D1220", border: "1px solid #2A3350", color: "#C0C8D8", fontFamily: "'JetBrains Mono', monospace", outline: "none" }}
                />
              </div>
            )}
            <div className="flex gap-2 mt-1">
              {isConnected ? (
                <button
                  onClick={() => onDisconnect(platform)}
                  className="flex-1 py-1.5 rounded text-xs font-bold flex items-center justify-center gap-1"
                  style={{ background: "#FF5A2C20", border: "1px solid #FF5A2C60", color: "#FF5A2C", fontFamily: "'DM Sans', sans-serif" }}
                >
                  <WifiOff size={11} /> Disconnect
                </button>
              ) : (
                <button
                  onClick={() => onConnect(platform)}
                  className="flex-1 py-1.5 rounded text-xs font-bold flex items-center justify-center gap-1"
                  style={{ background: meta.bg, border: `1px solid ${meta.color}60`, color: meta.color, fontFamily: "'DM Sans', sans-serif" }}
                >
                  <Wifi size={11} /> Connect
                </button>
              )}
            </div>
            {platform === "twitch" && (
              <p style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, color: "#50506A", lineHeight: 1.4 }}>
                Connects via Twitch IRC over WSS. No API key required for read-only chat.
              </p>
            )}
          </div>
        </div>
      )}
    </div>
  );
}

// ─── Message Row ─────────────────────────────────────────────────────────────
function MessageRow({ msg, onPin }: { msg: ChatMessage; onPin: (id: number) => void }) {
  const meta = PLATFORM_META[msg.platform];
  const isSpecial = msg.isHighlight || msg.isDonation || msg.isSub;

  return (
    <div
      className="px-3 py-1.5 group relative"
      style={{
        background: isSpecial
          ? (msg.isDonation ? "#FBBF2410" : "#A855F710")
          : msg.isPinned ? "#4F9EFF10" : "transparent",
        borderLeft: isSpecial ? `2px solid ${msg.isDonation ? "#FBBF24" : "#A855F7"}` : "2px solid transparent",
        borderBottom: "1px solid #1A2035",
      }}
    >
      {msg.isPinned && (
        <div className="flex items-center gap-1 mb-0.5">
          <Pin size={9} style={{ color: "#4F9EFF" }} />
          <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 9, color: "#4F9EFF", textTransform: "uppercase", letterSpacing: "0.08em" }}>Pinned</span>
        </div>
      )}
      <div className="flex items-start gap-1.5">
        {/* Platform badge */}
        <span className="mt-0.5 px-1 rounded text-xs font-bold shrink-0" style={{ background: meta.bg, color: meta.color, fontFamily: "'DM Sans', sans-serif", fontSize: 9 }}>
          {msg.platform === "youtube" ? "YT" : msg.platform === "facebook" ? "FB" : "TW"}
        </span>
        {/* Mod badge */}
        {msg.isMod && (
          <span className="mt-0.5 px-1 rounded shrink-0" style={{ background: "#22C55E20", color: "#22C55E", fontFamily: "'DM Sans', sans-serif", fontSize: 9, fontWeight: 700 }}>MOD</span>
        )}
        {/* Sub badge */}
        {msg.isSub && !msg.isDonation && (
          <Star size={10} className="mt-0.5 shrink-0" style={{ color: "#A855F7" }} />
        )}
        {/* Donation badge */}
        {msg.isDonation && (
          <Zap size={10} className="mt-0.5 shrink-0" style={{ color: "#FBBF24" }} />
        )}
        <div className="flex-1 min-w-0">
          <span className="font-bold text-xs" style={{ color: msg.color, fontFamily: "'DM Sans', sans-serif" }}>{msg.user}</span>
          {msg.donationAmount && (
            <span className="ml-1.5 px-1.5 py-0.5 rounded text-xs font-bold" style={{ background: "#FBBF2420", color: "#FBBF24", fontFamily: "'DM Sans', sans-serif", fontSize: 10 }}>{msg.donationAmount}</span>
          )}
          <span className="ml-1.5 text-xs" style={{ color: "#C0C8D8", fontFamily: "'DM Sans', sans-serif", wordBreak: "break-word" }}>{msg.msg}</span>
        </div>
        <span className="mono shrink-0 mt-0.5" style={{ fontSize: 9, color: "#50506A" }}>{msg.time}</span>
        {/* Pin button on hover */}
        <button
          onClick={() => onPin(msg.id)}
          className="opacity-0 group-hover:opacity-100 shrink-0 mt-0.5"
          style={{ transition: "opacity 0.15s", color: "#50506A" }}
          title="Pin message"
        >
          <Pin size={10} />
        </button>
      </div>
    </div>
  );
}

// ─── Activity Event Row ───────────────────────────────────────────────────────
function ActivityRow({ ev }: { ev: ActivityEvent }) {
  const Icon = ACTIVITY_ICONS[ev.type] || Heart;
  const meta = PLATFORM_META[ev.platform];
  return (
    <div className="flex items-center gap-2 px-3 py-2" style={{ borderBottom: "1px solid #1A2035" }}>
      <div className="w-6 h-6 rounded flex items-center justify-center shrink-0" style={{ background: `${ev.color}20` }}>
        <Icon size={12} style={{ color: ev.color }} />
      </div>
      <div className="flex-1 min-w-0">
        <span className="font-bold text-xs" style={{ color: "#C0C8D8", fontFamily: "'DM Sans', sans-serif" }}>{ev.user}</span>
        <span className="ml-1 text-xs" style={{ color: "#8892A4", fontFamily: "'DM Sans', sans-serif" }}>{ev.detail}</span>
        {ev.amount && (
          <span className="ml-1 font-bold text-xs" style={{ color: ev.color, fontFamily: "'DM Sans', sans-serif" }}>{ev.amount}</span>
        )}
      </div>
      <span className="px-1 rounded text-xs font-bold shrink-0" style={{ background: meta.bg, color: meta.color, fontFamily: "'DM Sans', sans-serif", fontSize: 9 }}>
        {ev.platform === "youtube" ? "YT" : ev.platform === "facebook" ? "FB" : "TW"}
      </span>
      <span className="mono shrink-0" style={{ fontSize: 9, color: "#50506A" }}>{ev.time}</span>
    </div>
  );
}

// ─── Main Component ───────────────────────────────────────────────────────────
export default function ChatPanel() {
  const [messages, setMessages] = useState<ChatMessage[]>(DEMO_MESSAGES);
  const [activity, setActivity] = useState<ActivityEvent[]>(DEMO_ACTIVITY);
  const [input, setInput] = useState("");
  const [activeTab, setActiveTab] = useState<"chat" | "activity" | "moderation">("chat");
  const [filterPlatform, setFilterPlatform] = useState<Platform | "all">("all");
  const [slowMode, setSlowMode] = useState(false);
  const [subOnly, setSubOnly] = useState(false);
  const [emoteOnly, setEmoteOnly] = useState(false);
  const [muteAlerts, setMuteAlerts] = useState(false);
  const [connections, setConnections] = useState<Record<Platform, PlatformConn>>({
    twitch:   { status: "connected",    channel: "railshottv",   viewers: 1842 },
    youtube:  { status: "connected",    channel: "@RailShotTV",  viewers: 1011 },
    facebook: { status: "disconnected", channel: "",             viewers: 0 },
  });
  const bottomRef = useRef<HTMLDivElement>(null);
  const nextId = useRef(100);

  // Auto-scroll to bottom when new messages arrive
  useEffect(() => {
    bottomRef.current?.scrollIntoView({ behavior: "smooth" });
  }, [messages]);

  // Simulate incoming messages every 4–8 seconds
  useEffect(() => {
    const DEMO_INCOMING = [
      { platform: "twitch" as Platform,  user: "FastViewer",    color: "#60A5FA", msg: "PogChamp PogChamp PogChamp" },
      { platform: "youtube" as Platform, user: "YT_Watcher",    color: "#FF6B6B", msg: "Incredible stream quality!" },
      { platform: "twitch" as Platform,  user: "CheerLeader",   color: "#A78BFA", msg: "Cheer100 Let's go!! 🎉" },
      { platform: "twitch" as Platform,  user: "LateNightFan",  color: "#34D399", msg: "Just got here, what did I miss?" },
      { platform: "youtube" as Platform, user: "GlobalViewer",  color: "#FBBF24", msg: "Watching from Australia 🇦🇺" },
      { platform: "facebook" as Platform,user: "FB_Fan_Sarah",  color: "#1877F2", msg: "Sharing this with everyone!" },
    ];
    let idx = 0;
    const interval = setInterval(() => {
      const demo = DEMO_INCOMING[idx % DEMO_INCOMING.length];
      const now = new Date();
      const time = `${String(now.getHours()).padStart(2,"0")}:${String(now.getMinutes()).padStart(2,"0")}:${String(now.getSeconds()).padStart(2,"0")}`;
      setMessages(prev => [...prev.slice(-99), { ...demo, id: nextId.current++, time }]);
      idx++;
    }, 5000 + Math.random() * 3000);
    return () => clearInterval(interval);
  }, []);

  const handleConnect = useCallback((platform: Platform) => {
    setConnections(prev => ({ ...prev, [platform]: { ...prev[platform], status: "connecting" } }));
    setTimeout(() => {
      setConnections(prev => ({ ...prev, [platform]: { ...prev[platform], status: "connected", viewers: Math.floor(Math.random() * 500) + 100 } }));
      toast.success(`Connected to ${PLATFORM_META[platform].label} chat`);
    }, 1800);
  }, []);

  const handleDisconnect = useCallback((platform: Platform) => {
    setConnections(prev => ({ ...prev, [platform]: { ...prev[platform], status: "disconnected", viewers: 0 } }));
    toast.info(`Disconnected from ${PLATFORM_META[platform].label}`);
  }, []);

  const handleChannelChange = useCallback((platform: Platform, value: string) => {
    setConnections(prev => ({ ...prev, [platform]: { ...prev[platform], channel: value } }));
  }, []);

  const handlePin = useCallback((id: number) => {
    setMessages(prev => prev.map(m => m.id === id ? { ...m, isPinned: !m.isPinned } : m));
  }, []);

  const handleClearChat = () => {
    setMessages([]);
    toast.info("Chat cleared");
  };

  const handleSend = () => {
    if (!input.trim()) return;
    toast.info("Send to chat: feature requires live Twitch/YouTube credentials");
    setInput("");
  };

  const totalViewers = Object.values(connections).reduce((s, c) => s + (c.status === "connected" ? c.viewers : 0), 0);
  const connectedCount = Object.values(connections).filter(c => c.status === "connected").length;

  const filteredMessages = filterPlatform === "all"
    ? messages
    : messages.filter(m => m.platform === filterPlatform);

  const tabs = [
    { id: "chat" as const,       label: "Live Chat",   count: messages.length },
    { id: "activity" as const,   label: "Activity",    count: activity.length },
    { id: "moderation" as const, label: "Moderation",  count: null },
  ];

  return (
    <AppSidebar>
      {/* ── Top bar ─────────────────────────────────────────────────────────── */}
      <div className="flex items-center gap-3 px-4 shrink-0" style={{ height: 46, background: "#1A2035", borderBottom: "1px solid #2A3350" }}>
        <div className="flex items-center gap-1 mr-1">
          <span style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 18, color: "#F8F8FF", letterSpacing: "0.06em", lineHeight: 1 }}>RAILSHOT</span>
          <span style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 18, color: "#FF5A2C", letterSpacing: "0.06em", lineHeight: 1 }}>TV</span>
        </div>
        <div className="w-px h-4 mx-1" style={{ background: "#303D5A" }} />
        <MessageSquare size={13} style={{ color: "#4F9EFF" }} />
        <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 600, fontSize: 11, color: "#8892A4", letterSpacing: "0.1em", textTransform: "uppercase" }}>Chat & Audience</span>
        {/* Platform status pills */}
        <div className="flex items-center gap-1.5 ml-1">
          {(Object.entries(connections) as [Platform, PlatformConn][]).map(([p, c]) => {
            const meta = PLATFORM_META[p];
            return (
              <span key={p} className="flex items-center gap-1 px-1.5 py-0.5 rounded" style={{ background: c.status === "connected" ? meta.bg : "#1A2035", border: `1px solid ${c.status === "connected" ? meta.color + "50" : "#2A3350"}` }}>
                <div className="w-1.5 h-1.5 rounded-full" style={{ background: c.status === "connected" ? meta.color : "#50506A" }} />
                <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 9, fontWeight: 700, color: c.status === "connected" ? meta.color : "#50506A" }}>
                  {p === "youtube" ? "YT" : p === "facebook" ? "FB" : "TW"}
                </span>
              </span>
            );
          })}
        </div>
        <div className="flex items-center gap-1.5 ml-1">
          <Users size={12} style={{ color: "#4F9EFF" }} />
          <span className="mono" style={{ fontSize: 12, color: "#4F9EFF", fontWeight: 600 }}>{totalViewers.toLocaleString()}</span>
        </div>
        <div className="flex-1" />
        <div className="flex items-center gap-1.5">
          <div className="w-1.5 h-1.5 rounded-full" style={{ background: connectedCount > 0 ? "#22C55E" : "#50506A" }} />
          <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 700, fontSize: 11, color: connectedCount > 0 ? "#22C55E" : "#50506A" }}>
            {connectedCount > 0 ? `${connectedCount} CONNECTED` : "OFFLINE"}
          </span>
        </div>
      </div>

      <div className="flex flex-1 overflow-hidden">
        {/* ── Left sidebar: connections + moderation ─────────────────────────── */}
        <div className="flex flex-col shrink-0" style={{ width: 220, background: "#111827", borderRight: "1px solid #2A3350", overflowY: "auto" }}>
          {/* Platform connections */}
          <div className="px-3 pt-3 pb-2">
            <div className="flex items-center gap-1.5 mb-2">
              <Wifi size={11} style={{ color: "#4F9EFF" }} />
              <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, fontWeight: 700, color: "#8892A4", letterSpacing: "0.1em", textTransform: "uppercase" }}>Connections</span>
            </div>
            <div className="flex flex-col gap-2">
              {(["twitch", "youtube", "facebook"] as Platform[]).map(p => (
                <PlatformConnectPanel
                  key={p}
                  platform={p}
                  conn={connections[p]}
                  onConnect={handleConnect}
                  onDisconnect={handleDisconnect}
                  onChannelChange={handleChannelChange}
                />
              ))}
            </div>
          </div>

          {/* Filter by platform */}
          <div className="px-3 py-2" style={{ borderTop: "1px solid #2A3350" }}>
            <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, fontWeight: 700, color: "#8892A4", letterSpacing: "0.1em", textTransform: "uppercase" }}>Filter</span>
            <div className="flex flex-wrap gap-1.5 mt-2">
              {(["all", "twitch", "youtube", "facebook"] as const).map(p => {
                const meta = p !== "all" ? PLATFORM_META[p] : null;
                const active = filterPlatform === p;
                return (
                  <button
                    key={p}
                    onClick={() => setFilterPlatform(p)}
                    className="px-2 py-0.5 rounded text-xs font-bold"
                    style={{
                      background: active ? (meta ? meta.bg : "#4F9EFF20") : "#0D1220",
                      border: `1px solid ${active ? (meta ? meta.color + "60" : "#4F9EFF60") : "#2A3350"}`,
                      color: active ? (meta ? meta.color : "#4F9EFF") : "#8892A4",
                      fontFamily: "'DM Sans', sans-serif",
                    }}
                  >
                    {p === "all" ? "All" : p === "youtube" ? "YT" : p === "facebook" ? "FB" : "TW"}
                  </button>
                );
              })}
            </div>
          </div>

          {/* Quick stats */}
          <div className="px-3 py-2" style={{ borderTop: "1px solid #2A3350" }}>
            <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, fontWeight: 700, color: "#8892A4", letterSpacing: "0.1em", textTransform: "uppercase" }}>Session Stats</span>
            <div className="grid grid-cols-2 gap-2 mt-2">
              {[
                { label: "Messages", value: messages.length, color: "#4F9EFF" },
                { label: "Highlights", value: messages.filter(m => m.isHighlight).length, color: "#FBBF24" },
                { label: "Subs", value: messages.filter(m => m.isSub).length, color: "#A855F7" },
                { label: "Donations", value: messages.filter(m => m.isDonation).length, color: "#22C55E" },
              ].map(s => (
                <div key={s.label} className="rounded p-2" style={{ background: "#0D1220", border: "1px solid #2A3350" }}>
                  <div className="mono font-bold" style={{ fontSize: 16, color: s.color }}>{s.value}</div>
                  <div style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 9, color: "#50506A", textTransform: "uppercase", letterSpacing: "0.06em" }}>{s.label}</div>
                </div>
              ))}
            </div>
          </div>
        </div>

        {/* ── Main chat area ─────────────────────────────────────────────────── */}
        <div className="flex flex-col flex-1 overflow-hidden">
          {/* Tabs */}
          <div className="flex items-center gap-0 shrink-0 px-3" style={{ background: "#111827", borderBottom: "1px solid #2A3350", height: 38 }}>
            {tabs.map(tab => (
              <button
                key={tab.id}
                onClick={() => setActiveTab(tab.id)}
                className="flex items-center gap-1.5 px-3 h-full text-xs font-bold relative"
                style={{
                  fontFamily: "'DM Sans', sans-serif",
                  color: activeTab === tab.id ? "#F8F8FF" : "#8892A4",
                  borderBottom: activeTab === tab.id ? "2px solid #4F9EFF" : "2px solid transparent",
                  background: "transparent",
                }}
              >
                {tab.label}
                {tab.count !== null && (
                  <span className="px-1.5 py-0.5 rounded-full" style={{ background: activeTab === tab.id ? "#4F9EFF20" : "#2A3350", color: activeTab === tab.id ? "#4F9EFF" : "#50506A", fontSize: 9 }}>
                    {tab.count}
                  </span>
                )}
              </button>
            ))}
            <div className="flex-1" />
            {/* Clear chat */}
            <button
              onClick={handleClearChat}
              className="flex items-center gap-1 px-2 py-1 rounded text-xs"
              style={{ color: "#50506A", fontFamily: "'DM Sans', sans-serif" }}
              title="Clear chat"
            >
              <Trash2 size={11} />
            </button>
          </div>

          {/* Chat tab */}
          {activeTab === "chat" && (
            <>
              <div className="flex-1 overflow-y-auto" style={{ background: "#0D1220" }}>
                {filteredMessages.length === 0 ? (
                  <div className="flex flex-col items-center justify-center h-full gap-3">
                    <MessageSquare size={32} style={{ color: "#2A3350" }} />
                    <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 13, color: "#50506A" }}>No messages yet</span>
                  </div>
                ) : (
                  filteredMessages.map(msg => (
                    <MessageRow key={msg.id} msg={msg} onPin={handlePin} />
                  ))
                )}
                <div ref={bottomRef} />
              </div>
              {/* Chat input */}
              <div className="flex items-center gap-2 px-3 py-2 shrink-0" style={{ background: "#111827", borderTop: "1px solid #2A3350" }}>
                <input
                  value={input}
                  onChange={e => setInput(e.target.value)}
                  onKeyDown={e => e.key === "Enter" && handleSend()}
                  placeholder="Send a message to all connected chats…"
                  className="flex-1 px-3 py-1.5 rounded text-xs"
                  style={{ background: "#0D1220", border: "1px solid #2A3350", color: "#C0C8D8", fontFamily: "'DM Sans', sans-serif", outline: "none" }}
                />
                <button
                  onClick={handleSend}
                  className="p-1.5 rounded"
                  style={{ background: "#4F9EFF20", border: "1px solid #4F9EFF40", color: "#4F9EFF" }}
                >
                  <Send size={13} />
                </button>
              </div>
            </>
          )}

          {/* Activity tab */}
          {activeTab === "activity" && (
            <div className="flex-1 overflow-y-auto" style={{ background: "#0D1220" }}>
              {activity.map(ev => <ActivityRow key={ev.id} ev={ev} />)}
            </div>
          )}

          {/* Moderation tab */}
          {activeTab === "moderation" && (
            <div className="flex-1 overflow-y-auto p-4" style={{ background: "#0D1220" }}>
              <div className="flex flex-col gap-3 max-w-sm">
                <div className="flex items-center gap-2 mb-1">
                  <Shield size={14} style={{ color: "#4F9EFF" }} />
                  <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 700, fontSize: 13, color: "#C0C8D8" }}>Chat Moderation</span>
                </div>
                {[
                  { label: "Slow Mode", desc: "Limit messages to 1 per 30 seconds", icon: Clock, state: slowMode, set: setSlowMode, color: "#FBBF24" },
                  { label: "Subscribers Only", desc: "Only subscribers can chat", icon: Star, state: subOnly, set: setSubOnly, color: "#A855F7" },
                  { label: "Emote Only", desc: "Only emotes allowed in chat", icon: Smile, state: emoteOnly, set: setEmoteOnly, color: "#22D3EE" },
                  { label: "Mute Alerts", desc: "Silence audio notifications", icon: muteAlerts ? VolumeX : Volume2, state: muteAlerts, set: setMuteAlerts, color: "#FF5A2C" },
                ].map(item => {
                  const Icon = item.icon;
                  return (
                    <div key={item.label} className="flex items-center gap-3 p-3 rounded" style={{ background: "#111827", border: `1px solid ${item.state ? item.color + "40" : "#2A3350"}` }}>
                      <div className="w-8 h-8 rounded flex items-center justify-center shrink-0" style={{ background: item.state ? `${item.color}20` : "#1A2035" }}>
                        <Icon size={15} style={{ color: item.state ? item.color : "#50506A" }} />
                      </div>
                      <div className="flex-1">
                        <div style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 700, fontSize: 12, color: "#C0C8D8" }}>{item.label}</div>
                        <div style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, color: "#50506A" }}>{item.desc}</div>
                      </div>
                      <button
                        onClick={() => { item.set(!item.state); toast.info(`${item.label} ${!item.state ? "enabled" : "disabled"}`); }}
                        className="w-10 h-5 rounded-full relative shrink-0"
                        style={{ background: item.state ? item.color : "#2A3350", transition: "background 0.2s" }}
                      >
                        <div className="w-4 h-4 rounded-full absolute top-0.5" style={{ background: "#F8F8FF", left: item.state ? "calc(100% - 18px)" : "2px", transition: "left 0.2s" }} />
                      </button>
                    </div>
                  );
                })}
                <button
                  onClick={handleClearChat}
                  className="flex items-center justify-center gap-2 py-2 rounded mt-2"
                  style={{ background: "#FF5A2C20", border: "1px solid #FF5A2C40", color: "#FF5A2C", fontFamily: "'DM Sans', sans-serif", fontWeight: 700, fontSize: 12 }}
                >
                  <Trash2 size={13} /> Clear All Chat
                </button>
              </div>
            </div>
          )}
        </div>

        {/* ── Right sidebar: highlights + activity feed ──────────────────────── */}
        <div className="flex flex-col shrink-0" style={{ width: 220, background: "#111827", borderLeft: "1px solid #2A3350", overflowY: "auto" }}>
          {/* Pinned messages */}
          <div className="px-3 pt-3 pb-2">
            <div className="flex items-center gap-1.5 mb-2">
              <Pin size={11} style={{ color: "#4F9EFF" }} />
              <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, fontWeight: 700, color: "#8892A4", letterSpacing: "0.1em", textTransform: "uppercase" }}>Pinned</span>
            </div>
            {messages.filter(m => m.isPinned).length === 0 ? (
              <div className="rounded p-3 text-center" style={{ background: "#0D1220", border: "1px solid #2A3350" }}>
                <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, color: "#50506A" }}>No pinned messages</span>
              </div>
            ) : (
              messages.filter(m => m.isPinned).map(m => (
                <div key={m.id} className="rounded p-2 mb-1.5" style={{ background: "#4F9EFF10", border: "1px solid #4F9EFF30" }}>
                  <span className="font-bold text-xs" style={{ color: m.color, fontFamily: "'DM Sans', sans-serif" }}>{m.user}: </span>
                  <span className="text-xs" style={{ color: "#C0C8D8", fontFamily: "'DM Sans', sans-serif" }}>{m.msg}</span>
                </div>
              ))
            )}
          </div>

          {/* Highlights */}
          <div className="px-3 py-2" style={{ borderTop: "1px solid #2A3350" }}>
            <div className="flex items-center gap-1.5 mb-2">
              <TrendingUp size={11} style={{ color: "#FBBF24" }} />
              <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, fontWeight: 700, color: "#8892A4", letterSpacing: "0.1em", textTransform: "uppercase" }}>Highlights</span>
            </div>
            <div className="flex flex-col gap-1.5">
              {messages.filter(m => m.isHighlight).slice(-5).map(m => (
                <div key={m.id} className="rounded p-2" style={{ background: m.isDonation ? "#FBBF2410" : "#A855F710", border: `1px solid ${m.isDonation ? "#FBBF2430" : "#A855F730"}` }}>
                  <div className="flex items-center gap-1 mb-0.5">
                    {m.isDonation ? <Zap size={9} style={{ color: "#FBBF24" }} /> : <Star size={9} style={{ color: "#A855F7" }} />}
                    <span className="font-bold" style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, color: m.isDonation ? "#FBBF24" : "#A855F7" }}>
                      {m.isDonation ? m.donationAmount : "Sub"}
                    </span>
                    <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 9, color: "#50506A" }}>· {m.time}</span>
                  </div>
                  <span className="font-bold text-xs" style={{ color: m.color, fontFamily: "'DM Sans', sans-serif" }}>{m.user}</span>
                  <p className="text-xs mt-0.5" style={{ color: "#8892A4", fontFamily: "'DM Sans', sans-serif", lineHeight: 1.3 }}>{m.msg}</p>
                </div>
              ))}
            </div>
          </div>

          {/* Recent activity */}
          <div className="px-3 py-2" style={{ borderTop: "1px solid #2A3350" }}>
            <div className="flex items-center gap-1.5 mb-2">
              <Zap size={11} style={{ color: "#22C55E" }} />
              <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, fontWeight: 700, color: "#8892A4", letterSpacing: "0.1em", textTransform: "uppercase" }}>Recent Activity</span>
            </div>
            <div className="flex flex-col gap-1">
              {activity.slice(0, 6).map(ev => {
                const Icon = ACTIVITY_ICONS[ev.type] || Heart;
                return (
                  <div key={ev.id} className="flex items-center gap-2 py-1">
                    <Icon size={10} style={{ color: ev.color }} className="shrink-0" />
                    <span className="text-xs truncate" style={{ color: "#8892A4", fontFamily: "'DM Sans', sans-serif" }}>
                      <span style={{ color: "#C0C8D8", fontWeight: 600 }}>{ev.user}</span> {ev.detail}
                    </span>
                    <span className="mono shrink-0" style={{ fontSize: 9, color: "#50506A" }}>{ev.time}</span>
                  </div>
                );
              })}
            </div>
          </div>
        </div>
      </div>
    </AppSidebar>
  );
}
