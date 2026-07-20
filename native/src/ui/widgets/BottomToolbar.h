#pragma once
#include <QWidget>
#include <QPushButton>
#include <QLabel>

namespace railshot {
class EngineController;

class BottomToolbar : public QWidget {
    Q_OBJECT
public:
    explicit BottomToolbar(EngineController* engine, QWidget* parent = nullptr);
    void setMixerOpen(bool open);

signals:
    void addInputRequested();
    void goLiveRequested();
    void mixerToggleRequested();
    void multiCorderRequested();
    void playListRequested();
    void overlayRequested();

private:
    EngineController* m_engine = nullptr;
    QPushButton* m_streamBtn = nullptr;
    QPushButton* m_recordBtn = nullptr;
    QPushButton* m_mixerBtn = nullptr;
    QLabel* m_statusPill = nullptr;
    bool m_mixerOpen = false;
};

} // namespace railshot
