#pragma once

#include "core/Project.h"
#include <QVector>
#include <QElapsedTimer>

namespace railshot {

/// Bounded project snapshot history with coalescing for rapid UI edits (filter sliders).
class UndoStack {
public:
    static constexpr int kMaxDepth = 50;
    static constexpr int kCoalesceMs = 450;

    void clear();
    void pushBefore(const Project& before);
    bool canUndo() const { return !m_undo.isEmpty(); }
    bool canRedo() const { return !m_redo.isEmpty(); }
    /// Pop undo → return previous project; push `current` onto redo.
    Project undo(const Project& current);
    /// Pop redo → return next project; push `current` onto undo.
    Project redo(const Project& current);

private:
    QVector<Project> m_undo;
    QVector<Project> m_redo;
    QElapsedTimer m_coalesceTimer;
    bool m_coalesceActive = false;
};

} // namespace railshot
