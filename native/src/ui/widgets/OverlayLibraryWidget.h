#pragma once
#include "core/Types.h"
#include <QWidget>
#include <QListWidget>

namespace railshot {

struct OverlayTemplateInfo {
    QString id;
    QString category; // scoreboard|lowerthird|ticker|alert|branding
    QString name;
    QString description;
    QString accent;
    SourceType sourceType = SourceType::Browser;
    QString htmlResource; // optional qrc overlay filename
};

class OverlayLibraryWidget : public QWidget {
    Q_OBJECT
public:
    explicit OverlayLibraryWidget(QWidget* parent = nullptr);

signals:
    void templateActivated(const OverlayTemplateInfo& tmpl);

private:
    void rebuild(const QString& categoryFilter);
    QListWidget* m_list = nullptr;
    QString m_filter;
};

} // namespace railshot
