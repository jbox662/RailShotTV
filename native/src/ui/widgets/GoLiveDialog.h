#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QStackedWidget>
#include <QLabel>
#include <QPushButton>
#include <QVector>

namespace railshot {
class EngineController;

class GoLiveDialog : public QDialog {
    Q_OBJECT
public:
    explicit GoLiveDialog(EngineController* engine, QWidget* parent = nullptr);

private:
    void showStep(int step);
    void runChecks();
    void startCountdown();
    void goLiveNow();

    EngineController* m_engine = nullptr;
    QStackedWidget* m_stack = nullptr;
    QLineEdit* m_url = nullptr;
    QLineEdit* m_key = nullptr;
    QLineEdit* m_title = nullptr;
    QLabel* m_countdown = nullptr;
    QLabel* m_checkStatus = nullptr;
    QString m_platform = QStringLiteral("youtube");
    QVector<QPushButton*> m_platformBtns;
    int m_checkIndex = 0;
};

} // namespace railshot
