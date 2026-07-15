import { createContext, useContext, useState, useCallback, useEffect, ReactNode } from "react";
import {
  Monitor, Camera, Globe, Type, ImageIcon, Bell, Trophy, AlignLeft,
  Layers, LayoutGrid, Mic, Video, Music, Tv,
} from "lucide-react";

// ── Icon registry (serialisable key → component) ──────────────────────────────
const ICON_MAP: Record<string, React.ElementType> = {
  Monitor, Camera, Globe, Type, ImageIcon, Bell, Trophy, AlignLeft,
  Layers, LayoutGrid, Mic, Video, Music, Tv,
};
const iconKey = (icon: React.ElementType): string =>
  Object.keys(ICON_MAP).find(k => ICON_MAP[k] === icon) ?? "Layers";
const iconFromKey = (key: string): React.ElementType => ICON_MAP[key] ?? Layers;

// ── Types ─────────────────────────────────────────────────────────────────────
export type SourceItem = {
  id: number;
  name: string;
  type: string;
  icon: React.ElementType;
  iconKey: string;
  color: string;
  visible: boolean;
  locked: boolean;
  settings: Record<string, string | number | boolean>;
};

export type SceneItem = {
  id: number;
  name: string;
  sources: SourceItem[];
};

// ── Serialisation helpers ─────────────────────────────────────────────────────
type SerialSource = Omit<SourceItem, "icon"> & { iconKey: string };
type SerialScene  = { id: number; name: string; sources: SerialSource[] };

function serialiseScenes(scenes: SceneItem[]): SerialScene[] {
  return scenes.map(s => ({
    ...s,
    sources: s.sources.map(src => {
      const { icon, ...rest } = src;
      return { ...rest, iconKey: iconKey(icon) };
    }),
  }));
}

function deserialiseScenes(raw: SerialScene[]): SceneItem[] {
  return raw.map(s => ({
    ...s,
    sources: s.sources.map(src => ({
      ...src,
      icon: iconFromKey(src.iconKey),
    })),
  }));
}

const LS_KEY = "railshot_scenes_v2";

function loadFromStorage(): { scenes: SceneItem[]; activeId: number | null; nextId: number } {
  try {
    const raw = localStorage.getItem(LS_KEY);
    if (!raw) return { scenes: [], activeId: null, nextId: 1 };
    const parsed = JSON.parse(raw) as { scenes: SerialScene[]; activeId: number | null; nextId: number };
    return {
      scenes: deserialiseScenes(parsed.scenes ?? []),
      activeId: parsed.activeId ?? null,
      nextId: parsed.nextId ?? 1,
    };
  } catch {
    return { scenes: [], activeId: null, nextId: 1 };
  }
}

// ── Context shape ─────────────────────────────────────────────────────────────
type SceneContextType = {
  scenes: SceneItem[];
  activeSceneId: number | null;
  editingSceneId: number | null;
  nextSceneId: number;

  setActiveSceneId: (id: number | null) => void;
  setEditingSceneId: (id: number | null) => void;

  addScene: () => void;
  duplicateScene: (scene: SceneItem) => void;
  deleteScene: (id: number) => void;
  renameScene: (id: number, name: string) => void;

  addSource: (sceneId: number, source: Omit<SourceItem, "id">) => number;
  removeSource: (sceneId: number, sourceId: number) => void;
  updateSource: (sceneId: number, sourceId: number, patch: Partial<SourceItem>) => void;
  moveSource: (sceneId: number, sourceId: number, dir: "up" | "down") => void;
  updateSourceSettings: (sceneId: number, sourceId: number, key: string, value: string | number | boolean) => void;
};

const SceneContext = createContext<SceneContextType | null>(null);

export function useScenes() {
  const ctx = useContext(SceneContext);
  if (!ctx) throw new Error("useScenes must be used inside SceneProvider");
  return ctx;
}

// ── Provider ──────────────────────────────────────────────────────────────────
let _nextId = 1;
const freshId = () => _nextId++;

