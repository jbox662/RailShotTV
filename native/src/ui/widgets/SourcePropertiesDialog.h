#pragma once

#include <QDialog>
#include <QWidget>
#include <QString>
#include <QImage>

class QTimer;
class QLabel;

namespace railshot {
class EngineController;
class SourcePropertiesWidget;

/// Paints a letterboxed source frame (CPU path — reliable inside modal dialogs).
class SourcePropsPreviewWidget : public QWidget {
    Q_OBJECT
public:
    explicit SourcePropsPreviewWidget(QWidget* parent = nullptr);
    void setFrame(const QImage& img);
    void setPlaceholder(const QString& msg);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QImage m_frame;
    QString m_placeholder = QStringLiteral("No Signal");
};

/// OBS-style floating Properties dialog for a single input source (with live preview).
class SourcePropertiesDialog : public QDialog {
    Q_OBJECT
public:
    SourcePropertiesDialog(EngineController* engine, const QString& sourceId, QWidget* parent = nullptr);
    ~SourcePropertiesDialog() override;

private:
    void refreshTitle();
    void onOk();
    void onDefaults();
    void tickPreview();
    void pinCaptureSource();
    void unpinCaptureSource();
    bool sourceHasVideoPreview() const;

    EngineController* m_engine = nullptr;
    QString m_sourceId;
    SourcePropertiesWidget* m_props = nullptr;
    QWidget* m_previewHost = nullptr;
    QLabel* m_previewCaption = nullptr;
    SourcePropsPreviewWidget* m_preview = nullptr;
    QTimer* m_previewTimer = nullptr;
};

} // namespace railshot
