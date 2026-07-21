#pragma once
#include "core/Types.h"
#include <QDialog>
#include <QJsonObject>

class QLineEdit;
class QListWidget;
class QRadioButton;
class QLabel;

namespace railshot {
class EngineController;

struct AddSourceResult {
    SourceType type = SourceType::Unknown;
    QString name;
    QJsonObject settings;
    bool accepted = false;
    bool openProperties = true;
};

/// OBS-style source type popup (Sources dock + button).
SourceType showAddSourceTypeMenu(QWidget* parent, const QPoint& globalPos);

/// OBS Create/Select Source dialog for a chosen type (Create New vs Add Existing).
class AddSourceDialog : public QDialog {
    Q_OBJECT
public:
    AddSourceDialog(EngineController* engine, SourceType type, QWidget* parent = nullptr);
    AddSourceResult result() const { return m_result; }

    static QString defaultNameForType(SourceType type);

private:
    void refreshExisting();
    void acceptChoice();
    void syncModeUi();

    EngineController* m_engine = nullptr;
    SourceType m_type = SourceType::Unknown;
    AddSourceResult m_result;
    QRadioButton* m_createNew = nullptr;
    QRadioButton* m_addExisting = nullptr;
    QLineEdit* m_name = nullptr;
    QListWidget* m_existing = nullptr;
    QLabel* m_hint = nullptr;
};

} // namespace railshot
