#include "ui/pages/DashboardPage.h"
#include "ui/widgets/PreviewWidget.h"
#include "ui/widgets/TransitionPanel.h"
#include "ui/widgets/SceneListWidget.h"
#include "ui/widgets/InputTilesWidget.h"
#include "ui/widgets/AudioMixerWidget.h"
#include "ui/widgets/BottomToolbar.h"
#include "ui/widgets/GoLiveDialog.h"
#include "ui/widgets/SourcePropertiesWidget.h"
#include "ui/widgets/AddSourceDialog.h"
#include "core/EngineController.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QMenu>
#include <QMessageBox>
#include <QFile>
#include <QDir>
#include <QUrl>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QResizeEvent>
#include <QEvent>

namespace railshot {

namespace {
bool addBrowserPreset(EngineController* engine, const QString& name, const QString& resourcePath)
{
    const QString qrc = QStringLiteral(":/overlays/%1").arg(resourcePath);
    QFile f(qrc);
    if (!f.open(QIODevice::ReadOnly)) return false;
    const QString destDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                            + QStringLiteral("/overlays");
    QDir().mkpath(destDir);
    const QString dest = destDir + QLatin1Char('/') + resourcePath;
    {
        QFile out(dest);
        if (out.open(QIODevice::WriteOnly | QIODevice::Truncate))
            out.write(f.readAll());
    }
    QJsonObject settings;
    settings.insert(QStringLiteral("url"), QUrl::fromLocalFile(dest).toString());
    settings.insert(QStringLiteral("width"), 1280);
    settings.insert(QStringLiteral("height"), 720);
    settings.insert(QStringLiteral("fps"), 30);
    const QString id = engine->addSource(SourceType::Browser, name);
    if (id.isEmpty()) return false;
    engine->updateSourceSettings(id, settings);
    Transform t;
    t.x = 0.05; t.y = 0.72; t.w = 0.55; t.h = 0.22;
    engine->updateSourceTransform(id, t);
    engine->setSelectedSourceId(id);
    return true;
}
} // namespace

DashboardPage::DashboardPage(EngineController* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* monitors = new QHBoxLayout();
    monitors->setContentsMargins(0, 0, 0, 0);
    monitors->setSpacing(0);
    m_preview = new PreviewWidget(engine, false, this);
    m_program = new PreviewWidget(engine, true, this);
    auto* transitions = new TransitionPanel(engine, this);
    monitors->addWidget(m_preview, 1);
    monitors->addWidget(transitions);
    monitors->addWidget(m_program, 1);
    root->addLayout(monitors, 1);

    auto* inputsRow = new QHBoxLayout();
    inputsRow->setContentsMargins(0, 0, 0, 0);
    inputsRow->setSpacing(0);

    auto* scenesCol = new QWidget(this);
    scenesCol->setFixedWidth(180);
    scenesCol->setStyleSheet(QStringLiteral("background:#0A0C0F; border-right:1px solid #2A2D35;"));
    auto* scenesLay = new QVBoxLayout(scenesCol);
    scenesLay->setContentsMargins(0, 0, 0, 0);
    scenesLay->setSpacing(0);
    auto* scenesHeader = new QWidget(scenesCol);
    scenesHeader->setFixedHeight(28);
    scenesHeader->setStyleSheet(QStringLiteral(
        "background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #1A1D22, stop:1 #141619);"
        "border-bottom:1px solid #1A1D24; border-top:2px solid #4F9EFF;"));
    auto* scenesHeaderLay = new QHBoxLayout(scenesHeader);
    scenesHeaderLay->setContentsMargins(10, 0, 8, 0);
    auto* scenesTitle = new QLabel(QStringLiteral("SCENES"), scenesHeader);
    scenesTitle->setObjectName(QStringLiteral("panelTitleBlue"));
    auto* addScene = new QPushButton(QStringLiteral("+"), scenesHeader);
    addScene->setObjectName(QStringLiteral("panelAddButton"));
    addScene->setFixedSize(24, 20);
    addScene->setStyleSheet(QStringLiteral(
        "QPushButton{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #1A3AFF,stop:1 #1230CC);"
        "border:1px solid #3A6AFF;border-radius:3px;color:white;font-weight:800;}"));
    connect(addScene, &QPushButton::clicked, this, [this] {
        m_engine->sceneGraph()->mutate([](Project& p) { p.addScene(); });
    });
    scenesHeaderLay->addWidget(scenesTitle);
    scenesHeaderLay->addStretch();
    scenesHeaderLay->addWidget(addScene);
    scenesLay->addWidget(scenesHeader);

    auto* scenes = new SceneListWidget(engine, scenesCol);
    scenesLay->addWidget(scenes, 1);
    inputsRow->addWidget(scenesCol);

    m_tiles = new InputTilesWidget(engine, this);
    inputsRow->addWidget(m_tiles, 1);

    m_mixer = new AudioMixerWidget(engine, this);
    inputsRow->addWidget(m_mixer);
    root->addLayout(inputsRow);

    m_toolbar = new BottomToolbar(engine, this);
    root->addWidget(m_toolbar);

    m_drawerBackdrop = new QFrame(this);
    m_drawerBackdrop->setObjectName(QStringLiteral("drawerBackdrop"));
    m_drawerBackdrop->hide();
    m_drawerBackdrop->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    connect(m_drawerBackdrop, &QFrame::destroyed, this, [] {});

    m_props = new SourcePropertiesWidget(engine, this);
    m_props->hide();
    connect(m_props, &SourcePropertiesWidget::closeRequested, this, [this] { closeDrawer(); });

    // Click backdrop to close — use event filter via mouse
    m_drawerBackdrop->installEventFilter(this);

    connect(m_preview, &PreviewWidget::sourceSelected, this, [this](const QString& id) {
        m_engine->setSelectedSourceId(id);
    });

    connect(m_tiles, &InputTilesWidget::configureSourceRequested, this, [this](const QString&) {
        openDrawer();
    });

    connect(m_toolbar, &BottomToolbar::addInputRequested, this, [this] {
        AddSourceDialog dlg(m_engine, this);
        if (dlg.exec() != QDialog::Accepted) return;
        const auto r = dlg.result();
        if (!r.accepted) return;
        const QString id = m_engine->addSource(r.type, r.name);
        if (!id.isEmpty() && !r.settings.isEmpty())
            m_engine->updateSourceSettings(id, r.settings);
        m_engine->setSelectedSourceId(id);
        openDrawer();
    });

    connect(m_toolbar, &BottomToolbar::goLiveRequested, this, [this] {
        GoLiveDialog dlg(m_engine, this);
        dlg.exec();
    });

    connect(m_toolbar, &BottomToolbar::mixerToggleRequested, this, [this] {
        setMixerOpen(!m_mixerOpen);
    });

    connect(m_toolbar, &BottomToolbar::multiCorderRequested, this, [this] {
        QMessageBox::information(this, QStringLiteral("MultiCorder"),
                                 QStringLiteral("MultiCorder panel — Phase 2."));
    });
    connect(m_toolbar, &BottomToolbar::playListRequested, this, [this] {
        QMessageBox::information(this, QStringLiteral("PlayList"),
                                 QStringLiteral("PlayList panel — Phase 2."));
    });
    connect(m_toolbar, &BottomToolbar::overlayRequested, this, [this] {
        emit openSceneEditorRequested();
    });

    m_toolbar->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_toolbar, &QWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        QMenu menu;
        menu.addAction(QStringLiteral("Preset: Player Intro"), this, [this] {
            if (!addBrowserPreset(m_engine, QStringLiteral("Player Intro"), QStringLiteral("player-intro.html")))
                QMessageBox::warning(this, QStringLiteral("Preset"), QStringLiteral("Could not load player-intro.html"));
        });
        menu.addAction(QStringLiteral("Preset: Match Point"), this, [this] {
            if (!addBrowserPreset(m_engine, QStringLiteral("Match Point"), QStringLiteral("match-point.html")))
                QMessageBox::warning(this, QStringLiteral("Preset"), QStringLiteral("Could not load match-point.html"));
        });
        menu.addAction(QStringLiteral("Preset: Break and Run"), this, [this] {
            if (!addBrowserPreset(m_engine, QStringLiteral("Break and Run"), QStringLiteral("break-and-run.html")))
                QMessageBox::warning(this, QStringLiteral("Preset"), QStringLiteral("Could not load break-and-run.html"));
        });
        menu.exec(m_toolbar->mapToGlobal(pos));
    });

    auto* tick = new QTimer(this);
    connect(tick, &QTimer::timeout, this, [this] {
        m_preview->tick();
        m_program->tick();
    });
    tick->start(16);

    setFocusPolicy(Qt::StrongFocus);
}

