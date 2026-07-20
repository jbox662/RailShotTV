#pragma once
#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QHash>
#include <QColor>

namespace railshot {

class SidebarRail : public QWidget {
    Q_OBJECT
public:
    explicit SidebarRail(QWidget* parent = nullptr);
    void setActivePage(const QString& pageId);
    void setLive(bool live);

signals:
    void navigate(const QString& pageId);

private:
    QPushButton* addNav(const QString& id, const QString& tip, const QColor& color);
    void restyleNav();
    void paintEvent(QPaintEvent* event) override;

    QString m_active = QStringLiteral("dashboard");
    bool m_live = false;
    QWidget* m_liveBlock = nullptr;
    QLabel* m_version = nullptr;
    QHash<QString, QPushButton*> m_buttons;
    QHash<QString, QColor> m_colors;
};

} // namespace railshot
