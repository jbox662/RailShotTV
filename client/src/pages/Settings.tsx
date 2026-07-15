// RailShotTV — Chromatic Command — Settings
import { useState } from "react";
import AppSidebar from "@/components/AppSidebar";
import { Settings as SettingsIcon, Radio, Video, Volume2, Keyboard, Sliders, Puzzle, Save, X } from "lucide-react";

const SETTINGS_TABS = [
  { id: "general", label: "General", icon: SettingsIcon },
  { id: "stream", label: "Stream", icon: Radio },
  { id: "output", label: "Output", icon: Video },
  { id: "video", label: "Video", icon: Video },
  { id: "audio", label: "Audio", icon: Volume2 },
  { id: "hotkeys", label: "Hotkeys", icon: Keyboard },
  { id: "advanced", label: "Advanced", icon: Sliders },
  { id: "plugins", label: "Plugins", icon: Puzzle },
];

function Row({ label, children }: { label: string; children: React.ReactNode }) {
  return (
    <div className="flex items-center justify-between py-2.5" style={{ borderBottom: "1px solid #1A1A24" }}>
      <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 13, color: "#A0A0B8" }}>{label}</span>
      <div className="flex items-center gap-2">{children}</div>
    </div>
  );
}

function Toggle({ on }: { on: boolean }) {
  return (
    <div className="relative cursor-pointer" style={{ width: 36, height: 20 }}>
      <div className="w-full h-full rounded-full" style={{ background: on ? "#FF4D1C" : "#1A1A24", border: `1px solid ${on ? "#FF4D1C" : "#2A2A3A"}`, transition: "background 0.2s" }} />
      <div className="absolute top-0.5 rounded-full bg-white transition-all" style={{ width: 16, height: 16, left: on ? 18 : 2, boxShadow: "0 1px 3px rgba(0,0,0,0.4)" }} />
    </div>
  );
}

function SegmentedButtons({ options, active, color = "#FF4D1C" }: { options: string[]; active: string; color?: string }) {
  return (
    <div className="flex rounded overflow-hidden" style={{ border: "1px solid #2A2A3A" }}>
      {options.map(o => (
        <button key={o} className="px-3 py-1 text-xs font-medium transition-all" style={{ background: o === active ? `${color}18` : "#111118", color: o === active ? color : "#606078", fontFamily: "'DM Sans', sans-serif", fontSize: 11, borderRight: "1px solid #2A2A3A" }}>
          {o}
        </button>
      ))}
    </div>
  );
}

function Input({ value, width = 160 }: { value: string; width?: number }) {
  return (
    <div className="flex items-center rounded px-3" style={{ height: 30, background: "#111118", border: "1px solid #2A2A3A", minWidth: width }}>
      <span style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 12, color: "#A0A0B8" }}>{value}</span>
    </div>
  );
}

