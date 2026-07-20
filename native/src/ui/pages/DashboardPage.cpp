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
#include "ui/widgets/StreamStatusWidget.h"
#include "core/EngineController.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QMenu>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QFile>
#include <QDir>
#include <QUrl>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDialog>
#include <QLabel>
#include <QPushButton>

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

    auto* topRow = new QHBoxLayout();
    topRow->setContentsMargins(0, 0, 0, 0);
    topRow->setSpacing(0);

    auto* monitors = new QHBoxLayout();
    monitors->setContentsMargins(0, 0, 0, 0);
    monitors->setSpacing(0);
    m_preview = new PreviewWidget(engine, false, this);
    m_program = new PreviewWidget(engine, true, this);
    auto* transitions = new TransitionPanel(engine, this);
    monitors->addWidget(m_preview, 1);
    monitors->addWidget(transitions);
    monitors->addWidget(m_program, 1);
    topRow->addLayout(monitors, 1);
    topRow->addWidget(new StreamStatusWidget(engine, this));
    root->addLayout(topRow, 1);

    auto* inputsRow = new QHBoxLayout();
    inputsRow->setContentsMargins(0, 0, 0, 0);
    inputsRow->setSpacing(0);

    auto* scenesCol = new QWidget(this);
    scenesCol->setFixedWidth(180);
    scenesCol->setStyleSheet(QStringLiteral("background:#0A0C0F; border-right:1px solid #1A1D24;"));
    auto* scenesLay = new QVBoxLayout(scenesCol);
    scenesLay->setContentsMargins(0, 0, 0, 0);
    scenesLay->setSpacing(0);
    auto* scenesHeader = new QWidget(scenesCol);
    scenesHeader->setFixedHeight(28);
    scenesHeader->setStyleSheet(QStringLiteral(
        "background:#0F1114; border-bottom:1px solid #1A1D24; border-top:2px solid #4F9EFF;"));
    auto* scenesHeaderLay = new QHBoxLayout(scenesHeader);
    scenesHeaderLay->setContentsMargins(10, 0, 8, 0);
    auto* scenesTitle = new QLabel(QStringLiteral("SCENES"), scenesHeader);
    scenesTitle->setStyleSheet(QStringLiteral(
        "color:#4F9EFF; font-weight:800; font-size:11px; letter-spacing:1.5px; background:transparent;"));
    auto* addScene = new QPushButton(QStringLiteral("+"), scenesHeader);
    addScene->setObjectName(QStringLiteral("panelAddButton"));
    addScene->setFixedSize(24, 20);
    connect(addScene, &QPushButton::clicked, this, [this] {
        m_engine->sceneGraph()->mutate([](Project& p) { p.addScene(); });
    });
    scenesHeaderLay->addWidget(scenesTitle);
    scenesHeaderLay->addStretch();
    scenesHeaderLay->addWidget(addScene);
    scenesLay->addWidget(scenesHeader);

    auto* scenes = new SceneListWidget(engine, scenesCol);
    scenes->setStyleSheet(QStringLiteral(
        "QListWidget { background:#0A0C0F; border:none; }"
        "QListWidget::item { padding:10px 12px; border-bottom:1px solid #15181E;"
        "  border-left:3px solid transparent; color:#C8CAD0; font-weight:600; }"
        "QListWidget::item:selected { background:#122033; color:#4F9EFF; border-left:3px solid #4F9EFF; }"
        "QListWidget::item:hover { background:#12151A; }"));
    scenes->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(scenes, &QWidget::customContextMenuRequested, this, [this, scenes](const QPoint& pos) {
        QMenu menu;
        menu.addAction(QStringLiteral("Add Scene"), this, [this] {
            m_engine->sceneGraph()->mutate([](Project& p) { p.addScene(); });
        });
        menu.addAction(QStringLiteral("Duplicate"), this, [this, scenes] {
            if (auto* item = scenes->currentItem())
                m_engine->sceneGraph()->mutate([&](Project& p) { p.duplicateScene(item->data(Qt::UserRole).toString()); });
        });
        menu.exec(scenes->mapToGlobal(pos));
    });
    scenesLay->addWidget(scenes, 1);
    inputsRow->addWidget(scenesCol);
    inputsRow->addWidget(new InputTilesWidget(engine, this), 1);
    inputsRow->addWidget(new SourcePropertiesWidget(engine, this));
    inputsRow->addWidget(new AudioMixerWidget(engine, this));
    root->addLayout(inputsRow);

    auto* toolbar = new BottomToolbar(engine, this);
    root->addWidget(toolbar);

    connect(m_preview, &PreviewWidget::sourceSelected, this, [this](const QString& id) {
        m_engine->setSelectedSourceId(id);
    });

    connect(toolbar, &BottomToolbar::addInputRequested, this, [this] {
        AddSourceDialog dlg(m_engine, this);
        if (dlg.exec() != QDialog::Accepted) return;
        const auto r = dlg.result();
        if (!r.accepted) return;
        const QString id = m_engine->addSource(r.type, r.name);
        if (!id.isEmpty() && !r.settings.isEmpty())
            m_engine->updateSourceSettings(id, r.settings);
        m_engine->setSelectedSourceId(id);
    });

    connect(toolbar, &BottomToolbar::goLiveRequested, this, [this] {
        GoLiveDialog dlg(m_engine, this);
        dlg.exec();
    });

    // Billiards overlay presets via toolbar context (right-click Add)
    toolbar->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(toolbar, &QWidget::customContextMenuRequested, this, [this, toolbar](const QPoint& pos) {
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
        menu.exec(toolbar->mapToGlobal(pos));
    });

    auto* tick = new QTimer(this);
    connect(tick, &QTimer::timeout, this, [this] {
        m_preview->tick();
        m_program->tick();
    });
    tick->start(16);

    setFocusPolicy(Qt::StrongFocus);
}

} // namespace railshot
