/**
 * Nexus Broadcast — Obsidian Studio Dark Theme
 * Persistent left sidebar with navigation icons + labels (220px)
 * Active item: blue left-border (3px) + #1E2130 background
 * Font: Inter for labels, Space Grotesk for brand name
 */
import { Link, useLocation } from "wouter";
import {
  LayoutDashboard,
  Layers,
  Settings,
  BarChart2,
  MessageSquare,
  Radio,
  Wifi,
  WifiOff,
} from "lucide-react";
import { cn } from "@/lib/utils";

const navItems = [
  { label: "Dashboard", icon: LayoutDashboard, href: "/" },
  { label: "Scene Editor", icon: Layers, href: "/scenes" },
  { label: "Chat & Audience", icon: MessageSquare, href: "/chat" },
  { label: "Analytics", icon: BarChart2, href: "/analytics" },
  { label: "Settings", icon: Settings, href: "/settings" },
];

export default function AppSidebar({ children }: { children: React.ReactNode }) {
  const [location] = useLocation();

  return (
    <div className="flex h-screen w-full overflow-hidden" style={{ background: "#0E0F14" }}>
      {/* Sidebar */}
      <aside
        className="flex flex-col shrink-0 h-full border-r"
        style={{ width: 220, background: "#111318", borderColor: "rgba(255,255,255,0.07)" }}
      >
        {/* Logo */}
        <div className="flex items-center gap-2.5 px-4 py-4 border-b" style={{ borderColor: "rgba(255,255,255,0.07)", minHeight: 56 }}>
          <div className="flex items-center justify-center rounded" style={{ width: 28, height: 28, background: "#3B82F6" }}>
            <Radio size={15} color="white" />
          </div>
          <div>
            <div style={{ fontFamily: "'Space Grotesk', sans-serif", fontWeight: 700, fontSize: 13, color: "#fff", letterSpacing: "0.04em" }}>
              NEXUS
            </div>
            <div style={{ fontFamily: "'Inter', sans-serif", fontWeight: 400, fontSize: 9, color: "rgba(255,255,255,0.4)", letterSpacing: "0.12em", marginTop: -1 }}>
              BROADCAST
            </div>
          </div>
          <div className="ml-auto flex items-center gap-1">
            <div className="rounded px-1.5 py-0.5" style={{ background: "#3B82F6", fontSize: 9, fontWeight: 700, color: "#fff", letterSpacing: "0.06em" }}>
              PRO
            </div>
          </div>
        </div>

        {/* Nav */}
        <nav className="flex-1 py-2 overflow-y-auto">
          {navItems.map((item) => {
            const active = location === item.href || (item.href !== "/" && location.startsWith(item.href));
            return (
              <Link key={item.href} href={item.href}>
                <div
                  className={cn(
                    "flex items-center gap-3 px-4 py-2.5 cursor-pointer transition-all duration-150 relative",
                    active ? "text-white" : "text-white/50 hover:text-white/80"
                  )}
                  style={active ? { background: "#1A1D2B", borderLeft: "3px solid #3B82F6", paddingLeft: 13 } : { borderLeft: "3px solid transparent" }}
                >
                  <item.icon size={16} strokeWidth={active ? 2 : 1.5} />
                  <span style={{ fontFamily: "'Inter', sans-serif", fontSize: 13, fontWeight: active ? 500 : 400 }}>
                    {item.label}
                  </span>
                </div>
              </Link>
            );
          })}
        </nav>

        {/* Connection status */}
        <div className="px-4 py-3 border-t" style={{ borderColor: "rgba(255,255,255,0.07)" }}>
          <div className="flex items-center gap-2">
            <Wifi size={13} color="#22C55E" />
            <span style={{ fontFamily: "'Inter', sans-serif", fontSize: 11, color: "#22C55E", fontWeight: 500 }}>Connected · Excellent</span>
          </div>
          <div className="flex items-center gap-2 mt-1">
            <div className="w-2 h-2 rounded-full animate-pulse" style={{ background: "#EF4444" }} />
            <span style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 11, color: "rgba(255,255,255,0.5)" }}>LIVE · 01:23:47</span>
          </div>
          <div className="mt-2 pt-2" style={{ borderTop: "1px solid rgba(255,255,255,0.06)" }}>
            <div className="flex items-center gap-1.5">
              <div className="flex gap-px">
                {[1,2,3,4,5].map(i => (
                  <div key={i} style={{ width: 3, height: 8 + i * 2, background: i <= 4 ? "#3B82F6" : "rgba(255,255,255,0.1)", borderRadius: 1 }} />
                ))}
              </div>
              <span style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: 9, color: "rgba(255,255,255,0.3)" }}>NEXUS v2.4.1</span>
            </div>
          </div>
        </div>
      </aside>

      {/* Main content */}
      <main className="flex-1 overflow-hidden flex flex-col" style={{ background: "#0E0F14" }}>
        {children}
      </main>
    </div>
  );
}
