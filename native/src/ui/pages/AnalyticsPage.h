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
    QLabel* m_peak = nullptr;
    QLabel* m_watch = nullptr;
    QLabel* m_followers = nullptr;
    QLabel* m_revenue = nullptr;
    QLabel* m_bitrate = nullptr;
    QLabel* m_cpu = nullptr;
    QLabel* m_gpu = nullptr;
    QLabel* m_fps = nullptr;
    QLabel* m_viewerPanel = nullptr;
    QLabel* m_audiencePanel = nullptr;
    QLabel* m_sessionsPanel = nullptr;
};

} // namespace railshot