export function SceneProvider({ children }: { children: ReactNode }) {
  const initial = loadFromStorage();
  // Sync _nextId with stored value so IDs don't collide after reload
  if (initial.nextId > _nextId) _nextId = initial.nextId;

  const [scenes, setScenes] = useState<SceneItem[]>(initial.scenes);
  const [activeSceneId, setActiveSceneId] = useState<number | null>(initial.activeId);
  const [editingSceneId, setEditingSceneId] = useState<number | null>(null);
  const [nextSceneId, setNextSceneId] = useState(initial.nextId);

  // ── Persist to localStorage on every change ─────────────────────────────────
  useEffect(() => {
    try {
      localStorage.setItem(LS_KEY, JSON.stringify({
        scenes: serialiseScenes(scenes),
        activeId: activeSceneId,
        nextId: nextSceneId,
      }));
    } catch { /* quota exceeded — silently ignore */ }
  }, [scenes, activeSceneId, nextSceneId]);

  // ── Scene CRUD ──────────────────────────────────────────────────────────────
  const addScene = useCallback(() => {
    const id = nextSceneId;
    setScenes(prev => [...prev, { id, name: `Scene ${id}`, sources: [] }]);
    setNextSceneId(id + 1);
    setActiveSceneId(id);
  }, [nextSceneId]);

  const duplicateScene = useCallback((scene: SceneItem) => {
    const id = nextSceneId;
    setScenes(prev => [...prev, { ...scene, id, name: `${scene.name} (copy)`, sources: scene.sources.map(s => ({ ...s, id: freshId() })) }]);
    setNextSceneId(id + 1);
    setActiveSceneId(id);
  }, [nextSceneId]);

  const deleteScene = useCallback((id: number) => {
    setScenes(prev => prev.filter(s => s.id !== id));
    setActiveSceneId(prev => (prev === id ? null : prev));
    setEditingSceneId(prev => (prev === id ? null : prev));
  }, []);

  const renameScene = useCallback((id: number, name: string) => {
    setScenes(prev => prev.map(s => s.id === id ? { ...s, name } : s));
  }, []);

  // ── Source CRUD (per scene) ─────────────────────────────────────────────────
  const addSource = useCallback((sceneId: number, source: Omit<SourceItem, "id">) => {
    const id = freshId();
    setScenes(prev => prev.map(s => s.id === sceneId
      ? { ...s, sources: [...s.sources, { ...source, id }] }
      : s
    ));
    return id;
  }, []);

  const removeSource = useCallback((sceneId: number, sourceId: number) => {
    setScenes(prev => prev.map(s => s.id === sceneId
      ? { ...s, sources: s.sources.filter(src => src.id !== sourceId) }
      : s
    ));
  }, []);

  const updateSource = useCallback((sceneId: number, sourceId: number, patch: Partial<SourceItem>) => {
    setScenes(prev => prev.map(s => s.id === sceneId
      ? { ...s, sources: s.sources.map(src => src.id === sourceId ? { ...src, ...patch } : src) }
      : s
    ));
  }, []);

  const moveSource = useCallback((sceneId: number, sourceId: number, dir: "up" | "down") => {
    setScenes(prev => prev.map(s => {
      if (s.id !== sceneId) return s;
      const idx = s.sources.findIndex(src => src.id === sourceId);
      if (idx < 0) return s;
      const next = [...s.sources];
      const swapIdx = dir === "up" ? idx - 1 : idx + 1;
      if (swapIdx < 0 || swapIdx >= next.length) return s;
      [next[idx], next[swapIdx]] = [next[swapIdx], next[idx]];
      return { ...s, sources: next };
    }));
  }, []);

  const updateSourceSettings = useCallback((sceneId: number, sourceId: number, key: string, value: string | number | boolean) => {
    setScenes(prev => prev.map(s => s.id === sceneId
      ? { ...s, sources: s.sources.map(src => src.id === sourceId ? { ...src, settings: { ...src.settings, [key]: value } } : src) }
      : s
    ));
  }, []);

  return (
    <SceneContext.Provider value={{
      scenes, activeSceneId, editingSceneId, nextSceneId,
      setActiveSceneId, setEditingSceneId,
      addScene, duplicateScene, deleteScene, renameScene,
      addSource, removeSource, updateSource, moveSource, updateSourceSettings,
    }}>
      {children}
    </SceneContext.Provider>
  );
}
