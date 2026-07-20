#pragma once
#include <QWidget>
#include <QSpinBox>
#include <QComboBox>
#include <QLineEdit>
#include <QKeySequenceEdit>
#include <QHash>
#include <memory>

namespace railshot {
class EngineController;

class SettingsPage : public QWidget {
    Q_OBJECT
public:
    explicit SettingsPage(EngineController* engine, QWidget* parent = nullptr);
private:
    EngineController* m_engine = nullptr;
    QSpinBox* m_outW = nullptr;
    QSpinBox* m_outH = nullptr;
    QSpinBox* m_streamBitrate = nullptr;
    QSpinBox* m_streamKeyframe = nullptr;
    QSpinBox* m_replaySec = nullptr;
    QComboBox* m_streamEnc = nullptr;
    QComboBox* m_streamRate = nullptr;
    QComboBox* m_streamPlatform = nullptr;
    QComboBox* m_videoFps = nullptr;
    QComboBox* m_videoRes = nullptr;
    QComboBox* m_desktopDevice = nullptr;
    QComboBox* m_micDevice = nullptr;
    QLineEdit* m_outDir = nullptr;
    std::shared_ptr<QHash<QString, QKeySequenceEdit*>> m_hotkeyEdits;
};

} // namespace railshot
