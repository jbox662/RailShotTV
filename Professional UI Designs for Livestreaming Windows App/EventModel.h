#pragma once
#include <QAbstractListModel>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QTimer>

// ─────────────────────────────────────────────────────────────────────────────
// BroadcastEvent — plain data struct for a single scheduled event
// ─────────────────────────────────────────────────────────────────────────────
struct BroadcastEvent {
    QString  id;
    QString  title;
    QString  description;
    QString  sport;          // "Custom Event", "Esports", "Basketball", etc.
    QString  venue;
    QDateTime startTime;
    int      durationMinutes = 120;
    QString  platform;       // "YouTube" | "Twitch" | "Facebook" | "Custom"
    QString  streamKey;
    QString  status;         // "upcoming" | "live" | "completed" | "cancelled"
    bool     remindersEnabled = true;
    QStringList tags;
    int      estimatedViewers = 0;

    // Serialization
    QJsonObject toJson() const;
    static BroadcastEvent fromJson(const QJsonObject &obj);

    // Helpers
    bool isUpcoming()  const { return status == "upcoming"; }
    bool isLive()      const { return status == "live"; }
    bool isCompleted() const { return status == "completed"; }
    bool isCancelled() const { return status == "cancelled"; }

    // Seconds until start (negative if in the past)
    qint64 secondsUntilStart() const {
        return QDateTime::currentDateTime().secsTo(startTime);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// EventModel — QAbstractListModel wrapping a list of BroadcastEvent
// ─────────────────────────────────────────────────────────────────────────────
class EventModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        TitleRole,
        DescriptionRole,
        SportRole,
        VenueRole,
        StartTimeRole,
        DurationRole,
        PlatformRole,
        StreamKeyRole,
        StatusRole,
        RemindersRole,
        TagsRole,
        EstViewersRole,
        SecondsUntilStartRole,
    };

    explicit EventModel(QObject *parent = nullptr);

    // QAbstractListModel interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // CRUD
    void addEvent(const BroadcastEvent &ev);
    void updateEvent(const BroadcastEvent &ev);
    void removeEvent(const QString &id);
    BroadcastEvent eventById(const QString &id) const;
    const QList<BroadcastEvent> &events() const { return m_events; }

    // Persistence
    void loadFromFile(const QString &path);
    void saveToFile(const QString &path) const;

    // Status helpers
    void refreshStatuses();   // call periodically to flip upcoming→live→completed

signals:
    void eventStartingSoon(const BroadcastEvent &ev, int minutesUntil);
    void eventStatusChanged(const BroadcastEvent &ev, const QString &oldStatus);

private:
    QList<BroadcastEvent> m_events;
    QTimer *m_refreshTimer = nullptr;

    int indexById(const QString &id) const;
};
