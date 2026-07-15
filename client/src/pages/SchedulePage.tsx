// RailShotTV — Chromatic Command — Bright Edition
// SchedulePage: Event scheduling with countdown timers and one-click Start Stream
import React, { useState, useEffect, useCallback } from "react";
import AppSidebar from "@/components/AppSidebar";
import { toast } from "sonner";
import {
  Calendar, Clock, Plus, Play, Edit2, Trash2, ChevronLeft, ChevronRight,
  Tv2, Users, MapPin, Tag, Bell, BellOff, CheckCircle2, AlertCircle,
  Radio, Zap, RefreshCw, MoreHorizontal, Search, Filter
} from "lucide-react";

// ── Types ──────────────────────────────────────────────────────────────────────
type EventStatus = "upcoming" | "live" | "completed" | "cancelled";
type Platform = "YouTube" | "Twitch" | "Facebook" | "Custom";

interface ScheduledEvent {
  id: string;
  title: string;
  description: string;
  sport: string;
  venue: string;
  startTime: Date;
  durationMinutes: number;
  platform: Platform;
  streamKey?: string;
  status: EventStatus;
  thumbnail: string;
  remindersEnabled: boolean;
  tags: string[];
  estimatedViewers?: number;
}

// ── Helpers ────────────────────────────────────────────────────────────────────
function pad(n: number) { return String(n).padStart(2, "0"); }

function formatCountdown(ms: number): { d: number; h: number; m: number; s: number; total: number } {
  if (ms <= 0) return { d: 0, h: 0, m: 0, s: 0, total: 0 };
  const total = Math.floor(ms / 1000);
  const d = Math.floor(total / 86400);
  const h = Math.floor((total % 86400) / 3600);
  const m = Math.floor((total % 3600) / 60);
  const s = total % 60;
  return { d, h, m, s, total };
}

function formatEventTime(date: Date): string {
  return date.toLocaleString("en-US", {
    weekday: "short", month: "short", day: "numeric",
    hour: "numeric", minute: "2-digit", hour12: true,
  });
}

function formatDuration(mins: number): string {
  if (mins < 60) return `${mins}m`;
  const h = Math.floor(mins / 60);
  const m = mins % 60;
  return m > 0 ? `${h}h ${m}m` : `${h}h`;
}

const PLATFORM_COLORS: Record<Platform, string> = {
  YouTube: "#FF0000",
  Twitch:  "#9147FF",
  Facebook:"#1877F2",
  Custom:  "#22D3EE",
};

const SPORT_ICONS: Record<string, string> = {
  "Pool / Billiards": "🎱",
  "Basketball": "🏀",
  "Soccer": "⚽",
  "Tennis": "🎾",
  "Esports": "🎮",
  "Boxing": "🥊",
  "Wrestling": "🤼",
  "Custom Event": "🏆",
};

// ── Mock data ──────────────────────────────────────────────────────────────────
function makeMockEvents(): ScheduledEvent[] {
  const now = new Date();
  const add = (h: number, m = 0) => new Date(now.getTime() + h * 3600000 + m * 60000);
  const sub = (h: number) => new Date(now.getTime() - h * 3600000);
  return [
    {
      id: "1", title: "Championship Finals — Table 1",
      description: "Season championship finals — main stage event.",
      sport: "Custom Event", venue: "Main Arena — Stage 1",
      startTime: add(0, 8), durationMinutes: 180,
      platform: "YouTube", status: "upcoming",
      thumbnail: "", remindersEnabled: true,
      tags: ["championship", "finals"], estimatedViewers: 1200,
    },
    {
      id: "2", title: "Pro League Quarterfinals",
      description: "Pro league quarterfinal match.",
      sport: "Esports", venue: "Main Arena — Stage 2",
      startTime: add(3), durationMinutes: 120,
      platform: "Twitch", status: "upcoming",
      thumbnail: "", remindersEnabled: false,
      tags: ["pro-league", "quarterfinals"], estimatedViewers: 850,
    },
    {
      id: "3", title: "Youth Division Semifinal",
      description: "Youth division semifinal — under 21 category.",
      sport: "Basketball", venue: "Side Room — Court A",
      startTime: add(6, 30), durationMinutes: 90,
      platform: "Facebook", status: "upcoming",
      thumbnail: "", remindersEnabled: true,
      tags: ["youth", "semifinal"], estimatedViewers: 320,
    },
    {
      id: "4", title: "Exhibition Match — Special Guests",
      description: "Special exhibition match with invited professionals.",
      sport: "Boxing", venue: "VIP Lounge — Ring 1",
      startTime: add(24), durationMinutes: 60,
      platform: "YouTube", status: "upcoming",
      thumbnail: "", remindersEnabled: false,
      tags: ["exhibition", "vip"], estimatedViewers: 2100,
    },
    {
      id: "5", title: "Regional Open — Round 1",
      description: "Regional open tournament, round 1.",
      sport: "Soccer", venue: "Main Arena — Field 1",
      startTime: add(48), durationMinutes: 240,
      platform: "YouTube", status: "upcoming",
      thumbnail: "", remindersEnabled: true,
      tags: ["regional", "open"], estimatedViewers: 560,
    },
    {
      id: "6", title: "Last Night — Grand Final",
      description: "Grand final from last night's event.",
      sport: "Custom Event", venue: "Main Arena",
      startTime: sub(18), durationMinutes: 200,
      platform: "YouTube", status: "completed",
      thumbnail: "", remindersEnabled: false,
      tags: ["grand-final"], estimatedViewers: 3400,
    },
  ];
}

