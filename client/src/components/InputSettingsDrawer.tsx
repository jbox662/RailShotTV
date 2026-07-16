// InputSettingsDrawer — vMix-inspired Input/Scene settings panel
// Tabs: General | Position | Colour Adjust | Effects | Layers | Audio Settings
import { useState, useEffect } from "react";
import { X, Settings, RotateCcw } from "lucide-react";
import type { SceneItem } from "@/contexts/SceneContext";

// ── Types ─────────────────────────────────────────────────────────────────────
export type InputSettings = {
  // General
  name: string;
  aspectRatio: string;
  category: string;
  loop: boolean;
  autoRestart: boolean;
  autoPlay: boolean;
  autoPause: boolean;
  deinterlace: boolean;
  sharpen: boolean;
  mirror: boolean;
  flattenLayers: boolean;
  // Position
  posX: number;
  posY: number;
  posZ: number;
  width: number;
  height: number;
  rotation: number;
  opacity: number;
  cropLeft: number;
  cropRight: number;
  cropTop: number;
  cropBottom: number;
  // Colour Adjust
  brightness: number;
  contrast: number;
  saturation: number;
  hue: number;
  alpha: number;
  // Effects
  blur: number;
  sharpenEffect: number;
  pixelate: number;
  lumaKey: boolean;
  chromaKey: boolean;
  chromaKeyColor: string;
  chromaKeyThreshold: number;
  // Audio
  audioVolume: number;
  audioMuted: boolean;
  audioDelay: number;
  audioPan: number;
  audioGain: number;
};

export const DEFAULT_INPUT_SETTINGS: InputSettings = {
  name: "",
  aspectRatio: "Source",
  category: "#333333",
  loop: false,
  autoRestart: false,
  autoPlay: true,
  autoPause: true,
  deinterlace: false,
  sharpen: false,
  mirror: false,
  flattenLayers: false,
  posX: 0,
  posY: 0,
  posZ: 0,
  width: 1920,
  height: 1080,
  rotation: 0,
  opacity: 100,
  cropLeft: 0,
  cropRight: 0,
  cropTop: 0,
  cropBottom: 0,
  brightness: 0,
  contrast: 0,
  saturation: 0,
  hue: 0,
  alpha: 100,
  blur: 0,
  sharpenEffect: 0,
  pixelate: 0,
  lumaKey: false,
  chromaKey: false,
  chromaKeyColor: "#00FF00",
  chromaKeyThreshold: 50,
  audioVolume: 100,
  audioMuted: false,
  audioDelay: 0,
  audioPan: 0,
  audioGain: 0,
};

// ── Category colours (vMix-style swatches) ────────────────────────────────────
const CATEGORY_COLORS = [
  "#1A1A1A", "#EF4444", "#22C55E", "#FBBF24",
  "#EC4899", "#3B82F6", "#A855F7",
];

// ── Aspect ratio options ──────────────────────────────────────────────────────
const ASPECT_RATIOS = ["Source", "16:9", "4:3", "1:1", "9:16", "21:9", "Custom"];

// ── Shared styled input helpers ───────────────────────────────────────────────
const inputStyle: React.CSSProperties = {
  width: "100%",
  padding: "4px 8px",
  background: "#0A0C10",
  border: "1px solid #3A3D45",
  borderRadius: 3,
  color: "#D0D2D8",
  fontSize: 11,
  fontFamily: "'DM Sans', sans-serif",
  outline: "none",
};

const labelStyle: React.CSSProperties = {
  fontSize: 10,
  color: "#808898",
  fontFamily: "'DM Sans', sans-serif",
  fontWeight: 500,
  letterSpacing: "0.04em",
  marginBottom: 2,
  display: "block",
};

const fieldStyle: React.CSSProperties = {
  display: "flex",
  flexDirection: "column",
  gap: 2,
};

const rowStyle: React.CSSProperties = {
  display: "grid",
  gridTemplateColumns: "1fr 1fr",
  gap: 8,
};

const sectionTitleStyle: React.CSSProperties = {
  fontSize: 9,
  fontWeight: 700,
  color: "#4F9EFF",
  letterSpacing: "0.1em",
  textTransform: "uppercase",
  fontFamily: "'DM Sans', sans-serif",
  marginBottom: 8,
  marginTop: 12,
  paddingBottom: 4,
  borderBottom: "1px solid #2A2D35",
};

function SliderField({ label, value, min, max, step = 1, unit = "", onChange }: {
  label: string; value: number; min: number; max: number; step?: number; unit?: string;
  onChange: (v: number) => void;
}) {
  return (
    <div style={fieldStyle}>
      <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center" }}>
        <span style={labelStyle}>{label}</span>
        <span style={{ fontSize: 10, color: "#4F9EFF", fontFamily: "'JetBrains Mono', monospace" }}>{value}{unit}</span>
      </div>
      <input type="range" min={min} max={max} step={step} value={value}
        onChange={e => onChange(Number(e.target.value))}
        style={{ width: "100%", accentColor: "#4F9EFF", cursor: "pointer", height: 3 }} />
    </div>
  );
}

