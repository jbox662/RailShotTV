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
#include <QtMath>

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
    struct Range { const char* label; int minutes; };
    const Range ranges[] = {
        {"5m", 5}, {"15m", 15}, {"1h", 60}, {"3h", 180}, {"6h", 360}, {"12h", 720},
    };
    for (const auto& r : ranges) {
        auto* b = new QPushButton(QString::fromLatin1(r.label), header);
        b->setObjectName(QStringLiteral("rangePill"));
        b->setCheckable(true);
        if (r.minutes == 60) b->setChecked(true);
        rangeGroup->addButton(b);
        if (headerLay) headerLay->addWidget(b);
        connect(b, &QPushButton::clicked, this, [this, mins = r.minutes] { setRangeMinutes(mins); });
    }
    root->addWidget(header);

    auto* body = new QVBoxLayout();
    body->setContentsMargins(16, 16, 16, 16);
    body->setSpacing(12);

    auto* kpiRow = new QHBoxLayout();
    auto makeKpi = [&](const QString& label, const QString& color, QLabel** out, QLabel** badgeOut = nullptr) {
        auto* card = new QFrame(this);
        card->setObjectName(QStringLiteral("kpiCard"));
        auto* lay = new QVBoxLayout(card);
        lay->setContentsMargins(16, 12, 16, 12);
        auto* v = new QLabel(QStringLiteral("—"), card);
        v->setStyleSheet(QStringLiteral(
            "font-family:'Bebas Neue'; font-size:28px; color:%1; background:transparent;").arg(color));
        auto* l = new QLabel(label, card);
        l->setStyleSheet(QStringLiteral("color:#8892A4; font-size:10px; font-weight:700; letter-spacing:1px; background:transparent;"));
        auto* badge = new QLabel(QStringLiteral("LOCAL"), card);
        badge->setStyleSheet(QStringLiteral("color:#606878; font-size:9px; font-weight:700; background:transparent;"));
        lay->addWidget(v);
        lay->addWidget(l);
        lay->addWidget(badge);
        kpiRow->addWidget(card);
        *out = v;
        if (badgeOut) *badgeOut = badge;
    };
    makeKpi(QStringLiteral("PEAK BITRATE"), QStringLiteral("#4F9EFF"), &m_peak, &m_peakBadge);
    makeKpi(QStringLiteral("STREAM TIME"), QStringLiteral("#A855F7"), &m_watch);
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
        auto p = makePanel(QStringLiteral("STREAM TELEMETRY"), theme::PanelAccent::Cyan, QStringLiteral("#22D3EE"));
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
        Sample sample;
        sample.tsMs = QDateTime::currentMSecsSinceEpoch();
        sample.bitrate = s.bitrateKbps;
        sample.cpu = s.cpuPercent;
        sample.gpu = s.gpuPercent;
        sample.fps = s.fpsRender;
        sample.streaming = s.streaming;
        sample.uptime = s.streamUptimeSec;
        sample.dropped = s.droppedFrames;
        m_history.push_back(sample);
        // Keep ~12h at 2Hz ≈ 86400 samples — cap to 20k
        while (m_history.size() > 20000)
            m_history.removeFirst();

        m_bitrate->setText(QStringLiteral("%1 kbps").arg(s.bitrateKbps));
        m_cpu->setText(QStringLiteral("%1%").arg(s.cpuPercent, 0, 'f', 1));
        m_gpu->setText(QStringLiteral("%1%").arg(s.gpuPercent, 0, 'f', 1));
        m_fps->setText(QStringLiteral("%1 / %2").arg(s.fpsRender, 0, 'f', 1).arg(s.fpsEncode, 0, 'f', 1));
        refreshFromHistory();
    });
    refreshFromHistory();
}

void AnalyticsPage::setRangeMinutes(int minutes)
{
    m_rangeMinutes = qMax(1, minutes);
    refreshFromHistory();
}

void AnalyticsPage::refreshFromHistory()
{
    const qint64 cutoff = QDateTime::currentMSecsSinceEpoch() - qint64(m_rangeMinutes) * 60 * 1000;
    qint64 peakBitrate = 0;
    qint64 maxUptime = 0;
    qint64 dropped = 0;
    double avgCpu = 0, avgGpu = 0, avgFps = 0;
    int n = 0;
    int streamingSamples = 0;
    for (const auto& s : m_history) {
        if (s.tsMs < cutoff) continue;
        peakBitrate = qMax(peakBitrate, s.bitrate);
        maxUptime = qMax(maxUptime, s.uptime);
        dropped = qMax(dropped, s.dropped);
        avgCpu += s.cpu;
        avgGpu += s.gpu;
        avgFps += s.fps;
        ++n;
        if (s.streaming) ++streamingSamples;
    }
    if (n > 0) {
        avgCpu /= n;
        avgGpu /= n;
        avgFps /= n;
    }

    m_peak->setText(peakBitrate > 0 ? QStringLiteral("%1k").arg(peakBitrate) : QStringLiteral("—"));
    if (m_peakBadge)
        m_peakBadge->setText(QStringLiteral("%1 RANGE").arg(m_rangeMinutes >= 60
                                                                ? QStringLiteral("%1h").arg(m_rangeMinutes / 60)
                                                                : QStringLiteral("%1m").arg(m_rangeMinutes)));
    m_watch->setText(maxUptime > 0 ? QStringLiteral("%1s").arg(maxUptime) : QStringLiteral("—"));
    m_followers->setText(QStringLiteral("—"));
    m_revenue->setText(QStringLiteral("—"));

    if (n == 0) {
        m_viewerPanel->setText(QStringLiteral("No telemetry in selected range yet.\nStart streaming/recording to fill history."));
        m_sessionsPanel->setText(QStringLiteral("No sessions in range"));
    } else {
        m_viewerPanel->setText(QStringLiteral("Samples %1\nPeak bitrate %2 kbps\nAvg CPU %3%\nAvg GPU %4%\nAvg FPS %5\nMax dropped %6")
                                   .arg(n)
                                   .arg(peakBitrate)
                                   .arg(avgCpu, 0, 'f', 1)
                                   .arg(avgGpu, 0, 'f', 1)
                                   .arg(avgFps, 0, 'f', 1)
                                   .arg(dropped));
        m_sessionsPanel->setText(QStringLiteral("Live samples %1 / %2\nLongest uptime %3s\nRange window %4 min")
                                     .arg(streamingSamples)
                                     .arg(n)
                                     .arg(maxUptime)
                                     .arg(m_rangeMinutes));
    }
    m_audiencePanel->setText(QStringLiteral("Followers / revenue need platform APIs.\nLocal stream health history is active."));
}

} // namespace railshot