// ── Countdown component ────────────────────────────────────────────────────────
function CountdownTimer({ target, status }: { target: Date; status: EventStatus }) {
  const [now, setNow] = useState(Date.now());
  useEffect(() => {
    const id = setInterval(() => setNow(Date.now()), 1000);
    return () => clearInterval(id);
  }, []);

  if (status === "live") {
    return (
      <div style={{ display: "flex", alignItems: "center", gap: 6 }}>
        <div style={{
          width: 8, height: 8, borderRadius: "50%", background: "#FF5A2C",
          animation: "pulse 1.2s ease-in-out infinite",
        }} />
        <span style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 13, color: "#FF5A2C", fontWeight: 600 }}>
          LIVE NOW
        </span>
      </div>
    );
  }
  if (status === "completed") {
    return <span style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 12, color: "#64748B" }}>Completed</span>;
  }
  if (status === "cancelled") {
    return <span style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 12, color: "#EF4444" }}>Cancelled</span>;
  }

  const { d, h, m, s, total } = formatCountdown(target.getTime() - now);
  if (total <= 0) {
    return (
      <span style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 12, color: "#22C55E" }}>Starting soon…</span>
    );
  }

  const urgent = total < 3600; // < 1 hour
  const color = urgent ? "#FF5A2C" : "#4F9EFF";

  return (
    <div style={{ display: "flex", gap: 4, alignItems: "center" }}>
      {d > 0 && (
        <span style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 13, color, fontWeight: 700 }}>
          {d}d
        </span>
      )}
      <span style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 13, color, fontWeight: 700 }}>
        {pad(h)}:{pad(m)}:{pad(s)}
      </span>
    </div>
  );
}

