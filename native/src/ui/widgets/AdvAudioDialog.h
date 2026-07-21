#pragma once

#include <QDialog>
#include <QHash>

class QGridLayout;
class QCheckBox;
class QLabel;
class QDoubleSpinBox;
class QSpinBox;
class QComboBox;
class QSlider;
class QWidget;

namespace railshot {
class EngineController;

/// OBS Advanced Audio Properties — per-channel volume, mono, balance, sync, monitoring, tracks.
class AdvAudioDialog : public QDialog {
    Q_OBJECT
public:
    explicit AdvAudioDialog(EngineController* engine, QWidget* parent = nullptr);

private slots:
    void rebuildRows();
    void onUsePercentToggled(bool usePercent);

private:
    void addHeader(QGridLayout* grid);
    void addChannelRow(QGridLayout* grid, int row, const QString& channelId);
    void applyChannel(const QString& id);

    EngineController* m_engine = nullptr;
    QGridLayout* m_grid = nullptr;
    QWidget* m_rowsHost = nullptr;
    QCheckBox* m_usePercent = nullptr;
    bool m_usePercentMode = false;

    struct RowWidgets {
        QLabel* name = nullptr;
        QLabel* status = nullptr;
        QWidget* volStack = nullptr;
        QDoubleSpinBox* volDb = nullptr;
        QSpinBox* volPct = nullptr;
        QCheckBox* mono = nullptr;
        QSlider* balance = nullptr;
        QSpinBox* sync = nullptr;
        QComboBox* monitoring = nullptr;
        QCheckBox* tracks[6] = {};
    };
    QHash<QString, RowWidgets> m_rows;
};

} // namespace railshot
