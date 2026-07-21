#pragma once

#include "core/Project.h"
#include <QDialog>
#include <QString>
#include <QVector>

class QTableWidget;
class QLabel;
class QLineEdit;
class QSpinBox;
class QCheckBox;

namespace railshot {
class EngineController;

struct MissingFileEntry {
    QString sourceId;
    QString sourceName;
    QString sceneName;
    QString settingKey;   // "path" or "url"
    QString originalPath;
    QString newPath;
};

QVector<MissingFileEntry> scanMissingFiles(const Project& project);

class MissingFilesDialog : public QDialog {
    Q_OBJECT
public:
    MissingFilesDialog(EngineController* engine, QVector<MissingFileEntry> entries,
                       QWidget* parent = nullptr);
    /// Applies remaps into the live project. Returns how many were updated.
    int applyRemaps();

private:
    void browseRow(int row);
    void searchFolder();

    EngineController* m_engine = nullptr;
    QVector<MissingFileEntry> m_entries;
    QTableWidget* m_table = nullptr;
};

class VCamConfigDialog : public QDialog {
    Q_OBJECT
public:
    explicit VCamConfigDialog(EngineController* engine, QWidget* parent = nullptr);

private:
    void refresh();
    void toggle();

    EngineController* m_engine = nullptr;
    QLabel* m_status = nullptr;
    QLabel* m_res = nullptr;
    QCheckBox* m_startOnLaunch = nullptr;
};

} // namespace railshot