function NumberField({ label, value, min, max, step = 1, unit = "", onChange }: {
  label: string; value: number; min: number; max: number; step?: number; unit?: string;
  onChange: (v: number) => void;
}) {
  return (
    <div style={fieldStyle}>
      <label style={labelStyle}>{label}</label>
      <div style={{ display: "flex", alignItems: "center", gap: 4 }}>
        <input type="number" min={min} max={max} step={step} value={value}
          onChange={e => onChange(Number(e.target.value))}
          style={{ ...inputStyle, width: "100%" }} />
        {unit && <span style={{ fontSize: 10, color: "#606878", fontFamily: "'DM Sans', sans-serif", flexShrink: 0 }}>{unit}</span>}
      </div>
    </div>
  );
}

function CheckboxField({ label, checked, onChange }: { label: string; checked: boolean; onChange: (v: boolean) => void }) {
  return (
    <label style={{ display: "flex", alignItems: "center", gap: 6, cursor: "pointer", userSelect: "none" }}>
      <input type="checkbox" checked={checked} onChange={e => onChange(e.target.checked)}
        style={{ accentColor: "#4F9EFF", width: 12, height: 12, cursor: "pointer" }} />
      <span style={{ fontSize: 11, color: "#C0C2C8", fontFamily: "'DM Sans', sans-serif" }}>{label}</span>
    </label>
  );
}

// ── Tab content components ────────────────────────────────────────────────────
function GeneralTab({ s, set }: { s: InputSettings; set: (patch: Partial<InputSettings>) => void }) {
  return (
    <div style={{ display: "flex", flexDirection: "column", gap: 10 }}>
      {/* Name */}
      <div style={fieldStyle}>
        <label style={labelStyle}>Name</label>
        <input type="text" value={s.name} onChange={e => set({ name: e.target.value })}
          style={inputStyle} placeholder="Input name..." />
      </div>

      <div style={rowStyle}>
        {/* Aspect Ratio */}
        <div style={fieldStyle}>
          <label style={labelStyle}>Aspect Ratio</label>
          <select value={s.aspectRatio} onChange={e => set({ aspectRatio: e.target.value })}
            style={{ ...inputStyle, cursor: "pointer" }}>
            {ASPECT_RATIOS.map(r => <option key={r} value={r}>{r}</option>)}
          </select>
        </div>
        {/* Mouse Click Action */}
        <div style={fieldStyle}>
          <label style={labelStyle}>Click Action</label>
          <select style={{ ...inputStyle, cursor: "pointer" }}>
            <option>Preview</option>
            <option>Program</option>
            <option>None</option>
          </select>
        </div>
      </div>

      {/* Category color swatches */}
      <div style={fieldStyle}>
        <label style={labelStyle}>Category</label>
        <div style={{ display: "flex", gap: 6, flexWrap: "wrap" }}>
          {CATEGORY_COLORS.map(c => (
            <div key={c} onClick={() => set({ category: c })}
              style={{ width: 22, height: 22, borderRadius: 3, background: c, cursor: "pointer", border: s.category === c ? "2px solid #4F9EFF" : "2px solid #3A3D45", boxShadow: s.category === c ? `0 0 8px ${c}80` : "none", transition: "all 0.15s" }} />
          ))}
        </div>
      </div>

      {/* Info row */}
      <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr 1fr", gap: 8 }}>
        {[["Resolution", "1920×1080"], ["Frame Rate", "29.97p"], ["Deinterlacing", "None"]].map(([k, v]) => (
          <div key={k} style={fieldStyle}>
            <label style={labelStyle}>{k}</label>
            <div style={{ ...inputStyle, color: "#606878", background: "#080A0D" }}>{v}</div>
          </div>
        ))}
      </div>

      {/* Checkboxes */}
      <div style={{ ...sectionTitleStyle }}>Playback Options</div>
      <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr", gap: 6 }}>
        <CheckboxField label="Deinterlace Blend" checked={s.deinterlace} onChange={v => set({ deinterlace: v })} />
        <CheckboxField label="Sharpen" checked={s.sharpen} onChange={v => set({ sharpen: v })} />
        <CheckboxField label="Mirror" checked={s.mirror} onChange={v => set({ mirror: v })} />
        <CheckboxField label="Flatten Layers" checked={s.flattenLayers} onChange={v => set({ flattenLayers: v })} />
        <CheckboxField label="Auto Mix Audio" checked={s.loop} onChange={v => set({ loop: v })} />
        <CheckboxField label="Auto Play w/ Transition" checked={s.autoPlay} onChange={v => set({ autoPlay: v })} />
        <CheckboxField label="Auto Restart w/ Transition" checked={s.autoRestart} onChange={v => set({ autoRestart: v })} />
        <CheckboxField label="Auto Pause after Transition" checked={s.autoPause} onChange={v => set({ autoPause: v })} />
      </div>
    </div>
  );
}

