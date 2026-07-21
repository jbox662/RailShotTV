#pragma once

#include "core/Types.h"
#include <QDialog>
#include <QString>

class QDoubleSpinBox;
class QLabel;
class QPushButton;

namespace railshot {
class EngineController;

/// OBS Edit Transform dialog + copy/paste/reset transform clipboard.
class TransformDialog : public QDialog {
    Q_OBJECT
public:
    TransformDialog(EngineController* engine, const QString& sourceId, QWidget* parent = nullptr);

    static void copyTransform(const Transform& t);
    static bool hasClipboard();
    static Transform clipboardTransform();
    static void pasteOnto(EngineController* engine, const QString& sourceId);
    static void resetTransform(EngineController* engine, const QString& sourceId);

private:
    void loadFromSource();
    void applyToSource();
    void setLockedUi(bool locked);
    QDoubleSpinBox* addSpin(double min, double max, double step, int decimals);

    EngineController* m_engine = nullptr;
    QString m_sourceId;
    QLabel* m_lockHint = nullptr;
    QPushButton* m_pasteBtn = nullptr;
    QPushButton* m_resetBtn = nullptr;
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
    bool m_loading = false;

    static Transform s_clipboard;
    static bool s_hasClipboard;
};

} // namespace railshot
