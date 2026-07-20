#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QComboBox>

namespace railshot {
class EngineController;

class GoLiveDialog : public QDialog {
    Q_OBJECT
public:
    explicit GoLiveDialog(EngineController* engine, QWidget* parent = nullptr);
private:
    EngineController* m_engine = nullptr;
    QComboBox* m_platform = nullptr;
    QLineEdit* m_url = nullptr;
    QLineEdit* m_key = nullptr;
};

} // namespace railshot