function PositionTab({ s, set }: { s: InputSettings; set: (patch: Partial<InputSettings>) => void }) {
  return (
    <div style={{ display: "flex", flexDirection: "column", gap: 10 }}>
      <div style={sectionTitleStyle}>Transform</div>
      <div style={rowStyle}>
        <NumberField label="X Position" value={s.posX} min={-4096} max={4096} onChange={v => set({ posX: v })} unit="px" />
        <NumberField label="Y Position" value={s.posY} min={-4096} max={4096} onChange={v => set({ posY: v })} unit="px" />
      </div>
      <div style={rowStyle}>
        <NumberField label="Width" value={s.width} min={1} max={7680} onChange={v => set({ width: v })} unit="px" />
        <NumberField label="Height" value={s.height} min={1} max={4320} onChange={v => set({ height: v })} unit="px" />
      </div>
      <div style={rowStyle}>
        <NumberField label="Z-Order" value={s.posZ} min={-100} max={100} onChange={v => set({ posZ: v })} />
        <NumberField label="Rotation" value={s.rotation} min={-360} max={360} onChange={v => set({ rotation: v })} unit="°" />
      </div>
      <SliderField label="Opacity" value={s.opacity} min={0} max={100} unit="%" onChange={v => set({ opacity: v })} />

      <div style={sectionTitleStyle}>Crop</div>
      <div style={rowStyle}>
        <NumberField label="Crop Left" value={s.cropLeft} min={0} max={100} onChange={v => set({ cropLeft: v })} unit="%" />
        <NumberField label="Crop Right" value={s.cropRight} min={0} max={100} onChange={v => set({ cropRight: v })} unit="%" />
      </div>
      <div style={rowStyle}>
        <NumberField label="Crop Top" value={s.cropTop} min={0} max={100} onChange={v => set({ cropTop: v })} unit="%" />
        <NumberField label="Crop Bottom" value={s.cropBottom} min={0} max={100} onChange={v => set({ cropBottom: v })} unit="%" />
      </div>

      <button onClick={() => set({ posX: 0, posY: 0, posZ: 0, width: 1920, height: 1080, rotation: 0, opacity: 100, cropLeft: 0, cropRight: 0, cropTop: 0, cropBottom: 0 })}
        style={{ display: "flex", alignItems: "center", gap: 5, padding: "5px 10px", background: "linear-gradient(180deg,#2A2D35,#1E2128)", border: "1px solid #4A4D55", borderRadius: 3, color: "#C0C2C8", fontSize: 10, cursor: "pointer", fontFamily: "'DM Sans', sans-serif", marginTop: 4 }}>
        <RotateCcw size={11} /> Reset to Default
      </button>
    </div>
  );
}

function ColourAdjustTab({ s, set }: { s: InputSettings; set: (patch: Partial<InputSettings>) => void }) {
  return (
    <div style={{ display: "flex", flexDirection: "column", gap: 12 }}>
      <div style={sectionTitleStyle}>Colour Correction</div>
      <SliderField label="Brightness" value={s.brightness} min={-100} max={100} onChange={v => set({ brightness: v })} />
      <SliderField label="Contrast" value={s.contrast} min={-100} max={100} onChange={v => set({ contrast: v })} />
      <SliderField label="Saturation" value={s.saturation} min={-100} max={100} onChange={v => set({ saturation: v })} />
      <SliderField label="Hue Shift" value={s.hue} min={-180} max={180} unit="°" onChange={v => set({ hue: v })} />
      <SliderField label="Alpha" value={s.alpha} min={0} max={100} unit="%" onChange={v => set({ alpha: v })} />
      <button onClick={() => set({ brightness: 0, contrast: 0, saturation: 0, hue: 0, alpha: 100 })}
        style={{ display: "flex", alignItems: "center", gap: 5, padding: "5px 10px", background: "linear-gradient(180deg,#2A2D35,#1E2128)", border: "1px solid #4A4D55", borderRadius: 3, color: "#C0C2C8", fontSize: 10, cursor: "pointer", fontFamily: "'DM Sans', sans-serif" }}>
        <RotateCcw size={11} /> Reset Colour
      </button>
    </div>
  );
}

