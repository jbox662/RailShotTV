#include "ui/pages/AnalyticsPage.h"
#include "ui/Theme.h"
#include "core/EngineController.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QButtonGroup>
#include <QFrame>
#include <QGridLayout>

namespace railshot {

AnalyticsPage::AnalyticsPage(EngineController* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setObjectName(QStringLiteral("analyticsPage"));
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* header = theme::makePageHeader(QStringLiteral("Analytics"), theme::PanelAccent::Cyan, this);
    auto* headerLay = qobject_cast<QHBoxLayout*>(header->layout());
    auto* rangeGroup = new QButtonGroup(this);
    rangeGroup->setExclusive(true);
    const QStringList ranges = {QStringLiteral("5m"), QStringLiteral("15m"), QStringLiteral("1h"),
                                QStringLiteral("3h"), QStringLiteral("6h"), QStringLiteral("12h")};
    for (const auto& r : ranges) {
        auto* b = new QPushButton(r, header);
        b->setObjectName(QStringLiteral("rangePill"));
        b->setCheckable(true);
        if (r == QLatin1String("1h")) b->setChecked(true);
        rangeGroup->addButton(b);
        if (headerLay) headerLay->addWidget(b);
    }
    root->addWidget(header);

    auto* body = new QVBoxLayout();
    body->setContentsMargins(16, 16, 16, 16);
    body->setSpacing(12);

    auto* kpiRow = new QHBoxLayout();
    auto makeKpi = [&](const QString& label, const QString& color, QLabel** out) {
        auto* card = new QFrame(this);
        card->setObjectName(QStringLiteral("kpiCard"));
        auto* lay = new QVBoxLayout(card);
        lay->setContentsMargins(16, 12, 16, 12);
        auto* v = new QLabel(QStringLiteral("—"), card);
        v->setStyleSheet(QStringLiteral(
            "font-family:'Bebas Neue'; font-size:28px; color:%1; background:transparent;").arg(color));
        auto* l = new QLabel(label, card);
        l->setStyleSheet(QStringLiteral("color:#8892A4; font-size:10px; font-weight:700; letter-spacing:1px; background:transparent;"));
        auto* badge = new QLabel(QStringLiteral("NO DATA"), card);
        badge->setStyleSheet(QStringLiteral("color:#606878; font-size:9px; font-weight:700; background:transparent;"));
        lay->addWidget(v);
        lay->addWidget(l);
        lay->addWidget(badge);
        kpiRow->addWidget(card);
        *out = v;
    };
    makeKpi(QStringLiteral("PEAK VIEWERS"), QStringLiteral("#4F9EFF"), &m_peak);
    makeKpi(QStringLiteral("AVG WATCH"), QStringLiteral("#A855F7"), &m_watch);
    makeKpi(QStringLiteral("FOLLOWERS"), QStringLiteral("#22C55E"), &m_followers);
    makeKpi(QStringLiteral("REVENUE"), QStringLiteral("#FBBF24"), &m_revenue);
    body->addLayout(kpiRow);

    auto* grid = new QGridLayout();
    grid->setSpacing(12);

    auto makePanel = [&](const QString& title, theme::PanelAccent accent, const QString& color) {
        auto* card = new QFrame(this);
        card->setObjectName(QStringLiteral("kpiCard"));
        auto* lay = new QVBoxLayout(card);
        lay->setContentsMargins(0, 0, 0, 0);
        auto* head = new QLabel(QStringLiteral("  ") + title, card);
        head->setFixedHeight(32);
        head->setStyleSheet(theme::panelHeaderStyle(accent)
                            + QStringLiteral("color:%1; font-weight:800; font-size:10px; letter-spacing:1.5px;")
                                  .arg(color));
        auto* content = new QLabel(QStringLiteral("No data yet"), card);
        content->setWordWrap(true);
        content->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        content->setMinimumHeight(100);
        content->setStyleSheet(QStringLiteral(
            "color:#808898; padding:12px; background:transparent; font-family:'JetBrains Mono'; font-size:11px;"));
        lay->addWidget(head);
        lay->addWidget(content, 1);
        return qMakePair(card, content);
    };

    {
        auto p = makePanel(QStringLiteral("VIEWER COUNT"), theme::PanelAccent::Cyan, QStringLiteral("#22D3EE"));
        m_viewerPanel = p.second;
        grid->addWidget(p.first, 0, 0);
    }

    {
        auto* healthCard = new QFrame(this);
        healthCard->setObjectName(QStringLiteral("kpiCard"));
        auto* lay = new QVBoxLayout(healthCard);
        lay->setContentsMargins(0, 0, 0, 0);
        auto* head = new QLabel(QStringLiteral("  STREAM HEALTH"), healthCard);
        head->setFixedHeight(32);
        head->setStyleSheet(theme::panelHeaderStyle(theme::PanelAccent::Emerald)
                            + QStringLiteral("color:#22C55E; font-weight:800; font-size:10px; letter-spacing:1.5px;"));
        auto* healthBody = new QWidget(healthCard);
        auto* hl = new QVBoxLayout(healthBody);
        hl->setContentsMargins(12, 8, 12, 12);
        auto mk = [&](const QString& name, const QString& color, QLabel** out) {
            auto* row = new QHBoxLayout();
            auto* n = new QLabel(name, healthBody);
            n->setStyleSheet(QStringLiteral("color:%1; font-weight:700; font-size:11px; background:transparent;").arg(color));
            auto* v = new QLabel(QStringLiteral("—"), healthBody);
            v->setStyleSheet(QStringLiteral("color:#D0D2D8; font-family:'JetBrains Mono'; font-size:11px; background:transparent;"));
            row->addWidget(n);
            row->addStretch();
            row->addWidget(v);
            hl->addLayout(row);
            *out = v;
        };
        mk(QStringLiteral("Bitrate"), QStringLiteral("#4F9EFF"), &m_bitrate);
        mk(QStringLiteral("CPU"), QStringLiteral("#22C55E"), &m_cpu);
        mk(QStringLiteral("GPU"), QStringLiteral("#22D3EE"), &m_gpu);
        mk(QStringLiteral("FPS"), QStringLiteral("#FBBF24"), &m_fps);
        lay->addWidget(head);
        lay->addWidget(healthBody, 1);
        grid->addWidget(healthCard, 0, 1);
    }

    {
        auto p = makePanel(QStringLiteral("AUDIENCE"), theme::PanelAccent::Violet, QStringLiteral("#A855F7"));
        m_audiencePanel = p.second;
        grid->addWidget(p.first, 1, 0);
    }
    {
        auto p = makePanel(QStringLiteral("SESSIONS"), theme::PanelAccent::Brand, QStringLiteral("#FF5A2C"));
        m_sessionsPanel = p.second;
        grid->addWidget(p.first, 1, 1);
    }

    body->addLayout(grid, 1);
    root->addLayout(body, 1);

    connect(engine, &EngineController::telemetryUpdated, this, [this](const TelemetrySnapshot& s) {
        m_bitrate->setText(QStringLiteral("%1 kbps").arg(s.bitrateKbps));
        m_cpu->setText(QStringLiteral("%1%").arg(s.cpuPercent, 0, 'f', 1));
        m_gpu->setText(QStringLiteral("%1%").arg(s.gpuPercent, 0, 'f', 1));
        m_fps->setText(QStringLiteral("%1 / %2").arg(s.fpsRender, 0, 'f', 1).arg(s.fpsEncode, 0, 'f', 1));

        if (s.streaming) {
            m_watch->setText(QStringLiteral("%1s").arg(s.streamUptimeSec));
            m_viewerPanel->setText(QStringLiteral("Live session\nUptime %1s\nDropped %2\nA/V drift %3 ms")
                                       .arg(s.streamUptimeSec)
                                       .arg(s.droppedFrames)
                                       .arg(s.avDriftMs, 0, 'f', 1));
            m_sessionsPanel->setText(QStringLiteral("Streaming · recording %1")
                                         .arg(s.recording ? QStringLiteral("yes") : QStringLiteral("no")));
        } else {
            m_viewerPanel->setText(QStringLiteral("No data yet"));
            m_sessionsPanel->setText(QStringLiteral("No sessions in range"));
        }
        m_audiencePanel->setText(QStringLiteral("Audience history unavailable.\nFollowers / revenue require platform APIs."));
        m_followers->setText(QStringLiteral("—"));
        m_revenue->setText(QStringLiteral("—"));
        m_peak->setText(QStringLiteral("—"));
    });
}

} // namespace railshot
