#include "ui/pages/SchedulePage.h"
#include "ui/Theme.h"
#include "core/Types.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
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
#include <QFormLayout>
#include <QFrame>
#include <QScrollArea>
#include <QSpinBox>
#include <QTextEdit>

namespace railshot {

SchedulePage::SchedulePage(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("schedulePage"));
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    root->addWidget(theme::makePageHeader(QStringLiteral("Schedule"), theme::PanelAccent::Blue, this));

    auto* content = new QVBoxLayout();
    content->setContentsMargins(16, 16, 16, 16);
    content->setSpacing(12);

    auto* banner = new QFrame(this);
    banner->setObjectName(QStringLiteral("nextUpBanner"));
    banner->setMinimumHeight(88);
    auto* banLay = new QHBoxLayout(banner);
    banLay->setContentsMargins(20, 12, 20, 12);
    auto* banCol = new QVBoxLayout();
    auto* nextLab = new QLabel(QStringLiteral("NEXT UP"), banner);
    nextLab->setStyleSheet(QStringLiteral("color:#8892A4; font-size:10px; font-weight:800; letter-spacing:1.5px; background:transparent;"));
    auto* nextTitle = new QLabel(QStringLiteral("No upcoming events"), banner);
    nextTitle->setStyleSheet(QStringLiteral("color:#F0F0F0; font-size:16px; font-weight:700; background:transparent;"));
    banCol->addWidget(nextLab);
    banCol->addWidget(nextTitle);
    auto* countdown = new QLabel(QStringLiteral("--:--:--"), banner);
    countdown->setStyleSheet(QStringLiteral(
        "font-family:'Bebas Neue'; font-size:32px; color:#4F9EFF; background:transparent;"));
    auto* startStream = new QPushButton(QStringLiteral("Start Stream"), banner);
    startStream->setStyleSheet(QStringLiteral(
        "QPushButton{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #FF5A2C,stop:1 #FF8C42);"
        "color:white;font-weight:800;border:none;border-radius:4px;padding:10px 18px;}"));
    banLay->addLayout(banCol, 1);
    banLay->addWidget(countdown);
    banLay->addWidget(startStream);
    content->addWidget(banner);

    auto* statsRow = new QHBoxLayout();
    auto makeStat = [&](const QString& label, const QString& value, const QString& color) {
        auto* card = new QFrame(this);
        card->setObjectName(QStringLiteral("kpiCard"));
        auto* lay = new QVBoxLayout(card);
        lay->setContentsMargins(14, 10, 14, 10);
        auto* v = new QLabel(value, card);
        v->setObjectName(QStringLiteral("statValue"));
        v->setStyleSheet(QStringLiteral("font-family:'Bebas Neue'; font-size:24px; color:%1; background:transparent;").arg(color));
        auto* l = new QLabel(label, card);
        l->setStyleSheet(QStringLiteral("color:#8892A4; font-size:10px; font-weight:700; letter-spacing:1px; background:transparent;"));
        lay->addWidget(v);
        lay->addWidget(l);
        statsRow->addWidget(card);
        return v;
    };
    auto* totalVal = makeStat(QStringLiteral("TOTAL EVENTS"), QStringLiteral("0"), QStringLiteral("#4F9EFF"));
    auto* upcomingVal = makeStat(QStringLiteral("UPCOMING"), QStringLiteral("0"), QStringLiteral("#A855F7"));
    auto* viewersVal = makeStat(QStringLiteral("EST. VIEWERS"), QStringLiteral("—"), QStringLiteral("#22C55E"));
    auto* doneVal = makeStat(QStringLiteral("COMPLETED"), QStringLiteral("0"), QStringLiteral("#FBBF24"));
    content->addLayout(statsRow);

    auto* listHost = new QScrollArea(this);
    listHost->setWidgetResizable(true);
    listHost->setFrameShape(QFrame::NoFrame);
    listHost->setStyleSheet(QStringLiteral("background:transparent; border:none;"));
    auto* listInner = new QWidget;
    auto* listLay = new QVBoxLayout(listInner);
    listLay->setContentsMargins(0, 0, 0, 0);
    listLay->setSpacing(8);
    auto* empty = new QLabel(QStringLiteral("No events found — click Add Event to schedule a stream"), listInner);
    empty->setAlignment(Qt::AlignCenter);
    empty->setMinimumHeight(120);
    empty->setStyleSheet(QStringLiteral(
        "color:#606878; border:1px dashed #2A3350; border-radius:8px; font-size:13px;"));
    listLay->addWidget(empty);
    listHost->setWidget(listInner);
    content->addWidget(listHost, 1);

