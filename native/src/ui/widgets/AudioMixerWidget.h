#pragma once
#include <QWidget>
#include <QHBoxLayout>

class QPushButton;

namespace railshot {
class EngineController;

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
    QHBoxLayout* m_row = nullptr;
    QPushButton* m_monitorBtn = nullptr;
    QPushButton* m_advBtn = nullptr;
    QStringList m_stripIds;
};

} // namespace railshot
