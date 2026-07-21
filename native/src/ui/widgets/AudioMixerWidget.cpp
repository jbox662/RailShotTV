#include "ui/widgets/AudioMixerWidget.h"
#include "ui/widgets/AdvAudioDialog.h"
#include "core/EngineController.h"
#include "audio/AudioGraph.h"
#include "core/Types.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QScrollArea>
#include <QFrame>
#include <QMenu>
#include <QCursor>
#include <QAction>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QPainter>
#include <QFontMetrics>
#include <QStyle>
#include <QStyleOption>
#include <cmath>
#include <algorithm>

namespace railshot {

namespace {

constexpr qreal kMeterMinDb = -60.0;
constexpr qreal kMeterWarnDb = -20.0;
constexpr qreal kMeterErrorDb = -9.0;
constexpr int kTickDbInterval = 6; // OBS: 0, -6, …, -60
constexpr int kIndicatorThickness = 3;
constexpr char kTickLabelToken[] = "-88";

// Yami-ish meter colors (OBS VolumeMeter defaults / theme)
const QColor kBgNominal(0x17, 0x64, 0x1E);
const QColor kBgWarning(0x6E, 0x52, 0x0D);
const QColor kBgError(0x7D, 0x12, 0x24);
const QColor kFgNominal(0x37, 0xD2, 0x47);
const QColor kFgWarning(0xE5, 0xAF, 0x24);
const QColor kFgError(0xE3, 0x3B, 0x57);
const QColor kTickMajor(0x96, 0x96, 0x96);
const QColor kMeterDisabledBg(90, 117, 65);
const QColor kMeterDisabledFg(163, 217, 113);

float peakToDb(float peak)
{
    if (peak <= 0.000001f)
        return float(kMeterMinDb);
    return std::max(float(kMeterMinDb), 20.f * std::log10(peak));
}

float volumeToDb(float vol)
{
    if (vol <= 0.000001f)
        return -96.f;
    return std::max(-96.f, 20.f * std::log10(std::clamp(vol, 0.f, 20.f)));
}

QString formatDb(float db)
{
    if (db <= -95.5f)
        return QStringLiteral("-inf dB");
    return QStringLiteral("%1 dB").arg(db, 0, 'f', 1);
}

int meterMinWidthPx(const QFont& font)
{
    const QFontMetrics fm(font);
    const int labelW = fm.horizontalAdvance(QLatin1String(kTickLabelToken));
    const int labelTotal = int(std::abs(kMeterMinDb / kTickDbInterval)) + 1;
    return labelTotal * labelW + kIndicatorThickness;
}

/// OBS-style stereo peak meter with green / yellow / red bands and dB ticks.
class MixerVolumeMeter : public QWidget {
public:
    explicit MixerVolumeMeter(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        QFont f = font();
        f.setPointSize(6);
        f.setBold(true);
        setFont(f);
        setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
        setAttribute(Qt::WA_OpaquePaintEvent, false);
    }

    void setLevels(float peakL, float peakR, bool muted)
    {
        const float nl = std::clamp(peakL, 0.f, 1.f);
        const float nr = std::clamp(peakR, 0.f, 1.f);
        if (qFuzzyCompare(nl, m_peakL) && qFuzzyCompare(nr, m_peakR) && muted == m_muted)
            return;
        m_peakL = nl;
        m_peakR = nr;
        m_muted = muted;
        // Peak hold
        if (nl >= m_holdL) {
            m_holdL = nl;
            m_holdTickL = 0;
        } else if (++m_holdTickL > 40) {
            m_holdL = std::max(nl, m_holdL * 0.92f);
        }
        if (nr >= m_holdR) {
            m_holdR = nr;
            m_holdTickR = 0;
        } else if (++m_holdTickR > 40) {
            m_holdR = std::max(nr, m_holdR * 0.92f);
        }
        update();
    }

    QSize sizeHint() const override
    {
        const int barH = 4;
        const int channels = 2;
        const int barsH = channels * (barH + 1) - 1;
        const QFontMetrics fm(font());
        return QSize(meterMinWidthPx(font()), barsH + fm.height());
    }

    QSize minimumSizeHint() const override { return sizeHint(); }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, false);

        const int w = width();
        const int h = height();
        if (w < 8 || h < 8)
            return;

