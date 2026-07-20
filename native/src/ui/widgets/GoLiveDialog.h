#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QStackedWidget>
#include <QLabel>
#include <QPushButton>
#include <QVector>
#include <QHash>

namespace railshot {
class EngineController;

class GoLiveDialog : public QDialog {
    Q_OBJECT
public:
    explicit GoLiveDialog(EngineController* engine, QWidget* parent = nullptr);
    void setPrefill(const QString& title, const QString& platform);

private:
    void showStep(int step);
    void runChecks();
    void startCountdown();
    void goLiveNow();
    void selectPlatform(const QString& id, bool exclusive);
    void persistFocusedKey();
    QStringList selectedPlatforms() const;
    QString defaultUrlFor(const QString& platform) const;
    QString keyForPlatform(const QString& platform) const;

    EngineController* m_engine = nullptr;
    QStackedWidget* m_stack = nullptr;
    QLineEdit* m_url = nullptr;
    QLineEdit* m_key = nullptr;
    QLineEdit* m_title = nullptr;
    QLabel* m_countdown = nullptr;
    QLabel* m_checkStatus = nullptr;
    QLabel* m_hint = nullptr;
    QString m_platform = QStringLiteral("youtube");
    QVector<QPushButton*> m_platformBtns;
    QHash<QString, QString> m_keysByPlatform;
    QHash<QString, QString> m_urlsByPlatform;
    int m_checkIndex = 0;
};

} // namespace railshot
