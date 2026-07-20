#pragma once
#include <QListWidget>

namespace railshot {
class EngineController;

class SceneListWidget : public QListWidget {
    Q_OBJECT
public:
    explicit SceneListWidget(EngineController* engine, QWidget* parent = nullptr);
    void refresh();
private:
    EngineController* m_engine = nullptr;
};

} // namespace railshot
