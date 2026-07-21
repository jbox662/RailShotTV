#include "ui/widgets/MultiviewWindow.h"
#include "compositor/PreviewSurface.h"
#include "compositor/D3D11Compositor.h"
#include "core/EngineController.h"
#include "core/Types.h"
#include <QTimer>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScreen>
#include <QGuiApplication>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QMouseEvent>

namespace railshot {

namespace {
QList<MultiviewWindow*>& liveMultiviews()
{
    static QList<MultiviewWindow*> list;
    return list;
}
} // namespace

MultiviewWindow::MultiviewWindow(EngineController* engine, QWidget* parent)
    : QMainWindow(parent), m_engine(engine)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlag(Qt::Window, true);
    setWindowTitle(QStringLiteral("Multiview"));
    resize(1280, 720);
    setMinimumSize(640, 360);

    auto* central = new QWidget(this);
    central->setStyleSheet(QStringLiteral("background:#0A0C0F;"));
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(6, 6, 6, 6);
    root->setSpacing(6);

    auto* feeds = new QHBoxLayout();
    auto makeFeed = [&](const QString& label, const QColor& color, PreviewSurface** out) {
        auto* col = new QVBoxLayout();
        auto* lab = new QLabel(label, central);
        lab->setStyleSheet(QStringLiteral("color:%1; font-weight:800; font-size:11px; letter-spacing:1px;")
                               .arg(color.name()));
        auto* surface = new PreviewSurface(central);
        if (engine && engine->graphicsDevice())
            surface->setDevice(engine->graphicsDevice());
        surface->setLabel(label, color);
        surface->setEmptyMessage(QStringLiteral("NO SIGNAL"));
        surface->setMinimumHeight(180);
        col->addWidget(lab);
        col->addWidget(surface, 1);
        feeds->addLayout(col, 1);
        *out = surface;
    };
    makeFeed(QStringLiteral("PREVIEW"), QColor(QStringLiteral("#22C55E")), &m_preview);
    makeFeed(QStringLiteral("PROGRAM"), QColor(QStringLiteral("#FF5A2C")), &m_program);
    root->addLayout(feeds, 2);

    auto* scenesLab = new QLabel(QStringLiteral("SCENES — click = Preview · double-click = GO"), central);
    scenesLab->setStyleSheet(QStringLiteral("color:#8892A4; font-size:10px; letter-spacing:1px;"));
    root->addWidget(scenesLab);

    m_gridHost = new QWidget(central);
    m_grid = new QGridLayout(m_gridHost);
    m_grid->setContentsMargins(0, 0, 0, 0);
    m_grid->setSpacing(4);
    root->addWidget(m_gridHost, 1);

    setCentralWidget(central);

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &MultiviewWindow::tick);
    m_timer->start(16);

    rebuildSceneGrid();
    liveMultiviews().append(this);
}

MultiviewWindow::~MultiviewWindow()
{
    liveMultiviews().removeAll(this);
}

void MultiviewWindow::rebuildSceneGrid()
{
    if (!m_grid || !m_engine) return;
    while (QLayoutItem* item = m_grid->takeAt(0)) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    const auto project = m_engine->projectSnapshot();
    m_lastPreviewId = project.previewSceneId;
    m_lastProgramId = project.programSceneId;
    m_lastSceneCount = project.scenes.size();

    const int cols = 4;
    for (int i = 0; i < project.scenes.size(); ++i) {
        const auto& sc = project.scenes[i];
        auto* btn = new QPushButton(sc.name, m_gridHost);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setMinimumHeight(56);
        const bool isPrev = sc.id == project.previewSceneId;
        const bool isProg = sc.id == project.programSceneId;
        QString border = QStringLiteral("#3A3D45");
        if (isProg) border = QStringLiteral("#FF5A2C");
        else if (isPrev) border = QStringLiteral("#22C55E");
        btn->setStyleSheet(QStringLiteral(
            "QPushButton{background:#14171C; color:#E0E2E8; border:2px solid %1;"
            " border-radius:4px; font-weight:700; text-align:left; padding:8px 12px;}"
            "QPushButton:hover{background:#1E2228; border-color:#4F9EFF;}")
                               .arg(border));
        btn->setProperty("sceneId", sc.id);
        btn->installEventFilter(this);
        connect(btn, &QPushButton::clicked, this, [this, id = sc.id] {
            if (m_engine) m_engine->setPreviewScene(id);
            rebuildSceneGrid();
        });
        m_grid->addWidget(btn, i / cols, i % cols);
    }
}

bool MultiviewWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonDblClick) {
        if (auto* w = qobject_cast<QWidget*>(watched)) {
            const QString id = w->property("sceneId").toString();
            if (!id.isEmpty() && m_engine) {
                m_engine->setPreviewScene(id);
                m_engine->go(TransitionType::Cut);
                rebuildSceneGrid();
                return true;
            }
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MultiviewWindow::tick()
{
    if (!m_engine || !m_engine->compositor()) return;
    if (m_engine->graphicsDevice()) {
        if (m_preview) m_preview->setDevice(m_engine->graphicsDevice());
        if (m_program) m_program->setDevice(m_engine->graphicsDevice());
    }
    if (m_preview && m_engine->compositor()->previewTexture())
        m_preview->presentTexture(m_engine->compositor()->previewTexture());
    if (m_program && m_engine->compositor()->programTexture())
        m_program->presentTexture(m_engine->compositor()->programTexture());

    const auto p = m_engine->projectSnapshot();
    if (p.previewSceneId != m_lastPreviewId || p.programSceneId != m_lastProgramId
        || p.scenes.size() != m_lastSceneCount)
        rebuildSceneGrid();
}

void MultiviewWindow::closeEvent(QCloseEvent* event)
{
    if (m_fullscreen) {
        showNormal();
        m_fullscreen = false;
    }
    QMainWindow::closeEvent(event);
}

void MultiviewWindow::keyPressEvent(QKeyEvent* event)
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

MultiviewWindow* MultiviewWindow::openWindowed(EngineController* engine, QWidget* parent)
{
    auto* w = new MultiviewWindow(engine, parent);
    w->show();
    w->raise();
    w->activateWindow();
    return w;
}

MultiviewWindow* MultiviewWindow::openFullscreen(EngineController* engine, QWidget* anchor)
{
    auto* w = new MultiviewWindow(engine, nullptr);
    QScreen* screen = anchor && anchor->screen() ? anchor->screen() : QGuiApplication::primaryScreen();
    if (screen)
        w->setGeometry(screen->geometry());
    w->m_fullscreen = true;
    w->showFullScreen();
    return w;
}

} // namespace railshot
