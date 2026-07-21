#pragma once

#include <QDialog>
#include <QString>

class QLabel;
class QTimer;
class QLineEdit;

namespace railshot {
class EngineController;
class PreviewSurface;

/// OBS-style Interact window: live preview feed + source-relative pointer readout.
/// Browser sources also get an Open URL action (full WebView interact stays in helper process).
class InteractDialog : public QDialog {
    Q_OBJECT
public:
    InteractDialog(EngineController* engine, const QString& sourceId, QWidget* parent = nullptr);

private slots:
    void tick();
    void openUrl();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void updateHint(const QPointF& widgetPos);

    EngineController* m_engine = nullptr;
    QString m_sourceId;
    PreviewSurface* m_surface = nullptr;
    QLabel* m_status = nullptr;
    QLabel* m_hint = nullptr;
    QTimer* m_timer = nullptr;
};

} // namespace railshot
