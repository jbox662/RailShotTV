#pragma once
#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>

namespace railshot {

class EngineController;

class SidebarRail : public QWidget {
    Q_OBJECT
public:
    explicit SidebarRail(QWidget* parent = nullptr);
signals:
    void navigate(const QString& pageId);
private:
    QPushButton* addNav(const QString& id, const QString& tip, const QColor& color);
    QString m_active = QStringLiteral("dashboard");
};

} // namespace railshot