// ── Add/Edit Event Modal ───────────────────────────────────────────────────────
interface EventModalProps {
  event?: ScheduledEvent | null;
  onSave: (ev: ScheduledEvent) => void;
  onClose: () => void;
}
function EventModal({ event, onSave, onClose }: EventModalProps) {
  const isEdit = !!event;
  const [title, setTitle] = useState(event?.title ?? "");
  const [description, setDescription] = useState(event?.description ?? "");
  const [sport, setSport] = useState(event?.sport ?? "Custom Event");
  const [venue, setVenue] = useState(event?.venue ?? "");
  const [platform, setPlatform] = useState<Platform>(event?.platform ?? "YouTube");
  const [duration, setDuration] = useState(String(event?.durationMinutes ?? 120));
  const [startDate, setStartDate] = useState(
    event ? event.startTime.toISOString().slice(0, 16) : ""
  );
  const [reminders, setReminders] = useState(event?.remindersEnabled ?? true);
  const [tags, setTags] = useState(event?.tags.join(", ") ?? "");

  function handleSave() {
    if (!title.trim() || !startDate) {
      toast.error("Title and start time are required.");
      return;
    }
    const saved: ScheduledEvent = {
      id: event?.id ?? String(Date.now()),
      title: title.trim(),
      description: description.trim(),
      sport,
      venue: venue.trim(),
      startTime: new Date(startDate),
      durationMinutes: parseInt(duration) || 120,
      platform,
      status: event?.status ?? "upcoming",
      thumbnail: event?.thumbnail ?? "",
      remindersEnabled: reminders,
      tags: tags.split(",").map(t => t.trim()).filter(Boolean),
    };
    onSave(saved);
  }

  const inputStyle: React.CSSProperties = {
    width: "100%", padding: "8px 12px",
    background: "rgba(255,255,255,0.05)", border: "1px solid rgba(255,255,255,0.12)",
    borderRadius: 6, color: "#E2E8F0", fontSize: 13, outline: "none",
    fontFamily: "'DM Sans', sans-serif",
  };
  const labelStyle: React.CSSProperties = {
    fontSize: 11, fontWeight: 600, color: "#64748B", letterSpacing: "0.08em",
    textTransform: "uppercase", marginBottom: 4, display: "block",
  };

  return (
    <div style={{
      position: "fixed", inset: 0, background: "rgba(0,0,0,0.75)", zIndex: 200,
      display: "flex", alignItems: "center", justifyContent: "center",
    }} onClick={onClose}>
      <div style={{
        background: "#1A2035", border: "1px solid rgba(255,255,255,0.12)",
        borderRadius: 12, padding: 28, width: 520, maxHeight: "90vh", overflowY: "auto",
        boxShadow: "0 24px 64px rgba(0,0,0,0.6)",
      }} onClick={e => e.stopPropagation()}>
        {/* Header */}
        <div style={{ display: "flex", alignItems: "center", justifyContent: "space-between", marginBottom: 24 }}>
          <div>
            <div style={{ fontSize: 11, color: "#4F9EFF", fontWeight: 700, letterSpacing: "0.1em", textTransform: "uppercase", marginBottom: 2 }}>
              {isEdit ? "Edit Event" : "Schedule New Event"}
            </div>
            <div style={{ fontSize: 18, fontWeight: 700, color: "#F1F5F9", fontFamily: "'Bebas Neue', sans-serif", letterSpacing: "0.04em" }}>
              {isEdit ? title || "Event Details" : "New Broadcast Event"}
            </div>
          </div>
          <button onClick={onClose} style={{ background: "none", border: "none", color: "#64748B", cursor: "pointer", fontSize: 20 }}>✕</button>
        </div>

        {/* Form */}
        <div style={{ display: "flex", flexDirection: "column", gap: 16 }}>
          <div>
            <label style={labelStyle}>Event Title *</label>
            <input style={inputStyle} value={title} onChange={e => setTitle(e.target.value)} placeholder="e.g. Championship Finals — Table 1" />
          </div>
          <div>
            <label style={labelStyle}>Description</label>
            <textarea style={{ ...inputStyle, resize: "vertical", minHeight: 60 }} value={description} onChange={e => setDescription(e.target.value)} placeholder="Brief description of the event…" />
          </div>
          <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr", gap: 12 }}>
            <div>
              <label style={labelStyle}>Sport / Event Type</label>
              <select style={inputStyle} value={sport} onChange={e => setSport(e.target.value)}>
                {Object.keys(SPORT_ICONS).map(s => <option key={s} value={s}>{s}</option>)}
              </select>
            </div>
            <div>
              <label style={labelStyle}>Platform</label>
              <select style={inputStyle} value={platform} onChange={e => setPlatform(e.target.value as Platform)}>
                {(["YouTube","Twitch","Facebook","Custom"] as Platform[]).map(p => <option key={p} value={p}>{p}</option>)}
              </select>
            </div>
          </div>
          <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr", gap: 12 }}>
            <div>
              <label style={labelStyle}>Start Date & Time *</label>
              <input type="datetime-local" style={inputStyle} value={startDate} onChange={e => setStartDate(e.target.value)} />
            </div>
            <div>
              <label style={labelStyle}>Duration (minutes)</label>
              <input type="number" style={inputStyle} value={duration} onChange={e => setDuration(e.target.value)} min={15} max={720} />
            </div>
          </div>
          <div>
            <label style={labelStyle}>Venue / Location</label>
            <input style={inputStyle} value={venue} onChange={e => setVenue(e.target.value)} placeholder="e.g. Main Arena — Table 1" />
          </div>
          <div>
            <label style={labelStyle}>Tags (comma-separated)</label>
            <input style={inputStyle} value={tags} onChange={e => setTags(e.target.value)} placeholder="e.g. finals, championship, live" />
          </div>
          <div style={{ display: "flex", alignItems: "center", gap: 10 }}>
            <div
              onClick={() => setReminders(!reminders)}
              style={{
                width: 36, height: 20, borderRadius: 10, cursor: "pointer",
                background: reminders ? "#4F9EFF" : "rgba(255,255,255,0.1)",
                position: "relative", transition: "background 0.2s",
              }}
            >
              <div style={{
                position: "absolute", top: 2, left: reminders ? 18 : 2,
                width: 16, height: 16, borderRadius: "50%", background: "white",
                transition: "left 0.2s",
              }} />
            </div>
            <span style={{ fontSize: 13, color: "#94A3B8" }}>Enable pre-stream reminders</span>
          </div>
        </div>

        {/* Actions */}
        <div style={{ display: "flex", gap: 10, marginTop: 24, justifyContent: "flex-end" }}>
          <button onClick={onClose} style={{
            padding: "8px 20px", borderRadius: 6, border: "1px solid rgba(255,255,255,0.12)",
            background: "transparent", color: "#94A3B8", cursor: "pointer", fontSize: 13,
          }}>Cancel</button>
          <button onClick={handleSave} style={{
            padding: "8px 24px", borderRadius: 6, border: "none",
            background: "linear-gradient(135deg, #4F9EFF, #7C3AED)",
            color: "white", cursor: "pointer", fontSize: 13, fontWeight: 600,
          }}>
            {isEdit ? "Save Changes" : "Schedule Event"}
          </button>
        </div>
      </div>
    </div>
  );
}

