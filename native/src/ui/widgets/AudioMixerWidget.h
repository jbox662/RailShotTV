#pragma once
#include <QWidget>
#include <QVBoxLayout>

class QPushButton;

namespace railshot {
class EngineController;

/// OBS-style bottom mixer: horizontal channel rows (no fat vertical strips).
class AudioMixerWidget : public QWidget {
    Q_OBJECT
public:
    explicit AudioMixerWidget(EngineController* engine, QWidget* parent = nullptr);

signals:
    void openAdvAudioRequested();

private slots:
    void refreshMeters();
    void openAdvAudio();

private:
    void rebuildStrips();
    void updateStripValues();

    EngineController* m_engine = nullptr;
    QVBoxLayout* m_list = nullptr;
    QPushButton* m_monitorBtn = nullptr;
    QPushButton* m_advBtn = nullptr;
    QStringList m_stripIds;
};

} // namespace railshot