        const int barH = 4;
        const int channels = 2;
        const int barsH = channels * (barH + 1) - 1;
        const int meterStart = kIndicatorThickness + 2;
        const int meterLength = qMax(1, w - meterStart);
        const qreal scale = meterLength / kMeterMinDb; // negative denom → positions from left

        const int warnPos = meterLength - int(kMeterWarnDb * scale);
        const int errPos = meterLength - int(kMeterErrorDb * scale);

        const QColor bgN = m_muted ? kMeterDisabledBg : kBgNominal;
        const QColor bgW = m_muted ? kMeterDisabledBg.darker(110) : kBgWarning;
        const QColor bgE = m_muted ? kMeterDisabledBg.darker(130) : kBgError;
        const QColor fgN = m_muted ? kMeterDisabledFg : kFgNominal;
        const QColor fgW = m_muted ? kMeterDisabledFg : kFgWarning;
        const QColor fgE = m_muted ? kMeterDisabledFg : kFgError;

        auto peakColor = [&](float db) -> QColor {
            if (db >= kMeterErrorDb)
                return fgE;
            if (db >= kMeterWarnDb)
                return fgW;
            return fgN;
        };

        const float peaks[2] = {m_peakL, m_peakR};
        const float holds[2] = {m_holdL, m_holdR};

        for (int ch = 0; ch < channels; ++ch) {
            const int y = ch * (barH + 1);

            // Background bands (nominal / warning / error)
            p.fillRect(meterStart, y, meterLength, barH, bgE);
            p.fillRect(meterStart, y, errPos, barH, bgW);
            p.fillRect(meterStart, y, warnPos, barH, bgN);

            // Active peak fill
            const float db = peakToDb(peaks[ch]);
            const int fillLen = qBound(0, meterLength - int(db * scale), meterLength);
            if (fillLen > 0)
                p.fillRect(meterStart, y, fillLen, barH, peakColor(db));

            // Peak hold tick
            const float hDb = peakToDb(holds[ch]);
            const int holdX = meterStart + qBound(0, meterLength - int(hDb * scale), meterLength);
            p.fillRect(holdX - 2, y, 2, barH, peakColor(hDb));

            // Input-level LED strip (left)
            p.fillRect(0, y, kIndicatorThickness, barH, peakColor(db));
        }

        // Tick marks + labels under the bars
        const int tickY = barsH;
        p.setPen(kTickMajor);
        p.setFont(font());
        const QFontMetrics fm(font());
        for (int db = 0; db >= int(kMeterMinDb); db -= kTickDbInterval) {
            const int pos = meterStart + meterLength - int(db * scale) - 1;
            p.drawLine(pos, tickY, pos, tickY + 2);
            const QString str = QString::number(db);
            const QRect tb = fm.boundingRect(str);
            int tx;
            if (db == 0)
                tx = pos - tb.width();
            else {
                tx = pos - tb.width() / 2;
                if (tx < 0)
                    tx = 0;
            }
            p.drawText(tx, tickY + 2 + fm.ascent(), str);
        }
        Q_UNUSED(h);
    }

private:
    float m_peakL = 0;
    float m_peakR = 0;
    float m_holdL = 0;
    float m_holdR = 0;
    int m_holdTickL = 0;
    int m_holdTickR = 0;
    bool m_muted = false;
};

QString iconBtnStyle(bool dangerWhenChecked)
{
    if (dangerWhenChecked) {
        return QStringLiteral(
            "QPushButton{background:transparent;border:none;color:#E5E7EB;"
            "font-size:14px;padding:2px;min-width:22px;max-width:26px;min-height:20px;}"
            "QPushButton:hover{color:#FFFFFF;}"
            "QPushButton:checked{color:#E33B57;}");
    }
    return QStringLiteral(
        "QPushButton{background:transparent;border:none;color:#9CA3AF;"
        "font-size:14px;padding:2px;min-width:22px;max-width:26px;min-height:20px;}"
        "QPushButton:hover{color:#E5E7EB;}"
        "QPushButton:checked{color:#60A5FA;}");
}

} // namespace