export default function Settings() {
  const [activeTab, setActiveTab] = useState("stream");

  return (
    <AppSidebar>
      {/* Top bar */}
      <div className="flex items-center gap-3 px-4 shrink-0" style={{ height: 46, background: "#0D0D15", borderBottom: "1px solid #1E1E2E" }}>
        <div className="flex items-center gap-1 mr-1">
          <span style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 18, color: "#F8F8FF", letterSpacing: "0.06em", lineHeight: 1 }}>RAILSHOT</span>
          <span style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 18, color: "#FF4D1C", letterSpacing: "0.06em", lineHeight: 1 }}>TV</span>
        </div>
        <div className="w-px h-4 mx-1" style={{ background: "#2A2A3A" }} />
        <span style={{ fontFamily: "'DM Sans', sans-serif", fontWeight: 600, fontSize: 11, color: "#606078", letterSpacing: "0.1em", textTransform: "uppercase" }}>Settings</span>
        <span style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 10, color: "#50506A" }}>/ {activeTab}</span>
        <div className="flex-1" />
        <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, color: "#F59E0B" }}>Changes apply on next stream</span>
      </div>

      <div className="flex flex-1 overflow-hidden">
        {/* Settings nav */}
        <div className="flex flex-col shrink-0 py-2" style={{ width: 168, background: "#0D0D15", borderRight: "1px solid #1E1E2E" }}>
          {SETTINGS_TABS.map(({ id, label, icon: Icon }) => (
            <button key={id} onClick={() => setActiveTab(id)} className="flex items-center gap-2.5 px-3 py-2.5 text-left transition-all" style={{ background: activeTab === id ? "#FF4D1C0F" : "transparent", borderLeft: activeTab === id ? "2px solid #FF4D1C" : "2px solid transparent" }}>
              <Icon size={14} style={{ color: activeTab === id ? "#FF4D1C" : "#50506A" }} />
              <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 13, fontWeight: activeTab === id ? 600 : 400, color: activeTab === id ? "#F8F8FF" : "#606078" }}>{label}</span>
            </button>
          ))}
        </div>

        {/* Settings content */}
        <div className="flex-1 overflow-y-auto px-6 py-4" style={{ background: "#0A0A0F" }}>
          {activeTab === "stream" && (
            <div className="max-w-2xl">
              {/* Service */}
              <div className="mb-6">
                <h3 className="mb-3" style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, fontWeight: 700, color: "#606078", letterSpacing: "0.1em", textTransform: "uppercase" }}>Service</h3>
                <div className="rounded p-4" style={{ background: "#111118", border: "1px solid #1E1E2E" }}>
                  <Row label="Platform">
                    <div className="flex gap-1.5">
                      {[["Twitch","#9146FF"],["YouTube","#FF4D1C"],["Facebook","#1877F2"],["Custom RTMP","#606078"]].map(([p, c]) => (
                        <button key={p} className="px-2.5 py-1 rounded text-xs font-medium transition-all" style={{ background: p === "YouTube" ? `${c}18` : "#1A1A24", border: p === "YouTube" ? `1px solid ${c}50` : "1px solid #2A2A3A", color: p === "YouTube" ? c : "#606078", fontFamily: "'DM Sans', sans-serif", fontSize: 11 }}>{p}</button>
                      ))}
                    </div>
                  </Row>
                  <Row label="Stream Key">
                    <div className="flex items-center gap-2">
                      <div className="flex items-center rounded px-3" style={{ height: 30, background: "#111118", border: "1px solid #2A2A3A", width: 200 }}>
                        <span style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 12, color: "#50506A" }}>••••••••••••••••••••</span>
                      </div>
                      {["Show","Get Key"].map(l => (
                        <button key={l} className="px-2.5 py-1 rounded text-xs" style={{ background: "#1A1A24", border: "1px solid #2A2A3A", color: "#A0A0B8", fontFamily: "'DM Sans', sans-serif", fontSize: 11 }}>{l}</button>
                      ))}
                    </div>
                  </Row>
                  <Row label="Server">
                    <Input value="Auto-select (recommended)" width={220} />
                  </Row>
                </div>
              </div>

              {/* Encoder */}
              <div className="mb-6">
                <h3 className="mb-3" style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, fontWeight: 700, color: "#606078", letterSpacing: "0.1em", textTransform: "uppercase" }}>Encoder</h3>
                <div className="rounded p-4" style={{ background: "#111118", border: "1px solid #1E1E2E" }}>
                  <Row label="Hardware Encoder">
                    <SegmentedButtons options={["NVENC","QSV","AMF","x264"]} active="NVENC" color="#3B82F6" />
                  </Row>
                  <Row label="Rate Control">
                    <SegmentedButtons options={["CBR","VBR","CQP"]} active="CBR" color="#3B82F6" />
                  </Row>
                  <Row label="Target Bitrate">
                    <div className="flex items-center gap-2">
                      <Input value="6000" width={80} />
                      <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, color: "#606078" }}>kbps</span>
                    </div>
                  </Row>
                  <Row label="Min Bitrate">
                    <div className="flex items-center gap-2">
                      <Input value="4500" width={80} />
                      <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, color: "#606078" }}>kbps</span>
                    </div>
                  </Row>
                  <Row label="Keyframe Interval">
                    <div className="flex items-center gap-2">
                      <Input value="2" width={60} />
                      <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, color: "#606078" }}>seconds</span>
                    </div>
                  </Row>
                </div>
              </div>

              {/* Output */}
              <div className="mb-6">
                <h3 className="mb-3" style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, fontWeight: 700, color: "#606078", letterSpacing: "0.1em", textTransform: "uppercase" }}>Recording Output</h3>
                <div className="rounded p-4" style={{ background: "#111118", border: "1px solid #1E1E2E" }}>
                  <Row label="Recording Path">
                    <div className="flex items-center gap-2">
                      <Input value="C:\Users\User\Videos\RailShotTV" width={240} />
                      <button className="px-2.5 py-1 rounded text-xs" style={{ background: "#1A1A24", border: "1px solid #2A2A3A", color: "#A0A0B8", fontFamily: "'DM Sans', sans-serif", fontSize: 11 }}>Browse</button>
                    </div>
                  </Row>
                  <Row label="Format">
                    <SegmentedButtons options={["MKV","MP4","MOV"]} active="MKV" color="#FF4D1C" />
                  </Row>
                  <Row label="Replay Buffer">
                    <div className="flex items-center gap-3">
                      <Toggle on={true} />
                      <div className="flex items-center gap-2">
                        <Input value="60" width={60} />
                        <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, color: "#606078" }}>seconds</span>
                      </div>
                    </div>
                  </Row>
                </div>
              </div>

              {/* Video */}
              <div className="mb-6">
                <h3 className="mb-3" style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 11, fontWeight: 700, color: "#606078", letterSpacing: "0.1em", textTransform: "uppercase" }}>Video</h3>
                <div className="rounded p-4" style={{ background: "#111118", border: "1px solid #1E1E2E" }}>
                  <Row label="Canvas Resolution">
                    <Input value="1920 × 1080" width={160} />
                  </Row>
                  <Row label="Output Resolution">
                    <Input value="1920 × 1080" width={160} />
                  </Row>
                  <Row label="Frame Rate">
                    <SegmentedButtons options={["30","60","120"]} active="60" color="#10B981" />
                  </Row>
                </div>
              </div>
            </div>
          )}
          {activeTab !== "stream" && (
            <div className="flex flex-col items-center justify-center h-full gap-3" style={{ opacity: 0.4 }}>
              <SettingsIcon size={32} style={{ color: "#50506A" }} />
              <span style={{ fontFamily: "'DM Sans', sans-serif", fontSize: 13, color: "#606078" }}>Select a settings category</span>
            </div>
          )}
        </div>
      </div>

      {/* Bottom action bar */}
      <div className="flex items-center justify-end gap-2 px-6 py-3 shrink-0" style={{ background: "#0D0D15", borderTop: "1px solid #1E1E2E" }}>
        <button className="flex items-center gap-1.5 px-4 py-1.5 rounded text-sm" style={{ background: "#1A1A24", border: "1px solid #2A2A3A", color: "#606078", fontFamily: "'DM Sans', sans-serif", fontSize: 12 }}>
          <X size={13} /> Cancel
        </button>
        <button className="flex items-center gap-1.5 px-4 py-1.5 rounded text-sm font-bold" style={{ background: "linear-gradient(135deg, #FF4D1C, #FF6B35)", boxShadow: "0 0 14px rgba(255,77,28,0.3)", color: "#fff", fontFamily: "'DM Sans', sans-serif", fontSize: 12 }}>
          <Save size={13} /> SAVE SETTINGS
        </button>
      </div>
    </AppSidebar>
  );
}
