#pragma once
#include <QWidget>
#include <QVBoxLayout>

class QPushButton;

namespace railshot {
class EngineController;

/// OBS-style Audio Mixer: horizontal channel strips that grow with the dock
/// but refuse to shrink below the meter tick-label width.
class AudioMixerWidget : public QWidget {
    Q_OBJECT
public:
    explicit AudioMixerWidget(EngineController* engine, QWidget* parent = nullptr);

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

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
