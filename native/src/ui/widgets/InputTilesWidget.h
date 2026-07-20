#pragma once
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
    void addSourceRequested();
    void configureSourceRequested(const QString& sourceId);

private:
    void onRemove();
    void onProperties();
    void onMoveUp();
    void onMoveDown();

    EngineController* m_engine = nullptr;
    QScrollArea* m_scroll = nullptr;
    QWidget* m_listHost = nullptr;
    QVBoxLayout* m_listLay = nullptr;
};

} // namespace railshot