    auto* row = new QHBoxLayout();
    auto* add = new QPushButton(QStringLiteral("Add Event"), this);
    auto* edit = new QPushButton(QStringLiteral("Edit"), this);
    auto* remove = new QPushButton(QStringLiteral("Remove"), this);
    row->addWidget(add);
    row->addWidget(edit);
    row->addWidget(remove);
    row->addStretch();
    content->addLayout(row);
    root->addLayout(content, 1);

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

    QVector<QFrame*>* cards = new QVector<QFrame*>();
    int* selected = new int(-1);

    auto refresh = [=] {
        while (QLayoutItem* child = listLay->takeAt(0)) {
            if (child->widget() && child->widget() != empty)
                child->widget()->deleteLater();
            else if (child->widget() == empty)
                empty->setParent(nullptr);
            delete child;
        }
        cards->clear();
        *selected = -1;
        const auto events = m_model.events();
        int total = events.size();
        int upcoming = 0, completed = 0;
        for (const auto& e : events) {
            if (e.status == QLatin1String("upcoming") || e.status == QLatin1String("live")) ++upcoming;
            if (e.status == QLatin1String("done") || e.status == QLatin1String("completed")) ++completed;
        }
        totalVal->setText(QString::number(total));
        upcomingVal->setText(QString::number(upcoming));
        doneVal->setText(QString::number(completed));
        viewersVal->setText(QStringLiteral("—"));

        ScheduledEvent* next = m_model.nextUp();
        if (next) {
            nextTitle->setText(QStringLiteral("%1 · %2").arg(next->title, next->platform));
        } else {
            nextTitle->setText(QStringLiteral("No upcoming events"));
            countdown->setText(QStringLiteral("--:--:--"));
        }

        if (events.isEmpty()) {
            empty->setParent(listInner);
            listLay->addWidget(empty);
            empty->show();
            return;
        }
        empty->hide();
        empty->setParent(nullptr);
        int idx = 0;
        for (const auto& e : events) {
            auto* card = new QFrame(listInner);
            const bool live = e.status == QLatin1String("live");
            card->setObjectName(live ? QStringLiteral("eventCardLive") : QStringLiteral("eventCard"));
            auto* cl = new QHBoxLayout(card);
            cl->setContentsMargins(14, 10, 14, 10);
            auto* col = new QVBoxLayout();
            auto* t = new QLabel(e.title, card);
            t->setStyleSheet(QStringLiteral("font-weight:700; font-size:13px; background:transparent;"));
            QString platColor = QStringLiteral("#22D3EE");
            if (e.platform.contains(QLatin1String("YouTube"), Qt::CaseInsensitive)) platColor = QStringLiteral("#FF0000");
            else if (e.platform.contains(QLatin1String("Twitch"), Qt::CaseInsensitive)) platColor = QStringLiteral("#9147FF");
            else if (e.platform.contains(QLatin1String("Facebook"), Qt::CaseInsensitive)) platColor = QStringLiteral("#1877F2");
            auto* meta = new QLabel(QStringLiteral("%1  ·  %2  ·  %3")
                                        .arg(e.start.toLocalTime().toString(QStringLiteral("MMM d hh:mm")),
                                             e.platform, e.status), card);
            meta->setStyleSheet(QStringLiteral("color:%1; font-size:11px; background:transparent;").arg(platColor));
            col->addWidget(t);
            col->addWidget(meta);
            if (live) {
                auto* pulse = new QLabel(QStringLiteral("● LIVE"), card);
                pulse->setObjectName(QStringLiteral("livePulse"));
                col->addWidget(pulse);
            }
            cl->addLayout(col, 1);
            listLay->addWidget(card);
            const int cardIdx = idx;
            card->setCursor(Qt::PointingHandCursor);
            cards->append(card);
            auto* pick = new QPushButton(card);
            pick->setFlat(true);
            pick->setStyleSheet(QStringLiteral("background:transparent; border:none;"));
            pick->setGeometry(0, 0, 800, 60);
            connect(pick, &QPushButton::clicked, this, [selected, cardIdx, cards] {
                *selected = cardIdx;
                for (int i = 0; i < cards->size(); ++i) {
                    (*cards)[i]->setStyleSheet(i == cardIdx
                        ? QStringLiteral("QFrame{border:1px solid #4F9EFF; background:#1A2035; border-radius:6px;}")
                        : QString());
                }
            });
            ++idx;
        }
        listLay->addStretch();
    };

