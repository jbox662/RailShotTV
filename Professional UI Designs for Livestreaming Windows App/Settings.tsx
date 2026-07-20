// RailShotTV — Chromatic Command — Settings
import { useState, useCallback } from "react";
import { toast } from "sonner";
import AppSidebar from "@/components/AppSidebar";
import { Settings as SettingsIcon, Radio, Video, Volume2, Keyboard, Sliders, Puzzle, Save, X, FolderOpen, Plus, Trash2, RefreshCw } from "lucide-react";

// ─── Shared sub-components ────────────────────────────────────────────────────
function Row({ label, children, hint }: { label: string; children: React.ReactNode; hint?: string }) {
  return (
    <div className="flex items-center justify-between py-2.5" style={{ borderBottom: "1px solid #1A1A24" }}>
      <div>
        <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 13, color: "#A0A0B8" }}>{label}</span>
        {hint && <div style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 10, color: "#50506A", marginTop: 1 }}>{hint}</div>}
      </div>
      <div className="flex items-center gap-2">{children}</div>
    </div>
  );
}
function SectionHeader({ title }: { title: string }) {
  return <h3 className="mb-3 mt-1" style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, fontWeight: 700, color: "#8892A4", letterSpacing: "0.1em", textTransform: "uppercase" }}>{title}</h3>;
}
function Card({ children }: { children: React.ReactNode }) {
  return <div className="rounded p-4 mb-6" style={{ background: "#1E2640", border: "1px solid #2A3350" }}>{children}</div>;
}
function Toggle({ on, onChange }: { on: boolean; onChange: (v: boolean) => void }) {
  return (
    <div className="relative cursor-pointer" style={{ width: 36, height: 20 }} onClick={() => onChange(!on)}>
      <div className="w-full h-full rounded-full" style={{ background: on ? "#FF5A2C" : "#1A1A24", border: `1px solid ${on ? "#FF5A2C" : "#303D5A"}`, transition: "background 0.2s" }} />
      <div className="absolute top-0.5 rounded-full bg-white" style={{ width: 16, height: 16, left: on ? 18 : 2, boxShadow: "0 1px 3px rgba(0,0,0,0.4)", transition: "left 0.15s" }} />
    </div>
  );
}
function Seg({ options, active, onChange, color = "#FF5A2C" }: { options: string[]; active: string; onChange: (v: string) => void; color?: string }) {
  return (
    <div className="flex rounded overflow-hidden" style={{ border: "1px solid #303D5A" }}>
      {options.map(o => (
        <button key={o} onClick={() => onChange(o)} className="px-3 py-1 transition-all" style={{ background: o === active ? `${color}18` : "#1E2640", color: o === active ? color : "#606078", fontFamily: "'DM Sans', sans-serif", fontSize: 11, fontWeight: o === active ? 600 : 400, borderRight: "1px solid #303D5A", cursor: "pointer" }}>{o}</button>
      ))}
    </div>
  );
}
function TxtInput({ value, onChange, width = 160, mono = false, type = "text", placeholder = "" }: { value: string; onChange: (v: string) => void; width?: number; mono?: boolean; type?: string; placeholder?: string }) {
  return <input type={type} value={value} onChange={e => onChange(e.target.value)} placeholder={placeholder} style={{ height: 30, background: "#1E2640", border: "1px solid #303D5A", borderRadius: 4, minWidth: width, color: "#A0A0B8", fontFamily: mono ? "'JetBrains Mono', monospace" : "'DM Sans', sans-serif", fontSize: 12, padding: "0 10px", outline: "none" }} />;
}
function NumInput({ value, onChange, min, max, step = 1, width = 80, suffix }: { value: number; onChange: (v: number) => void; min?: number; max?: number; step?: number; width?: number; suffix?: string }) {
  return (
    <div className="flex items-center gap-2">
      <input type="number" value={value} min={min} max={max} step={step} onChange={e => onChange(Number(e.target.value))} style={{ height: 30, background: "#1E2640", border: "1px solid #303D5A", borderRadius: 4, width, color: "#A0A0B8", fontFamily: "'JetBrains Mono', monospace", fontSize: 12, padding: "0 8px", outline: "none" }} />
      {suffix && <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, color: "#8892A4" }}>{suffix}</span>}
    </div>
  );
}
function Sel({ value, onChange, options }: { value: string; onChange: (v: string) => void; options: string[] }) {
  return <select value={value} onChange={e => onChange(e.target.value)} style={{ height: 30, background: "#1E2640", border: "1px solid #303D5A", borderRadius: 4, color: "#A0A0B8", fontFamily: "'DM Sans', sans-serif", fontSize: 12, padding: "0 8px", outline: "none" }}>{options.map(o => <option key={o} value={o}>{o}</option>)}</select>;
}

