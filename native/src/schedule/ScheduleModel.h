#pragma once

#include <QObject>
#include <QVector>
#include <QDateTime>
#include <QJsonObject>

namespace railshot {

struct ScheduledEvent {
    QString id;
    QString title;
    QDateTime start;
    QString platform;
    QString notes;
    bool reminder = true;
    QString status = QStringLiteral("upcoming"); // upcoming|live|done

    QJsonObject toJson() const;
    static ScheduledEvent fromJson(const QJsonObject& o);
};

class ScheduleModel : public QObject {
    Q_OBJECT
public:
    explicit ScheduleModel(QObject* parent = nullptr);
    QVector<ScheduledEvent> events() const { return m_events; }
    void addEvent(const ScheduledEvent& e);
    void removeEvent(const QString& id);
    void updateEvent(const ScheduledEvent& e);
    ScheduledEvent* nextUp();

signals:
    void changed();

private:
    QVector<ScheduledEvent> m_events;
};

} // namespace railshot