AudioMixerWidget::AudioMixerWidget(EngineController* engine, QWidget* parent)
    : QWidget(parent)
    , m_engine(engine)
{
    setObjectName(QStringLiteral("audioMixer"));
    setContextMenuPolicy(Qt::CustomContextMenu);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    setStyleSheet(QStringLiteral(
        "QWidget#audioMixer{background:#0D0F12;border:none;}"
        "QWidget#audioMixer QLabel#volLabel{"
        "  color:#E5E7EB;font-size:11px;font-weight:600;min-width:48px;"
        "  background:transparent;border:none;}"
        "QWidget#audioMixer QPushButton#mixerToolBtn{"
        "  background:transparent;border:none;color:#9CA3AF;"
        "  font-size:14px;padding:4px 6px;min-width:24px;}"
        "QWidget#audioMixer QPushButton#mixerToolBtn:hover{color:#FFFFFF;}"
        "QFrame#volMeterFrame{background:transparent;border:none;}"
        "QFrame#mixStrip{"
        "  background:#12151A;border:none;border-bottom:1px solid #1F2329;}"));

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(4, 2, 4, 2);
    outer->setSpacing(0);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setStyleSheet(QStringLiteral(
        "QScrollArea{background:transparent;border:none;}"
        "QScrollBar:vertical{width:8px;background:#0A0C0F;}"
        "QScrollBar::handle:vertical{background:#3A3D45;border-radius:3px;min-height:24px;}"));

    auto* host = new QWidget(scroll);
    host->setObjectName(QStringLiteral("mixerHost"));
    host->setStyleSheet(QStringLiteral("QWidget#mixerHost{background:transparent;}"));
    m_list = new QVBoxLayout(host);
    m_list->setContentsMargins(0, 0, 0, 0);
    m_list->setSpacing(0);
    m_list->setAlignment(Qt::AlignTop);
    scroll->setWidget(host);
    outer->addWidget(scroll, 1);

    // OBS-style footer: gear + overflow
    auto* footer = new QHBoxLayout();
    footer->setContentsMargins(2, 2, 2, 0);
    footer->setSpacing(2);
    m_advBtn = new QPushButton(QStringLiteral("⚙"), this);
    m_advBtn->setObjectName(QStringLiteral("mixerToolBtn"));
    m_advBtn->setCursor(Qt::PointingHandCursor);
    m_advBtn->setToolTip(QStringLiteral("Advanced Audio Properties"));
    connect(m_advBtn, &QPushButton::clicked, this, &AudioMixerWidget::openAdvAudio);
    footer->addWidget(m_advBtn);

    auto* moreBtn = new QPushButton(QStringLiteral("⋮"), this);
    moreBtn->setObjectName(QStringLiteral("mixerToolBtn"));
    moreBtn->setCursor(Qt::PointingHandCursor);
    moreBtn->setToolTip(QStringLiteral("Mixer options"));
    connect(moreBtn, &QPushButton::clicked, this, [this] {
        QMenu menu(this);
        menu.addAction(QStringLiteral("Advanced Audio Properties…"), this, &AudioMixerWidget::openAdvAudio);
        if (m_monitorBtn)
            menu.addAction(m_monitorBtn->isChecked() ? QStringLiteral("Disable Monitoring")
                                                     : QStringLiteral("Enable Monitoring"),
                           this, [this] { m_monitorBtn->toggle(); });
        menu.exec(QCursor::pos());
    });
    footer->addWidget(moreBtn);

    m_monitorBtn = new QPushButton(this);
    m_monitorBtn->setVisible(false);
    m_monitorBtn->setCheckable(true);
    m_monitorBtn->setChecked(true);
    connect(m_monitorBtn, &QPushButton::toggled, this, [this](bool on) {
        if (m_engine && m_engine->audio())
            m_engine->audio()->setMonitorEnabled(on);
    });

    footer->addStretch(1);
    outer->addLayout(footer);

    connect(this, &QWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        QMenu menu(this);
        menu.addAction(QStringLiteral("Advanced Audio Properties…"), this, &AudioMixerWidget::openAdvAudio);
        menu.exec(mapToGlobal(pos));
    });

    if (m_engine && m_engine->audio()) {
        connect(m_engine->audio(), &AudioGraph::channelsChanged, this, [this] {
            m_stripIds.clear();
            rebuildStrips();
        });
    }

    auto* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &AudioMixerWidget::refreshMeters);
    timer->start(33);
    refreshMeters();
}

