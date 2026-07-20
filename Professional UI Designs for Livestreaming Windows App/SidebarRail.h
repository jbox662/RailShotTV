#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QVector>

/**
 * SidebarRail
 *
 * The 56px-wide vertical icon navigation column on the left edge.
 *
 * Structure (top to bottom):
 *   ┌──────────────────┐
 *   │  Logo mark (32px)│  ← QLabel with SVG icon, 32×32
 *   │  ─────────────── │  ← 1px separator
 *   │  [Dashboard]     │  ← NavButton, icon 20×20
 *   │  [Scene Editor]  │
 *   │  [Chat]          │
 *   │  [Analytics]     │
 *   │  ─────────────── │  ← spacer pushes settings to bottom
 *   │  [Settings]      │
 *   │  ─────────────── │
 *   │  Signal bar      │  ← 5-bar signal strength indicator (custom paint)
 *   │  v2.5            │  ← version label
 *   └──────────────────┘
 *
 * Emits pageRequested(int) when a nav button is clicked.
 * Call setActivePage(int) to update the active button state.
 */
class SidebarRail : public QWidget
{
    Q_OBJECT

public:
    explicit SidebarRail(QWidget *parent = nullptr);
    ~SidebarRail() override = default;

    void setActivePage(int page);

signals:
    void pageRequested(int page);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void setupUi();
    QPushButton *makeNavButton(const QString &iconPath,
                               const QString &tooltip,
                               const QString &pageId,
                               int pageIndex);

    QVector<QPushButton *> m_navButtons;
    int m_activePage = 0;
};
