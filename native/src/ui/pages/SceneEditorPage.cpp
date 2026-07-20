#include "ui/pages/SceneEditorPage.h"
#include "ui/Theme.h"
#include "ui/widgets/OverlayLibraryWidget.h"
#include "ui/widgets/PreviewWidget.h"
#include "ui/widgets/SceneListWidget.h"
#include "ui/widgets/SourcePropertiesWidget.h"
#include "core/EngineController.h"
#include "core/SceneGraph.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QTimer>
#include <QFile>
#include <QDir>
#include <QUrl>
#include <QJsonObject>
#include <QStandardPaths>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QAbstractAnimation>
#include <QPainter>
#include <QPaintEvent>

namespace railshot {

namespace {
class CanvasHost : public QFrame {
public:
    explicit CanvasHost(QWidget* parent = nullptr) : QFrame(parent)
    {
        setObjectName(QStringLiteral("sceneCanvasHost"));
        setStyleSheet(QStringLiteral("QFrame#sceneCanvasHost{background:#060608; border:none;}"));
    }
protected:
    void paintEvent(QPaintEvent* e) override
    {
        QFrame::paintEvent(e);
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, false);
        QColor grid(79, 158, 255, 15);
        p.setPen(QPen(grid, 1));
        constexpr int step = 40;
        for (int x = 0; x < width(); x += step)
            p.drawLine(x, 0, x, height());
        for (int y = 0; y < height(); y += step)
            p.drawLine(0, y, width(), y);

        p.setPen(QPen(QColor(QStringLiteral("#4F9EFF")), 2));
        constexpr int L = 18;
        // corner brackets
        p.drawLine(8, 8, 8 + L, 8);
        p.drawLine(8, 8, 8, 8 + L);
        p.drawLine(width() - 8, 8, width() - 8 - L, 8);
        p.drawLine(width() - 8, 8, width() - 8, 8 + L);
        p.drawLine(8, height() - 8, 8 + L, height() - 8);
        p.drawLine(8, height() - 8, 8, height() - 8 - L);
        p.drawLine(width() - 8, height() - 8, width() - 8 - L, height() - 8);
        p.drawLine(width() - 8, height() - 8, width() - 8, height() - 8 - L);
    }
};
} // namespace

SceneEditorPage::SceneEditorPage(EngineController* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setObjectName(QStringLiteral("sceneEditorPage"));
    setFocusPolicy(Qt::StrongFocus);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* header = theme::makePageHeader(QStringLiteral("Scene Editor"), theme::PanelAccent::Blue, this);
    auto* headerLay = qobject_cast<QHBoxLayout*>(header->layout());
    auto* back = new QPushButton(QStringLiteral("← Dashboard"), header);
    back->setObjectName(QStringLiteral("chromeBtnPrimary"));
    connect(back, &QPushButton::clicked, this, &SceneEditorPage::backToDashboard);
    if (headerLay) headerLay->addWidget(back);
    root->addWidget(header);

    auto* body = new QHBoxLayout();
    body->setContentsMargins(0, 0, 0, 0);
    body->setSpacing(0);

    // Left: scenes
    auto* left = new QFrame(this);
    left->setObjectName(QStringLiteral("sceneEditorLeft"));
    left->setFixedWidth(180);
    auto* leftLay = new QVBoxLayout(left);
    leftLay->setContentsMargins(0, 0, 0, 0);
    auto* scenesHead = new QLabel(QStringLiteral("  SCENES"), left);
    scenesHead->setFixedHeight(28);
    scenesHead->setStyleSheet(QStringLiteral(
        "color:#4F9EFF; font-weight:900; font-size:10px; letter-spacing:2px;"
        "background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 rgba(79,158,255,0.28),stop:0.55 transparent);"
        "border-bottom:1px solid #3A3D45; border-left:3px solid #4F9EFF;"));
    leftLay->addWidget(scenesHead);
    leftLay->addWidget(new SceneListWidget(engine, left), 1);
    body->addWidget(left);

    // Canvas with grid + brackets
    auto* canvasCol = new QVBoxLayout();
    canvasCol->setContentsMargins(0, 0, 0, 0);
    canvasCol->setSpacing(0);
    m_canvasHost = new CanvasHost(this);
    auto* hostLay = new QVBoxLayout(m_canvasHost);
    hostLay->setContentsMargins(12, 12, 12, 12);
    m_canvas = new PreviewWidget(engine, false, m_canvasHost);
    m_canvas->setStyleSheet(QStringLiteral("background:transparent; border:2px solid #3A3D45;"));
    hostLay->addWidget(m_canvas, 1);
    canvasCol->addWidget(m_canvasHost, 1);

    // Transitions rail
    auto* trans = new QFrame(this);
    trans->setObjectName(QStringLiteral("sceneTransRail"));
    trans->setFixedHeight(40);
    auto* tLay = new QHBoxLayout(trans);
    tLay->setContentsMargins(8, 4, 8, 4);
    const QStringList types = {QStringLiteral("Cut"), QStringLiteral("Fade"), QStringLiteral("Wipe"),
                               QStringLiteral("Slide"), QStringLiteral("Stinger")};
    for (const auto& t : types) {
        auto* b = new QPushButton(t, trans);
        b->setObjectName(QStringLiteral("chromeBtn"));
        b->setCursor(Qt::PointingHandCursor);
        connect(b, &QPushButton::clicked, this, [this, t] {
            TransitionType ty = TransitionType::Cut;
            if (t == QLatin1String("Fade")) ty = TransitionType::Fade;
            else if (t == QLatin1String("Wipe")) ty = TransitionType::Wipe;
            else if (t == QLatin1String("Slide")) ty = TransitionType::Merge;
            else if (t == QLatin1String("Stinger")) ty = TransitionType::CubeZoom;
            m_engine->setTransition(ty, 300);
            m_engine->go(ty);
        });
        tLay->addWidget(b);
    }
    tLay->addStretch();
    auto* go = new QPushButton(QStringLiteral("GO"), trans);
    go->setObjectName(QStringLiteral("goButton"));
    go->setMinimumWidth(72);
    connect(go, &QPushButton::clicked, this, [this] { m_engine->go(); });
    tLay->addWidget(go);
    canvasCol->addWidget(trans);
    body->addLayout(canvasCol, 1);

    // Overlay library
    auto* lib = new OverlayLibraryWidget(this);
    connect(lib, &OverlayLibraryWidget::templateActivated, this, &SceneEditorPage::applyOverlayTemplate);
    body->addWidget(lib);

    // Compact properties (not full drawer)
    auto* props = new SourcePropertiesWidget(engine, this);
    // SourcePropertiesWidget is fixed 460 — shrink for editor
    props->setFixedWidth(320);
    props->setMaximumWidth(320);
    body->addWidget(props);

    root->addLayout(body, 1);

    auto* tick = new QTimer(this);
    connect(tick, &QTimer::timeout, this, [this] {
        if (m_canvas) m_canvas->tick();
    });
    tick->start(16);

    // Fade-in
    auto* fx = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(fx);
    fx->setOpacity(0.0);
    auto* anim = new QPropertyAnimation(fx, "opacity", this);
    anim->setDuration(200);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void SceneEditorPage::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        emit backToDashboard();
        return;
    }
    QWidget::keyPressEvent(event);
}