// ─── Default settings ─────────────────────────────────────────────────────────
const DEFAULTS = {
  theme: "dark", language: "English", autoStart: false, minimizeToTray: true, checkUpdates: true, hardwareAccel: true,
  platform: "YouTube", streamKey: "", server: "auto",
  streamEncoder: "NVENC", rateControl: "CBR", bitrate: 6000, minBitrate: 4500, keyframeInterval: 2,
  recordingPath: "C:\\Users\\User\\Videos\\RailShotTV", recordingFormat: "MKV", replayBuffer: true, replayBufferSeconds: 60,
  canvasRes: "1920x1080", outputRes: "1920x1080", fps: "60",
  sampleRate: "48000", channels: "stereo", desktopAudio: true, micAudio: true, desktopDevice: "Default", micDevice: "Default",
  hotkeyStartStop: "Ctrl+Alt+S", hotkeyRecord: "Ctrl+Alt+R", hotkeyScene1: "Ctrl+1", hotkeyScene2: "Ctrl+2", hotkeyScene3: "Ctrl+3", hotkeySwitchCam: "Ctrl+Alt+C",
  processPriority: "Normal", bindIP: "0.0.0.0", networkOptimize: true, lowLatency: false,
  plugins: [
    { id: "obs-ndi", name: "obs-ndi", version: "5.1.0", enabled: true },
    { id: "obs-websocket", name: "obs-websocket", version: "5.4.2", enabled: true },
    { id: "source-clone", name: "source-clone", version: "0.1.5", enabled: false },
  ] as { id: string; name: string; version: string; enabled: boolean }[],
};
type S = typeof DEFAULTS;

function loadSettings(): Partial<S> {
  try { const s = localStorage.getItem("railshot_settings"); if (s) return JSON.parse(s); } catch { /* ignore */ }
  return {};
}

// ─── Tab panels ───────────────────────────────────────────────────────────────
function GeneralTab({ s, set }: { s: S; set: (k: string, v: unknown) => void }) {
  return <>
    <SectionHeader title="Appearance" />
    <Card>
      <Row label="Theme"><Seg options={["dark","light","system"]} active={s.theme} onChange={v => set("theme", v)} /></Row>
      <Row label="Language"><Sel value={s.language} onChange={v => set("language", v)} options={["English","Español","Français","Deutsch","Português"]} /></Row>
    </Card>
    <SectionHeader title="Startup" />
    <Card>
      <Row label="Launch on system startup"><Toggle on={s.autoStart} onChange={v => set("autoStart", v)} /></Row>
      <Row label="Minimize to tray on close"><Toggle on={s.minimizeToTray} onChange={v => set("minimizeToTray", v)} /></Row>
      <Row label="Check for updates automatically"><Toggle on={s.checkUpdates} onChange={v => set("checkUpdates", v)} /></Row>
      <Row label="Hardware acceleration (GPU rendering)"><Toggle on={s.hardwareAccel} onChange={v => set("hardwareAccel", v)} /></Row>
    </Card>
  </>;
}