QSize AudioMixerWidget::minimumSizeHint() const
{
    // OBS: meter refuses to shrink below tick-label width; chrome around it
    const int meterMin = meterMinWidthPx(font());
    return QSize(meterMin + 40, 120);
}

QSize AudioMixerWidget::sizeHint() const
{
    return QSize(qMax(360, minimumSizeHint().width()), 200);
}

void AudioMixerWidget::openAdvAudio()
{
    emit openAdvAudioRequested();
    AdvAudioDialog dlg(m_engine, window());
    dlg.exec();
}

void AudioMixerWidget::rebuildStrips()
{
    while (QLayoutItem* child = m_list->takeAt(0)) {
        if (child->widget())
            child->widget()->deleteLater();
        delete child;
    }
    m_stripIds.clear();

    const auto channels = m_engine->audioChannels();
    for (const auto& ch : channels) {
        const bool master = ch.id == QLatin1String("master");
        m_stripIds.append(ch.id);

        auto* row = new QFrame(this);
        row->setObjectName(QStringLiteral("mixStrip"));
        row->setProperty("channelId", ch.id);
        row->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
        row->setContextMenuPolicy(Qt::CustomContextMenu);

        auto showMenu = [this, id = ch.id](const QPoint&) {
            QMenu menu(this);
            menu.addAction(QStringLiteral("Advanced Audio Properties…"), this, &AudioMixerWidget::openAdvAudio);
            if (id != QLatin1String("master")) {
                menu.addSeparator();
                auto state = m_engine->audio()->channelState(id);
                auto* lockAct = menu.addAction(state.locked ? QStringLiteral("Unlock Volume")
                                                            : QStringLiteral("Lock Volume"));
                connect(lockAct, &QAction::triggered, this, [this, id] {
                    auto s = m_engine->audio()->channelState(id);
                    s.locked = !s.locked;
                    m_engine->audio()->setChannelState(id, s);
                    m_stripIds.clear();
                    rebuildStrips();
                });
                auto* soloAct = menu.addAction(state.solo ? QStringLiteral("Unsolo")
                                                          : QStringLiteral("Solo"));
                connect(soloAct, &QAction::triggered, this, [this, id] {
                    auto s = m_engine->audio()->channelState(id);
                    s.solo = !s.solo;
                    m_engine->audio()->setChannelState(id, s);
                });
                menu.addSeparator();
                menu.addAction(QStringLiteral("Monitor Off"), this, [this, id] {
                    auto s = m_engine->audio()->channelState(id);
                    s.monitoring = AudioMonitoringType::None;
                    m_engine->audio()->setChannelState(id, s);
                });
                menu.addAction(QStringLiteral("Monitor and Output"), this, [this, id] {
                    auto s = m_engine->audio()->channelState(id);
                    s.monitoring = AudioMonitoringType::MonitorAndOutput;
                    m_engine->audio()->setChannelState(id, s);
                });
                menu.addAction(QStringLiteral("Monitor Only (mute output)"), this, [this, id] {
                    auto s = m_engine->audio()->channelState(id);
                    s.monitoring = AudioMonitoringType::MonitorOnly;
                    m_engine->audio()->setChannelState(id, s);
                });
            }
            menu.exec(QCursor::pos());
        };
        connect(row, &QWidget::customContextMenuRequested, this, showMenu);

        auto* mainLay = new QVBoxLayout(row);
        mainLay->setContentsMargins(6, 4, 6, 6);
        mainLay->setSpacing(2);

        // Header: name (expands / elides) …… volume dB
        auto* textLay = new QHBoxLayout();
        textLay->setContentsMargins(0, 0, 0, 0);
        textLay->setSpacing(4);

        auto* nameBtn = new QPushButton(master ? QStringLiteral("Master") : ch.name, row);
        nameBtn->setObjectName(QStringLiteral("stripName"));
        nameBtn->setCursor(Qt::PointingHandCursor);
        nameBtn->setFlat(true);
        nameBtn->setMaximumWidth(280);
        nameBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        nameBtn->setStyleSheet(QStringLiteral(
            "QPushButton{background:transparent;border:none;text-align:left;"
            "color:#E5E7EB;font-size:12px;font-weight:600;padding:0 2px;}"
            "QPushButton:hover{color:#FFFFFF;}"));
        nameBtn->setToolTip(ch.name);
        connect(nameBtn, &QPushButton::clicked, this, [showMenu] { showMenu(QPoint()); });
        textLay->addWidget(nameBtn, 1);

        auto* volLabel = new QLabel(formatDb(volumeToDb(ch.volume)), row);
        volLabel->setObjectName(QStringLiteral("volLabel"));
        volLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        volLabel->setMinimumWidth(48);
        volLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        textLay->addWidget(volLabel);
        mainLay->addLayout(textLay, 3);

        // Controls: mute/monitor | slider + meter (stretch together)
        auto* controlLay = new QHBoxLayout();
        controlLay->setContentsMargins(0, 0, 0, 0);
        controlLay->setSpacing(4);

        auto* buttonCol = new QVBoxLayout();
        buttonCol->setContentsMargins(0, 0, 0, 0);
        buttonCol->setSpacing(0);

        auto* mute = new QPushButton(QStringLiteral("🔊"), row);
        mute->setObjectName(QStringLiteral("muteBtn"));
        mute->setCheckable(true);
        mute->setChecked(ch.muted);
        mute->setCursor(Qt::PointingHandCursor);
        mute->setStyleSheet(iconBtnStyle(true));
        mute->setToolTip(QStringLiteral("Mute"));
        mute->setText(ch.muted ? QStringLiteral("🔇") : QStringLiteral("🔊"));
        connect(mute, &QPushButton::clicked, this, [this, id = ch.id, master] {
            if (master) {
                m_engine->audio()->setMasterMuted(!m_engine->audio()->masterMuted());
                return;
            }
            auto state = m_engine->audio()->channelState(id);
            state.muted = !state.muted;
            m_engine->audio()->setChannelState(id, state);
        });
        buttonCol->addWidget(mute);

        if (!master) {
            auto* mon = new QPushButton(QStringLiteral("🎧"), row);
            mon->setObjectName(QStringLiteral("monBtn"));
            mon->setCheckable(true);
            mon->setChecked(ch.monitoring != AudioMonitoringType::None);
            mon->setCursor(Qt::PointingHandCursor);
            mon->setStyleSheet(iconBtnStyle(false));
            mon->setToolTip(QStringLiteral("Audio Monitoring"));
            connect(mon, &QPushButton::clicked, this, [this, id = ch.id] {
                auto state = m_engine->audio()->channelState(id);
                if (state.monitoring == AudioMonitoringType::None)
                    state.monitoring = AudioMonitoringType::MonitorAndOutput;
                else if (state.monitoring == AudioMonitoringType::MonitorAndOutput)
                    state.monitoring = AudioMonitoringType::MonitorOnly;
                else
                    state.monitoring = AudioMonitoringType::None;
                m_engine->audio()->setChannelState(id, state);
            });
            buttonCol->addWidget(mon);
        }
        buttonCol->addStretch(1);
        controlLay->addLayout(buttonCol);

        auto* meterFrame = new QFrame(row);
        meterFrame->setObjectName(QStringLiteral("volMeterFrame"));
        meterFrame->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
        auto* meterLay = new QVBoxLayout(meterFrame);
        meterLay->setContentsMargins(0, 0, 0, 0);
        meterLay->setSpacing(2);

        auto* fader = new QSlider(Qt::Horizontal, meterFrame);
        fader->setObjectName(QStringLiteral("volSlider"));
        fader->setRange(0, 100);
        fader->setValue(int(std::clamp(ch.volume, 0.f, 1.f) * 100));
        fader->setFixedHeight(18);
        fader->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        fader->setEnabled(!ch.locked || master);
        fader->setStyleSheet(QStringLiteral(
            "QSlider::groove:horizontal{"
            "  background:#3A3F4A;height:4px;border-radius:2px;}"
            "QSlider::sub-page:horizontal{"
            "  background:#476BD7;border-radius:2px;}"
            "QSlider::handle:horizontal{"
            "  background:#FFFFFF;width:10px;height:20px;margin:-8px 0;border-radius:2px;}"
            "QSlider::handle:horizontal:hover{background:#F0F0F0;}"
            "QSlider:disabled{opacity:0.45;}"));
        connect(fader, &QSlider::valueChanged, this, [this, id = ch.id, master](int v) {
            if (!m_engine || !m_engine->audio())
                return;
            if (master) {
                m_engine->audio()->setMasterVolume(v / 100.f);
                return;
            }
            auto state = m_engine->audio()->channelState(id);
            if (state.locked)
                return;
            state.volume = v / 100.f;
            m_engine->audio()->setChannelState(id, state);
        });
        meterLay->addWidget(fader, 2);

        auto* meter = new MixerVolumeMeter(meterFrame);
        meter->setObjectName(QStringLiteral("volMeter"));
        meter->setLevels(ch.peakL, ch.peakR, ch.muted);
        meterLay->addWidget(meter, 2);

        controlLay->addWidget(meterFrame, 1);
        mainLay->addLayout(controlLay, 6);

        m_list->addWidget(row);
    }
    m_list->addStretch(1);
    updateGeometry();
}