function EffectsTab({ s, set }: { s: InputSettings; set: (patch: Partial<InputSettings>) => void }) {
  return (
    <div style={{ display: "flex", flexDirection: "column", gap: 12 }}>
      <div style={sectionTitleStyle}>Filters</div>
      <SliderField label="Blur" value={s.blur} min={0} max={100} onChange={v => set({ blur: v })} />
      <SliderField label="Sharpen" value={s.sharpenEffect} min={0} max={100} onChange={v => set({ sharpenEffect: v })} />
      <SliderField label="Pixelate" value={s.pixelate} min={0} max={100} onChange={v => set({ pixelate: v })} />

      <div style={sectionTitleStyle}>Keying</div>
      <CheckboxField label="Luma Key" checked={s.lumaKey} onChange={v => set({ lumaKey: v })} />
      <CheckboxField label="Chroma Key" checked={s.chromaKey} onChange={v => set({ chromaKey: v })} />
      {s.chromaKey && (
        <>
          <div style={fieldStyle}>
            <label style={labelStyle}>Key Colour</label>
            <div style={{ display: "flex", alignItems: "center", gap: 8 }}>
              <input type="color" value={s.chromaKeyColor} onChange={e => set({ chromaKeyColor: e.target.value })}
                style={{ width: 36, height: 28, border: "1px solid #3A3D45", borderRadius: 3, cursor: "pointer", background: "none", padding: 2 }} />
              <span style={{ fontSize: 11, color: "#808898", fontFamily: "'JetBrains Mono', monospace" }}>{s.chromaKeyColor}</span>
            </div>
          </div>
          <SliderField label="Threshold" value={s.chromaKeyThreshold} min={0} max={100} unit="%" onChange={v => set({ chromaKeyThreshold: v })} />
        </>
      )}
    </div>
  );
}

function LayersTab({ scene }: { scene: SceneItem }) {
  return (
    <div style={{ display: "flex", flexDirection: "column", gap: 8 }}>
      <div style={sectionTitleStyle}>Source Layers</div>
      {scene.sources.length === 0 ? (
        <div style={{ padding: "20px 0", textAlign: "center", color: "#404450", fontSize: 11, fontFamily: "'DM Sans', sans-serif" }}>
          No sources in this scene.<br />
          <span style={{ fontSize: 10, color: "#303540" }}>Add sources via the main panel.</span>
        </div>
      ) : (
        <div style={{ display: "flex", flexDirection: "column", gap: 2 }}>
          {[...scene.sources].reverse().map((src, i) => (
            <div key={src.id} style={{ display: "flex", alignItems: "center", gap: 8, padding: "6px 8px", background: "#0A0C10", border: "1px solid #2A2D35", borderRadius: 3 }}>
              <div style={{ width: 8, height: 8, borderRadius: "50%", background: src.color, flexShrink: 0 }} />
              <src.icon size={12} style={{ color: src.color, flexShrink: 0 }} />
              <span style={{ fontSize: 11, color: "#C0C2C8", fontFamily: "'DM Sans', sans-serif", flex: 1, overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }}>{src.name}</span>
              <span style={{ fontSize: 9, color: "#606878", fontFamily: "'DM Sans', sans-serif", flexShrink: 0 }}>{src.type}</span>
              <div style={{ width: 8, height: 8, borderRadius: 1, background: src.visible ? "#22C55E" : "#3A3D45", flexShrink: 0 }} title={src.visible ? "Visible" : "Hidden"} />
            </div>
          ))}
        </div>
      )}
      <div style={{ marginTop: 8, padding: "8px", background: "#080A0D", border: "1px solid #2A2D35", borderRadius: 3 }}>
        <div style={{ fontSize: 10, color: "#606878", fontFamily: "'DM Sans', sans-serif" }}>
          Layer order: top of list = front of canvas. Edit layer visibility and order from the Sources panel on the main dashboard.
        </div>
      </div>
    </div>
  );
}

function AudioSettingsTab({ s, set }: { s: InputSettings; set: (patch: Partial<InputSettings>) => void }) {
  return (
    <div style={{ display: "flex", flexDirection: "column", gap: 12 }}>
      <div style={sectionTitleStyle}>Audio Mix</div>
      <SliderField label="Volume" value={s.audioVolume} min={0} max={200} unit="%" onChange={v => set({ audioVolume: v })} />
      <SliderField label="Pan" value={s.audioPan} min={-100} max={100} onChange={v => set({ audioPan: v })} />
      <SliderField label="Gain" value={s.audioGain} min={-20} max={20} unit=" dB" onChange={v => set({ audioGain: v })} />
      <NumberField label="Delay" value={s.audioDelay} min={0} max={5000} onChange={v => set({ audioDelay: v })} unit="ms" />
      <CheckboxField label="Mute" checked={s.audioMuted} onChange={v => set({ audioMuted: v })} />

      <div style={sectionTitleStyle}>Bus Routing</div>
      <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr 1fr", gap: 6 }}>
        {["Master", "Bus A", "Bus B", "Bus C", "Headphones", "Rec"].map(bus => (
          <label key={bus} style={{ display: "flex", alignItems: "center", gap: 5, cursor: "pointer" }}>
            <input type="checkbox" defaultChecked={bus === "Master"}
              style={{ accentColor: "#4F9EFF", width: 12, height: 12, cursor: "pointer" }} />
            <span style={{ fontSize: 10, color: "#C0C2C8", fontFamily: "'DM Sans', sans-serif" }}>{bus}</span>
          </label>
        ))}
      </div>
    </div>
  );
}

