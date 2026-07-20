#include "schedule/ScheduleModel.h"
#include "core/Types.h"
#include <algorithm>

namespace railshot {

QJsonObject ScheduledEvent::toJson() const
{
    return QJsonObject{
        {QStringLiteral("id"), id},
        {QStringLiteral("title"), title},
        {QStringLiteral("start"), start.toString(Qt::ISODate)},
        {QStringLiteral("platform"), platform},
        {QStringLiteral("notes"), notes},
        {QStringLiteral("reminder"), reminder},
        {QStringLiteral("status"), status},
    };
}

ScheduledEvent ScheduledEvent::fromJson(const QJsonObject& o)
{
    ScheduledEvent e;
    e.id = o.value(QStringLiteral("id")).toString(newId(QStringLiteral("evt")));
    e.title = o.value(QStringLiteral("title")).toString();
    e.start = QDateTime::fromString(o.value(QStringLiteral("start")).toString(), Qt::ISODate);
    e.platform = o.value(QStringLiteral("platform")).toString();
    e.notes = o.value(QStringLiteral("notes")).toString();
    e.reminder = o.value(QStringLiteral("reminder")).toBool(true);
    e.status = o.value(QStringLiteral("status")).toString(QStringLiteral("upcoming"));
    return e;
}

ScheduleModel::ScheduleModel(QObject* parent)
    : QObject(parent)
{
}

void ScheduleModel::addEvent(const ScheduledEvent& e)
{
    m_events.append(e);
    emit changed();
}

void ScheduleModel::removeEvent(const QString& id)
{
    m_events.erase(std::remove_if(m_events.begin(), m_events.end(),
                                  [&](const ScheduledEvent& e) { return e.id == id; }),
                   m_events.end());
    emit changed();
}

void ScheduleModel::updateEvent(const ScheduledEvent& e)
{
    for (auto& x : m_events)
        if (x.id == e.id) { x = e; emit changed(); return; }
}

ScheduledEvent* ScheduleModel::nextUp()
{
    ScheduledEvent* best = nullptr;
    const auto now = QDateTime::currentDateTimeUtc();
    for (auto& e : m_events) {
        if (e.status != QLatin1String("upcoming")) continue;
        if (e.start < now) continue;
        if (!best || e.start < best->start) best = &e;
    }
    return best;
}

} // namespace railshot
