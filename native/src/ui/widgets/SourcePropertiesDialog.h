#pragma once

#include <QDialog>
#include <QString>

namespace railshot {
class EngineController;
class SourcePropertiesWidget;

/// OBS-style floating Properties dialog for a single input source.
class SourcePropertiesDialog : public QDialog {
    Q_OBJECT
public:
    SourcePropertiesDialog(EngineController* engine, const QString& sourceId, QWidget* parent = nullptr);

private:
    void refreshTitle();
    void onOk();
    void onDefaults();

    EngineController* m_engine = nullptr;
    QString m_sourceId;
    SourcePropertiesWidget* m_props = nullptr;
};

} // namespace railshot
