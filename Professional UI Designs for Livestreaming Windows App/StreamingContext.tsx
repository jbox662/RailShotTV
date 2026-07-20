import { createContext, useContext, useState, useCallback, ReactNode } from "react";

const LS_KEY = "railshot_streaming_state";

type StreamingContextType = {
  isLive: boolean;
  setIsLive: (v: boolean) => void;
  toggleLive: () => void;
};

const StreamingContext = createContext<StreamingContextType | null>(null);

export function useStreaming() {
  const ctx = useContext(StreamingContext);
  if (!ctx) throw new Error("useStreaming must be used inside StreamingProvider");
  return ctx;
}

export function StreamingProvider({ children }: { children: ReactNode }) {
  const [isLive, setIsLiveState] = useState<boolean>(() => {
    try {
      const raw = localStorage.getItem(LS_KEY);
      return raw ? JSON.parse(raw) : false;
    } catch {
      return false;
    }
  });

  const setIsLive = useCallback((v: boolean) => {
    setIsLiveState(v);
    try { localStorage.setItem(LS_KEY, JSON.stringify(v)); } catch { /* quota */ }
  }, []);

  const toggleLive = useCallback(() => {
    setIsLive(!isLive);
  }, [isLive, setIsLive]);

  return (
    <StreamingContext.Provider value={{ isLive, setIsLive, toggleLive }}>
      {children}
    </StreamingContext.Provider>
  );
}
