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
private slots:
    void refreshMeters();
private:
    EngineController* m_engine = nullptr;
    QHBoxLayout* m_row = nullptr;
    QPushButton* m_monitorBtn = nullptr;
};

} // namespace railshot
