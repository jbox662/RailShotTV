// RailShotTV — Chromatic Command
// AppSidebar: 56px icon-only rail, colored glows per section, cue-ball mark
import { useLocation, Link } from "wouter";
import { Tv2, Layers, MessageSquare, BarChart2, Settings } from "lucide-react";
import { Tooltip, TooltipContent, TooltipTrigger } from "@/components/ui/tooltip";

const NAV_ITEMS = [
  { path: "/",          icon: Tv2,           label: "Dashboard",    glow: "rgba(255,77,28,0.55)",  active: "#FF4D1C" },
  { path: "/scenes",    icon: Layers,         label: "Scene Editor", glow: "rgba(59,130,246,0.55)", active: "#3B82F6" },
  { path: "/chat",      icon: MessageSquare,  label: "Chat",         glow: "rgba(139,92,246,0.55)", active: "#8B5CF6" },
  { path: "/analytics", icon: BarChart2,      label: "Analytics",    glow: "rgba(6,182,212,0.55)",  active: "#06B6D4" },
  { path: "/settings",  icon: Settings,       label: "Settings",     glow: "rgba(100,100,140,0.4)", active: "#A0A0B8" },
];

export default function AppSidebar({ children }: { children: React.ReactNode }) {
  const [location] = useLocation();

  return (
    <div className="flex h-screen w-full overflow-hidden" style={{ background: "#0A0A0F" }}>
      {/* Icon-only rail */}
      <aside
        className="flex flex-col items-center shrink-0 h-full"
        style={{ width: 56, minWidth: 56, background: "#0D0D15", borderRight: "1px solid #1E1E2E" }}
      >
        {/* Logo mark */}
        <div
          className="flex items-center justify-center w-full shrink-0"
          style={{ height: 46, borderBottom: "1px solid #1E1E2E" }}
        >
          <div
            className="flex items-center justify-center rounded-sm"
            style={{
              width: 30, height: 30,
              background: "linear-gradient(135deg, #FF4D1C 0%, #FF7A4D 100%)",
              boxShadow: "0 0 16px rgba(255,77,28,0.55)"
            }}
          >
            <svg width="18" height="18" viewBox="0 0 18 18" fill="none">
              <circle cx="9" cy="9" r="7.5" fill="white" fillOpacity="0.95"/>
              <line x1="4.5" y1="13.5" x2="13.5" y2="4.5" stroke="#FF4D1C" strokeWidth="2.2" strokeLinecap="round"/>
            </svg>
          </div>
        </div>
        {/* Diagonal cue-angle accent line below logo */}
        <div style={{ width: 32, height: 1, background: "linear-gradient(90deg, transparent, #FF4D1C40, transparent)", marginTop: 0 }} />

        {/* Nav */}
        <nav className="flex flex-col items-center gap-1 w-full pt-3 flex-1 px-2">
          {NAV_ITEMS.map(({ path, icon: Icon, label, glow, active }) => {
            const isActive = location === path;
            return (
              <Tooltip key={path} delayDuration={200}>
                <TooltipTrigger asChild>
                  <Link href={path}>
                    <div
                      className="flex items-center justify-center rounded-md transition-all duration-150"
                      style={{
                        width: 38, height: 38,
                        background: isActive ? `${active}1A` : "transparent",
                        boxShadow: isActive ? `0 0 16px ${glow}` : "none",
                        border: isActive ? `1px solid ${active}35` : "1px solid transparent",
                      }}
                    >
                      <Icon size={17} style={{ color: isActive ? active : "#50506A", transition: "color 0.15s" }} />
                    </div>
                  </Link>
                </TooltipTrigger>
                <TooltipContent
                  side="right"
                  style={{ background: "#1A1A24", border: "1px solid #2A2A3A", color: "#F8F8FF", fontSize: 12, fontFamily: "'DM Sans', sans-serif" }}
                >
                  {label}
                </TooltipContent>
              </Tooltip>
            );
          })}
        </nav>

        {/* Bottom — signal bars + version */}
        <div className="flex flex-col items-center gap-2 pb-3">
          <div className="flex items-end gap-px">
            {[4, 6, 8, 10, 12].map((h, i) => (
              <div key={i} style={{ width: 3, height: h, background: i < 4 ? "#FF4D1C" : "#2A2A3A", borderRadius: 1, opacity: i < 4 ? 0.9 - i * 0.1 : 0.4 }} />
            ))}
          </div>
          <span style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 8, color: "#3A3A50", letterSpacing: "0.05em" }}>v2.5</span>
        </div>
      </aside>

      {/* Main content */}
      <main className="flex-1 overflow-hidden flex flex-col" style={{ background: "#0A0A0F" }}>
        {children}
      </main>
    </div>
  );
}