function StreamTab({ s, set }: { s: S; set: (k: string, v: unknown) => void }) {
  return <>
    <SectionHeader title="Stream Destination" />
    <Card>
      <Row label="Platform">
        <Seg options={["YouTube","Twitch","Facebook","Custom RTMP"]} active={s.platform} onChange={v => set("platform", v)} />
      </Row>
      <Row label="Stream Key" hint="Never share your stream key publicly">
        <TxtInput value={s.streamKey} onChange={v => set("streamKey", v)} type="password" width={220} placeholder="Paste your stream key here" mono />
      </Row>
      {s.platform === "Custom RTMP" && (
        <Row label="RTMP Server URL">
          <TxtInput value={s.server === "auto" ? "" : s.server} onChange={v => set("server", v)} width={220} placeholder="rtmp://your.server/live" mono />
        </Row>
      )}
    </Card>
    <SectionHeader title="Encoder" />
    <Card>
      <Row label="Video Encoder"><Seg options={["NVENC","QSV","AMF","x264"]} active={s.streamEncoder} onChange={v => set("streamEncoder", v)} color="#4F9EFF" /></Row>
      <Row label="Rate Control"><Seg options={["CBR","VBR","CQP"]} active={s.rateControl} onChange={v => set("rateControl", v)} color="#4F9EFF" /></Row>
      <Row label="Target Bitrate"><NumInput value={s.bitrate} onChange={v => set("bitrate", v)} min={500} max={51000} step={500} suffix="kbps" /></Row>
      <Row label="Min Bitrate"><NumInput value={s.minBitrate} onChange={v => set("minBitrate", v)} min={500} max={51000} step={500} suffix="kbps" /></Row>
      <Row label="Keyframe Interval"><NumInput value={s.keyframeInterval} onChange={v => set("keyframeInterval", v)} min={0} max={10} step={1} suffix="seconds" /></Row>
    </Card>
  </>;
}

function OutputTab({ s, set }: { s: S; set: (k: string, v: unknown) => void }) {
  return <>
    <SectionHeader title="Recording Output" />
    <Card>
      <Row label="Recording Path">
        <div className="flex items-center gap-2">
          <TxtInput value={s.recordingPath} onChange={v => set("recordingPath", v)} width={220} mono />
          <button onClick={() => toast.info("File browser requires the desktop app")} style={{ height: 30, padding: "0 10px", background: "#1A1A24", border: "1px solid #303D5A", borderRadius: 4, color: "#A0A0B8", fontFamily: "'DM Sans', sans-serif", fontSize: 11, cursor: "pointer", display: "flex", alignItems: "center", gap: 4 }}><FolderOpen size={11} /> Browse</button>
        </div>
      </Row>
      <Row label="Container Format"><Seg options={["MKV","MP4","MOV","FLV"]} active={s.recordingFormat} onChange={v => set("recordingFormat", v)} color="#FF5A2C" /></Row>
      <Row label="Replay Buffer">
        <div className="flex items-center gap-3">
          <Toggle on={s.replayBuffer} onChange={v => set("replayBuffer", v)} />
          {s.replayBuffer && <NumInput value={s.replayBufferSeconds} onChange={v => set("replayBufferSeconds", v)} min={10} max={300} step={10} suffix="seconds" />}
        </div>
      </Row>
    </Card>
  </>;
}

function VideoTab({ s, set }: { s: S; set: (k: string, v: unknown) => void }) {
  const RES = ["1920x1080","1280x720","3840x2160","1366x768"];
  return <>
    <SectionHeader title="Video" />
    <Card>
      <Row label="Canvas Resolution" hint="Base resolution for the scene canvas"><Sel value={s.canvasRes} onChange={v => set("canvasRes", v)} options={RES} /></Row>
      <Row label="Output Resolution" hint="Scaled resolution sent to stream/recording"><Sel value={s.outputRes} onChange={v => set("outputRes", v)} options={RES} /></Row>
      <Row label="Frame Rate"><Seg options={["24","30","60","120"]} active={s.fps} onChange={v => set("fps", v)} color="#22C55E" /></Row>
    </Card>
  </>;
}

