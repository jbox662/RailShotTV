#pragma once
#include <QWidget>
#include <QTabWidget>

namespace railshot {
class EngineController;

class SettingsPage : public QWidget {
    Q_OBJECT
public:
    explicit SettingsPage(EngineController* engine, QWidget* parent = nullptr);
private:
    EngineController* m_engine = nullptr;
};

} // namespace railshot
