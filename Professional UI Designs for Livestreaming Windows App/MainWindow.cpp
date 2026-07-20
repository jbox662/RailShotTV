#include "MainWindow.h"
#include "SidebarRail.h"
#include "pages/DashboardPage.h"
#include "pages/SceneEditorPage.h"
#include "pages/ChatPage.h"
#include "pages/AnalyticsPage.h"
#include "pages/SettingsPage.h"

#include <QHBoxLayout>
#include <QStackedWidget>
#include <QPainter>
#include <QPaintEvent>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setObjectName("AppRoot");
    setupUi();
}

void MainWindow::setupUi()
{
    // Central widget holds the entire layout
    QWidget *central = new QWidget(this);
    central->setObjectName("AppRoot");
    setCentralWidget(central);

    // ── Horizontal root layout: Sidebar | Pages ────────────────────────────
    QHBoxLayout *rootLayout = new QHBoxLayout(central);
    rootLayout->setContentsMargins(0, 3, 0, 0); // 3px top for live border
    rootLayout->setSpacing(0);

    // ── Sidebar rail ───────────────────────────────────────────────────────
    m_sidebar = new SidebarRail(this);
    rootLayout->addWidget(m_sidebar, 0);

    // ── Page stack ─────────────────────────────────────────────────────────
    m_stack = new QStackedWidget(this);
    m_stack->setObjectName("PageStack");

    m_dashboard = new DashboardPage(this);
    m_scenes    = new SceneEditorPage(this);
    m_chat      = new ChatPage(this);
    m_analytics = new AnalyticsPage(this);
    m_settings  = new SettingsPage(this);

    m_stack->addWidget(m_dashboard);   // index 0
    m_stack->addWidget(m_scenes);      // index 1
    m_stack->addWidget(m_chat);        // index 2
    m_stack->addWidget(m_analytics);   // index 3
    m_stack->addWidget(m_settings);    // index 4

    rootLayout->addWidget(m_stack, 1);

    // ── Connect sidebar nav to page switching ──────────────────────────────
    connect(m_sidebar, &SidebarRail::pageRequested,
            this, &MainWindow::navigateTo);

    // Start on Dashboard
    navigateTo(PageDashboard);
}

void MainWindow::navigateTo(int page)
{
    m_stack->setCurrentIndex(page);
    m_sidebar->setActivePage(page);
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    QMainWindow::paintEvent(event);

    // Draw the 3px orange "live top border" across the full window width
    QPainter painter(this);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0xFF, 0x5A, 0x2C)); // #FF5A2C brand-orange
    painter.drawRect(0, 0, width(), 3);
}

