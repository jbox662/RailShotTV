#pragma once

#include <QDialog>
#include <QString>

class QListWidget;
class QLabel;
class QPlainTextEdit;
class QLineEdit;
class QPushButton;
class QProcess;

namespace railshot {
class EngineController;

class ProfilesDialog : public QDialog {
    Q_OBJECT
public:
    explicit ProfilesDialog(EngineController* engine, QWidget* parent = nullptr);

signals:
    void profileApplied(const QString& name);

private slots:
    void reload();
    void onNew();
    void onDuplicate();
    void onRename();
    void onRemove();
    void onApply();

private:
    EngineController* m_engine = nullptr;
    QListWidget* m_list = nullptr;
    QLabel* m_detail = nullptr;
    QString selectedName() const;
};

class SceneCollectionsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SceneCollectionsDialog(EngineController* engine, QWidget* parent = nullptr);

signals:
    void collectionSwitchRequested(const QString& name);

private slots:
    void reload();
    void onNew();
    void onDuplicate();
    void onRename();
    void onRemove();
    void onSwitch();

private:
    EngineController* m_engine = nullptr;
    QListWidget* m_list = nullptr;
    QString selectedName() const;
};

class RemuxDialog : public QDialog {
    Q_OBJECT
public:
    explicit RemuxDialog(QWidget* parent = nullptr);

private slots:
    void browseInput();
    void browseOutput();
    void startRemux();
    void onProcFinished(int exitCode);

private:
    QLineEdit* m_input = nullptr;
    QLineEdit* m_output = nullptr;
    QLabel* m_status = nullptr;
    QPushButton* m_go = nullptr;
    QProcess* m_proc = nullptr;
};

class LogViewerDialog : public QDialog {
    Q_OBJECT
public:
    explicit LogViewerDialog(QWidget* parent = nullptr);

private slots:
    void refresh();
    void openFolder();
    void openFile();

private:
    QPlainTextEdit* m_text = nullptr;
    QLabel* m_pathLabel = nullptr;
};

} // namespace railshot
