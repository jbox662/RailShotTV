#include "ui/pages/SchedulePage.h"
#include "core/Types.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QInputDialog>
#include <QLineEdit>
#include <QLabel>
#include <QDateTimeEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QMessageBox>
#include <QTimer>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDialog>
#include <QDateTimeEdit>

namespace railshot {

SchedulePage::SchedulePage(QWidget* parent)
    : QWidget(parent)
{
    auto* root = new QVBoxLayout(this);
    root->addWidget(new QLabel(QStringLiteral("SCHEDULE"), this));
    auto* list = new QListWidget(this);
    root->addWidget(list, 1);

    auto* row = new QHBoxLayout();
    auto* add = new QPushButton(QStringLiteral("Add Event"), this);
    auto* edit = new QPushButton(QStringLiteral("Edit"), this);
    auto* remove = new QPushButton(QStringLiteral("Remove"), this);
    row->addWidget(add);
    row->addWidget(edit);
    row->addWidget(remove);
    root->addLayout(row);

    // Load persisted
    {
        QSettings s(QStringLiteral("RailShotTV"), QStringLiteral("RailShotTV"));
        const auto raw = s.value(QStringLiteral("schedule/events")).toByteArray();
        const auto arr = QJsonDocument::fromJson(raw).array();
        for (const auto& v : arr)
            m_model.addEvent(ScheduledEvent::fromJson(v.toObject()));
    }

    auto persist = [this] {
        QJsonArray arr;
        for (const auto& e : m_model.events())
            arr.append(e.toJson());
        QSettings s(QStringLiteral("RailShotTV"), QStringLiteral("RailShotTV"));
        s.setValue(QStringLiteral("schedule/events"),
                   QJsonDocument(arr).toJson(QJsonDocument::Compact));
    };

    auto refresh = [this, list] {
        list->clear();
        for (const auto& e : m_model.events()) {
            list->addItem(QStringLiteral("%1 — %2 (%3)%4")
                              .arg(e.start.toLocalTime().toString(QStringLiteral("MMM d hh:mm")),
                                   e.title, e.platform,
                                   e.reminder ? QStringLiteral(" · remind") : QString()));
        }
    };
    connect(&m_model, &ScheduleModel::changed, this, [refresh, persist] {
        refresh();
        persist();
    });
    refresh();

    auto editEvent = [this](ScheduledEvent e, bool isNew) -> bool {
        QDialog dlg(this);
        dlg.setWindowTitle(isNew ? QStringLiteral("Add Event") : QStringLiteral("Edit Event"));
        auto* form = new QVBoxLayout(&dlg);
        auto* title = new QLineEdit(e.title, &dlg);
        auto* when = new QDateTimeEdit(e.start.isValid() ? e.start : QDateTime::currentDateTime().addSecs(3600), &dlg);
        when->setCalendarPopup(true);
        auto* platform = new QComboBox(&dlg);
        platform->addItems({QStringLiteral("YouTube"), QStringLiteral("Twitch"), QStringLiteral("Facebook"), QStringLiteral("Custom")});
        platform->setCurrentText(e.platform.isEmpty() ? QStringLiteral("YouTube") : e.platform);
        auto* notes = new QLineEdit(e.notes, &dlg);
        auto* remind = new QCheckBox(QStringLiteral("Reminder toast before start"), &dlg);
        remind->setChecked(e.reminder);
        form->addWidget(new QLabel(QStringLiteral("Title")));
        form->addWidget(title);
        form->addWidget(new QLabel(QStringLiteral("Start")));
        form->addWidget(when);
        form->addWidget(new QLabel(QStringLiteral("Platform")));
        form->addWidget(platform);
        form->addWidget(new QLabel(QStringLiteral("Notes")));
        form->addWidget(notes);
        form->addWidget(remind);
        auto* ok = new QPushButton(QStringLiteral("Save"), &dlg);
        form->addWidget(ok);
        connect(ok, &QPushButton::clicked, &dlg, &QDialog::accept);
        if (dlg.exec() != QDialog::Accepted) return false;
        e.title = title->text().trimmed();
        if (e.title.isEmpty()) return false;
        e.start = when->dateTime();
        e.platform = platform->currentText();
        e.notes = notes->text();
        e.reminder = remind->isChecked();
        if (isNew) {
            e.id = newId(QStringLiteral("evt"));
            e.status = QStringLiteral("upcoming");
            m_model.addEvent(e);
        } else {
            m_model.updateEvent(e);
        }
        return true;
    };

    connect(add, &QPushButton::clicked, this, [editEvent] {
        ScheduledEvent e;
        e.start = QDateTime::currentDateTime().addSecs(3600);
        e.platform = QStringLiteral("YouTube");
        e.reminder = true;
        editEvent(e, true);
    });
    connect(edit, &QPushButton::clicked, this, [this, list, editEvent] {
        const int row = list->currentRow();
        if (row < 0 || row >= m_model.events().size()) return;
        editEvent(m_model.events().at(row), false);
    });
    connect(remove, &QPushButton::clicked, this, [this, list] {
        const int row = list->currentRow();
        if (row < 0 || row >= m_model.events().size()) return;
        m_model.removeEvent(m_model.events().at(row).id);
    });

    auto* reminder = new QTimer(this);
    connect(reminder, &QTimer::timeout, this, [this] {
        const auto now = QDateTime::currentDateTime();
        for (const auto& e : m_model.events()) {
            if (!e.reminder || e.status != QLatin1String("upcoming")) continue;
            const qint64 secs = now.secsTo(e.start);
            if (secs > 0 && secs <= 300 && secs > 240) {
                QMessageBox::information(this, QStringLiteral("Upcoming stream"),
                                         QStringLiteral("%1 starts in about 5 minutes (%2)")
                                             .arg(e.title, e.platform));
            }
        }
    });
    reminder->start(60000);
}

} // namespace railshot
