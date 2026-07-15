import { Toaster } from "@/components/ui/sonner";
import { TooltipProvider } from "@/components/ui/tooltip";
import NotFound from "@/pages/NotFound";
import { Route, Switch } from "wouter";
import ErrorBoundary from "./components/ErrorBoundary";
import { ThemeProvider } from "./contexts/ThemeContext";
import { SceneProvider } from "./contexts/SceneContext";
import Dashboard from "./pages/Dashboard";
import SceneEditor from "./pages/SceneEditor";
import ChatPanel from "./pages/ChatPanel";
import Analytics from "./pages/Analytics";
import Settings from "./pages/Settings";
import ScoreboardPage from "./pages/ScoreboardPage";
import SchedulePage from "./pages/SchedulePage";
function Router() {
  return (
    <Switch>
      <Route path="/" component={Dashboard} />
      <Route path="/scenes" component={SceneEditor} />
      <Route path="/chat" component={ChatPanel} />
      <Route path="/analytics" component={Analytics} />
      <Route path="/settings" component={Settings} />
      <Route path="/scoreboard" component={ScoreboardPage} />
      <Route path="/schedule" component={SchedulePage} />
      <Route path="/404" component={NotFound} />
      <Route component={NotFound} />
    </Switch>
  );
}

export default function App() {
  return (
    <ErrorBoundary>
      <ThemeProvider defaultTheme="dark">
        <SceneProvider>
        <TooltipProvider>
          <Toaster />
          <Router />
        </TooltipProvider>
        </SceneProvider>
      </ThemeProvider>
    </ErrorBoundary>
  );
}
