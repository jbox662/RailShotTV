#pragma once
#include "core/Types.h"
#include <QDialog>
#include <QJsonObject>

class QListWidget;
class QStackedWidget;
class QLineEdit;
class QPlainTextEdit;
class QComboBox;
class QLabel;

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
    void selectType(int index);

    EngineController* m_engine = nullptr;
    AddSourceResult m_result;
    QListWidget* m_typeList = nullptr;
    QStackedWidget* m_stack = nullptr;
    QLabel* m_typeTitle = nullptr;
    QComboBox* m_camera = nullptr;
    QComboBox* m_monitor = nullptr;
    QLineEdit* m_imagePath = nullptr;
    QPlainTextEdit* m_text = nullptr;
    QLineEdit* m_browserUrl = nullptr;
    QLineEdit* m_name = nullptr;
    QLineEdit* m_colorHex = nullptr;
    QLineEdit* m_mediaPath = nullptr;
    QListWidget* m_ndiList = nullptr;
    SourceType m_selectedType = SourceType::Camera;
};

} // namespace railshot
