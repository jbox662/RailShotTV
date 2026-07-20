#pragma once
#include <QWidget>
#include <QHBoxLayout>

namespace railshot {
class EngineController;

class InputTilesWidget : public QWidget {
    Q_OBJECT
public:
    explicit InputTilesWidget(EngineController* engine, QWidget* parent = nullptr);
    void refresh();
private:
    EngineController* m_engine = nullptr;
    QHBoxLayout* m_row = nullptr;
};

} // namespace railshot