    auto* tick = new QTimer(this);
    connect(tick, &QTimer::timeout, this, [=] {
        if (auto* next = m_model.nextUp()) {
            const qint64 secs = QDateTime::currentDateTime().secsTo(next->start);
            if (secs <= 0) {
                countdown->setText(QStringLiteral("NOW"));
            } else {
                const int h = int(secs / 3600);
                const int m = int((secs % 3600) / 60);
                const int s = int(secs % 60);
                countdown->setText(QStringLiteral("%1:%2:%3")
                                       .arg(h, 2, 10, QLatin1Char('0'))
                                       .arg(m, 2, 10, QLatin1Char('0'))
                                       .arg(s, 2, 10, QLatin1Char('0')));
            }
        }
    });
    tick->start(1000);

    connect(&m_model, &ScheduleModel::changed, this, [refresh, persist] {
        refresh();
        persist();
    });
    refresh();

    connect(startStream, &QPushButton::clicked, this, [this] {
        QMessageBox::information(this, QStringLiteral("Schedule"),
                                 QStringLiteral("Open Dashboard and use Stream / Go Live to start broadcasting."));
    });

    auto editEvent = [this](ScheduledEvent e, bool isNew) -> bool {
        QDialog dlg(this);
        dlg.setWindowTitle(isNew ? QStringLiteral("Add Event") : QStringLiteral("Edit Event"));
        dlg.setFixedWidth(520);
        dlg.setStyleSheet(QStringLiteral("QDialog{background:#1A2035;} QLabel{color:#D0D2D8;}"));
        auto* form = new QFormLayout(&dlg);
        auto* title = new QLineEdit(e.title, &dlg);
        auto* desc = new QTextEdit(e.notes, &dlg);
        desc->setFixedHeight(60);
        auto* sport = new QLineEdit(&dlg);
        sport->setPlaceholderText(QStringLiteral("Sport"));
        auto* when = new QDateTimeEdit(e.start.isValid() ? e.start : QDateTime::currentDateTime().addSecs(3600), &dlg);
        when->setCalendarPopup(true);
        auto* duration = new QSpinBox(&dlg);
        duration->setRange(15, 480);
        duration->setValue(120);
        duration->setSuffix(QStringLiteral(" min"));
        auto* platform = new QComboBox(&dlg);
        platform->addItems({QStringLiteral("YouTube"), QStringLiteral("Twitch"), QStringLiteral("Facebook"), QStringLiteral("Custom")});
        platform->setCurrentText(e.platform.isEmpty() ? QStringLiteral("YouTube") : e.platform);
        auto* venue = new QLineEdit(&dlg);
        venue->setPlaceholderText(QStringLiteral("Venue"));
        auto* tags = new QLineEdit(&dlg);
        tags->setPlaceholderText(QStringLiteral("Tags"));
        auto* remind = new QCheckBox(QStringLiteral("Reminder before start"), &dlg);
        remind->setChecked(e.reminder);
        form->addRow(QStringLiteral("Title *"), title);
        form->addRow(QStringLiteral("Description"), desc);
        form->addRow(QStringLiteral("Sport"), sport);
        form->addRow(QStringLiteral("Platform"), platform);
        form->addRow(QStringLiteral("Start *"), when);
        form->addRow(QStringLiteral("Duration"), duration);
        form->addRow(QStringLiteral("Venue"), venue);
        form->addRow(QStringLiteral("Tags"), tags);
        form->addRow(remind);
        auto* ok = new QPushButton(QStringLiteral("Save"), &dlg);
        form->addRow(ok);
        connect(ok, &QPushButton::clicked, &dlg, &QDialog::accept);
        if (dlg.exec() != QDialog::Accepted) return false;
        e.title = title->text().trimmed();
        if (e.title.isEmpty()) return false;
        e.start = when->dateTime();
        e.platform = platform->currentText();
        e.notes = desc->toPlainText();
        if (!sport->text().isEmpty() || !venue->text().isEmpty() || !tags->text().isEmpty()) {
            e.notes = QStringLiteral("%1\nSport: %2 | Venue: %3 | Tags: %4 | %5min")
                          .arg(desc->toPlainText(), sport->text(), venue->text(), tags->text())
                          .arg(duration->value());
        }
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
    connect(edit, &QPushButton::clicked, this, [=] {
        if (*selected < 0 || *selected >= m_model.events().size()) return;
        editEvent(m_model.events().at(*selected), false);
    });
    connect(remove, &QPushButton::clicked, this, [=] {
        if (*selected < 0 || *selected >= m_model.events().size()) return;
        m_model.removeEvent(m_model.events().at(*selected).id);
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