void SceneEditorPage::applyOverlayTemplate(const OverlayTemplateInfo& tmpl)
{
    QString name = tmpl.name;
    QJsonObject settings;
    if (!tmpl.htmlResource.isEmpty()) {
        const QString qrc = QStringLiteral(":/overlays/%1").arg(tmpl.htmlResource);
        QFile f(qrc);
        if (f.open(QIODevice::ReadOnly)) {
            const QString destDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                                    + QStringLiteral("/overlays");
            QDir().mkpath(destDir);
            const QString dest = destDir + QLatin1Char('/') + tmpl.htmlResource;
            QFile out(dest);
            if (out.open(QIODevice::WriteOnly | QIODevice::Truncate))
                out.write(f.readAll());
            settings.insert(QStringLiteral("url"), QUrl::fromLocalFile(dest).toString());
            settings.insert(QStringLiteral("width"), 1280);
            settings.insert(QStringLiteral("height"), 720);
            settings.insert(QStringLiteral("fps"), 30);
        }
    }
    settings.insert(QStringLiteral("overlayTemplate"), tmpl.id);
    settings.insert(QStringLiteral("layout"), tmpl.id);
    const QString id = m_engine->addSource(tmpl.sourceType, name);
    if (!id.isEmpty() && !settings.isEmpty())
        m_engine->updateSourceSettings(id, settings);
    m_engine->setSelectedSourceId(id);

    Transform t;
    if (tmpl.category == QLatin1String("lowerthird") || tmpl.id.startsWith(QLatin1String("sb-lower"))) {
        t.x = 0.05; t.y = 0.78; t.w = 0.55; t.h = 0.18;
    } else if (tmpl.category == QLatin1String("ticker")) {
        t.x = 0.0; t.y = 0.92; t.w = 1.0; t.h = 0.08;
    } else if (tmpl.category == QLatin1String("alert")) {
        t.x = 0.3; t.y = 0.3; t.w = 0.4; t.h = 0.3;
    } else {
        t.x = 0.05; t.y = 0.05; t.w = 0.25; t.h = 0.15;
    }
    if (!id.isEmpty())
        m_engine->updateSourceTransform(id, t);
    if (tmpl.sourceType == SourceType::Scoreboard)
        m_engine->pushScoreboardToProgram();
    flashDrop();
}

void SceneEditorPage::flashDrop()
{
    if (!m_canvasHost) return;
    m_canvasHost->setStyleSheet(QStringLiteral(
        "QFrame#sceneCanvasHost{background:#060608; border:2px solid #22C55E;}"));
    QTimer::singleShot(280, this, [this] {
        if (m_canvasHost)
            m_canvasHost->setStyleSheet(QStringLiteral(
                "QFrame#sceneCanvasHost{background:#060608; border:none;}"));
    });
}

} // namespace railshot