// ── Event Card ─────────────────────────────────────────────────────────────────
interface EventCardProps {
  event: ScheduledEvent;
  onStart: (ev: ScheduledEvent) => void;
  onEdit: (ev: ScheduledEvent) => void;
  onDelete: (id: string) => void;
  onToggleReminder: (id: string) => void;
}
function EventCard({ event, onStart, onEdit, onDelete, onToggleReminder }: EventCardProps) {
  const isUpcoming = event.status === "upcoming";
  const isLive = event.status === "live";
  const isCompleted = event.status === "completed";
  const platformColor = PLATFORM_COLORS[event.platform];
  const sportIcon = SPORT_ICONS[event.sport] ?? "🏆";

  const borderColor = isLive ? "#FF5A2C" : isCompleted ? "rgba(255,255,255,0.06)" : "rgba(255,255,255,0.1)";
  const bgColor = isLive ? "rgba(255,90,44,0.06)" : "rgba(255,255,255,0.02)";

  return (
    <div style={{
      background: bgColor,
      border: `1px solid ${borderColor}`,
      borderRadius: 10,
      padding: "16px 18px",
      display: "flex",
      gap: 16,
      alignItems: "flex-start",
      transition: "border-color 0.18s, background 0.18s",
      position: "relative",
      overflow: "hidden",
    }}>
      {/* Left accent bar */}
      <div style={{
        position: "absolute", left: 0, top: 0, bottom: 0, width: 3,
        background: isLive ? "#FF5A2C" : isCompleted ? "rgba(255,255,255,0.1)" : platformColor,
        borderRadius: "10px 0 0 10px",
      }} />

      {/* Sport icon */}
      <div style={{
        width: 44, height: 44, borderRadius: 8, flexShrink: 0,
        background: "rgba(255,255,255,0.06)",
        display: "flex", alignItems: "center", justifyContent: "center",
        fontSize: 22,
      }}>
        {sportIcon}
      </div>

      {/* Main info */}
      <div style={{ flex: 1, minWidth: 0 }}>
        <div style={{ display: "flex", alignItems: "flex-start", justifyContent: "space-between", gap: 8, marginBottom: 4 }}>
          <div>
            <div style={{
              fontSize: 15, fontWeight: 700, color: isCompleted ? "#64748B" : "#F1F5F9",
              fontFamily: "'DM Sans', sans-serif", marginBottom: 2,
              whiteSpace: "nowrap", overflow: "hidden", textOverflow: "ellipsis",
            }}>
              {event.title}
            </div>
            <div style={{ display: "flex", alignItems: "center", gap: 8, flexWrap: "wrap" }}>
              <span style={{
                fontSize: 11, fontWeight: 700, padding: "1px 7px", borderRadius: 4,
                background: `${platformColor}22`, color: platformColor, border: `1px solid ${platformColor}44`,
              }}>
                {event.platform}
              </span>
              {event.tags.slice(0, 2).map(tag => (
                <span key={tag} style={{
                  fontSize: 10, color: "#64748B", background: "rgba(255,255,255,0.05)",
                  padding: "1px 6px", borderRadius: 4, border: "1px solid rgba(255,255,255,0.08)",
                }}>
                  #{tag}
                </span>
              ))}
            </div>
          </div>
          {/* Countdown */}
          <div style={{ flexShrink: 0, textAlign: "right" }}>
            <CountdownTimer target={event.startTime} status={event.status} />
          </div>
        </div>

        {/* Meta row */}
        <div style={{ display: "flex", gap: 14, flexWrap: "wrap", marginTop: 8 }}>
          <div style={{ display: "flex", alignItems: "center", gap: 4, color: "#64748B", fontSize: 12 }}>
            <Calendar size={11} />
            <span>{formatEventTime(event.startTime)}</span>
          </div>
          <div style={{ display: "flex", alignItems: "center", gap: 4, color: "#64748B", fontSize: 12 }}>
            <Clock size={11} />
            <span>{formatDuration(event.durationMinutes)}</span>
          </div>
          {event.venue && (
            <div style={{ display: "flex", alignItems: "center", gap: 4, color: "#64748B", fontSize: 12 }}>
              <MapPin size={11} />
              <span>{event.venue}</span>
            </div>
          )}
          {event.estimatedViewers && (
            <div style={{ display: "flex", alignItems: "center", gap: 4, color: "#64748B", fontSize: 12 }}>
              <Users size={11} />
              <span>~{event.estimatedViewers.toLocaleString()} est.</span>
            </div>
          )}
        </div>
      </div>

      {/* Actions */}
      <div style={{ display: "flex", flexDirection: "column", gap: 6, flexShrink: 0, alignItems: "flex-end" }}>
        {isUpcoming && (
          <button
            onClick={() => onStart(event)}
            style={{
              display: "flex", alignItems: "center", gap: 6,
              padding: "7px 14px", borderRadius: 6, border: "none",
              background: "linear-gradient(135deg, #FF5A2C, #FF8C42)",
              color: "white", cursor: "pointer", fontSize: 12, fontWeight: 700,
              fontFamily: "'DM Sans', sans-serif",
              boxShadow: "0 0 12px rgba(255,90,44,0.4)",
              transition: "transform 0.1s, box-shadow 0.1s",
              whiteSpace: "nowrap",
            }}
            onMouseEnter={e => {
              (e.currentTarget as HTMLButtonElement).style.transform = "scale(1.03)";
              (e.currentTarget as HTMLButtonElement).style.boxShadow = "0 0 20px rgba(255,90,44,0.6)";
            }}
            onMouseLeave={e => {
              (e.currentTarget as HTMLButtonElement).style.transform = "scale(1)";
              (e.currentTarget as HTMLButtonElement).style.boxShadow = "0 0 12px rgba(255,90,44,0.4)";
            }}
          >
            <Play size={11} fill="white" />
            Start Stream
          </button>
        )}
        {isLive && (
          <div style={{
            display: "flex", alignItems: "center", gap: 6,
            padding: "7px 14px", borderRadius: 6,
            background: "rgba(255,90,44,0.15)", border: "1px solid rgba(255,90,44,0.4)",
            color: "#FF5A2C", fontSize: 12, fontWeight: 700,
          }}>
            <Radio size={11} />
            LIVE
          </div>
        )}
        {isCompleted && (
          <div style={{
            display: "flex", alignItems: "center", gap: 5,
            color: "#22C55E", fontSize: 12,
          }}>
            <CheckCircle2 size={13} />
            <span>Done</span>
          </div>
        )}
        <div style={{ display: "flex", gap: 4 }}>
          <button
            onClick={() => onToggleReminder(event.id)}
            title={event.remindersEnabled ? "Disable reminder" : "Enable reminder"}
            style={{
              width: 28, height: 28, borderRadius: 5, border: "1px solid rgba(255,255,255,0.1)",
              background: "transparent", cursor: "pointer",
              display: "flex", alignItems: "center", justifyContent: "center",
              color: event.remindersEnabled ? "#4F9EFF" : "#475569",
            }}
          >
            {event.remindersEnabled ? <Bell size={12} /> : <BellOff size={12} />}
          </button>
          {!isCompleted && (
            <button
              onClick={() => onEdit(event)}
              style={{
                width: 28, height: 28, borderRadius: 5, border: "1px solid rgba(255,255,255,0.1)",
                background: "transparent", cursor: "pointer",
                display: "flex", alignItems: "center", justifyContent: "center",
                color: "#64748B",
              }}
            >
              <Edit2 size={12} />
            </button>
          )}
          <button
            onClick={() => onDelete(event.id)}
            style={{
              width: 28, height: 28, borderRadius: 5, border: "1px solid rgba(255,255,255,0.1)",
              background: "transparent", cursor: "pointer",
              display: "flex", alignItems: "center", justifyContent: "center",
              color: "#64748B",
            }}
          >
            <Trash2 size={12} />
          </button>
        </div>
      </div>
    </div>
  );
}

