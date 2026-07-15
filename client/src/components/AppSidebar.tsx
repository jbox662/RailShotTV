// RailShotTV — Chromatic Command — Bright Edition
// AppSidebar: 56px icon-only rail + children content area
import React from "react";
import { useLocation, Link } from "wouter";
import { Tv2, MessageSquare, BarChart2, Settings, Trophy, CalendarDays } from "lucide-react";
import { Tooltip, TooltipContent, TooltipTrigger } from "@/components/ui/tooltip";

const NAV_ITEMS = [
  { path: "/",           icon: Tv2,           label: "Dashboard",    glow: "rgba(255,90,44,0.65)",   active: "#FF5A2C" },
  { path: "/chat",       icon: MessageSquare, label: "Chat",         glow: "rgba(168,85,247,0.65)",  active: "#A855F7" },
  { path: "/analytics",  icon: BarChart2,     label: "Analytics",    glow: "rgba(34,211,238,0.65)",  active: "#22D3EE" },
  { path: "/scoreboard", icon: Trophy,        label: "Scoreboard",   glow: "rgba(34,197,94,0.65)",   active: "#22C55E" },
  { path: "/schedule",   icon: CalendarDays,  label: "Schedule",     glow: "rgba(250,204,21,0.65)",  active: "#FACC15" },
  { path: "/settings",   icon: Settings,      label: "Settings",     glow: "rgba(120,120,160,0.45)", active: "#94A3B8" },
];

type Props = { children?: React.ReactNode };

export default function AppSidebar({ children }: Props) {
  const [location] = useLocation();
  return (
    <div style={{ display: "flex", width: "100%", height: "100vh", overflow: "hidden" }}>
      {/* Icon rail */}
      <aside style={{
        width: 56, minWidth: 56, height: "100vh", flexShrink: 0,
        background: "#111827",
        borderRight: "1px solid rgba(255,255,255,0.08)",
        display: "flex", flexDirection: "column", alignItems: "center",
        paddingTop: 0, paddingBottom: 12, zIndex: 50,
      }}>
        {/* Logo mark */}
        <div style={{
          width: 56, height: 46, display: "flex", alignItems: "center", justifyContent: "center",
          borderBottom: "1px solid rgba(255,255,255,0.08)", marginBottom: 8, flexShrink: 0,
        }}>
          <div style={{
            width: 30, height: 30, borderRadius: "50%",
            background: "linear-gradient(135deg, #FF5A2C 0%, #FF8C42 100%)",
            display: "flex", alignItems: "center", justifyContent: "center",
            boxShadow: "0 0 18px rgba(255,90,44,0.55)",
          }}>
            <svg width="16" height="16" viewBox="0 0 16 16" fill="none">
              <circle cx="8" cy="8" r="3" fill="white" opacity="0.9"/>
              <line x1="1" y1="8" x2="6" y2="8" stroke="white" strokeWidth="1.5" strokeLinecap="round" opacity="0.7"/>
              <line x1="10" y1="8" x2="15" y2="8" stroke="white" strokeWidth="1.5" strokeLinecap="round" opacity="0.7"/>
              <line x1="8" y1="1" x2="8" y2="6" stroke="white" strokeWidth="1.5" strokeLinecap="round" opacity="0.7"/>
              <line x1="8" y1="10" x2="8" y2="15" stroke="white" strokeWidth="1.5" strokeLinecap="round" opacity="0.7"/>
            </svg>
          </div>
        </div>

        {/* Nav items */}
        <nav style={{ display: "flex", flexDirection: "column", gap: 4, flex: 1, width: "100%", alignItems: "center", paddingTop: 4 }}>
          {NAV_ITEMS.map(({ path, icon: Icon, label, glow, active }) => {
            const isActive = location === path;
            return (
              <Tooltip key={path} delayDuration={300}>
                <TooltipTrigger asChild>
                  <Link href={path}>
                    <div
                      style={{
                        width: 40, height: 40, borderRadius: 8,
                        display: "flex", alignItems: "center", justifyContent: "center",
                        background: isActive ? `${active}22` : "transparent",
                        border: isActive ? `1px solid ${active}55` : "1px solid transparent",
                        boxShadow: isActive ? `0 0 16px ${glow}` : "none",
                        transition: "all 0.18s cubic-bezier(0.23,1,0.32,1)",
                        cursor: "pointer",
                      }}
                      onMouseEnter={e => {
                        if (!isActive) {
                          (e.currentTarget as HTMLDivElement).style.background = "rgba(255,255,255,0.06)";
                          (e.currentTarget as HTMLDivElement).style.border = "1px solid rgba(255,255,255,0.1)";
                        }
                      }}
                      onMouseLeave={e => {
                        if (!isActive) {
                          (e.currentTarget as HTMLDivElement).style.background = "transparent";
                          (e.currentTarget as HTMLDivElement).style.border = "1px solid transparent";
                        }
                      }}
                    >
                      <Icon size={18} color={isActive ? active : "#6B7280"} strokeWidth={isActive ? 2.2 : 1.8} />
                    </div>
                  </Link>
                </TooltipTrigger>
                <TooltipContent side="right" style={{ background: "#1E2A3A", border: "1px solid rgba(255,255,255,0.12)", color: "#E2E8F0", fontSize: 12 }}>
                  {label}
                </TooltipContent>
              </Tooltip>
            );
          })}
        </nav>

        {/* Signal strength at bottom */}
        <div style={{ display: "flex", flexDirection: "column", alignItems: "center", gap: 2, paddingBottom: 4 }}>
          <div style={{ display: "flex", alignItems: "flex-end", gap: 2 }}>
            {[3, 5, 7, 9, 11].map((h, i) => (
              <div key={i} style={{ width: 3, height: h, borderRadius: 1.5, background: i < 4 ? "#22C55E" : "#2D3748" }} />
            ))}
          </div>
          <span style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 8, color: "#4B5563", letterSpacing: "0.05em" }}>v2.5</span>
        </div>
      </aside>

      {/* Page content */}
      <div style={{ flex: 1, minWidth: 0, height: "100vh", overflow: "hidden" }}>
        {children}
      </div>
    </div>
  );
}
