#pragma once

#include "core/Types.h"
#include <QObject>
#include <QSettings>
#include <QJsonObject>

namespace railshot {

class SettingsStore : public QObject {
    Q_OBJECT
public:
    explicit SettingsStore(QObject* parent = nullptr);

    OutputProfile outputProfile() const;
    void setOutputProfile(const OutputProfile& profile);

    QString lastProjectPath() const;
    void setLastProjectPath(const QString& path);

    QJsonObject uiState() const;
    void setUiState(const QJsonObject& state);

    QString recordingDirectory() const;
    void setRecordingDirectory(const QString& path);

    bool startOnLaunch() const;
    void setStartOnLaunch(bool v);

    QJsonObject hotkeys() const;
    void setHotkeys(const QJsonObject& keys);

    void sync();

signals:
    void settingsChanged();

private:
    QSettings m_settings;
};

} // namespace railshot
