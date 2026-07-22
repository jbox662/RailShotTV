#include "ui/widgets/AdvAudioDialog.h"
#include "core/EngineController.h"
#include "audio/AudioGraph.h"
#include "core/Types.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QCheckBox>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QComboBox>
#include <QSlider>
#include <QPushButton>
#include <QFrame>
#include <QSignalBlocker>
#include <cmath>
#include <algorithm>

namespace railshot {

namespace {
float linearToDb(float linear)
{
    if (linear <= 0.000001f) return -96.f;
    return 20.f * std::log10(linear);
}
float dbToLinear(float db)
{
    if (db <= -96.f) return 0.f;
    return std::pow(10.f, db / 20.f);
}
} // namespace

AdvAudioDialog::AdvAudioDialog(EngineController* engine, QWidget* parent)
    : QDialog(parent), m_engine(engine)
{
    setWindowTitle(QStringLiteral("Advanced Audio Properties"));
    setMinimumSize(1480, 360);
    resize(1600, 420);
    setSizeGripEnabled(true);
    setStyleSheet(QStringLiteral(
        "QDialog{background:#0F1114;}"
        "QLabel{color:#C8CCD4; font-family:'DM Sans'; font-size:11px;}"
        "QLabel#hdr{color:#E0E2E8; font-weight:800; font-size:11px;}"
        "QCheckBox{color:#E0E2E8; spacing:4px;}"
        "QCheckBox::indicator{width:14px;height:14px;}"
        "QDoubleSpinBox,QSpinBox,QComboBox{"
        "  background:#12151A; border:1px solid #3A3D45; border-radius:3px;"
        "  color:#E0E2E8; padding:2px 4px; min-height:22px;}"
        "QComboBox::drop-down{border:none; width:18px;}"
        "QSlider::groove:horizontal{background:#1A1D22; height:6px; border-radius:3px; border:1px solid #3A3D45;}"
        "QSlider::handle:horizontal{background:#A855F7; width:12px; margin:-4px 0; border-radius:6px; border:1px solid #E9D5FF;}"
        "QPushButton{background:#1A1E26; border:1px solid #5A5E68; border-radius:3px;"
        "  color:#E0E2E8; font-weight:700; padding:6px 14px;}"
        "QPushButton:hover{border-color:#4F9EFF;}"
        "QScrollArea{border:1px solid #2A2D35; background:#0A0C0F;}"));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(10);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    m_rowsHost = new QWidget(scroll);
    m_grid = new QGridLayout(m_rowsHost);
    m_grid->setContentsMargins(8, 8, 8, 8);
    m_grid->setHorizontalSpacing(10);
    m_grid->setVerticalSpacing(6);
    addHeader(m_grid);
    scroll->setWidget(m_rowsHost);
    root->addWidget(scroll, 1);

    auto* bottom = new QHBoxLayout();
    auto* activeOnly = new QCheckBox(QStringLiteral("Active Sources Only"), this);
    activeOnly->setChecked(true);
    activeOnly->setEnabled(false);
    activeOnly->setToolTip(QStringLiteral("RailShot lists mixer channels currently in the graph."));
    bottom->addWidget(activeOnly);
    bottom->addStretch();
    auto* closeBtn = new QPushButton(QStringLiteral("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, [this] {
        if (m_engine && m_engine->audio() && m_engine->settings())
            m_engine->settings()->setAudioChannels(m_engine->audio()->channelsToJson());
        accept();
    });
    bottom->addWidget(closeBtn);
    root->addLayout(bottom);

    if (m_engine && m_engine->audio()) {
        connect(m_engine->audio(), &AudioGraph::channelsChanged, this, &AdvAudioDialog::rebuildRows);
        connect(m_engine->audio(), &AudioGraph::metersUpdated, this, [this] {
            // Refresh Active status labels lightly
            if (!m_engine) return;
            for (auto it = m_rows.begin(); it != m_rows.end(); ++it) {
                const auto st = m_engine->audio()->channelState(it.key());
                if (it->status) {
                    const bool hot = st.peakL > 0.01f || st.peakR > 0.01f;
                    it->status->setText(hot ? QStringLiteral("Active") : QStringLiteral("Idle"));
                    it->status->setStyleSheet(hot
                        ? QStringLiteral("color:#4ADE80; font-weight:700;")
                        : QStringLiteral("color:#8892A4;"));
                }
            }
        });
    }
    rebuildRows();
}

void AdvAudioDialog::addHeader(QGridLayout* grid)
{
    auto mk = [&](int col, const QString& text, QWidget* extra = nullptr) {
        if (extra) {
            auto* wrap = new QWidget(m_rowsHost);
            auto* h = new QHBoxLayout(wrap);
            h->setContentsMargins(0, 0, 0, 0);
            h->setSpacing(4);
            auto* lab = new QLabel(text, wrap);
            lab->setObjectName(QStringLiteral("hdr"));
            h->addWidget(lab);
            h->addWidget(extra);
            h->addStretch();
            grid->addWidget(wrap, 0, col);
        } else {
            auto* lab = new QLabel(text, m_rowsHost);
            lab->setObjectName(QStringLiteral("hdr"));
            grid->addWidget(lab, 0, col);
        }
    };

    m_usePercent = new QCheckBox(QStringLiteral("%"), m_rowsHost);
    m_usePercent->setObjectName(QStringLiteral("hdr"));
    connect(m_usePercent, &QCheckBox::toggled, this, &AdvAudioDialog::onUsePercentToggled);

    mk(0, QStringLiteral("Name"));
    mk(1, QStringLiteral("Status"));
    mk(2, QStringLiteral("Volume"), m_usePercent);
    mk(3, QStringLiteral("Gain"));
    mk(4, QStringLiteral("Mono"));
    mk(5, QStringLiteral("Balance"));
    mk(6, QStringLiteral("Sync Offset"));
    mk(7, QStringLiteral("Gate"));
    mk(8, QStringLiteral("Compressor"));
    mk(9, QStringLiteral("EQ L/M/H"));
    mk(10, QStringLiteral("Limiter"));
    mk(11, QStringLiteral("Audio Monitoring"));
    mk(12, QStringLiteral("Tracks"));
}

void AdvAudioDialog::rebuildRows()
{
    // Clear old rows (keep header at row 0)
    while (m_grid->count() > 13) {
        QLayoutItem* item = m_grid->takeAt(13);
        if (item->widget())
            item->widget()->deleteLater();
        delete item;
    }
    // Also clear widgets that were multi-col track containers from previous builds
    for (auto& w : m_rows)
        Q_UNUSED(w);
    m_rows.clear();

    // Re-add header if wiped — simpler: wipe everything and rebuild header + rows
    while (QLayoutItem* item = m_grid->takeAt(0)) {
        if (item->widget())
            item->widget()->deleteLater();
        delete item;
    }
    m_usePercent = nullptr;
    addHeader(m_grid);
    if (m_usePercent) {
        QSignalBlocker b(m_usePercent);
        m_usePercent->setChecked(m_usePercentMode);
    }

    if (!m_engine || !m_engine->audio())
        return;

    const auto channels = m_engine->audio()->channels();
    int row = 1;
    for (const auto& ch : channels) {
        if (ch.id == QLatin1String("master"))
            continue;
        addChannelRow(m_grid, row++, ch.id);
    }
}

void AdvAudioDialog::addChannelRow(QGridLayout* grid, int row, const QString& channelId)
{
    if (!m_engine || !m_engine->audio())
        return;
    const auto ch = m_engine->audio()->channelState(channelId);
    RowWidgets w;

    w.name = new QLabel(ch.name.isEmpty() ? channelId : ch.name, m_rowsHost);
    w.name->setStyleSheet(QStringLiteral("color:#F0F2F6; font-weight:700;"));
    grid->addWidget(w.name, row, 0);

    w.status = new QLabel(QStringLiteral("Idle"), m_rowsHost);
    w.status->setStyleSheet(QStringLiteral("color:#8892A4;"));
    grid->addWidget(w.status, row, 1);

    auto* volHost = new QWidget(m_rowsHost);
    auto* volLay = new QHBoxLayout(volHost);
    volLay->setContentsMargins(0, 0, 0, 0);
    w.volDb = new QDoubleSpinBox(volHost);
    w.volDb->setRange(-96.0, 26.0);
    w.volDb->setDecimals(1);
    w.volDb->setSuffix(QStringLiteral(" dB"));
    w.volDb->setValue(linearToDb(ch.volume));
    w.volPct = new QSpinBox(volHost);
    w.volPct->setRange(0, 200);
    w.volPct->setSuffix(QStringLiteral(" %"));
    w.volPct->setValue(int(std::lround(ch.volume * 100.0)));
    w.volPct->setVisible(m_usePercentMode);
    w.volDb->setVisible(!m_usePercentMode);
    volLay->addWidget(w.volDb);
    volLay->addWidget(w.volPct);
    grid->addWidget(volHost, row, 2);

    w.gainDb = new QDoubleSpinBox(m_rowsHost);
    w.gainDb->setRange(-30.0, 30.0);
    w.gainDb->setDecimals(1);
    w.gainDb->setSuffix(QStringLiteral(" dB"));
    w.gainDb->setValue(ch.gainDb);
    w.gainDb->setToolTip(QStringLiteral("Trim / Gain filter"));
    grid->addWidget(w.gainDb, row, 3);

    w.mono = new QCheckBox(m_rowsHost);
    w.mono->setChecked(ch.forceMono);
    grid->addWidget(w.mono, row, 4, Qt::AlignCenter);

    auto* balHost = new QWidget(m_rowsHost);
    auto* balLay = new QHBoxLayout(balHost);
    balLay->setContentsMargins(0, 0, 0, 0);
    balLay->setSpacing(4);
    auto* lLab = new QLabel(QStringLiteral("L"), balHost);
    auto* rLab = new QLabel(QStringLiteral("R"), balHost);
    w.balance = new QSlider(Qt::Horizontal, balHost);
    w.balance->setRange(-100, 100);
    w.balance->setValue(int(std::lround(ch.pan * 100.0)));
    w.balance->setFixedWidth(120);
    balLay->addWidget(lLab);
    balLay->addWidget(w.balance, 1);
    balLay->addWidget(rLab);
    grid->addWidget(balHost, row, 5);

    w.sync = new QSpinBox(m_rowsHost);
    w.sync->setRange(-2000, 2000);
    w.sync->setSuffix(QStringLiteral(" ms"));
    w.sync->setValue(ch.syncOffsetMs);
    grid->addWidget(w.sync, row, 6);

    auto* gateHost = new QWidget(m_rowsHost);
    auto* gateLay = new QHBoxLayout(gateHost);
    gateLay->setContentsMargins(0, 0, 0, 0);
    gateLay->setSpacing(4);
    w.gateOn = new QCheckBox(gateHost);
    w.gateOn->setChecked(ch.gateEnabled);
    w.gateOpenDb = new QDoubleSpinBox(gateHost);
    w.gateOpenDb->setRange(-80.0, 0.0);
    w.gateOpenDb->setDecimals(0);
    w.gateOpenDb->setSuffix(QStringLiteral(" dB"));
    w.gateOpenDb->setValue(ch.gateOpenDb);
    w.gateOpenDb->setFixedWidth(78);
    w.gateOpenDb->setToolTip(QStringLiteral("Noise gate open threshold"));
    gateLay->addWidget(w.gateOn);
    gateLay->addWidget(w.gateOpenDb);
    grid->addWidget(gateHost, row, 7);

    auto* compHost = new QWidget(m_rowsHost);
    auto* compLay = new QHBoxLayout(compHost);
    compLay->setContentsMargins(0, 0, 0, 0);
    compLay->setSpacing(4);
    w.compOn = new QCheckBox(compHost);
    w.compOn->setChecked(ch.compEnabled);
    w.compThresh = new QDoubleSpinBox(compHost);
    w.compThresh->setRange(-60.0, 0.0);
    w.compThresh->setDecimals(0);
    w.compThresh->setSuffix(QStringLiteral(" dB"));
    w.compThresh->setValue(ch.compThresholdDb);
    w.compThresh->setFixedWidth(72);
    w.compRatio = new QDoubleSpinBox(compHost);
    w.compRatio->setRange(1.0, 20.0);
    w.compRatio->setDecimals(1);
    w.compRatio->setPrefix(QStringLiteral("1:"));
    w.compRatio->setValue(ch.compRatio);
    w.compRatio->setFixedWidth(64);
    w.compMakeup = new QDoubleSpinBox(compHost);
    w.compMakeup->setRange(0.0, 24.0);
    w.compMakeup->setDecimals(0);
    w.compMakeup->setSuffix(QStringLiteral(" dB"));
    w.compMakeup->setValue(ch.compMakeupDb);
    w.compMakeup->setFixedWidth(64);
    w.compMakeup->setToolTip(QStringLiteral("Makeup gain"));
    compLay->addWidget(w.compOn);
    compLay->addWidget(w.compThresh);
    compLay->addWidget(w.compRatio);
    compLay->addWidget(w.compMakeup);
    grid->addWidget(compHost, row, 8);

    auto* eqHost = new QWidget(m_rowsHost);
    auto* eqLay = new QHBoxLayout(eqHost);
    eqLay->setContentsMargins(0, 0, 0, 0);
    eqLay->setSpacing(3);
    auto mkEq = [&](QDoubleSpinBox*& box, float v, const QString& tip) {
        box = new QDoubleSpinBox(eqHost);
        box->setRange(-20.0, 20.0);
        box->setDecimals(1);
        box->setSuffix(QStringLiteral(" dB"));
        box->setValue(v);
        box->setFixedWidth(72);
        box->setToolTip(tip);
        eqLay->addWidget(box);
    };
    mkEq(w.eqLow, ch.eqLowDb, QStringLiteral("Low band (< ~800 Hz)"));
    mkEq(w.eqMid, ch.eqMidDb, QStringLiteral("Mid band"));
    mkEq(w.eqHigh, ch.eqHighDb, QStringLiteral("High band (> ~5 kHz)"));
    grid->addWidget(eqHost, row, 9);

    auto* limHost = new QWidget(m_rowsHost);
    auto* limLay = new QHBoxLayout(limHost);
    limLay->setContentsMargins(0, 0, 0, 0);
    limLay->setSpacing(4);
    w.limOn = new QCheckBox(limHost);
    w.limOn->setChecked(ch.limiterEnabled);
    w.limThresh = new QDoubleSpinBox(limHost);
    w.limThresh->setRange(-60.0, 0.0);
    w.limThresh->setDecimals(1);
    w.limThresh->setSuffix(QStringLiteral(" dB"));
    w.limThresh->setValue(ch.limiterThresholdDb);
    w.limThresh->setFixedWidth(78);
    w.limThresh->setToolTip(QStringLiteral("Limiter threshold"));
    w.limRelease = new QSpinBox(limHost);
    w.limRelease->setRange(1, 1000);
    w.limRelease->setSuffix(QStringLiteral(" ms"));
    w.limRelease->setValue(int(std::lround(ch.limiterReleaseMs)));
    w.limRelease->setFixedWidth(72);
    w.limRelease->setToolTip(QStringLiteral("Limiter release"));
    limLay->addWidget(w.limOn);
    limLay->addWidget(w.limThresh);
    limLay->addWidget(w.limRelease);
    grid->addWidget(limHost, row, 10);

    w.monitoring = new QComboBox(m_rowsHost);
    w.monitoring->addItem(QStringLiteral("Monitor Off"), int(AudioMonitoringType::None));
    w.monitoring->addItem(QStringLiteral("Monitor Only (MUTE)"), int(AudioMonitoringType::MonitorOnly));
    w.monitoring->addItem(QStringLiteral("Monitor and Output"), int(AudioMonitoringType::MonitorAndOutput));
    const int monIdx = w.monitoring->findData(int(ch.monitoring));
    w.monitoring->setCurrentIndex(monIdx >= 0 ? monIdx : 2);
    grid->addWidget(w.monitoring, row, 11);

    auto* trackHost = new QWidget(m_rowsHost);
    auto* trackLay = new QHBoxLayout(trackHost);
    trackLay->setContentsMargins(0, 0, 0, 0);
    trackLay->setSpacing(2);
    for (int t = 0; t < 6; ++t) {
        w.tracks[t] = new QCheckBox(QString::number(t + 1), trackHost);
        w.tracks[t]->setChecked(ch.trackMask & (1u << t));
        trackLay->addWidget(w.tracks[t]);
    }
    grid->addWidget(trackHost, row, 12);

    m_rows.insert(channelId, w);

    auto wire = [this, channelId] { applyChannel(channelId); };
    connect(w.volDb, qOverload<double>(&QDoubleSpinBox::valueChanged), this, wire);
    connect(w.volPct, qOverload<int>(&QSpinBox::valueChanged), this, wire);
    connect(w.gainDb, qOverload<double>(&QDoubleSpinBox::valueChanged), this, wire);
    connect(w.mono, &QCheckBox::toggled, this, wire);
    connect(w.balance, &QSlider::valueChanged, this, wire);
    connect(w.sync, qOverload<int>(&QSpinBox::valueChanged), this, wire);
    connect(w.gateOn, &QCheckBox::toggled, this, wire);
    connect(w.gateOpenDb, qOverload<double>(&QDoubleSpinBox::valueChanged), this, wire);
    connect(w.compOn, &QCheckBox::toggled, this, wire);
    connect(w.compThresh, qOverload<double>(&QDoubleSpinBox::valueChanged), this, wire);
    connect(w.compRatio, qOverload<double>(&QDoubleSpinBox::valueChanged), this, wire);
    connect(w.compMakeup, qOverload<double>(&QDoubleSpinBox::valueChanged), this, wire);
    connect(w.eqLow, qOverload<double>(&QDoubleSpinBox::valueChanged), this, wire);
    connect(w.eqMid, qOverload<double>(&QDoubleSpinBox::valueChanged), this, wire);
    connect(w.eqHigh, qOverload<double>(&QDoubleSpinBox::valueChanged), this, wire);
    connect(w.limOn, &QCheckBox::toggled, this, wire);
    connect(w.limThresh, qOverload<double>(&QDoubleSpinBox::valueChanged), this, wire);
    connect(w.limRelease, qOverload<int>(&QSpinBox::valueChanged), this, wire);
    connect(w.monitoring, qOverload<int>(&QComboBox::currentIndexChanged), this, wire);
    for (int t = 0; t < 6; ++t)
        connect(w.tracks[t], &QCheckBox::toggled, this, wire);
}

void AdvAudioDialog::onUsePercentToggled(bool usePercent)
{
    m_usePercentMode = usePercent;
    for (auto it = m_rows.begin(); it != m_rows.end(); ++it) {
        if (it->volDb) it->volDb->setVisible(!usePercent);
        if (it->volPct) it->volPct->setVisible(usePercent);
        if (usePercent && it->volDb && it->volPct) {
            QSignalBlocker b(it->volPct);
            it->volPct->setValue(int(std::lround(dbToLinear(float(it->volDb->value())) * 100.0)));
        } else if (!usePercent && it->volDb && it->volPct) {
            QSignalBlocker b(it->volDb);
            it->volDb->setValue(linearToDb(it->volPct->value() / 100.f));
        }
    }
}

void AdvAudioDialog::applyChannel(const QString& id)
{
    if (!m_engine || !m_engine->audio() || !m_rows.contains(id))
        return;
    const auto& w = m_rows[id];
    auto state = m_engine->audio()->channelState(id);
    if (m_usePercentMode && w.volPct)
        state.volume = std::clamp(w.volPct->value() / 100.f, 0.f, 20.f);
    else if (w.volDb)
        state.volume = dbToLinear(float(w.volDb->value()));
    state.volume = std::clamp(state.volume, 0.f, 20.f);
    if (w.gainDb) state.gainDb = float(w.gainDb->value());
    if (w.mono) state.forceMono = w.mono->isChecked();
    if (w.balance) state.pan = w.balance->value() / 100.f;
    if (w.sync) state.syncOffsetMs = w.sync->value();
    if (w.gateOn) state.gateEnabled = w.gateOn->isChecked();
    if (w.gateOpenDb) state.gateOpenDb = float(w.gateOpenDb->value());
    if (w.compOn) state.compEnabled = w.compOn->isChecked();
    if (w.compThresh) state.compThresholdDb = float(w.compThresh->value());
    if (w.compRatio) state.compRatio = float(w.compRatio->value());
    if (w.compMakeup) state.compMakeupDb = float(w.compMakeup->value());
    if (w.eqLow) state.eqLowDb = float(w.eqLow->value());
    if (w.eqMid) state.eqMidDb = float(w.eqMid->value());
    if (w.eqHigh) state.eqHighDb = float(w.eqHigh->value());
    if (w.limOn) state.limiterEnabled = w.limOn->isChecked();
    if (w.limThresh) state.limiterThresholdDb = float(w.limThresh->value());
    if (w.limRelease) state.limiterReleaseMs = float(w.limRelease->value());
    if (w.monitoring)
        state.monitoring = AudioMonitoringType(w.monitoring->currentData().toInt());
    quint8 mask = 0;
    for (int t = 0; t < 6; ++t) {
        if (w.tracks[t] && w.tracks[t]->isChecked())
            mask |= quint8(1u << t);
    }
    state.trackMask = mask ? mask : quint8(0x01);
    m_engine->audio()->setChannelState(id, state);

    // Keep dB/% pair in sync without feedback loops
    if (w.volDb && w.volPct) {
        if (m_usePercentMode) {
            QSignalBlocker b(w.volDb);
            w.volDb->setValue(linearToDb(state.volume));
        } else {
            QSignalBlocker b(w.volPct);
            w.volPct->setValue(int(std::lround(state.volume * 100.0)));
        }
    }
}

} // namespace railshot
