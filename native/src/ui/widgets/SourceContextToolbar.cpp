#include "ui/widgets/SourceContextToolbar.h"
#include "core/EngineController.h"
#include "core/SceneGraph.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLayoutItem>

namespace railshot {

SourceContextToolbar::SourceContextToolbar(EngineController* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setObjectName(QStringLiteral("sourceContextToolbar"));
    setFixedHeight(30);
    setStyleSheet(QStringLiteral(
        "QWidget#sourceContextToolbar{"
        "  background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #1A1D22,stop:1 #0D0F12);"
        "  border-top:1px solid #2A2D35;"
        "}"
        "QWidget#sourceContextToolbar QLabel#ctxTitle{"
        "  color:#8892A4; font-family:'DM Sans'; font-size:10px; font-weight:700; padding-left:8px;"
        "}"
        "QWidget#sourceContextToolbar QPushButton{"
        "  background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #32363F,stop:1 #1A1E26);"
        "  border:1px solid #5A5E68; border-radius:3px; color:#E0E2E8;"
        "  font-family:'DM Sans'; font-size:10px; font-weight:700;"
        "  padding:2px 8px; min-height:20px; max-height:22px;"
        "}"
        "QWidget#sourceContextToolbar QPushButton:hover{border-color:#4F9EFF; color:#FFFFFF;}"
        "QWidget#sourceContextToolbar QPushButton:disabled{"
        "  color:#505860; border-color:#2A2D35; background:#16181E;}"));

    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(4, 3, 6, 3);
    row->setSpacing(4);

    m_title = new QLabel(QStringLiteral("No source selected"), this);
    m_title->setObjectName(QStringLiteral("ctxTitle"));
    row->addWidget(m_title);

    m_actions = new QWidget(this);
    auto* actionsLay = new QHBoxLayout(m_actions);
    actionsLay->setContentsMargins(0, 0, 0, 0);
    actionsLay->setSpacing(4);
    row->addWidget(m_actions, 1);
    row->addStretch();

    connect(m_engine->sceneGraph(), &SceneGraph::projectChanged, this, &SourceContextToolbar::rebuild);
    connect(m_engine, &EngineController::selectedSourceChanged, this, &SourceContextToolbar::setSourceId);
    connect(m_engine, &EngineController::showHideFadeChanged, this, &SourceContextToolbar::rebuild);
    setSourceId(m_engine->selectedSourceId());
}

QPushButton* SourceContextToolbar::addBtn(const QString& text, const QString& tip)
{
    auto* b = new QPushButton(text, m_actions);
    b->setCursor(Qt::PointingHandCursor);
    b->setToolTip(tip);
    m_actions->layout()->addWidget(b);
    return b;
}

void SourceContextToolbar::setSourceId(const QString& sourceId)
{
    m_sourceId = sourceId;
    rebuild();
}

void SourceContextToolbar::rebuild()
{
    while (auto* item = m_actions->layout()->takeAt(0)) {
        delete item->widget();
        delete item;
    }

    if (m_sourceId.isEmpty()) {
        m_title->setText(QStringLiteral("No source selected"));
        return;
    }

    const auto* src = m_engine->projectSnapshot().findSourceAnywhere(m_sourceId);
    if (!src) {
        m_title->setText(QStringLiteral("No source selected"));
        m_sourceId.clear();
        return;
    }

    m_title->setText(QStringLiteral("%1 · %2").arg(sourceTypeToString(src->type), src->name));

    auto* vis = addBtn(m_engine->sourceVisibilityTarget(m_sourceId) ? QStringLiteral("Hide") : QStringLiteral("Show"),
                       QStringLiteral("Toggle visibility"));
    connect(vis, &QPushButton::clicked, this, [this] {
        m_engine->toggleSourceVisible(m_sourceId);
    });

    auto* lock = addBtn(src->locked ? QStringLiteral("Unlock") : QStringLiteral("Lock"),
                        QStringLiteral("Toggle lock"));
    connect(lock, &QPushButton::clicked, this, [this] {
        const auto* cur = m_engine->projectSnapshot().findSourceAnywhere(m_sourceId);
        if (!cur) return;
        m_engine->setSourceLocked(m_sourceId, !cur->locked);
    });

    auto* filters = addBtn(QStringLiteral("Filters"), QStringLiteral("Open Filters"));
    connect(filters, &QPushButton::clicked, this, [this] { emit filtersRequested(m_sourceId); });

    auto* xform = addBtn(QStringLiteral("Transform"), QStringLiteral("Edit Transform"));
    connect(xform, &QPushButton::clicked, this, [this] { emit transformRequested(m_sourceId); });

    auto* props = addBtn(QStringLiteral("Properties"), QStringLiteral("Source Properties"));
    connect(props, &QPushButton::clicked, this, [this] { emit propertiesRequested(m_sourceId); });

    auto* interact = addBtn(QStringLiteral("Interact"), QStringLiteral("Open Interact window"));
    connect(interact, &QPushButton::clicked, this, [this] { emit interactRequested(m_sourceId); });

    switch (src->type) {
    case SourceType::Browser: {
        auto* refresh = addBtn(QStringLiteral("Refresh"), QStringLiteral("Reload browser page (OBS-style)"));
        connect(refresh, &QPushButton::clicked, this, [this] {
            const auto* live = m_engine->projectSnapshot().findSourceAnywhere(m_sourceId);
            if (!live) return;
            auto settings = live->settings;
            settings.insert(QStringLiteral("refreshToken"),
                            settings.value(QStringLiteral("refreshToken")).toInt(0) + 1);
            m_engine->updateSourceSettings(m_sourceId, settings);
        });
        break;
    }
    case SourceType::Text: {
        auto* edit = addBtn(QStringLiteral("Edit Text"), QStringLiteral("Open text properties"));
        connect(edit, &QPushButton::clicked, this, [this] { emit propertiesRequested(m_sourceId); });
        break;
    }
    case SourceType::Media:
    case SourceType::Image: {
        auto* open = addBtn(QStringLiteral("Open File"), QStringLiteral("Change media file"));
        connect(open, &QPushButton::clicked, this, [this] { emit propertiesRequested(m_sourceId); });
        break;
    }
    case SourceType::Display:
    case SourceType::Window:
    case SourceType::Game: {
        auto* capture = addBtn(QStringLiteral("Capture"), QStringLiteral("Capture settings"));
        connect(capture, &QPushButton::clicked, this, [this] { emit propertiesRequested(m_sourceId); });
        break;
    }
    default:
        break;
    }
}

} // namespace railshot
