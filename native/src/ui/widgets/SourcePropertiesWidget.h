#pragma once
#include <QWidget>

class QLineEdit;
class QCheckBox;
class QDoubleSpinBox;
class QLabel;

namespace railshot {
class EngineController;

class SourcePropertiesWidget : public QWidget {
    Q_OBJECT
public:
    explicit SourcePropertiesWidget(EngineController* engine, QWidget* parent = nullptr);

private:
    void rebuild();
    void applyTransformFromUi();

    EngineController* m_engine = nullptr;
    QLineEdit* m_name = nullptr;
    QCheckBox* m_visible = nullptr;
    QCheckBox* m_locked = nullptr;
    QDoubleSpinBox* m_opacity = nullptr;
    QDoubleSpinBox* m_cropL = nullptr;
    QDoubleSpinBox* m_cropR = nullptr;
    QDoubleSpinBox* m_cropT = nullptr;
    QDoubleSpinBox* m_cropB = nullptr;
    QLabel* m_empty = nullptr;
    QWidget* m_formHost = nullptr;
    bool m_block = false;
};

} // namespace railshot
