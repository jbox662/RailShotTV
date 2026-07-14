/**
 * Nexus Broadcast — Stream Settings (Screen 2)
 * Obsidian Studio Dark Theme
 * Vertical tab nav + form panels
 */
import { useState } from "react";
import AppSidebar from "@/components/AppSidebar";
import { Settings as SettingsIcon, Radio, Monitor, Volume2, Keyboard, Sliders, LogOut, Plug } from "lucide-react";

const tabs = [
  { id: "general", label: "General", icon: SettingsIcon },
  { id: "stream", label: "Stream", icon: Radio },
  { id: "output", label: "Output", icon: Monitor },
  { id: "video", label: "Video", icon: Monitor },
  { id: "audio", label: "Audio", icon: Volume2 },
  { id: "hotkeys", label: "Hotkeys", icon: Keyboard },
  { id: "advanced", label: "Advanced", icon: Sliders },
  { id: "plugins", label: "Plugins", icon: Plug },
];

const platforms = [
  { id: "twitch", label: "Twitch", color: "#9146FF" },
  { id: "youtube", label: "YouTube", color: "#FF0000" },
  { id: "facebook", label: "Facebook", color: "#1877F2" },
  { id: "custom", label: "Custom RTMP", color: "#6B7280" },
];

function FormRow({ label, children }: { label: string; children: React.ReactNode }) {
  return (
    <div className="flex items-center gap-4 py-3" style={{ borderBottom: "1px solid rgba(255,255,255,0.05)" }}>
      <label style={{ width: 160, fontSize: 12, color: "rgba(255,255,255,0.6)", fontWeight: 500, flexShrink: 0 }}>{label}</label>
      <div className="flex-1">{children}</div>
    </div>
  );
}

function StyledInput({ value, type = "text", placeholder }: { value?: string; type?: string; placeholder?: string }) {
  return (
    <input
      defaultValue={value}
      type={type}
      placeholder={placeholder}
      className="w-full rounded px-3 py-1.5 outline-none transition-all"
      style={{
        background: "#0E0F14", border: "1px solid rgba(255,255,255,0.12)",
        color: "#fff", fontSize: 12, fontFamily: "'Inter', sans-serif",
      }}
    />
  );
}

function StyledSelect({ options, value }: { options: string[]; value: string }) {
  return (
    <select
      defaultValue={value}
      className="rounded px-3 py-1.5 outline-none"
      style={{
        background: "#0E0F14", border: "1px solid rgba(255,255,255,0.12)",
        color: "#fff", fontSize: 12, fontFamily: "'Inter', sans-serif", minWidth: 200,
      }}
    >
      {options.map(o => <option key={o}>{o}</option>)}
    </select>
  );
}

function Toggle({ defaultChecked = false }: { defaultChecked?: boolean }) {
  const [on, setOn] = useState(defaultChecked);
  return (
    <button
      onClick={() => setOn(v => !v)}
      className="rounded-full transition-colors duration-200"
      style={{ width: 36, height: 20, background: on ? "#3B82F6" : "rgba(255,255,255,0.15)", position: "relative", flexShrink: 0 }}
    >
      <div style={{
        position: "absolute", top: 2, left: on ? 18 : 2, width: 16, height: 16,
        background: "#fff", borderRadius: "50%", transition: "left 0.2s ease",
      }} />
    </button>
  );
}

