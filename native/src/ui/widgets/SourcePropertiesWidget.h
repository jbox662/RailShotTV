#pragma once
#include <QWidget>

class QLineEdit;
class QCheckBox;
class QDoubleSpinBox;
class QLabel;
class QTabWidget;
class QSlider;

namespace railshot {
class EngineController;

class SourcePropertiesWidget : public QWidget {
    Q_OBJECT
public:
    explicit SourcePropertiesWidget(EngineController* engine, QWidget* parent = nullptr);

signals:
    void closeRequested();

private:
    void rebuild();
    void applyTransformFromUi();
    void applyAll();

    EngineController* m_engine = nullptr;
    QLabel* m_title = nullptr;
    QLabel* m_empty = nullptr;
    QTabWidget* m_tabs = nullptr;
    QWidget* m_formHost = nullptr;

    QLineEdit* m_name = nullptr;
    QCheckBox* m_visible = nullptr;
    QCheckBox* m_locked = nullptr;
    QDoubleSpinBox* m_x = nullptr;
    QDoubleSpinBox* m_y = nullptr;
    QDoubleSpinBox* m_w = nullptr;
    QDoubleSpinBox* m_h = nullptr;
    QDoubleSpinBox* m_rot = nullptr;
    QDoubleSpinBox* m_opacity = nullptr;
    QDoubleSpinBox* m_cropL = nullptr;
    QDoubleSpinBox* m_cropR = nullptr;
    QDoubleSpinBox* m_cropT = nullptr;
    QDoubleSpinBox* m_cropB = nullptr;
    QSlider* m_brightness = nullptr;
    QSlider* m_contrast = nullptr;
    QSlider* m_saturation = nullptr;
    QCheckBox* m_chromaKey = nullptr;
    QSlider* m_chromaSim = nullptr;
    QSlider* m_blur = nullptr;
    QSlider* m_volume = nullptr;
    QCheckBox* m_audioMute = nullptr;
    bool m_block = false;
};

} // namespace railshot
