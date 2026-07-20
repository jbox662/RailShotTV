#pragma once
#include <QWidget>
#include "schedule/ScheduleModel.h"

namespace railshot {

class SchedulePage : public QWidget {
    Q_OBJECT
public:
    explicit SchedulePage(QWidget* parent = nullptr);

signals:
    void goLiveRequested(const QString& title, const QString& platform);

private:
    ScheduleModel m_model;
};

} // namespace railshot
