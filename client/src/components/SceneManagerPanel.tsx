// SceneManagerPanel — vMix-style vertical scene list
// Chromatic Command design system: dark #0F1114 bg, accent #4F9EFF, danger #EF4444
import { useState, useRef, useCallback } from "react";
import { Plus, Copy, Trash2, GripVertical, Pencil, ChevronUp, ChevronDown } from "lucide-react";
import { ContextMenu, ContextMenuTrigger, ContextMenuContent, ContextMenuItem, ContextMenuSeparator, ContextMenuLabel } from "@/components/ui/context-menu";
import type { SceneItem } from "@/contexts/SceneContext";

interface Props {
  scenes: SceneItem[];
  activeSceneId: number | null;
  previewSceneId: number | null;
  programSceneId: number | null;
  onSelect: (id: number) => void;
  onAdd: () => void;
  onDuplicate: (scene: SceneItem) => void;
  onDelete: (id: number) => void;
  onRename: (id: number, name: string) => void;
  onReorder: (fromIdx: number, toIdx: number) => void;
  onMoveUp: (idx: number) => void;
  onMoveDown: (idx: number) => void;
}

export default function SceneManagerPanel({
  scenes, activeSceneId, previewSceneId, programSceneId,
  onSelect, onAdd, onDuplicate, onDelete, onRename, onReorder, onMoveUp, onMoveDown,
}: Props) {
  const [renamingId, setRenamingId] = useState<number | null>(null);
  const [renameValue, setRenameValue] = useState("");
  const inputRef = useRef<HTMLInputElement>(null);

  // Drag-to-reorder state
  const dragIdx = useRef<number | null>(null);
  const [dragOverIdx, setDragOverIdx] = useState<number | null>(null);

  const startRename = useCallback((scene: SceneItem) => {
    setRenamingId(scene.id);
    setRenameValue(scene.name);
    setTimeout(() => inputRef.current?.select(), 30);
  }, []);

  const commitRename = useCallback(() => {
    if (renamingId !== null && renameValue.trim()) {
      onRename(renamingId, renameValue.trim());
    }
    setRenamingId(null);
  }, [renamingId, renameValue, onRename]);

  const S = {
    panel: {
      width: 180,
      display: "flex" as const,
      flexDirection: "column" as const,
      background: "#0A0C0F",
      borderRight: "1px solid #2A2D35",
      flexShrink: 0,
      overflow: "hidden",
    },
    header: {
      display: "flex" as const,
      alignItems: "center" as const,
      justifyContent: "space-between" as const,
      padding: "5px 8px",
      background: "linear-gradient(180deg,#1A1D22,#141619)",
      borderBottom: "1px solid #2A2D35",
      flexShrink: 0,
    },
    headerLabel: {
      fontFamily: "'DM Sans',sans-serif",
      fontSize: 9,
      fontWeight: 700,
      color: "#4F9EFF",
      letterSpacing: "0.12em",
      textTransform: "uppercase" as const,
    },
    addBtn: {
      width: 20,
      height: 20,
      padding: 0,
      background: "linear-gradient(180deg,#1A3AFF,#1230CC)",
      border: "1px solid #3A6AFF",
      borderRadius: 3,
      color: "#fff",
      cursor: "pointer" as const,
      display: "flex" as const,
      alignItems: "center" as const,
      justifyContent: "center" as const,
      boxShadow: "0 0 8px rgba(26,106,255,0.4)",
    },
    list: {
      flex: 1,
      overflowY: "auto" as const,
      scrollbarWidth: "thin" as const,
      scrollbarColor: "#2A2D35 transparent",
    },
    emptyHint: {
      padding: "16px 8px",
      textAlign: "center" as const,
      fontFamily: "'DM Sans',sans-serif",
      fontSize: 10,
      color: "#404450",
      lineHeight: 1.5,
    },
  };

  return (
    <div style={S.panel}>
      {/* Header */}
      <div style={S.header}>
        <span style={S.headerLabel}>Scenes</span>
        <button style={S.addBtn} onClick={onAdd} title="Add Scene">
          <Plus size={12} />
        </button>
      </div>

      {/* Scene list */}
      <div style={S.list}>
        {scenes.length === 0 ? (
          <div style={S.emptyHint}>
            No scenes yet.<br />Click + to add one.
          </div>
        ) : scenes.map((scene, idx) => {
          const isActive  = scene.id === activeSceneId;
          const isPreview = scene.id === previewSceneId;
          const isProgram = scene.id === programSceneId;
          const isDragOver = dragOverIdx === idx;

          const rowBg = isProgram
            ? "linear-gradient(90deg,#2A0A0A,#1A0808)"
            : isPreview
            ? "linear-gradient(90deg,#0A1A0A,#081208)"
            : isActive
            ? "linear-gradient(90deg,#0D1A2A,#0A1220)"
            : "none";

          const accentColor = isProgram ? "#FF5A2C" : isPreview ? "#22C55E" : isActive ? "#4F9EFF" : "transparent";

          return (
            <ContextMenu key={scene.id}>
              <ContextMenuTrigger asChild>
                <div
                  draggable
                  onDragStart={() => { dragIdx.current = idx; }}
                  onDragOver={e => { e.preventDefault(); setDragOverIdx(idx); }}
                  onDragLeave={() => setDragOverIdx(null)}
                  onDrop={() => {
                    if (dragIdx.current !== null && dragIdx.current !== idx) {
                      onReorder(dragIdx.current, idx);
                    }
                    dragIdx.current = null;
                    setDragOverIdx(null);
                  }}
                  onDragEnd={() => { dragIdx.current = null; setDragOverIdx(null); }}
                  onClick={() => { if (renamingId === null) onSelect(scene.id); }}
                  onDoubleClick={() => startRename(scene)}
                  style={{
                    display: "flex",
                    alignItems: "center",
                    gap: 4,
                    padding: "6px 6px 6px 0",
                    background: rowBg,
                    borderLeft: `3px solid ${accentColor}`,
                    borderBottom: isDragOver ? "2px solid #4F9EFF" : "1px solid #1A1D22",
                    cursor: "pointer",
                    userSelect: "none" as const,
                    transition: "background 0.1s",
                    position: "relative" as const,
                  }}
                >
                  {/* Drag handle */}
                  <div style={{ color: "#404450", cursor: "grab", display: "flex", flexShrink: 0, paddingLeft: 4 }}>
                    <GripVertical size={10} />
                  </div>

                  {/* Scene number badge */}
                  <div style={{
                    width: 18, height: 18, borderRadius: 2, flexShrink: 0,
                    background: isProgram ? "#FF5A2C" : isPreview ? "#22C55E" : isActive ? "#4F9EFF20" : "#1A1D22",
                    border: `1px solid ${isProgram ? "#FF5A2C80" : isPreview ? "#22C55E80" : isActive ? "#4F9EFF60" : "#2A2D35"}`,
                    display: "flex", alignItems: "center", justifyContent: "center",
                    fontFamily: "'JetBrains Mono',monospace", fontSize: 9, fontWeight: 700,
                    color: isProgram || isPreview ? "#000" : isActive ? "#4F9EFF" : "#606878",
                  }}>
                    {idx + 1}
                  </div>

                  {/* Scene name / rename input */}
                  <div style={{ flex: 1, minWidth: 0 }}>
                    {renamingId === scene.id ? (
                      <input
                        ref={inputRef}
                        value={renameValue}
                        onChange={e => setRenameValue(e.target.value)}
                        onBlur={commitRename}
                        onKeyDown={e => {
                          if (e.key === "Enter") commitRename();
                          if (e.key === "Escape") setRenamingId(null);
                          e.stopPropagation();
                        }}
                        onClick={e => e.stopPropagation()}
                        autoFocus
                        style={{
                          width: "100%",
                          background: "#0D1017",
                          border: "1px solid #4F9EFF",
                          borderRadius: 2,
                          color: "#E0E2E8",
                          fontFamily: "'DM Sans',sans-serif",
                          fontSize: 11,
                          padding: "1px 4px",
                          outline: "none",
                        }}
                      />
                    ) : (
                      <span style={{
                        fontFamily: "'DM Sans',sans-serif",
                        fontSize: 11,
                        fontWeight: isActive ? 700 : 400,
                        color: isProgram ? "#FF8A6A" : isPreview ? "#6EE7A0" : isActive ? "#C8DAFF" : "#8892A4",
                        overflow: "hidden",
                        textOverflow: "ellipsis",
                        whiteSpace: "nowrap",
                        display: "block",
                      }}>
                        {scene.name}
                      </span>
                    )}
                  </div>

                  {/* Source count badge */}
                  {scene.sources.length > 0 && renamingId !== scene.id && (
                    <div style={{
                      flexShrink: 0,
                      padding: "0 4px",
                      background: "#1A2030",
                      border: "1px solid #2A3050",
                      borderRadius: 8,
                      fontFamily: "'JetBrains Mono',monospace",
                      fontSize: 8,
                      color: "#4F9EFF80",
                      marginRight: 4,
                    }}>
                      {scene.sources.length}
                    </div>
                  )}

                  {/* Up/Down arrows — show on hover via CSS trick with inline state */}
                  <div style={{ display: "flex", flexDirection: "column", gap: 0, flexShrink: 0, marginRight: 2, opacity: 0.4 }}
                    onMouseEnter={e => (e.currentTarget.style.opacity = "1")}
                    onMouseLeave={e => (e.currentTarget.style.opacity = "0.4")}>
                    <button
                      onClick={e => { e.stopPropagation(); onMoveUp(idx); }}
                      disabled={idx === 0}
                      style={{ background: "none", border: "none", padding: 0, cursor: idx === 0 ? "default" : "pointer", color: "#606878", display: "flex", lineHeight: 1 }}>
                      <ChevronUp size={9} />
                    </button>
                    <button
                      onClick={e => { e.stopPropagation(); onMoveDown(idx); }}
                      disabled={idx === scenes.length - 1}
                      style={{ background: "none", border: "none", padding: 0, cursor: idx === scenes.length - 1 ? "default" : "pointer", color: "#606878", display: "flex", lineHeight: 1 }}>
                      <ChevronDown size={9} />
                    </button>
                  </div>
                </div>
              </ContextMenuTrigger>

              {/* Right-click context menu */}
              <ContextMenuContent style={{ minWidth: 170, background: "#1A1D22", border: "1px solid #3A3D45", borderRadius: 4, padding: "4px 0", boxShadow: "0 8px 32px rgba(0,0,0,0.7)" }}>
                <ContextMenuLabel style={{ padding: "3px 10px", fontSize: 9, color: "#606878", fontFamily: "'DM Sans',sans-serif", letterSpacing: "0.08em", textTransform: "uppercase" }}>
                  {scene.name}
                </ContextMenuLabel>
                <ContextMenuSeparator style={{ height: 1, background: "#2A2D35", margin: "2px 0" }} />
                <ContextMenuItem onSelect={() => onSelect(scene.id)}
                  style={{ padding: "5px 10px", fontSize: 11, color: "#4F9EFF", fontFamily: "'DM Sans',sans-serif", cursor: "pointer" }}>
                  Set as Active
                </ContextMenuItem>
                <ContextMenuSeparator style={{ height: 1, background: "#2A2D35", margin: "2px 0" }} />
                <ContextMenuItem onSelect={() => startRename(scene)}
                  style={{ padding: "5px 10px", fontSize: 11, color: "#D0D2D8", fontFamily: "'DM Sans',sans-serif", cursor: "pointer", display: "flex", alignItems: "center", gap: 8 }}>
                  <Pencil size={11} /> Rename
                </ContextMenuItem>
                <ContextMenuItem onSelect={() => onDuplicate(scene)}
                  style={{ padding: "5px 10px", fontSize: 11, color: "#D0D2D8", fontFamily: "'DM Sans',sans-serif", cursor: "pointer", display: "flex", alignItems: "center", gap: 8 }}>
                  <Copy size={11} /> Duplicate
                </ContextMenuItem>
                <ContextMenuSeparator style={{ height: 1, background: "#2A2D35", margin: "2px 0" }} />
                <ContextMenuItem onSelect={() => { const sc = scenes.find(s => s.id !== scene.id); onDelete(scene.id); if (sc) onSelect(sc.id); }}
                  style={{ padding: "5px 10px", fontSize: 11, color: "#EF4444", fontFamily: "'DM Sans',sans-serif", cursor: "pointer", display: "flex", alignItems: "center", gap: 8 }}>
                  <Trash2 size={11} /> Delete Scene
                </ContextMenuItem>
              </ContextMenuContent>
            </ContextMenu>
          );
        })}
      </div>

      {/* Footer: scene count */}
      <div style={{ padding: "4px 8px", borderTop: "1px solid #2A2D35", background: "#0A0C0F", flexShrink: 0 }}>
        <span style={{ fontFamily: "'JetBrains Mono',monospace", fontSize: 9, color: "#404450" }}>
          {scenes.length} scene{scenes.length !== 1 ? "s" : ""}
        </span>
      </div>
    </div>
  );
}
