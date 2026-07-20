// RailShotTV — GoLiveModal
// Pre-stream checklist modal: 5 steps → countdown → LIVE
// Colors: Brand=#FF5A2C, Blue=#4F9EFF, Violet=#A855F7, Emerald=#22C55E, Cyan=#22D3EE
import { useState, useEffect, useRef } from "react";
import { CheckCircle2, Circle, XCircle, Loader2, Wifi, Mic, Monitor, Video, Radio, ChevronRight, X, Zap, Youtube, Facebook, Twitch } from "lucide-react";

// ── Types ─────────────────────────────────────────────────────────────────────
type CheckStatus = "pending" | "checking" | "ok" | "warn" | "fail";

interface CheckItem {
  id: string;
  label: string;
  detail: string;
  icon: React.ElementType;
  color: string;
  status: CheckStatus;
  warnMsg?: string;
}

interface Platform {
  id: string;
  name: string;
  icon: React.ElementType;
  color: string;
  placeholder: string;
}

const PLATFORMS: Platform[] = [
  { id: "youtube",  name: "YouTube",  icon: Youtube,  color: "#FF0000", placeholder: "rtmp://a.rtmp.youtube.com/live2" },
  { id: "twitch",   name: "Twitch",   icon: Twitch,   color: "#9146FF", placeholder: "rtmp://live.twitch.tv/app" },
  { id: "facebook", name: "Facebook", icon: Facebook, color: "#1877F2", placeholder: "rtmps://live-api-s.facebook.com:443/rtmp" },
  { id: "custom",   name: "Custom",   icon: Radio,    color: "#FF5A2C", placeholder: "rtmp://your-server/live" },
];

// ── Checklist items (simulated checks) ───────────────────────────────────────
function buildChecks(): CheckItem[] {
  return [
    { id: "audio",   label: "Audio Devices",   detail: "Desktop Audio + Mic/Aux detected",    icon: Mic,     color: "#A855F7", status: "pending" },
    { id: "scene",   label: "Active Scene",    detail: "Scene with at least one source active", icon: Monitor, color: "#4F9EFF", status: "pending" },
    { id: "encoder", label: "Video Encoder",   detail: "NVENC H.264 @ 8,000 kbps CBR",         icon: Video,   color: "#22C55E", status: "pending" },
    { id: "network", label: "Network",         detail: "RTMP server reachable",                 icon: Wifi,    color: "#22D3EE", status: "pending" },
    { id: "stream",  label: "Stream Key",      detail: "Platform credentials configured",       icon: Zap,     color: "#FBBF24", status: "pending" },
  ];
}

// ── Animated check icon ───────────────────────────────────────────────────────
function StatusIcon({ status, color }: { status: CheckStatus; color: string }) {
  if (status === "checking") return <Loader2 size={18} className="animate-spin" style={{ color }} />;
  if (status === "ok")       return <CheckCircle2 size={18} style={{ color: "#22C55E" }} />;
  if (status === "warn")     return <CheckCircle2 size={18} style={{ color: "#FBBF24" }} />;
  if (status === "fail")     return <XCircle size={18} style={{ color: "#EF4444" }} />;
  return <Circle size={18} style={{ color: "#303D5A" }} />;
}

// ── Platform button ───────────────────────────────────────────────────────────
function PlatformBtn({ p, selected, onClick }: { p: Platform; selected: boolean; onClick: () => void }) {
  const Icon = p.icon;
  return (
    <button
      onClick={onClick}
      className="flex flex-col items-center gap-1.5 rounded-lg transition-all duration-150"
      style={{
        width: 72, height: 64, padding: "10px 8px",
        background: selected ? `${p.color}18` : "#111827",
        border: `1.5px solid ${selected ? p.color : "#2A3350"}`,
        boxShadow: selected ? `0 0 12px ${p.color}30` : "none",
        transform: selected ? "scale(1.04)" : "scale(1)",
      }}
    >
      <Icon size={20} style={{ color: selected ? p.color : "#8892A4" }} />
      <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, fontWeight: 600, color: selected ? p.color : "#8892A4", letterSpacing: "0.04em" }}>
        {p.name}
      </span>
    </button>
  );
}