void AudioMixerWidget::refreshMeters()
{
    if (!m_engine || !m_engine->audio())
        return;
    const auto channels = m_engine->audioChannels();
    QStringList ids;
    for (const auto& ch : channels)
        ids.append(ch.id);
    if (ids != m_stripIds) {
        rebuildStrips();
        return;
    }
    updateStripValues();
}

void AudioMixerWidget::updateStripValues()
{
    const auto channels = m_engine->audioChannels();
    for (int i = 0; i < channels.size() && i < m_list->count(); ++i) {
        auto* item = m_list->itemAt(i);
        if (!item || !item->widget())
            continue;
        auto* row = item->widget();
        const auto& ch = channels[i];
        const bool master = ch.id == QLatin1String("master");

        if (auto* meterW = row->findChild<QWidget*>(QStringLiteral("volMeter"))) {
            if (auto* meter = static_cast<MixerVolumeMeter*>(meterW))
                meter->setLevels(ch.peakL, ch.peakR, ch.muted);
        }

        if (auto* volDb = row->findChild<QLabel*>(QStringLiteral("volLabel")))
            volDb->setText(formatDb(volumeToDb(ch.volume)));

        if (auto* name = row->findChild<QPushButton*>(QStringLiteral("stripName"))) {
            const QString full = master ? QStringLiteral("Master") : ch.name;
            name->setToolTip(full);
            const int avail = qMax(40, name->width() - 4);
            name->setText(name->fontMetrics().elidedText(full, Qt::ElideMiddle, avail));
            name->setStyleSheet(QStringLiteral(
                "QPushButton{background:transparent;border:none;text-align:left;"
                "color:%1;font-size:12px;font-weight:600;padding:0 2px;}"
                "QPushButton:hover{color:#FFFFFF;}")
                                    .arg(ch.muted ? QStringLiteral("#E33B57")
                                                  : (master ? QStringLiteral("#37D247")
                                                            : QStringLiteral("#E5E7EB"))));
        }

        if (auto* mute = row->findChild<QPushButton*>(QStringLiteral("muteBtn"))) {
            QSignalBlocker b(mute);
            mute->setChecked(ch.muted);
            mute->setText(ch.muted ? QStringLiteral("🔇") : QStringLiteral("🔊"));
            mute->setToolTip(ch.muted ? QStringLiteral("Unmute") : QStringLiteral("Mute"));
        }
        if (auto* mon = row->findChild<QPushButton*>(QStringLiteral("monBtn"))) {
            QSignalBlocker b(mon);
            mon->setChecked(ch.monitoring != AudioMonitoringType::None);
            if (ch.monitoring == AudioMonitoringType::MonitorOnly)
                mon->setToolTip(QStringLiteral("Monitor Only — click to turn off"));
            else if (ch.monitoring == AudioMonitoringType::MonitorAndOutput)
                mon->setToolTip(QStringLiteral("Monitor and Output — click for Monitor Only"));
            else
                mon->setToolTip(QStringLiteral("Monitor Off — click to enable"));
        }
        if (auto* fader = row->findChild<QSlider*>(QStringLiteral("volSlider"))) {
            if (!fader->isSliderDown()) {
                QSignalBlocker b(fader);
                fader->setValue(int(std::clamp(ch.volume, 0.f, 1.f) * 100));
                fader->setEnabled(!ch.locked || master);
            }
        }
    }
}

} // namespace railshot
