#pragma once
#include "core/Types.h"
#include <QWidget>

class QVBoxLayout;
class QScrollArea;
class QToolButton;

namespace railshot {
class EngineController;

/// OBS Sources dock: vertical source bars + bottom toolbar (+ − ⚙ ↑ ↓).
class InputTilesWidget : public QWidget {
    Q_OBJECT
public:
    explicit InputTilesWidget(EngineController* engine, QWidget* parent = nullptr);
    void refresh();

signals:
    /// Fired after the OBS-style source-type popup; Unknown if cancelled.
    void addSourceTypeRequested(railshot::SourceType type);
    void configureSourceRequested(const QString& sourceId);

private:
    void onAddClicked();
    void onRemove();
    void onProperties();
    void onMoveUp();
    void onMoveDown();

    EngineController* m_engine = nullptr;
    QScrollArea* m_scroll = nullptr;
    QWidget* m_listHost = nullptr;
    QVBoxLayout* m_listLay = nullptr;
    QToolButton* m_addBtn = nullptr;
};

} // namespace railshot
