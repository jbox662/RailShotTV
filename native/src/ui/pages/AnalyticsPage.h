#pragma once
#include <QWidget>
#include <QVector>
#include <QDateTime>
class QLabel;
class QPushButton;

namespace railshot {
class EngineController;
struct TelemetrySnapshot;

class AnalyticsPage : public QWidget {
    Q_OBJECT
public:
    explicit AnalyticsPage(EngineController* engine, QWidget* parent = nullptr);
private:
    void setRangeMinutes(int minutes);
    void refreshFromHistory();

    EngineController* m_engine = nullptr;
    int m_rangeMinutes = 60;
    struct Sample {
        qint64 tsMs = 0;
        qint64 bitrate = 0;
        double cpu = 0;
        double gpu = 0;
        double fps = 0;
        bool streaming = false;
        qint64 uptime = 0;
        qint64 dropped = 0;
    };
    QVector<Sample> m_history;

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
    QLabel* m_peakBadge = nullptr;
};

} // namespace railshot