function AudioTab({ s, set }: { s: S; set: (k: string, v: unknown) => void }) {
  const DESK = ["Default","Speakers (Realtek)","Headphones","HDMI Audio","Virtual Cable"];
  const MICS = ["Default","Microphone (Realtek)","USB Microphone","Headset Mic","Virtual Cable"];
  return <>
    <SectionHeader title="Audio Settings" />
    <Card>
      <Row label="Sample Rate"><Seg options={["44100","48000"]} active={s.sampleRate} onChange={v => set("sampleRate", v)} color="#A855F7" /></Row>
      <Row label="Channels"><Seg options={["mono","stereo","5.1","7.1"]} active={s.channels} onChange={v => set("channels", v)} color="#A855F7" /></Row>
    </Card>
    <SectionHeader title="Desktop Audio" />
    <Card>
      <Row label="Enable Desktop Audio"><Toggle on={s.desktopAudio} onChange={v => set("desktopAudio", v)} /></Row>
      {s.desktopAudio && <Row label="Desktop Audio Device"><Sel value={s.desktopDevice} onChange={v => set("desktopDevice", v)} options={DESK} /></Row>}
    </Card>
    <SectionHeader title="Microphone / Aux" />
    <Card>
      <Row label="Enable Microphone"><Toggle on={s.micAudio} onChange={v => set("micAudio", v)} /></Row>
      {s.micAudio && <Row label="Microphone Device"><Sel value={s.micDevice} onChange={v => set("micDevice", v)} options={MICS} /></Row>}
    </Card>
  </>;
}

function HotkeysTab({ s, set }: { s: S; set: (k: string, v: unknown) => void }) {
  const KEYS: { key: keyof S; label: string }[] = [
    { key: "hotkeyStartStop", label: "Start / Stop Stream" },
    { key: "hotkeyRecord", label: "Start / Stop Recording" },
    { key: "hotkeyScene1", label: "Switch to Scene 1" },
    { key: "hotkeyScene2", label: "Switch to Scene 2" },
    { key: "hotkeyScene3", label: "Switch to Scene 3" },
    { key: "hotkeySwitchCam", label: "Switch Active Camera" },
  ];
  return <>
    <SectionHeader title="Keyboard Shortcuts" />
    <Card>
      {KEYS.map(({ key, label }) => (
        <Row key={key} label={label}>
          <input value={s[key] as string} onChange={e => set(key, e.target.value)} placeholder="e.g. Ctrl+Alt+S" style={{ height: 30, width: 160, background: "#1E2640", border: "1px solid #303D5A", borderRadius: 4, color: "#A0A0B8", fontFamily: "'JetBrains Mono', monospace", fontSize: 12, padding: "0 8px", outline: "none" }} />
        </Row>
      ))}
    </Card>
    <p style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, color: "#50506A" }}>Type your desired key combination directly into the field. Global hotkeys require the desktop app.</p>
  </>;
}

function AdvancedTab({ s, set }: { s: S; set: (k: string, v: unknown) => void }) {
  return <>
    <SectionHeader title="Performance" />
    <Card>
      <Row label="Process Priority" hint="Higher priority may improve stream stability"><Seg options={["Low","Normal","High","Realtime"]} active={s.processPriority} onChange={v => set("processPriority", v)} color="#FF5A2C" /></Row>
      <Row label="Network Optimization" hint="Optimize send buffer for streaming"><Toggle on={s.networkOptimize} onChange={v => set("networkOptimize", v)} /></Row>
      <Row label="Low Latency Mode" hint="Reduces latency at the cost of higher CPU usage"><Toggle on={s.lowLatency} onChange={v => set("lowLatency", v)} /></Row>
    </Card>
    <SectionHeader title="Network" />
    <Card>
      <Row label="Bind to IP Address" hint="Use 0.0.0.0 to bind to all interfaces"><TxtInput value={s.bindIP} onChange={v => set("bindIP", v)} width={160} mono placeholder="0.0.0.0" /></Row>
    </Card>
  </>;
}

