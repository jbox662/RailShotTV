#include "core/UndoStack.h"

namespace railshot {

void UndoStack::clear()
{
    m_undo.clear();
    m_redo.clear();
    m_coalesceActive = false;
}

void UndoStack::pushBefore(const Project& before)
{
    if (m_coalesceActive && m_coalesceTimer.isValid()
        && m_coalesceTimer.elapsed() < kCoalesceMs && !m_undo.isEmpty()) {
        // Keep the first "before" of this burst; extend the window.
        m_coalesceTimer.restart();
        m_redo.clear();
        return;
    }
    m_undo.push_back(before);
    while (m_undo.size() > kMaxDepth)
        m_undo.removeFirst();
    m_redo.clear();
    m_coalesceTimer.restart();
    m_coalesceActive = true;
}

Project UndoStack::undo(const Project& current)
{
    Project prev = m_undo.takeLast();
    m_redo.push_back(current);
    while (m_redo.size() > kMaxDepth)
        m_redo.removeFirst();
    m_coalesceActive = false;
    return prev;
}

Project UndoStack::redo(const Project& current)
{
    Project next = m_redo.takeLast();
    m_undo.push_back(current);
    while (m_undo.size() > kMaxDepth)
        m_undo.removeFirst();
    m_coalesceActive = false;
    return next;
}

} // namespace railshot