// ── Next Up Banner ─────────────────────────────────────────────────────────────
function NextUpBanner({ event, onStart }: { event: ScheduledEvent; onStart: (ev: ScheduledEvent) => void }) {
  const [now, setNow] = useState(Date.now());
  useEffect(() => {
    const id = setInterval(() => setNow(Date.now()), 1000);
    return () => clearInterval(id);
  }, []);
  const { d, h, m, s } = formatCountdown(event.startTime.getTime() - now);
  const platformColor = PLATFORM_COLORS[event.platform];

  return (
    <div style={{
      background: "linear-gradient(135deg, rgba(79,158,255,0.12) 0%, rgba(124,58,237,0.12) 100%)",
      border: "1px solid rgba(79,158,255,0.3)",
      borderRadius: 10, padding: "16px 20px",
      display: "flex", alignItems: "center", gap: 20,
      marginBottom: 20,
    }}>
      <div style={{
        width: 44, height: 44, borderRadius: 8, flexShrink: 0,
        background: "rgba(79,158,255,0.15)", border: "1px solid rgba(79,158,255,0.3)",
        display: "flex", alignItems: "center", justifyContent: "center",
      }}>
        <Zap size={20} color="#4F9EFF" />
      </div>
      <div style={{ flex: 1 }}>
        <div style={{ fontSize: 10, color: "#4F9EFF", fontWeight: 700, letterSpacing: "0.1em", textTransform: "uppercase", marginBottom: 2 }}>
          Next Up
        </div>
        <div style={{ fontSize: 15, fontWeight: 700, color: "#F1F5F9", fontFamily: "'DM Sans', sans-serif" }}>
          {event.title}
        </div>
        <div style={{ display: "flex", gap: 12, marginTop: 4 }}>
          <span style={{ fontSize: 12, color: "#64748B" }}>{formatEventTime(event.startTime)}</span>
          <span style={{ fontSize: 12, color: platformColor }}>{event.platform}</span>
          <span style={{ fontSize: 12, color: "#64748B" }}>{event.venue}</span>
        </div>
      </div>
      {/* Big countdown */}
      <div style={{ textAlign: "center", flexShrink: 0 }}>
        <div style={{ display: "flex", gap: 8, alignItems: "center" }}>
          {d > 0 && (
            <div style={{ textAlign: "center" }}>
              <div style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 32, color: "#4F9EFF", lineHeight: 1 }}>{d}</div>
              <div style={{ fontSize: 9, color: "#475569", letterSpacing: "0.08em" }}>DAYS</div>
            </div>
          )}
          <div style={{ textAlign: "center" }}>
            <div style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 32, color: "#4F9EFF", lineHeight: 1 }}>{pad(h)}</div>
            <div style={{ fontSize: 9, color: "#475569", letterSpacing: "0.08em" }}>HRS</div>
          </div>
          <div style={{ color: "#4F9EFF", fontSize: 24, fontFamily: "'Bebas Neue', sans-serif", marginTop: -4 }}>:</div>
          <div style={{ textAlign: "center" }}>
            <div style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 32, color: "#4F9EFF", lineHeight: 1 }}>{pad(m)}</div>
            <div style={{ fontSize: 9, color: "#475569", letterSpacing: "0.08em" }}>MIN</div>
          </div>
          <div style={{ color: "#4F9EFF", fontSize: 24, fontFamily: "'Bebas Neue', sans-serif", marginTop: -4 }}>:</div>
          <div style={{ textAlign: "center" }}>
            <div style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 32, color: "#4F9EFF", lineHeight: 1 }}>{pad(s)}</div>
            <div style={{ fontSize: 9, color: "#475569", letterSpacing: "0.08em" }}>SEC</div>
          </div>
        </div>
      </div>
      <button
        onClick={() => onStart(event)}
        style={{
          display: "flex", alignItems: "center", gap: 8,
          padding: "10px 20px", borderRadius: 7, border: "none",
          background: "linear-gradient(135deg, #FF5A2C, #FF8C42)",
          color: "white", cursor: "pointer", fontSize: 13, fontWeight: 700,
          boxShadow: "0 0 16px rgba(255,90,44,0.45)",
          flexShrink: 0,
        }}
      >
        <Play size={13} fill="white" />
        Start Stream
      </button>
    </div>
  );
}

