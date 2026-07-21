#include "core/SettingsStore.h"
#include <QJsonDocument>
#include <QStandardPaths>

namespace railshot {

SettingsStore::SettingsStore(QObject* parent)
    : QObject(parent)
    , m_settings(QStringLiteral("RailShotTV"), QStringLiteral("RailShotTV"))
{
}

OutputProfile SettingsStore::outputProfile() const
{
    const auto raw = m_settings.value(QStringLiteral("output/profile")).toByteArray();
    if (raw.isEmpty()) return {};
    return OutputProfile::fromJson(QJsonDocument::fromJson(raw).object());
}

void SettingsStore::setOutputProfile(const OutputProfile& profile)
{
    m_settings.setValue(QStringLiteral("output/profile"),
                        QJsonDocument(profile.toJson()).toJson(QJsonDocument::Compact));
    emit settingsChanged();
}

QString SettingsStore::lastProjectPath() const
{
    return m_settings.value(QStringLiteral("project/lastPath")).toString();
}

void SettingsStore::setLastProjectPath(const QString& path)
{
    m_settings.setValue(QStringLiteral("project/lastPath"), path);
}

QJsonObject SettingsStore::uiState() const
{
    const auto raw = m_settings.value(QStringLiteral("ui/state")).toByteArray();
    if (raw.isEmpty()) return {};
    return QJsonDocument::fromJson(raw).object();
}

void SettingsStore::setUiState(const QJsonObject& state)
{
    m_settings.setValue(QStringLiteral("ui/state"),
                        QJsonDocument(state).toJson(QJsonDocument::Compact));
}

QString SettingsStore::recordingDirectory() const
{
    const auto def = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation)
                     + QStringLiteral("/RailShotTV");
    return m_settings.value(QStringLiteral("recording/directory"), def).toString();
}

void SettingsStore::setRecordingDirectory(const QString& path)
{
    m_settings.setValue(QStringLiteral("recording/directory"), path);
    emit settingsChanged();
}

bool SettingsStore::startOnLaunch() const
{
    return m_settings.value(QStringLiteral("general/startOnLaunch"), false).toBool();
}

void SettingsStore::setStartOnLaunch(bool v)
{
    m_settings.setValue(QStringLiteral("general/startOnLaunch"), v);
}

QJsonObject SettingsStore::hotkeys() const
{
    const QJsonObject defaults{
        {QStringLiteral("go"), QStringLiteral("Space")},
        {QStringLiteral("streamToggle"), QStringLiteral("F7")},
        {QStringLiteral("recordToggle"), QStringLiteral("F9")},
        {QStringLiteral("saveReplay"), QStringLiteral("F10")},
        {QStringLiteral("muteMic"), QStringLiteral("M")},
        {QStringLiteral("scoreAPlus"), QStringLiteral("Q")},
        {QStringLiteral("scoreAMinus"), QStringLiteral("A")},
        {QStringLiteral("scoreBPlus"), QStringLiteral("E")},
        {QStringLiteral("scoreBMinus"), QStringLiteral("D")},
        {QStringLiteral("scoreReset"), QStringLiteral("R")},
        {QStringLiteral("scoreSwap"), QStringLiteral("S")},
        {QStringLiteral("scene1"), QStringLiteral("1")},
        {QStringLiteral("scene2"), QStringLiteral("2")},
        {QStringLiteral("scene3"), QStringLiteral("3")},
        {QStringLiteral("scene4"), QStringLiteral("4")},
        {QStringLiteral("scene5"), QStringLiteral("5")},
        {QStringLiteral("scene6"), QStringLiteral("6")},
        {QStringLiteral("scene7"), QStringLiteral("7")},
        {QStringLiteral("scene8"), QStringLiteral("8")},
        {QStringLiteral("fullscreen"), QStringLiteral("F")},
    };
    const auto raw = m_settings.value(QStringLiteral("hotkeys")).toByteArray();
    if (raw.isEmpty()) return defaults;
    QJsonObject stored = QJsonDocument::fromJson(raw).object();
    if (stored.isEmpty()) return defaults;
    for (auto it = defaults.begin(); it != defaults.end(); ++it) {
        if (!stored.contains(it.key()))
            stored.insert(it.key(), it.value());
    }
    return stored;
}

void SettingsStore::setHotkeys(const QJsonObject& keys)
{
    if (keys.isEmpty())
        m_settings.remove(QStringLiteral("hotkeys"));
    else
        m_settings.setValue(QStringLiteral("hotkeys"),
                            QJsonDocument(keys).toJson(QJsonDocument::Compact));
    emit settingsChanged();
}

QString SettingsStore::desktopDeviceId() const
{
    return m_settings.value(QStringLiteral("audio/desktopDeviceId")).toString();
}

void SettingsStore::setDesktopDeviceId(const QString& id)
{
    m_settings.setValue(QStringLiteral("audio/desktopDeviceId"), id);
    emit settingsChanged();
}

QString SettingsStore::micDeviceId() const
{
    return m_settings.value(QStringLiteral("audio/micDeviceId")).toString();
}

void SettingsStore::setMicDeviceId(const QString& id)
{
    m_settings.setValue(QStringLiteral("audio/micDeviceId"), id);
    emit settingsChanged();
}

QJsonArray SettingsStore::audioChannels() const
{
    const auto raw = m_settings.value(QStringLiteral("audio/channels")).toByteArray();
    if (raw.isEmpty()) return {};
    const auto doc = QJsonDocument::fromJson(raw);
    return doc.isArray() ? doc.array() : QJsonArray{};
}

void SettingsStore::setAudioChannels(const QJsonArray& channels)
{
    m_settings.setValue(QStringLiteral("audio/channels"),
                        QJsonDocument(channels).toJson(QJsonDocument::Compact));
    emit settingsChanged();
}

void SettingsStore::sync()
{
    m_settings.sync();
}

} // namespace railshot
