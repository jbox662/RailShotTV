#pragma once

#include "core/Types.h"

#include <QWidget>
#include <QPushButton>
#include <QVector>
#include <QLabel>
#include <QSlider>
#include <QLineEdit>
#include <QString>
#include <QCheckBox>

namespace railshot {
class EngineController;

class TransitionPanel : public QWidget {
    Q_OBJECT
public:
    explicit TransitionPanel(EngineController* engine, QWidget* parent = nullptr);

private:
    void refreshScenePad();
    void updateGoArmed();
    void syncSpeedFromProject();
    void applyEffectToEngine();
    void updateModeButton();
    void updateEffectButton();
    void showModeMenu();
    void showEffectMenu();
    TransitionType takeType() const;

    EngineController* m_engine = nullptr;
    QPushButton* m_modeBtn = nullptr;
    QPushButton* m_effectBtn = nullptr;
    QPushButton* m_go = nullptr;
    QLabel* m_speedValue = nullptr;
    QSlider* m_speed = nullptr;
    QLabel* m_showHideValue = nullptr;
    QSlider* m_showHideFade = nullptr;
    QLineEdit* m_stingerPath = nullptr;
    QSlider* m_stingerPoint = nullptr;
    QCheckBox* m_stingerTrackMatte = nullptr;
    QCheckBox* m_stingerMatteInvert = nullptr;
    QWidget* m_stingerBox = nullptr;
    QWidget* m_lumaWipeBox = nullptr;
    QLineEdit* m_lumaWipePath = nullptr;
    QCheckBox* m_lumaWipeInvert = nullptr;
    QSlider* m_lumaWipeSoft = nullptr;
    /// "Cut" = instant take; "Smooth" = use selected effect.
    QString m_mode = QStringLiteral("Cut");
    QString m_effect = QStringLiteral("Cross Dissolve");
    QVector<QPushButton*> m_scenePad;
};

} // namespace railshot