// ── Source Config Tab — reuses InputConfigPanel-style fields inline ───────────
function SourceConfigTab({ typeId, name, setName, config, setConfig }: {
  typeId: string;
  name: string;
  setName: (v: string) => void;
  config: Record<string, string | number | boolean>;
  setConfig: (k: string, v: string | number | boolean) => void;
}) {
  const S = {
    label: { fontFamily: "'DM Sans',sans-serif", fontSize: 11, color: "#8892A4", display: "block" as const, marginBottom: 4, letterSpacing: "0.05em" },
    input: { width: "100%", background: "#0D1017", border: "1px solid #2A2D35", borderRadius: 3, color: "#D0D2D8", fontFamily: "'DM Sans',sans-serif", fontSize: 12, padding: "5px 8px", outline: "none", boxSizing: "border-box" as const },
    select: { width: "100%", background: "#0D1017", border: "1px solid #2A2D35", borderRadius: 3, color: "#D0D2D8", fontFamily: "'DM Sans',sans-serif", fontSize: 12, padding: "5px 8px", outline: "none" },
    row: { marginBottom: 12 },
    check: { display: "flex" as const, alignItems: "center" as const, gap: 8, marginBottom: 8, cursor: "pointer" as const },
    checkLabel: { fontFamily: "'DM Sans',sans-serif", fontSize: 12, color: "#C0C2C8" },
    hint: { fontFamily: "'DM Sans',sans-serif", fontSize: 11, color: "#606878", marginTop: 0, marginBottom: 14, lineHeight: 1.5 },
  };
  const nameRow = (
    <div style={S.row}>
      <label style={S.label}>Input Name</label>
      <input value={name} onChange={e => setName(e.target.value)} style={S.input} />
    </div>
  );
  const numInput = (label: string, key: string, def: number, min?: number, max?: number) => (
    <div style={{ ...S.row, display: "flex", alignItems: "center", justifyContent: "space-between" }}>
      <label style={{ ...S.label, marginBottom: 0 }}>{label}</label>
      <input type="number" value={(config[key] as number) ?? def} min={min} max={max}
        onChange={e => setConfig(key, Number(e.target.value))}
        style={{ ...S.input, width: 100, textAlign: "right" }} />
    </div>
  );
  const selectInput = (label: string, key: string, options: string[], def: string) => (
    <div style={S.row}>
      <label style={S.label}>{label}</label>
      <select value={(config[key] as string) ?? def} onChange={e => setConfig(key, e.target.value)} style={S.select}>
        {options.map(o => <option key={o}>{o}</option>)}
      </select>
    </div>
  );
  const checkRow = (label: string, key: string) => (
    <label style={S.check}>
      <input type="checkbox" checked={!!(config[key])} onChange={e => setConfig(key, e.target.checked)}
        style={{ width: 14, height: 14, accentColor: "#4F9EFF" }} />
      <span style={S.checkLabel}>{label}</span>
    </label>
  );
  const urlRow = (label: string, key: string, placeholder: string) => (
    <div style={S.row}>
      <label style={S.label}>{label}</label>
      <input value={(config[key] as string) ?? ""} onChange={e => setConfig(key, e.target.value)}
        placeholder={placeholder} style={S.input} />
    </div>
  );
  const fileRow = (label: string, key: string, placeholder: string) => (
    <div style={S.row}>
      <label style={S.label}>{label}</label>
      <div style={{ display: "flex", gap: 6 }}>
        <input value={(config[key] as string) ?? ""} onChange={e => setConfig(key, e.target.value)}
          placeholder={placeholder} style={{ ...S.input, flex: 1 }} />
        <button style={{ padding: "5px 12px", background: "linear-gradient(180deg,#2A2D35,#1E2128)", border: "1px solid #3A3D45", borderRadius: 3, color: "#C0C2C8", fontFamily: "'DM Sans',sans-serif", fontSize: 11, cursor: "pointer", whiteSpace: "nowrap" }}>Browse</button>
      </div>
    </div>
  );

  switch (typeId) {
    case "video": case "dvd":
      return (<div>{nameRow}{fileRow("File Path", "filePath", "C:\\Videos\\clip.mp4")}{checkRow("Alpha Channel", "alphaChannel")}{checkRow("Interlaced", "interlaced")}</div>);
    case "list":
      return (<div>{nameRow}{selectInput("Playback Mode", "playbackMode", ["Sequential","Shuffle","Loop"], "Sequential")}{checkRow("Auto-advance", "autoAdvance")}{checkRow("Loop playlist", "loopList")}</div>);
    case "camera": case "videocall":
      return (<div>{nameRow}{selectInput("Device", "device", ["Default Camera","USB Camera 1","USB Camera 2","Capture Card (HDMI)","Virtual Camera"], "Default Camera")}{selectInput("Resolution", "resolution", ["1920x1080","1280x720","3840x2160"], "1920x1080")}{selectInput("Frame Rate", "fps", ["29.97","30","60","25","24"], "29.97")}{checkRow("Deinterlace", "deinterlace")}</div>);
    case "ndi": case "display":
      return (<div>{nameRow}{selectInput("Source", "ndiSource", ["NDI Source 1","NDI Source 2","Primary Monitor","Secondary Monitor","Application Window"], "Primary Monitor")}{checkRow("Show Cursor", "showCursor")}{checkRow("Capture Audio", "captureAudio")}</div>);
    case "stream":
      return (<div>{nameRow}{selectInput("Protocol", "protocol", ["RTMP","RTSP","SRT","HLS","UDP"], "RTMP")}{urlRow("Stream URL", "url", "rtmp://live.example.com/app/streamkey")}{numInput("Latency (ms)", "latency", 120, 0, 10000)}{checkRow("Auto-reconnect", "autoReconnect")}</div>);
    case "image": case "photos":
      return (<div>{nameRow}{fileRow("File Path", "filePath", "C:\\Images\\photo.png")}{checkRow("Alpha Channel", "alphaChannel")}</div>);
    case "imagseq":
      return (<div>{nameRow}{fileRow("Folder / First Frame", "folderPath", "C:\\Stingers\\intro_0001.png")}{numInput("Frame Rate", "fps", 30, 1, 120)}{checkRow("Loop", "loop")}</div>);
    case "videodelay":
      return (<div>{nameRow}{selectInput("Source Input", "sourceInput", ["Input 1","Input 2","Input 3","Input 4"], "Input 1")}{numInput("Delay (frames)", "delayFrames", 30, 0, 3600)}</div>);
    case "powerpoint":
      return (<div>{nameRow}{fileRow("File Path", "filePath", "C:\\Presentations\\slides.pptx")}{checkRow("Auto-advance slides", "autoAdvance")}{numInput("Slide duration (s)", "slideDuration", 5, 1, 300)}</div>);
    case "colour":
      return (<div>{nameRow}<div style={S.row}><label style={S.label}>Colour</label><div style={{ display: "flex", gap: 8, alignItems: "center" }}><input type="color" value={(config["colour"] as string) ?? "#000000"} onChange={e => setConfig("colour", e.target.value)} style={{ width: 40, height: 28, border: "1px solid #2A2D35", borderRadius: 3, background: "none", cursor: "pointer" }} /><input value={(config["colour"] as string) ?? "#000000"} onChange={e => setConfig("colour", e.target.value)} style={{ ...S.input, flex: 1 }} /></div></div></div>);
    case "audio": case "audioinput":
      return (<div>{nameRow}{typeId === "audio" ? fileRow("Audio File", "filePath", "C:\\Audio\\music.mp3") : selectInput("Audio Device", "device", ["Default Microphone","USB Mic","Line In","Virtual Audio Cable"], "Default Microphone")}{numInput("Volume (%)", "volume", 100, 0, 200)}{checkRow("Loop", "loop")}</div>);
    case "title": case "lowerthird":
      return (<div>{nameRow}{urlRow("Title Text", "title", "Enter title...")}{typeId === "lowerthird" && urlRow("Subtitle", "subtitle", "Enter subtitle...")}{selectInput("Font", "font", ["DM Sans","Arial","Impact","Helvetica Neue","Bebas Neue"], "DM Sans")}{numInput("Font Size", "fontSize", 48, 8, 400)}</div>);
    case "virtualset":
      return (<div>{nameRow}{fileRow("Set File (.vs)", "filePath", "C:\\VirtualSets\\studio.vs")}{selectInput("Camera Angle", "angle", ["Wide","Medium","Close-up","Over Shoulder"], "Wide")}{checkRow("Enable Chroma Key", "chromaKey")}</div>);
    case "browser": case "zoom":
      return (
        <div>
          {nameRow}
          <div style={S.row}>
            <label style={S.label}>{typeId === "zoom" ? "Meeting ID" : "URL"}</label>
            <input value={(config[typeId === "zoom" ? "meetingId" : "url"] as string) ?? ""}
              onChange={e => setConfig(typeId === "zoom" ? "meetingId" : "url", e.target.value)}
              placeholder={typeId === "zoom" ? "123 456 7890" : "https://example.com"}
              style={S.input} />
          </div>
          {typeId === "browser" && numInput("Width (px)", "width", 1920, 100, 7680)}
          {typeId === "browser" && numInput("Height (px)", "height", 1080, 100, 4320)}
          {typeId === "browser" && checkRow("Transparent Background", "transparent")}
          {typeId === "browser" && checkRow("Capture Audio", "captureAudio")}
          {typeId === "zoom" && selectInput("Layout", "layout", ["Gallery","Speaker","Side-by-side"], "Gallery")}
        </div>
      );
    case "scoreboard":
      return (<div>{nameRow}{selectInput("Sport", "sport", ["Billiards","Basketball","Soccer","Tennis","Baseball","Hockey","Football"], "Billiards")}{selectInput("Template", "template", ["Classic","Modern","Minimal","Broadcast"], "Classic")}{checkRow("Show Clock", "showClock")}{checkRow("Auto-update", "autoUpdate")}</div>);
    case "lowerthird":
      return (<div>{nameRow}{urlRow("Text Line 1", "line1", "Name / Title...")}{urlRow("Text Line 2", "line2", "Subtitle / Info...")}</div>);
    case "overlay":
      return (<div>{nameRow}{urlRow("Overlay URL / File", "url", "https://overlays.example.com/lower-third")}{checkRow("Transparent Background", "transparent")}</div>);
    case "alert":
      return (<div>{nameRow}{urlRow("Alert URL", "url", "https://alerts.example.com/widget")}{numInput("Display Duration (s)", "duration", 5, 1, 60)}</div>);
    default:
      return (<div>{nameRow}<p style={S.hint}>No additional configuration required for this input type.</p></div>);
  }
}

