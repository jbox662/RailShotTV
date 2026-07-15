import { createContext, useContext, useState, useCallback, ReactNode } from "react";

// ── Types ─────────────────────────────────────────────────────────────────────
export type SourceItem = {
  id: number;
  name: string;
  type: string;
  icon: React.ElementType;
  color: string;
  visible: boolean;
  locked: boolean;
};

export type SceneItem = {
  id: number;
  name: string;
  sources: SourceItem[];
};

// ── Context shape ─────────────────────────────────────────────────────────────
type SceneContextType = {
  scenes: SceneItem[];
  activeSceneId: number | null;
  editingSceneId: number | null;   // scene currently open in Scene Editor
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
  const [scenes, setScenes] = useState<SceneItem[]>([]);
  const [activeSceneId, setActiveSceneId] = useState<number | null>(null);
  const [editingSceneId, setEditingSceneId] = useState<number | null>(null);
  const [nextSceneId, setNextSceneId] = useState(1);

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

  return (
    <SceneContext.Provider value={{
      scenes, activeSceneId, editingSceneId, nextSceneId,
      setActiveSceneId, setEditingSceneId,
      addScene, duplicateScene, deleteScene, renameScene,
      addSource, removeSource, updateSource, moveSource,
    }}>
      {children}
    </SceneContext.Provider>
  );
}