// ── Main Page ──────────────────────────────────────────────────────────────────
export default function SchedulePage() {
  const [events, setEvents] = useState<ScheduledEvent[]>(makeMockEvents);
  const [showModal, setShowModal] = useState(false);
  const [editingEvent, setEditingEvent] = useState<ScheduledEvent | null>(null);
  const [filter, setFilter] = useState<"all" | "upcoming" | "completed">("all");
  const [search, setSearch] = useState("");

  const upcoming = events.filter(e => e.status === "upcoming" || e.status === "live");
  const nextEvent = upcoming.sort((a, b) => a.startTime.getTime() - b.startTime.getTime())[0] ?? null;

  const filtered = events.filter(ev => {
    if (filter === "upcoming" && ev.status !== "upcoming" && ev.status !== "live") return false;
    if (filter === "completed" && ev.status !== "completed") return false;
    if (search && !ev.title.toLowerCase().includes(search.toLowerCase()) &&
        !ev.venue.toLowerCase().includes(search.toLowerCase())) return false;
    return true;
  }).sort((a, b) => a.startTime.getTime() - b.startTime.getTime());

  function handleStart(ev: ScheduledEvent) {
    toast.success(`Starting stream for "${ev.title}"…`, {
      description: `Launching Go Live flow for ${ev.platform}`,
      duration: 4000,
    });
  }

  function handleSave(ev: ScheduledEvent) {
    setEvents(prev => {
      const idx = prev.findIndex(e => e.id === ev.id);
      if (idx >= 0) { const next = [...prev]; next[idx] = ev; return next; }
      return [...prev, ev];
    });
    setShowModal(false);
    setEditingEvent(null);
    toast.success(editingEvent ? "Event updated." : "Event scheduled.");
  }

  function handleDelete(id: string) {
    setEvents(prev => prev.filter(e => e.id !== id));
    toast.info("Event removed from schedule.");
  }

  function handleEdit(ev: ScheduledEvent) {
    setEditingEvent(ev);
    setShowModal(true);
  }

  function handleToggleReminder(id: string) {
    setEvents(prev => prev.map(e => e.id === id ? { ...e, remindersEnabled: !e.remindersEnabled } : e));
  }

  const panelHeader = (label: string, accent: string) => (
    <div style={{
      height: 28, display: "flex", alignItems: "center", gap: 8,
      borderBottom: `1px solid rgba(255,255,255,0.07)`, marginBottom: 14,
      paddingBottom: 8,
    }}>
      <div style={{ width: 3, height: 14, borderRadius: 2, background: accent, flexShrink: 0 }} />
      <span style={{ fontSize: 10, fontWeight: 700, color: "#64748B", letterSpacing: "0.1em", textTransform: "uppercase" }}>
        {label}
      </span>
    </div>
  );

  return (
    <AppSidebar>
      <div style={{
        flex: 1, display: "flex", flexDirection: "column", height: "100vh",
        background: "#0F1623", overflow: "hidden",
      }}>
        {/* Top bar */}
        <div style={{
          height: 46, flexShrink: 0, display: "flex", alignItems: "center",
          justifyContent: "space-between", padding: "0 16px",
          background: "#111827", borderBottom: "1px solid rgba(255,255,255,0.07)",
        }}>
          <div style={{ display: "flex", alignItems: "center", gap: 10 }}>
            <span style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 18, letterSpacing: "0.12em", color: "#F1F5F9" }}>
              RAILSHOT <span style={{ color: "#FF5A2C" }}>TV</span>
            </span>
            <div style={{ width: 1, height: 16, background: "rgba(255,255,255,0.12)" }} />
            <span style={{ fontSize: 12, fontWeight: 600, color: "#94A3B8", letterSpacing: "0.06em" }}>
              SCHEDULE
            </span>
          </div>
          <div style={{ display: "flex", alignItems: "center", gap: 10 }}>
            <div style={{ fontSize: 11, color: "#4F9EFF", fontFamily: "'JetBrains Mono', monospace" }}>
              {upcoming.length} UPCOMING
            </div>
            <div style={{ width: 1, height: 14, background: "rgba(255,255,255,0.1)" }} />
            <div style={{ fontSize: 11, color: "#64748B", fontFamily: "'JetBrains Mono', monospace" }}>
              {events.filter(e => e.status === "completed").length} COMPLETED
            </div>
          </div>
        </div>

        {/* Body */}
        <div style={{ flex: 1, overflow: "auto", padding: 16 }}>
          {/* Next up banner */}
          {nextEvent && <NextUpBanner event={nextEvent} onStart={handleStart} />}

          {/* Controls row */}
          <div style={{ display: "flex", alignItems: "center", gap: 10, marginBottom: 16 }}>
            {/* Search */}
            <div style={{ position: "relative", flex: 1, maxWidth: 280 }}>
              <Search size={13} style={{ position: "absolute", left: 10, top: "50%", transform: "translateY(-50%)", color: "#475569" }} />
              <input
                value={search}
                onChange={e => setSearch(e.target.value)}
                placeholder="Search events…"
                style={{
                  width: "100%", padding: "7px 10px 7px 30px",
                  background: "rgba(255,255,255,0.05)", border: "1px solid rgba(255,255,255,0.1)",
                  borderRadius: 6, color: "#E2E8F0", fontSize: 12, outline: "none",
                  fontFamily: "'DM Sans', sans-serif",
                }}
              />
            </div>
            {/* Filter tabs */}
            {(["all", "upcoming", "completed"] as const).map(f => (
              <button
                key={f}
                onClick={() => setFilter(f)}
                style={{
                  padding: "6px 14px", borderRadius: 6, border: "1px solid",
                  borderColor: filter === f ? "rgba(79,158,255,0.4)" : "rgba(255,255,255,0.1)",
                  background: filter === f ? "rgba(79,158,255,0.12)" : "transparent",
                  color: filter === f ? "#4F9EFF" : "#64748B",
                  cursor: "pointer", fontSize: 12, fontWeight: 600,
                  textTransform: "capitalize",
                }}
              >
                {f}
              </button>
            ))}
            <div style={{ flex: 1 }} />
            {/* Add event */}
            <button
              onClick={() => { setEditingEvent(null); setShowModal(true); }}
              style={{
                display: "flex", alignItems: "center", gap: 6,
                padding: "7px 16px", borderRadius: 6, border: "none",
                background: "linear-gradient(135deg, #4F9EFF, #7C3AED)",
                color: "white", cursor: "pointer", fontSize: 12, fontWeight: 700,
                boxShadow: "0 0 12px rgba(79,158,255,0.3)",
              }}
            >
              <Plus size={13} />
              New Event
            </button>
          </div>

          {/* Stats row */}
          <div style={{ display: "grid", gridTemplateColumns: "repeat(4, 1fr)", gap: 10, marginBottom: 16 }}>
            {[
              { label: "Total Events", value: events.length, color: "#4F9EFF", icon: <Calendar size={14} /> },
              { label: "Upcoming", value: upcoming.length, color: "#FF5A2C", icon: <Clock size={14} /> },
              { label: "Est. Viewers", value: upcoming.reduce((s, e) => s + (e.estimatedViewers ?? 0), 0).toLocaleString(), color: "#22D3EE", icon: <Users size={14} /> },
              { label: "Completed", value: events.filter(e => e.status === "completed").length, color: "#22C55E", icon: <CheckCircle2 size={14} /> },
            ].map(stat => (
              <div key={stat.label} style={{
                background: "rgba(255,255,255,0.03)", border: "1px solid rgba(255,255,255,0.07)",
                borderRadius: 8, padding: "12px 14px",
                display: "flex", alignItems: "center", gap: 10,
              }}>
                <div style={{ color: stat.color, opacity: 0.8 }}>{stat.icon}</div>
                <div>
                  <div style={{ fontFamily: "'Bebas Neue', sans-serif", fontSize: 22, color: stat.color, lineHeight: 1 }}>
                    {stat.value}
                  </div>
                  <div style={{ fontSize: 10, color: "#475569", letterSpacing: "0.06em", textTransform: "uppercase", marginTop: 1 }}>
                    {stat.label}
                  </div>
                </div>
              </div>
            ))}
          </div>

          {/* Event list */}
          <div style={{ marginBottom: 8 }}>
            {panelHeader(`${filtered.length} Events`, "#4F9EFF")}
          </div>
          {filtered.length === 0 ? (
            <div style={{
              textAlign: "center", padding: "48px 0", color: "#475569",
              border: "1px dashed rgba(255,255,255,0.08)", borderRadius: 10,
            }}>
              <Calendar size={32} style={{ margin: "0 auto 12px", opacity: 0.3 }} />
              <div style={{ fontSize: 14, fontWeight: 600, marginBottom: 4 }}>No events found</div>
              <div style={{ fontSize: 12 }}>Schedule a new event to get started.</div>
            </div>
          ) : (
            <div style={{ display: "flex", flexDirection: "column", gap: 10 }}>
              {filtered.map(ev => (
                <EventCard
                  key={ev.id}
                  event={ev}
                  onStart={handleStart}
                  onEdit={handleEdit}
                  onDelete={handleDelete}
                  onToggleReminder={handleToggleReminder}
                />
              ))}
            </div>
          )}
        </div>
      </div>

      {/* Modal */}
      {showModal && (
        <EventModal
          event={editingEvent}
          onSave={handleSave}
          onClose={() => { setShowModal(false); setEditingEvent(null); }}
        />
      )}
    </AppSidebar>
  );
}
