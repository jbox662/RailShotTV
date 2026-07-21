#include "ui/widgets/ProjectorWindow.h"
#include "compositor/PreviewSurface.h"
#include "compositor/D3D11Compositor.h"
#include "core/EngineController.h"
#include <QTimer>
#include <QScreen>
#include <QGuiApplication>
#include <QContextMenuEvent>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QMenu>
#include <QAction>
#include <QVBoxLayout>
#include <QWidget>

namespace railshot {

namespace {
QList<ProjectorWindow*>& liveProjectors()
{
    static QList<ProjectorWindow*> list;
    return list;
}
} // namespace

ProjectorWindow::ProjectorWindow(EngineController* engine, ProjectorKind kind, QWidget* parent)
    : QMainWindow(parent), m_engine(engine), m_kind(kind)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlag(Qt::Window, true);
    resize(960, 540);
    setMinimumSize(320, 180);

    auto* central = new QWidget(this);
    central->setStyleSheet(QStringLiteral("background:#000;"));
    auto* lay = new QVBoxLayout(central);
    lay->setContentsMargins(0, 0, 0, 0);
    m_surface = new PreviewSurface(central);
    if (engine && engine->graphicsDevice())
        m_surface->setDevice(engine->graphicsDevice());
    const bool program = kind == ProjectorKind::Program;
    m_surface->setLabel(program ? QStringLiteral("PROGRAM") : QStringLiteral("PREVIEW"),
                        program ? QColor(QStringLiteral("#FF5A2C")) : QColor(QStringLiteral("#22C55E")));
    m_surface->setEmptyMessage(program ? QStringLiteral("NO OUTPUT") : QStringLiteral("NO PREVIEW"));
    lay->addWidget(m_surface, 1);
    setCentralWidget(central);

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &ProjectorWindow::tick);
    m_timer->start(16);

    rebuildTitle();
    liveProjectors().append(this);
}

ProjectorWindow::~ProjectorWindow()
{
    liveProjectors().removeAll(this);
}

void ProjectorWindow::rebuildTitle()
{
    const QString base = m_kind == ProjectorKind::Program ? QStringLiteral("Program Projector")
                                                          : QStringLiteral("Preview Projector");
    setWindowTitle(m_alwaysOnTop ? base + QStringLiteral(" — Always on Top") : base);
}

void ProjectorWindow::setAlwaysOnTop(bool on)
{
    m_alwaysOnTop = on;
    setWindowFlag(Qt::WindowStaysOnTopHint, on);
    show(); // re-apply flags
    rebuildTitle();
}

void ProjectorWindow::tick()
{
    if (!m_engine || !m_engine->compositor() || !m_surface)
        return;
    if (m_engine->graphicsDevice())
        m_surface->setDevice(m_engine->graphicsDevice());
    auto* tex = m_kind == ProjectorKind::Program ? m_engine->compositor()->programTexture()
                                                 : m_engine->compositor()->previewTexture();
    if (tex)
        m_surface->presentTexture(tex);
}

void ProjectorWindow::closeEvent(QCloseEvent* event)
{
    if (m_fullscreen) {
        showNormal();
        m_fullscreen = false;
    }
    QMainWindow::closeEvent(event);
}

void ProjectorWindow::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        if (m_fullscreen) {
            showNormal();
            m_fullscreen = false;
        } else {
            close();
        }
        return;
    }
    QMainWindow::keyPressEvent(event);
}

void ProjectorWindow::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu(this);
    auto* fs = menu.addAction(m_fullscreen ? QStringLiteral("Windowed") : QStringLiteral("Fullscreen"));
    auto* aot = menu.addAction(QStringLiteral("Always on Top"));
    aot->setCheckable(true);
    aot->setChecked(m_alwaysOnTop);
    menu.addSeparator();
    auto* closeAct = menu.addAction(QStringLiteral("Close"));
    QAction* chosen = menu.exec(event->globalPos());
    if (chosen == fs) {
        if (m_fullscreen) {
            showNormal();
            m_fullscreen = false;
        } else {
            showFullScreen();
            m_fullscreen = true;
        }
    } else if (chosen == aot) {
        setAlwaysOnTop(!m_alwaysOnTop);
    } else if (chosen == closeAct) {
        close();
    }
}

ProjectorWindow* ProjectorWindow::openWindowed(EngineController* engine, ProjectorKind kind, QWidget* parent)
{
    auto* w = new ProjectorWindow(engine, kind, parent);
    w->show();
    w->raise();
    w->activateWindow();
    return w;
}

ProjectorWindow* ProjectorWindow::openFullscreen(EngineController* engine, ProjectorKind kind, QWidget* anchor)
{
    auto* w = new ProjectorWindow(engine, kind, nullptr);
    QScreen* screen = anchor && anchor->screen() ? anchor->screen() : QGuiApplication::primaryScreen();
    if (screen)
        w->setGeometry(screen->geometry());
    w->m_fullscreen = true;
    w->showFullScreen();
    return w;
}

} // namespace railshot