function PluginsTab({ s, set }: { s: S; set: (k: string, v: unknown) => void }) {
  const plugins = s.plugins;
  return <>
    <SectionHeader title="Installed Plugins" />
    <Card>
      {plugins.length === 0 && <div className="py-6 text-center" style={{ color: "#50506A", fontFamily: "'DM Sans', sans-serif", fontSize: 13 }}>No plugins installed</div>}
      {plugins.map(p => (
        <div key={p.id} className="flex items-center justify-between py-2.5" style={{ borderBottom: "1px solid #1A1A24" }}>
          <div>
            <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 13, color: "#A0A0B8", fontWeight: 600 }}>{p.name}</span>
            <span style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 10, color: "#50506A", marginLeft: 8 }}>v{p.version}</span>
          </div>
          <div className="flex items-center gap-3">
            <Toggle on={p.enabled} onChange={() => set("plugins", plugins.map(x => x.id === p.id ? { ...x, enabled: !x.enabled } : x))} />
            <button onClick={() => { set("plugins", plugins.filter(x => x.id !== p.id)); toast.success("Plugin removed"); }} style={{ background: "none", border: "none", cursor: "pointer", color: "#EF4444", display: "flex", alignItems: "center" }}><Trash2 size={13} /></button>
          </div>
        </div>
      ))}
    </Card>
    <button onClick={() => toast.info("Plugin installation requires the desktop app")} style={{ display: "flex", alignItems: "center", gap: 6, padding: "6px 14px", background: "#1E2640", border: "1px solid #303D5A", borderRadius: 4, color: "#A0A0B8", fontFamily: "'DM Sans', sans-serif", fontSize: 12, cursor: "pointer" }}><Plus size={13} /> Install Plugin</button>
  </>;
}

// ─── Main ─────────────────────────────────────────────────────────────────────
const SETTINGS_TABS = [
  { id: "general",  label: "General",  icon: SettingsIcon },
  { id: "stream",   label: "Stream",   icon: Radio },
  { id: "output",   label: "Output",   icon: Video },
  { id: "video",    label: "Video",    icon: Video },
  { id: "audio",    label: "Audio",    icon: Volume2 },
  { id: "hotkeys",  label: "Hotkeys",  icon: Keyboard },
  { id: "advanced", label: "Advanced", icon: Sliders },
  { id: "plugins",  label: "Plugins",  icon: Puzzle },
];