function StreamTab() {
  const [platform, setPlatform] = useState("youtube");
  const [showKey, setShowKey] = useState(false);
  const [encoder, setEncoder] = useState("nvenc");

  return (
    <div className="flex flex-col gap-5">
      {/* Service */}
      <div>
        <div style={{ fontSize: 11, fontWeight: 600, color: "rgba(255,255,255,0.4)", letterSpacing: "0.1em", marginBottom: 10 }}>SERVICE</div>
        <div className="flex gap-2">
          {platforms.map(p => (
            <button
              key={p.id}
              onClick={() => setPlatform(p.id)}
              className="rounded px-4 py-2 transition-all duration-150"
              style={{
                background: platform === p.id ? `${p.color}22` : "#0E0F14",
                border: `1px solid ${platform === p.id ? p.color : "rgba(255,255,255,0.1)"}`,
                color: platform === p.id ? p.color : "rgba(255,255,255,0.5)",
                fontSize: 12, fontWeight: 600,
              }}
            >
              {p.label}
            </button>
          ))}
        </div>
      </div>

      {/* Stream Key */}
      <FormRow label="Stream Key">
        <div className="flex gap-2">
          <input
            type={showKey ? "text" : "password"}
            defaultValue="abcd-efgh-ijkl-mnop-qrst"
            className="flex-1 rounded px-3 py-1.5 outline-none"
            style={{ background: "#0E0F14", border: "1px solid rgba(255,255,255,0.12)", color: "#fff", fontSize: 12, fontFamily: "'JetBrains Mono', monospace" }}
          />
          <button onClick={() => setShowKey(v => !v)} className="rounded px-3 py-1.5 text-xs" style={{ background: "rgba(255,255,255,0.06)", color: "rgba(255,255,255,0.6)", border: "1px solid rgba(255,255,255,0.1)" }}>
            {showKey ? "Hide" : "Show"}
          </button>
          <button className="rounded px-3 py-1.5 text-xs" style={{ background: "rgba(59,130,246,0.15)", color: "#3B82F6", border: "1px solid rgba(59,130,246,0.3)", fontSize: 11 }}>
            Get Key ↗
          </button>
        </div>
      </FormRow>

      <FormRow label="Server">
        <StyledSelect options={["Auto-select (recommended)", "US East", "US West", "EU West", "Asia Pacific"]} value="Auto-select (recommended)" />
      </FormRow>

      {/* Encoder */}
      <div>
        <div style={{ fontSize: 11, fontWeight: 600, color: "rgba(255,255,255,0.4)", letterSpacing: "0.1em", marginBottom: 10 }}>ENCODER</div>
        <div className="flex gap-2 mb-4">
          {[{ id: "nvenc", label: "NVENC (NVIDIA)" }, { id: "qsv", label: "QSV (Intel)" }, { id: "x264", label: "x264 (Software)" }].map(e => (
            <button
              key={e.id}
              onClick={() => setEncoder(e.id)}
              className="rounded px-3 py-1.5 transition-all"
              style={{
                background: encoder === e.id ? "#3B82F6" : "#0E0F14",
                border: `1px solid ${encoder === e.id ? "#3B82F6" : "rgba(255,255,255,0.1)"}`,
                color: encoder === e.id ? "#fff" : "rgba(255,255,255,0.5)",
                fontSize: 12, fontWeight: encoder === e.id ? 600 : 400,
              }}
            >
              {e.label}
            </button>
          ))}
        </div>
        <div className="grid grid-cols-4 gap-3">
          {[
            { label: "Rate Control", value: "CBR", type: "select", options: ["CBR", "VBR", "CQP"] },
            { label: "Bitrate (Target)", value: "6000", suffix: "kbps" },
            { label: "Bitrate (Min)", value: "4500", suffix: "kbps" },
            { label: "Keyframe Interval", value: "2", suffix: "s" },
          ].map(f => (
            <div key={f.label}>
              <div style={{ fontSize: 10, color: "rgba(255,255,255,0.4)", marginBottom: 4 }}>{f.label}</div>
              <div className="flex items-center gap-1">
                <input
                  defaultValue={f.value}
                  className="rounded px-2 py-1 outline-none w-full"
                  style={{ background: "#0E0F14", border: "1px solid rgba(255,255,255,0.12)", color: "#fff", fontSize: 12, fontFamily: "'JetBrains Mono', monospace" }}
                />
                {f.suffix && <span style={{ fontSize: 10, color: "rgba(255,255,255,0.4)", whiteSpace: "nowrap" }}>{f.suffix}</span>}
              </div>
            </div>
          ))}
        </div>
      </div>

      {/* Output */}
      <div>
        <div style={{ fontSize: 11, fontWeight: 600, color: "rgba(255,255,255,0.4)", letterSpacing: "0.1em", marginBottom: 10 }}>OUTPUT</div>
        <FormRow label="Recording Path">
          <div className="flex gap-2">
            <input defaultValue="D:\StreamPro\Recordings" className="flex-1 rounded px-3 py-1.5 outline-none" style={{ background: "#0E0F14", border: "1px solid rgba(255,255,255,0.12)", color: "#fff", fontSize: 12, fontFamily: "'JetBrains Mono', monospace" }} />
            <button className="rounded px-3 py-1.5 text-xs" style={{ background: "rgba(255,255,255,0.06)", color: "rgba(255,255,255,0.6)", border: "1px solid rgba(255,255,255,0.1)" }}>Browse</button>
          </div>
        </FormRow>
        <FormRow label="Recording Format">
          <div className="flex gap-2">
            {["MKV", "MP4", "MOV"].map(f => (
              <button key={f} className="rounded px-3 py-1.5" style={{ background: f === "MKV" ? "#3B82F6" : "#0E0F14", border: `1px solid ${f === "MKV" ? "#3B82F6" : "rgba(255,255,255,0.1)"}`, color: f === "MKV" ? "#fff" : "rgba(255,255,255,0.5)", fontSize: 12 }}>{f}</button>
            ))}
          </div>
        </FormRow>
        <FormRow label="Replay Buffer">
          <div className="flex items-center gap-3">
            <Toggle defaultChecked={true} />
            <span style={{ fontSize: 11, color: "rgba(255,255,255,0.5)" }}>60 seconds</span>
          </div>
        </FormRow>
      </div>

      {/* Video */}
      <div>
        <div style={{ fontSize: 11, fontWeight: 600, color: "rgba(255,255,255,0.4)", letterSpacing: "0.1em", marginBottom: 10 }}>VIDEO</div>
        <div className="grid grid-cols-3 gap-3">
          <div>
            <div style={{ fontSize: 10, color: "rgba(255,255,255,0.4)", marginBottom: 4 }}>Canvas Resolution</div>
            <StyledSelect options={["1920×1080 (16:9)", "2560×1440 (16:9)", "3840×2160 (16:9)"]} value="1920×1080 (16:9)" />
          </div>
          <div>
            <div style={{ fontSize: 10, color: "rgba(255,255,255,0.4)", marginBottom: 4 }}>Output Resolution</div>
            <StyledSelect options={["1920×1080 (16:9)", "1280×720 (16:9)"]} value="1920×1080 (16:9)" />
          </div>
          <div>
            <div style={{ fontSize: 10, color: "rgba(255,255,255,0.4)", marginBottom: 4 }}>FPS</div>
            <div className="flex gap-2">
              {["30", "60", "120"].map(f => (
                <button key={f} className="rounded px-3 py-1.5" style={{ background: f === "60" ? "#3B82F6" : "#0E0F14", border: `1px solid ${f === "60" ? "#3B82F6" : "rgba(255,255,255,0.1)"}`, color: f === "60" ? "#fff" : "rgba(255,255,255,0.5)", fontSize: 12, fontWeight: f === "60" ? 600 : 400 }}>{f}</button>
              ))}
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}

export default function Settings() {
  const [activeTab, setActiveTab] = useState("stream");

  return (
    <AppSidebar>
      <div className="flex flex-col h-full" style={{ fontFamily: "'Inter', sans-serif" }}>
        {/* Top bar */}
        <div className="flex items-center px-4 border-b shrink-0" style={{ borderColor: "rgba(255,255,255,0.07)", minHeight: 46, background: "#0D0E12" }}>
          <span style={{ fontFamily: "'Space Grotesk', sans-serif", fontWeight: 700, fontSize: 12, color: "#fff", letterSpacing: "0.1em" }}>SETTINGS</span>
          <div className="mx-3 w-px h-4" style={{ background: "rgba(255,255,255,0.1)" }} />
          <span style={{ fontSize: 10, color: "rgba(255,255,255,0.3)", fontFamily: "'JetBrains Mono', monospace" }}>nexus-broadcast / stream-config</span>
          <div className="ml-auto flex items-center gap-3">
            <div className="flex items-center gap-1.5">
              <div className="w-1.5 h-1.5 rounded-full animate-pulse" style={{ background: "#EF4444" }} />
              <span style={{ fontSize: 9, color: "#EF4444", fontFamily: "'JetBrains Mono', monospace", fontWeight: 700, letterSpacing: "0.08em" }}>LIVE</span>
            </div>
            <div className="w-px h-4" style={{ background: "rgba(255,255,255,0.08)" }} />
            <span style={{ fontSize: 9, color: "rgba(255,255,255,0.3)", fontFamily: "'JetBrains Mono', monospace" }}>Changes apply on next stream</span>
          </div>
        </div>

        <div className="flex flex-1 overflow-hidden">
          {/* Tab nav */}
          <div className="flex flex-col py-2 shrink-0 overflow-y-auto" style={{ width: 180, borderRight: "1px solid rgba(255,255,255,0.07)", background: "#111318" }}>
            {tabs.map(tab => (
              <button
                key={tab.id}
                onClick={() => setActiveTab(tab.id)}
                className="flex items-center gap-2.5 px-4 py-2.5 text-left transition-all duration-150"
                style={{
                  background: activeTab === tab.id ? "#1A1D2B" : "transparent",
                  borderLeft: `3px solid ${activeTab === tab.id ? "#3B82F6" : "transparent"}`,
                  paddingLeft: activeTab === tab.id ? 13 : 16,
                  color: activeTab === tab.id ? "#fff" : "rgba(255,255,255,0.45)",
                  fontSize: 13, fontWeight: activeTab === tab.id ? 500 : 400,
                }}
              >
                <tab.icon size={14} />
                {tab.label}
              </button>
            ))}
          </div>

          {/* Content */}
          <div className="flex-1 overflow-y-auto p-6">
            {activeTab === "stream" ? <StreamTab /> : (
              <div className="flex flex-col items-center justify-center h-full gap-3" style={{ color: "rgba(255,255,255,0.3)" }}>
                <SettingsIcon size={32} strokeWidth={1} />
                <span style={{ fontSize: 13 }}>Select a settings category from the left panel</span>
              </div>
            )}
          </div>
        </div>

        {/* Footer */}
        <div className="flex items-center justify-end gap-3 px-6 py-3 border-t shrink-0" style={{ borderColor: "rgba(255,255,255,0.07)" }}>
          <button className="rounded px-4 py-1.5 text-sm" style={{ background: "transparent", border: "1px solid rgba(255,255,255,0.15)", color: "rgba(255,255,255,0.6)" }}>Cancel</button>
          <button className="rounded px-5 py-1.5 text-sm font-semibold" style={{ background: "#3B82F6", color: "#fff", boxShadow: "0 2px 12px rgba(59,130,246,0.3)" }}>Save Settings</button>
        </div>
      </div>
    </AppSidebar>
  );
}