void DashboardPage::setMixerOpen(bool open)
{
    m_mixerOpen = open;
    m_toolbar->setMixerOpen(open);
    const int target = open ? 320 : 0;
    auto* anim = new QPropertyAnimation(m_mixer, "maximumWidth", this);
    anim->setDuration(250);
    anim->setStartValue(m_mixer->width());
    anim->setEndValue(target);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    m_mixer->setMinimumWidth(0);
    if (open)
        m_mixer->setMaximumWidth(320);
    connect(anim, &QPropertyAnimation::valueChanged, this, [this](const QVariant& v) {
        m_mixer->setMaximumWidth(v.toInt());
        m_mixer->setMinimumWidth(0);
    });
    connect(anim, &QPropertyAnimation::finished, this, [this, target, anim] {
        m_mixer->setMaximumWidth(target);
        m_mixer->setMinimumWidth(0);
        anim->deleteLater();
    });
    anim->start();
}

void DashboardPage::openDrawer()
{
    layoutDrawer();
    m_drawerBackdrop->show();
    m_drawerBackdrop->raise();
    m_props->show();
    m_props->raise();
}

void DashboardPage::closeDrawer()
{
    m_props->hide();
    m_drawerBackdrop->hide();
}

void DashboardPage::layoutDrawer()
{
    if (!m_drawerBackdrop || !m_props) return;
    m_drawerBackdrop->setGeometry(0, 0, width(), height());
    m_props->setGeometry(width() - 460, 0, 460, height());
}

void DashboardPage::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    if (m_props && m_props->isVisible())
        layoutDrawer();
}

bool DashboardPage::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_drawerBackdrop && event->type() == QEvent::MouseButtonPress) {
        closeDrawer();
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

} // namespace railshot