export default function Settings() {
  const [activeTab, setActiveTab] = useState("stream");
  const [settings, setSettings] = useState<S>(() => ({ ...DEFAULTS, ...loadSettings() }));
  const [dirty, setDirty] = useState(false);

  const set = useCallback((k: string, v: unknown) => {
    setSettings(prev => ({ ...prev, [k]: v }));
    setDirty(true);
  }, []);

  const handleSave = () => {
    localStorage.setItem("railshot_settings", JSON.stringify(settings));
    setDirty(false);
    toast.success("Settings saved");
  };

  const handleCancel = () => {
    setSettings({ ...DEFAULTS, ...loadSettings() });
    setDirty(false);
    toast.info("Changes discarded");
  };

  return (
    <AppSidebar>
      <div className="flex items-center gap-3 px-4 shrink-0" style={{ height: 46, background: "#1A2035", borderBottom: "1px solid #2A3350" }}>
        <div className="flex items-center gap-1 mr-1">
          <span style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 18, color: "#F8F8FF", letterSpacing: "0.06em", lineHeight: 1 }}>RAILSHOT</span>
          <span style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 18, color: "#FF5A2C", letterSpacing: "0.06em", lineHeight: 1 }}>TV</span>
        </div>
        <div className="w-px h-4 mx-1" style={{ background: "#303D5A" }} />
        <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 600, fontSize: 11, color: "#8892A4", letterSpacing: "0.1em", textTransform: "uppercase" }}>Settings</span>
        <span style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 10, color: "#50506A" }}>/ {activeTab}</span>
        <div className="flex-1" />
        {dirty && <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, color: "#FBBF24" }}>● Unsaved changes</span>}
        <button onClick={() => { setSettings({ ...DEFAULTS }); setDirty(true); toast.info("Reset to defaults"); }} style={{ display: "flex", alignItems: "center", gap: 4, padding: "4px 10px", background: "transparent", border: "1px solid #303D5A", borderRadius: 4, color: "#50506A", fontFamily: "'DM Sans', sans-serif", fontSize: 11, cursor: "pointer" }}><RefreshCw size={11} /> Reset</button>
      </div>

      <div className="flex flex-1 overflow-hidden">
        <div className="flex flex-col shrink-0 py-2" style={{ width: 168, background: "#1A2035", borderRight: "1px solid #2A3350" }}>
          {SETTINGS_TABS.map(({ id, label, icon: Icon }) => (
            <button key={id} onClick={() => setActiveTab(id)} className="flex items-center gap-2.5 px-3 py-2.5 text-left transition-all" style={{ background: activeTab === id ? "#FF5A2C0F" : "transparent", borderLeft: activeTab === id ? "2px solid #FF5A2C" : "2px solid transparent", cursor: "pointer" }}>
              <Icon size={14} style={{ color: activeTab === id ? "#FF5A2C" : "#50506A" }} />
              <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 13, fontWeight: activeTab === id ? 600 : 400, color: activeTab === id ? "#F8F8FF" : "#606078" }}>{label}</span>
            </button>
          ))}
        </div>
        <div className="flex-1 overflow-y-auto px-6 py-4" style={{ background: "#141928" }}>
          <div style={{ maxWidth: 640 }}>
            {activeTab === "general"  && <GeneralTab  s={settings} set={set} />}
            {activeTab === "stream"   && <StreamTab   s={settings} set={set} />}
            {activeTab === "output"   && <OutputTab   s={settings} set={set} />}
            {activeTab === "video"    && <VideoTab    s={settings} set={set} />}
            {activeTab === "audio"    && <AudioTab    s={settings} set={set} />}
            {activeTab === "hotkeys"  && <HotkeysTab  s={settings} set={set} />}
            {activeTab === "advanced" && <AdvancedTab s={settings} set={set} />}
            {activeTab === "plugins"  && <PluginsTab  s={settings} set={set} />}
          </div>
        </div>
      </div>

      <div className="flex items-center justify-end gap-2 px-6 py-3 shrink-0" style={{ background: "#1A2035", borderTop: "1px solid #2A3350" }}>
        <button onClick={handleCancel} style={{ display: "flex", alignItems: "center", gap: 6, padding: "6px 14px", background: "#1A1A24", border: "1px solid #303D5A", borderRadius: 4, color: "#8892A4", fontFamily: "'DM Sans', sans-serif", fontSize: 12, cursor: "pointer" }}><X size={13} /> Cancel</button>
        <button onClick={handleSave} style={{ display: "flex", alignItems: "center", gap: 6, padding: "6px 14px", background: dirty ? "linear-gradient(135deg, #FF5A2C, #FF6B35)" : "#1E2640", boxShadow: dirty ? "0 0 14px rgba(255,77,28,0.3)" : "none", border: dirty ? "none" : "1px solid #303D5A", borderRadius: 4, color: dirty ? "#fff" : "#50506A", fontFamily: "'DM Sans', sans-serif", fontSize: 12, fontWeight: 700, cursor: "pointer" }}><Save size={13} /> SAVE SETTINGS</button>
      </div>
    </AppSidebar>
  );
}
