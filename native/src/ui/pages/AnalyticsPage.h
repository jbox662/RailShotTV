#pragma once
#include <QWidget>
class QLabel;

namespace railshot {
class EngineController;

class AnalyticsPage : public QWidget {
    Q_OBJECT
public:
    explicit AnalyticsPage(EngineController* engine, QWidget* parent = nullptr);
private:
    EngineController* m_engine = nullptr;
    QLabel* m_stats = nullptr;
};

} // namespace railshot