// ── Main modal ────────────────────────────────────────────────────────────────
interface GoLiveModalProps {
  open: boolean;
  onClose: () => void;
  onGoLive: (config: { platform: string; title: string; key: string }) => void;
}

type ModalStep = "config" | "checking" | "countdown" | "live";

export default function GoLiveModal({ open, onClose, onGoLive }: GoLiveModalProps) {
  const [step, setStep] = useState<ModalStep>("config");
  const [platform, setPlatform] = useState("youtube");
  const [title, setTitle] = useState("");
  const [streamKey, setStreamKey] = useState("");
  const [category, setCategory] = useState("Sports");
  const [checks, setChecks] = useState<CheckItem[]>(buildChecks());
  const [countdown, setCountdown] = useState(3);
  const [allOk, setAllOk] = useState(false);
  const timerRef = useRef<ReturnType<typeof setTimeout> | null>(null);

  // Reset when modal opens
  useEffect(() => {
    if (open) {
      setStep("config");
      setChecks(buildChecks());
      setCountdown(3);
      setAllOk(false);
    }
    return () => { if (timerRef.current) clearTimeout(timerRef.current); };
  }, [open]);

  // Run animated checks
  function runChecks() {
    setStep("checking");
    const items = buildChecks();
    setChecks(items);

    // Simulate sequential checks with delays
    const delays = [400, 900, 1500, 2200, 2900];
    const results: CheckStatus[] = ["ok", "ok", "ok", "ok", streamKey.trim() ? "ok" : "warn"];
    const warnMsgs = [, , , , "Stream key not set — using saved credentials"];

    delays.forEach((delay, i) => {
      // Set to "checking"
      timerRef.current = setTimeout(() => {
        setChecks(prev => prev.map((c, idx) => idx === i ? { ...c, status: "checking" } : c));
      }, delay - 300);
      // Set to result
      timerRef.current = setTimeout(() => {
        setChecks(prev => prev.map((c, idx) =>
          idx === i ? { ...c, status: results[i], warnMsg: warnMsgs[i] } : c
        ));
        if (i === delays.length - 1) {
          const hasFailure = results.some(r => r === "fail");
          setAllOk(!hasFailure);
          if (!hasFailure) {
            timerRef.current = setTimeout(() => {
              setStep("countdown");
              setCountdown(3);
            }, 600);
          }
        }
      }, delay);
    });
  }

  // Countdown to live
  useEffect(() => {
    if (step !== "countdown") return;
    if (countdown <= 0) {
      setStep("live");
      timerRef.current = setTimeout(() => {
        onGoLive({ platform, title, key: streamKey });
        onClose();
      }, 800);
      return;
    }
    timerRef.current = setTimeout(() => setCountdown(c => c - 1), 1000);
  }, [step, countdown]);

  if (!open) return null;

  const selectedPlatform = PLATFORMS.find(p => p.id === platform)!;

  return (
    // Backdrop
    <div
      className="fixed inset-0 z-50 flex items-center justify-center"
      style={{ background: "rgba(0,0,0,0.72)", backdropFilter: "blur(6px)" }}
      onClick={e => { if (e.target === e.currentTarget && step === "config") onClose(); }}
    >
      {/* Modal panel */}
      <div
        className="relative flex flex-col"
        style={{
          width: 520,
          maxHeight: "90vh",
          background: "#141926",
          border: "1px solid #2A3350",
          borderRadius: 12,
          boxShadow: "0 32px 80px rgba(0,0,0,0.7), 0 0 0 1px rgba(255,90,44,0.12)",
          overflow: "hidden",
          animation: "modalIn 0.22s cubic-bezier(0.23,1,0.32,1)",
        }}
      >
        {/* Top accent bar */}
        <div style={{ height: 3, background: "linear-gradient(90deg, #FF5A2C 0%, #FF6B35 40%, #A855F7 100%)" }} />

        {/* ── CONFIG STEP ─────────────────────────────────────────────── */}
        {step === "config" && (
          <>
            {/* Header */}
            <div className="flex items-center justify-between px-6 py-4" style={{ borderBottom: "1px solid #2A3350" }}>
              <div>
                <div className="flex items-center gap-2">
                  <Radio size={16} style={{ color: "#FF5A2C" }} />
                  <span style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 20, color: "#F8F8FF", letterSpacing: "0.08em" }}>GO LIVE</span>
                </div>
                <p style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 12, color: "#8892A4", marginTop: 2 }}>
                  Configure your stream before going live
                </p>
              </div>
              <button onClick={onClose} className="flex items-center justify-center rounded transition-colors" style={{ width: 28, height: 28, background: "#1A2035", border: "1px solid #2A3350", color: "#8892A4" }}>
                <X size={14} />
              </button>
            </div>

            {/* Body */}
            <div className="flex flex-col gap-5 px-6 py-5 overflow-y-auto">

              {/* Platform selector */}
              <div>
                <label style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, fontWeight: 600, color: "#8892A4", letterSpacing: "0.1em", textTransform: "uppercase" }}>
                  Platform
                </label>
                <div className="flex gap-2 mt-2">
                  {PLATFORMS.map(p => (
                    <PlatformBtn key={p.id} p={p} selected={platform === p.id} onClick={() => setPlatform(p.id)} />
                  ))}
                </div>
              </div>

              {/* Stream title */}
              <div>
                <label style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, fontWeight: 600, color: "#8892A4", letterSpacing: "0.1em", textTransform: "uppercase" }}>
                  Stream Title
                </label>
                <input
                  value={title}
                  onChange={e => setTitle(e.target.value)}
                  placeholder="e.g. Championship Finals — Live Coverage"
                  style={{
                    width: "100%", marginTop: 8, height: 38,
                    background: "#111827", border: "1px solid #2A3350",
                    borderRadius: 6, padding: "0 12px",
                    fontFamily: "'DM Sans', sans-serif", fontSize: 13, color: "#F8F8FF",
                    outline: "none", boxSizing: "border-box",
                  }}
                  onFocus={e => { e.target.style.borderColor = "#4F9EFF"; e.target.style.boxShadow = "0 0 0 2px rgba(79,158,255,0.15)"; }}
                  onBlur={e => { e.target.style.borderColor = "#2A3350"; e.target.style.boxShadow = "none"; }}
                />
              </div>

              {/* Category + Stream key row */}
              <div className="flex gap-3">
                <div style={{ flex: 1 }}>
                  <label style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, fontWeight: 600, color: "#8892A4", letterSpacing: "0.1em", textTransform: "uppercase" }}>
                    Category
                  </label>
                  <select
                    value={category}
                    onChange={e => setCategory(e.target.value)}
                    style={{
                      width: "100%", marginTop: 8, height: 38,
                      background: "#111827", border: "1px solid #2A3350",
                      borderRadius: 6, padding: "0 12px",
                      fontFamily: "'DM Sans', sans-serif", fontSize: 13, color: "#F8F8FF",
                      outline: "none", appearance: "none", cursor: "pointer",
                    }}
                  >
                    {["Sports", "Gaming", "Entertainment", "Music", "Education", "News", "Other"].map(c => (
                      <option key={c} value={c} style={{ background: "#111827" }}>{c}</option>
                    ))}
                  </select>
                </div>
                <div style={{ flex: 1 }}>
                  <label style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, fontWeight: 600, color: "#8892A4", letterSpacing: "0.1em", textTransform: "uppercase" }}>
                    Stream Key
                  </label>
                  <input
                    type="password"
                    value={streamKey}
                    onChange={e => setStreamKey(e.target.value)}
                    placeholder="••••••••••••••••"
                    style={{
                      width: "100%", marginTop: 8, height: 38,
                      background: "#111827", border: "1px solid #2A3350",
                      borderRadius: 6, padding: "0 12px",
                      fontFamily: "'JetBrains Mono', monospace", fontSize: 13, color: "#F8F8FF",
                      outline: "none", boxSizing: "border-box",
                    }}
                    onFocus={e => { e.target.style.borderColor = "#FF5A2C"; e.target.style.boxShadow = "0 0 0 2px rgba(255,90,44,0.12)"; }}
                    onBlur={e => { e.target.style.borderColor = "#2A3350"; e.target.style.boxShadow = "none"; }}
                  />
                </div>
              </div>

              {/* Quick-check summary row */}
              <div className="flex items-center gap-3 rounded-lg px-4 py-3" style={{ background: "#0F1520", border: "1px solid #2A3350" }}>
                {[
                  { label: "Scene", ok: true,  color: "#22C55E" },
                  { label: "Audio", ok: true,  color: "#22C55E" },
                  { label: "Encoder", ok: true, color: "#22C55E" },
                  { label: "Network", ok: true, color: "#22C55E" },
                  { label: "Key", ok: !!streamKey.trim(), color: streamKey.trim() ? "#22C55E" : "#FBBF24" },
                ].map(({ label, ok, color }) => (
                  <div key={label} className="flex items-center gap-1.5">
                    <div style={{ width: 7, height: 7, borderRadius: "50%", background: color, boxShadow: `0 0 6px ${color}80` }} />
                    <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, color: ok ? "#A0A0B8" : "#FBBF24" }}>{label}</span>
                  </div>
                ))}
                <div className="ml-auto flex items-center gap-1">
                  <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, color: "#22C55E" }}>Ready</span>
                  <CheckCircle2 size={13} style={{ color: "#22C55E" }} />
                </div>
              </div>
            </div>

            {/* Footer */}
            <div className="flex items-center justify-between px-6 py-4" style={{ borderTop: "1px solid #2A3350" }}>
              <button onClick={onClose} style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 13, color: "#8892A4", background: "none", border: "none", cursor: "pointer" }}>
                Cancel
              </button>
              <button
                onClick={runChecks}
                className="flex items-center gap-2 rounded-lg font-bold transition-all duration-150"
                style={{
                  height: 42, paddingLeft: 24, paddingRight: 24,
                  background: "linear-gradient(135deg, #FF5A2C 0%, #FF6B35 100%)",
                  boxShadow: "0 0 24px rgba(255,90,44,0.4)",
                  color: "#fff", fontFamily: "'DM Sans', sans-serif", fontSize: 14,
                  fontWeight: 700, letterSpacing: "0.06em", border: "none", cursor: "pointer",
                }}
                onMouseDown={e => { (e.currentTarget as HTMLButtonElement).style.transform = "scale(0.97)"; }}
                onMouseUp={e => { (e.currentTarget as HTMLButtonElement).style.transform = "scale(1)"; }}
              >
                <ChevronRight size={16} />
                RUN CHECKS &amp; GO LIVE
              </button>
            </div>
          </>
        )}

        {/* ── CHECKING STEP ───────────────────────────────────────────── */}
        {step === "checking" && (
          <>
            <div className="flex items-center gap-3 px-6 py-4" style={{ borderBottom: "1px solid #2A3350" }}>
              <Loader2 size={16} className="animate-spin" style={{ color: "#4F9EFF" }} />
              <span style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 20, color: "#F8F8FF", letterSpacing: "0.08em" }}>RUNNING CHECKS</span>
            </div>
            <div className="flex flex-col gap-0 px-6 py-4">
              {checks.map((c, i) => {
                const Icon = c.icon;
                return (
                  <div
                    key={c.id}
                    className="flex items-center gap-4 py-3"
                    style={{
                      borderBottom: i < checks.length - 1 ? "1px solid #1E2840" : "none",
                      opacity: c.status === "pending" ? 0.4 : 1,
                      transition: "opacity 0.3s ease",
                    }}
                  >
                    <div className="flex items-center justify-center rounded" style={{ width: 32, height: 32, background: `${c.color}15`, border: `1px solid ${c.color}30`, flexShrink: 0 }}>
                      <Icon size={15} style={{ color: c.color }} />
                    </div>
                    <div className="flex flex-col flex-1">
                      <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 13, fontWeight: 600, color: "#F8F8FF" }}>{c.label}</span>
                      <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, color: c.status === "warn" ? "#FBBF24" : "#8892A4" }}>
                        {c.warnMsg ?? c.detail}
                      </span>
                    </div>
                    <StatusIcon status={c.status} color={c.color} />
                  </div>
                );
              })}
            </div>
            {!allOk && checks.every(c => c.status !== "pending" && c.status !== "checking") && (
              <div className="px-6 pb-4">
                <div className="rounded-lg px-4 py-3 flex items-center gap-3" style={{ background: "#1A1A24", border: "1px solid #EF444440" }}>
                  <XCircle size={16} style={{ color: "#EF4444" }} />
                  <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 13, color: "#EF4444" }}>One or more checks failed. Fix issues and try again.</span>
                </div>
                <button onClick={() => setStep("config")} className="mt-3 w-full rounded-lg" style={{ height: 38, background: "#1A2035", border: "1px solid #2A3350", color: "#8892A4", fontFamily: "'DM Sans', sans-serif", fontSize: 13, cursor: "pointer" }}>
                  ← Back to Configuration
                </button>
              </div>
            )}
          </>
        )}

        {/* ── COUNTDOWN STEP ──────────────────────────────────────────── */}
        {step === "countdown" && (
          <div className="flex flex-col items-center justify-center py-14 gap-6">
            <div className="flex items-center gap-2">
              <CheckCircle2 size={20} style={{ color: "#22C55E" }} />
              <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 14, color: "#22C55E", fontWeight: 600 }}>All checks passed</span>
            </div>
            <div
              style={{
                width: 120, height: 120, borderRadius: "50%",
                background: "radial-gradient(circle, #FF5A2C18 0%, transparent 70%)",
                border: "2px solid #FF5A2C40",
                display: "flex", alignItems: "center", justifyContent: "center",
                boxShadow: "0 0 60px rgba(255,90,44,0.25)",
                animation: "pulse 1s ease-in-out infinite",
              }}
            >
              <span style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 72, color: "#FF5A2C", lineHeight: 1 }}>
                {countdown}
              </span>
            </div>
            <div className="flex flex-col items-center gap-1">
              <span style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 22, color: "#F8F8FF", letterSpacing: "0.1em" }}>GOING LIVE IN</span>
              <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 13, color: "#8892A4" }}>
                Streaming to {selectedPlatform.name}{title ? ` · "${title}"` : ""}
              </span>
            </div>
          </div>
        )}

        {/* ── LIVE STEP ───────────────────────────────────────────────── */}
        {step === "live" && (
          <div className="flex flex-col items-center justify-center py-14 gap-4">
            <div
              style={{
                width: 80, height: 80, borderRadius: "50%",
                background: "radial-gradient(circle, #FF5A2C30 0%, transparent 70%)",
                border: "2px solid #FF5A2C",
                display: "flex", alignItems: "center", justifyContent: "center",
                boxShadow: "0 0 40px rgba(255,90,44,0.5)",
                animation: "livePulse 0.8s ease-in-out infinite",
              }}
            >
              <Radio size={32} style={{ color: "#FF5A2C" }} />
            </div>
            <span style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 36, color: "#FF5A2C", letterSpacing: "0.12em" }}>YOU ARE LIVE</span>
            <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 14, color: "#8892A4" }}>Stream started successfully</span>
          </div>
        )}
      </div>

      {/* Keyframe animations */}
      <style>{`
        @keyframes modalIn {
          from { opacity: 0; transform: scale(0.95) translateY(8px); }
          to   { opacity: 1; transform: scale(1)    translateY(0);   }
        }
        @keyframes pulse {
          0%, 100% { box-shadow: 0 0 40px rgba(255,90,44,0.2); }
          50%       { box-shadow: 0 0 80px rgba(255,90,44,0.45); }
        }
        @keyframes livePulse {
          0%, 100% { box-shadow: 0 0 30px rgba(255,90,44,0.4); transform: scale(1); }
          50%       { box-shadow: 0 0 60px rgba(255,90,44,0.7); transform: scale(1.05); }
        }
      `}</style>
    </div>
  );
}
