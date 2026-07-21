#pragma once

#include <QDialog>
#include <QString>

class QTimer;
class QLabel;

namespace railshot {
class EngineController;
class SourcePropertiesWidget;
class PreviewSurface;

/// OBS-style floating Properties dialog for a single input source (with live preview).
class SourcePropertiesDialog : public QDialog {
    Q_OBJECT
public:
    SourcePropertiesDialog(EngineController* engine, const QString& sourceId, QWidget* parent = nullptr);

private:
    void refreshTitle();
    void onOk();
    void onDefaults();
    void tickPreview();
    bool sourceHasVideoPreview() const;

    EngineController* m_engine = nullptr;
    QString m_sourceId;
    SourcePropertiesWidget* m_props = nullptr;
    QWidget* m_previewHost = nullptr;
    QLabel* m_previewCaption = nullptr;
    PreviewSurface* m_preview = nullptr;
    QTimer* m_previewTimer = nullptr;
};

} // namespace railshot