// ── Main Drawer ───────────────────────────────────────────────────────────────
const TABS = ["Source Config", "General", "Position", "Colour Adjust", "Effects", "Layers", "Audio Settings"] as const;
type Tab = typeof TABS[number];

interface InputSettingsDrawerProps {
  scene: SceneItem | null;
  source: SceneItem["sources"][0] | null;
  open: boolean;
  onClose: () => void;
  onSave: (sourceId: number, settings: InputSettings, sourceConfig: Record<string, string | number | boolean>) => void;
  initialSettings?: InputSettings;
}

export function InputSettingsDrawer({ scene, source, open, onClose, onSave, initialSettings }: InputSettingsDrawerProps) {
  const [activeTab, setActiveTab] = useState<Tab>("Source Config");
  const [settings, setSettings] = useState<InputSettings>(() => ({
    ...DEFAULT_INPUT_SETTINGS,
    name: source?.name ?? "",
    ...initialSettings,
  }));
  // Source-specific config state (URL, file path, device, etc.) — seeded from source.settings
  const [sourceConfig, setSourceConfigState] = useState<Record<string, string | number | boolean>>(
    () => (source?.settings ?? {}) as Record<string, string | number | boolean>
  );
  const setSourceConfig = (k: string, v: string | number | boolean) =>
    setSourceConfigState(prev => ({ ...prev, [k]: v }));

  // Sync name when scene changes
  useEffect(() => {
    if (source) {
      setSettings(prev => ({ ...prev, name: source.name, ...initialSettings }));
      setSourceConfigState((source.settings ?? {}) as Record<string, string | number | boolean>);
    }
  }, [source?.id]); // eslint-disable-line react-hooks/exhaustive-deps

  const set = (patch: Partial<InputSettings>) => setSettings(prev => ({ ...prev, ...patch }));

  const handleSave = () => {
    if (source) {
      onSave(source.id, settings, sourceConfig);
      onClose();
    }
  };

  // Slide-in animation
  const drawerStyle: React.CSSProperties = {
    position: "fixed",
    top: 0,
    right: 0,
    bottom: 0,
    width: 460,
    background: "linear-gradient(180deg, #13151A 0%, #0F1114 100%)",
    borderLeft: "1px solid #2A2D35",
    boxShadow: "-8px 0 40px rgba(0,0,0,0.7)",
    display: "flex",
    flexDirection: "column",
    zIndex: 1000,
    transform: open ? "translateX(0)" : "translateX(100%)",
    transition: "transform 0.25s cubic-bezier(0.23, 1, 0.32, 1)",
    fontFamily: "'DM Sans', sans-serif",
  };

  if (!scene || !source) return null;

  return (
    <>
      {/* Backdrop */}
      {open && (
        <div
          style={{ position: "fixed", inset: 0, background: "rgba(0,0,0,0.4)", zIndex: 999 }}
          onClick={onClose}
        />
      )}

      <div style={drawerStyle}>
        {/* Header */}
        <div style={{ display: "flex", alignItems: "center", gap: 8, padding: "10px 14px", background: "linear-gradient(180deg,#1A1D22,#141619)", borderBottom: "1px solid #2A2D35", flexShrink: 0 }}>
          <Settings size={14} style={{ color: "#4F9EFF" }} />
          <div style={{ flex: 1, minWidth: 0 }}>
            <div style={{ fontSize: 12, fontWeight: 700, color: "#E0E2E8", letterSpacing: "0.02em", overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }}>
              Input: {source.id} {settings.name || source.name}
            </div>
            <div style={{ fontSize: 9, color: "#606878", letterSpacing: "0.04em", marginTop: 1 }}>
              <span style={{ color: source.color }}>{source.type}</span> · {scene.name}
            </div>
          </div>
          {/* Category dot */}
          <div style={{ width: 12, height: 12, borderRadius: 2, background: settings.category, border: "1px solid #3A3D45", flexShrink: 0 }} />
          <button onClick={onClose}
            style={{ width: 24, height: 24, background: "none", border: "1px solid #3A3D45", borderRadius: 3, color: "#808898", cursor: "pointer", display: "flex", alignItems: "center", justifyContent: "center", flexShrink: 0 }}>
            <X size={12} />
          </button>
        </div>

        {/* Tab bar */}
        <div style={{ display: "flex", borderBottom: "1px solid #2A2D35", background: "#0F1114", flexShrink: 0, overflowX: "auto" }}>
          {TABS.map(tab => (
            <button key={tab} onClick={() => setActiveTab(tab)}
              style={{
                padding: "8px 12px",
                background: activeTab === tab ? "linear-gradient(180deg,#1A3AFF15,#1A3AFF08)" : "none",
                border: "none",
                borderBottom: activeTab === tab ? "2px solid #4F9EFF" : "2px solid transparent",
                color: activeTab === tab ? "#4F9EFF" : "#808898",
                fontSize: 10,
                fontWeight: activeTab === tab ? 700 : 500,
                cursor: "pointer",
                fontFamily: "'DM Sans', sans-serif",
                letterSpacing: "0.03em",
                whiteSpace: "nowrap",
                transition: "all 0.15s",
                flexShrink: 0,
              }}>
              {tab}
            </button>
          ))}
        </div>

        {/* Tab content */}
        <div style={{ flex: 1, overflowY: "auto", padding: "14px 16px", minHeight: 0 }}>
          {activeTab === "Source Config"  && (
            <SourceConfigTab
              typeId={source.type}
              name={settings.name}
              setName={v => set({ name: v })}
              config={sourceConfig}
              setConfig={setSourceConfig}
            />
          )}
          {activeTab === "General"       && <GeneralTab s={settings} set={set} />}
          {activeTab === "Position"      && <PositionTab s={settings} set={set} />}
          {activeTab === "Colour Adjust" && <ColourAdjustTab s={settings} set={set} />}
          {activeTab === "Effects"       && <EffectsTab s={settings} set={set} />}
          {activeTab === "Layers"        && <LayersTab scene={scene} />}
          {activeTab === "Audio Settings" && <AudioSettingsTab s={settings} set={set} />}
        </div>

        {/* Footer */}
        <div style={{ display: "flex", alignItems: "center", justifyContent: "space-between", padding: "10px 14px", background: "linear-gradient(180deg,#141619,#0F1114)", borderTop: "1px solid #2A2D35", flexShrink: 0, gap: 8 }}>
          <button onClick={() => toast.info("Copy From — coming soon")}
            style={{ padding: "6px 14px", background: "linear-gradient(180deg,#2A2D35,#1E2128)", border: "1px solid #4A4D55", borderRadius: 3, color: "#C0C2C8", fontSize: 11, cursor: "pointer", fontFamily: "'DM Sans', sans-serif" }}>
            Copy From...
          </button>
          <div style={{ display: "flex", gap: 6 }}>
            <button onClick={onClose}
              style={{ padding: "6px 14px", background: "linear-gradient(180deg,#2A2D35,#1E2128)", border: "1px solid #4A4D55", borderRadius: 3, color: "#C0C2C8", fontSize: 11, cursor: "pointer", fontFamily: "'DM Sans', sans-serif" }}>
              Cancel
            </button>
            <button onClick={handleSave}
              style={{ padding: "6px 18px", background: "linear-gradient(180deg,#4F9EFF,#3A7ACC)", border: "1px solid #4F9EFF80", borderRadius: 3, color: "#fff", fontSize: 11, fontWeight: 700, cursor: "pointer", fontFamily: "'DM Sans', sans-serif", boxShadow: "0 0 10px rgba(79,158,255,0.3)" }}>
              Apply
            </button>
          </div>
        </div>
      </div>
    </>
  );
}

// Need to import toast for the Copy From button
import { toast } from "sonner";
