#pragma once
#include "core/Types.h"
#include <QDialog>
#include <QJsonObject>

class QComboBox;
class QStackedWidget;
class QLineEdit;
class QPlainTextEdit;

namespace railshot {
class EngineController;

struct AddSourceResult {
    SourceType type = SourceType::Unknown;
    QString name;
    QJsonObject settings;
    bool accepted = false;
};

class AddSourceDialog : public QDialog {
    Q_OBJECT
public:
    explicit AddSourceDialog(EngineController* engine, QWidget* parent = nullptr);
    AddSourceResult result() const { return m_result; }

private:
    void rebuildFields();
    void acceptConfigured();

    EngineController* m_engine = nullptr;
    AddSourceResult m_result;
    QComboBox* m_type = nullptr;
    QStackedWidget* m_stack = nullptr;
    QComboBox* m_camera = nullptr;
    QComboBox* m_monitor = nullptr;
    QLineEdit* m_imagePath = nullptr;
    QPlainTextEdit* m_text = nullptr;
    QLineEdit* m_browserUrl = nullptr;
    QLineEdit* m_name = nullptr;
};

} // namespace railshot
