#pragma once
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QPoint>

namespace railshot {
class EngineController;

class BottomToolbar : public QWidget {
    Q_OBJECT
public:
    explicit BottomToolbar(EngineController* engine, QWidget* parent = nullptr);
    void setBasicMode(bool on);

signals:
    void addInputRequested();
    void goLiveRequested();
    void multiCorderRequested();
    void playListRequested();
    void overlayRequested();
    void overlayMenuRequested(const QPoint& globalPos);
    /// OBS Studio Mode: when true, show Preview+Program; when false, Program only.
    void studioModeToggled(bool enabled);

private:
    EngineController* m_engine = nullptr;
    QPushButton* m_streamBtn = nullptr;
    QPushButton* m_recordBtn = nullptr;
    QPushButton* m_studioBtn = nullptr;
    QPushButton* m_swapBtn = nullptr;
    QPushButton* m_replayToggleBtn = nullptr;
    QPushButton* m_multiBtn = nullptr;
    QPushButton* m_playlistBtn = nullptr;
    QPushButton* m_overlayBtn = nullptr;
    QLabel* m_statusPill = nullptr;
};

} // namespace railshot
