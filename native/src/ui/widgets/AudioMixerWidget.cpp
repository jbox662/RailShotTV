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
#include <QProgressBar>
#include <QScrollArea>
#include <QFrame>
#include <QMenu>
#include <QContextMenuEvent>
#include <QCursor>
#include <QAction>
#include <QSignalBlocker>
#include <cmath>
#include <algorithm>

namespace railshot {

namespace {
float peakToDb(float peak)
{
    if (peak <= 0.000001f) return -60.f;
    return std::max(-60.f, 20.f * std::log10(peak));
}
QString formatDb(float db)
{
    if (db <= -59.5f) return QStringLiteral("-inf");
    return QStringLiteral("%1").arg(db, 0, 'f', 1);
}
} // namespace

AudioMixerWidget::AudioMixerWidget(EngineController* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setObjectName(QStringLiteral("audioMixer"));
    setMinimumWidth(200);
    setMinimumHeight(140);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setStyleSheet(QStringLiteral(
        "QWidget#audioMixer {"
        "  background:#0D0F12;"
        "  border-top: 3px solid #A855F7;"
        "  border-left: 2px solid #A855F7;"
        "  border-right: 1px solid #3A3D45;"
        "}"));

    auto* outer = new QHBoxLayout(this);
    outer->setContentsMargins(6, 6, 6, 6);
    outer->setSpacing(6);

    auto* outputsLab = new QLabel(QStringLiteral("O\nU\nT\nP\nU\nT\nS"), this);
    outputsLab->setAlignment(Qt::AlignCenter);
    outputsLab->setFixedWidth(18);
    outputsLab->setStyleSheet(QStringLiteral(
        "color:#7AB8FF; font-size:8px; font-weight:900; letter-spacing:1px;"
        "background:rgba(58,106,255,0.22); border:1px solid #3A6AFF66; border-radius:3px;"));
    outer->addWidget(outputsLab);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setStyleSheet(QStringLiteral("background:transparent; border:none;"));
    auto* host = new QWidget(scroll);
    m_row = new QHBoxLayout(host);
    m_row->setContentsMargins(0, 0, 0, 0);
    m_row->setSpacing(6);
    scroll->setWidget(host);
    outer->addWidget(scroll, 1);

    auto* inputsLab = new QLabel(QStringLiteral("I\nN\nP\nU\nT\nS"), this);
    inputsLab->setAlignment(Qt::AlignCenter);
    inputsLab->setFixedWidth(18);
    inputsLab->setStyleSheet(QStringLiteral(
        "color:#C084FC; font-size:8px; font-weight:900; letter-spacing:1px;"
        "background:rgba(168,85,247,0.22); border:1px solid #A855F766; border-radius:3px;"));
    outer->addWidget(inputsLab);

    auto* side = new QVBoxLayout();
    side->setSpacing(4);
    auto* title = new QLabel(QStringLiteral("MIX"), this);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet(QStringLiteral(
        "color:#C084FC; font-size:10px; font-weight:900; letter-spacing:1px; background:transparent;"));
    m_monitorBtn = new QPushButton(QStringLiteral("MON"), this);
    m_monitorBtn->setCheckable(true);
    m_monitorBtn->setChecked(true);
    m_monitorBtn->setFixedHeight(22);
    m_monitorBtn->setToolTip(QStringLiteral("Headphone / speaker monitor master"));
    m_monitorBtn->setStyleSheet(QStringLiteral(
        "QPushButton{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #2A2D35,stop:1 #1A1D22);"
        "border:1px solid #A855F7;color:#C084FC;font-size:9px;font-weight:800;border-radius:3px;}"
        "QPushButton:checked{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #6D28D9,stop:1 #4C1D95);"
        "color:#F5E8FF;border-color:#C084FC;}"
        "QPushButton:hover{border-color:#E9D5FF;}"));
    connect(m_monitorBtn, &QPushButton::toggled, this, [this](bool on) {
        if (m_engine && m_engine->audio())
            m_engine->audio()->setMonitorEnabled(on);
    });

    m_advBtn = new QPushButton(QStringLiteral("ADV"), this);
    m_advBtn->setFixedHeight(22);
    m_advBtn->setToolTip(QStringLiteral("Advanced Audio Properties"));
    m_advBtn->setStyleSheet(QStringLiteral(
        "QPushButton{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #2A2D35,stop:1 #1A1D22);"
        "border:1px solid #3A6AFF;color:#7AB8FF;font-size:9px;font-weight:800;border-radius:3px;}"
        "QPushButton:hover{border-color:#8AB4FF;color:#fff;}"));
    connect(m_advBtn, &QPushButton::clicked, this, &AudioMixerWidget::openAdvAudio);

    side->addWidget(title);
    side->addWidget(m_monitorBtn);
    side->addWidget(m_advBtn);
    side->addStretch();
    outer->addLayout(side);

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
    timer->start(60);
    refreshMeters();
}

void AudioMixerWidget::openAdvAudio()
{
    emit openAdvAudioRequested();
    AdvAudioDialog dlg(m_engine, window());
    dlg.exec();
}

void AudioMixerWidget::rebuildStrips()
{
    while (QLayoutItem* child = m_row->takeAt(0)) {
        if (child->widget()) child->widget()->deleteLater();
        delete child;
    }
    m_stripIds.clear();

    const auto channels = m_engine->audioChannels();
    for (const auto& ch : channels) {
        const bool master = ch.id == QLatin1String("master");
        m_stripIds.append(ch.id);

        auto* strip = new QWidget(this);
        strip->setObjectName(QStringLiteral("mixStrip_") + ch.id);
        strip->setProperty("channelId", ch.id);
        strip->setFixedWidth(master ? 80 : 72);
        strip->setContextMenuPolicy(Qt::CustomContextMenu);
        strip->setStyleSheet(QStringLiteral(
            "QWidget{background:#12151A; border:1px solid #3A3D45; border-radius:4px;}"));
        connect(strip, &QWidget::customContextMenuRequested, this, [this, id = ch.id](const QPoint& pos) {
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
            }
            menu.exec(QCursor::pos());
            Q_UNUSED(pos);
        });

        auto* col = new QVBoxLayout(strip);
        col->setContentsMargins(4, 4, 4, 4);
        col->setSpacing(3);

        auto* badge = new QLabel(master ? QStringLiteral("MASTER") : ch.name.left(10), strip);
        badge->setObjectName(QStringLiteral("stripName"));
        badge->setAlignment(Qt::AlignCenter);
        badge->setToolTip(ch.name);
        if (master) {
            badge->setStyleSheet(QStringLiteral(
                "font-size:8px; font-weight:900; color:#04140A;"
                "background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #4ADE80,stop:1 #22C55E);"
                "border:1px solid #86EFAC; border-radius:3px; padding:2px 4px;"));
        } else {
            badge->setStyleSheet(QStringLiteral(
                "font-size:8px; font-weight:800; color:%1; background:#1A1D22;"
                "border:1px solid #2A2D35; border-radius:3px; padding:2px 4px;")
                                     .arg(ch.muted ? QStringLiteral("#EF4444")
                                                   : QStringLiteral("#C8CAD0")));
        }
        col->addWidget(badge);

        auto* meterRow = new QHBoxLayout();
        meterRow->setSpacing(2);
        meterRow->setContentsMargins(0, 0, 0, 0);
        auto makeBar = [&](const QString& suffix) {
            auto* meter = new QProgressBar(strip);
            meter->setObjectName(ch.id + suffix);
            meter->setOrientation(Qt::Vertical);
            meter->setRange(0, 100);
            meter->setValue(0);
            meter->setTextVisible(false);
            meter->setFixedSize(8, 72);
            const QString base = ch.id.contains(QLatin1String("mic"), Qt::CaseInsensitive)
                                     ? QStringLiteral("#22C55E")
                                     : QStringLiteral("#4F9EFF");
            meter->setStyleSheet(QStringLiteral(
                "QProgressBar{background:#0A0E1A;border:1px solid rgba(255,255,255,0.08);border-radius:2px;}"
                "QProgressBar::chunk{background:qlineargradient(x1:0,y1:1,x2:0,y2:0,"
                "stop:0 %1, stop:0.4 #84CC16, stop:0.7 #FBBF24, stop:1 #EF4444);}")
                                     .arg(base));
            return meter;
        };
        meterRow->addStretch();
        meterRow->addWidget(makeBar(QStringLiteral("_L")));
        meterRow->addWidget(makeBar(QStringLiteral("_R")));
        meterRow->addStretch();
        col->addLayout(meterRow);

        auto* db = new QLabel(QStringLiteral("-inf"), strip);
        db->setObjectName(QStringLiteral("dbLabel"));
        db->setAlignment(Qt::AlignCenter);
        db->setStyleSheet(QStringLiteral(
            "font-family:'JetBrains Mono'; font-size:8px; color:#8892A4; background:transparent;"));
        col->addWidget(db);

        // Balance (skip for master)
        if (!master) {
            auto* bal = new QSlider(Qt::Horizontal, strip);
            bal->setObjectName(QStringLiteral("balSlider"));
            bal->setRange(-100, 100);
            bal->setValue(int(ch.pan * 100));
            bal->setFixedHeight(14);
            bal->setToolTip(QStringLiteral("Balance L/R"));
            bal->setStyleSheet(QStringLiteral(
                "QSlider::groove:horizontal{background:#1A1D22;height:4px;border:1px solid #3A3D45;border-radius:2px;}"
                "QSlider::handle:horizontal{background:#7AB8FF;width:8px;margin:-4px 0;border-radius:4px;}"));
            connect(bal, &QSlider::valueChanged, this, [this, id = ch.id](int v) {
                auto state = m_engine->audio()->channelState(id);
                if (state.locked) return;
                state.pan = v / 100.f;
                m_engine->audio()->setChannelState(id, state);
            });
            col->addWidget(bal);
        }

        auto* fader = new QSlider(Qt::Vertical, strip);
        fader->setObjectName(QStringLiteral("volSlider"));
        fader->setRange(0, 100);
        fader->setValue(int(std::clamp(ch.volume, 0.f, 1.f) * 100));
        fader->setFixedHeight(64);
        fader->setEnabled(!ch.locked || master);
        fader->setStyleSheet(QStringLiteral(
            "QSlider::groove:vertical{background:#1A1D22;width:6px;border:1px solid #3A3D45;border-radius:3px;}"
            "QSlider::handle:vertical{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #EC4899,stop:1 #A855F7);"
            "height:12px;width:14px;margin:-2px -4px;border-radius:3px;border:1px solid #F9A8D4;}"
            "QSlider::handle:vertical:hover{background:#F472B6;}"
            "QSlider:disabled{opacity:0.45;}"));
        connect(fader, &QSlider::valueChanged, this, [this, id = ch.id, master](int v) {
            if (!m_engine || !m_engine->audio()) return;
            if (master) {
                m_engine->audio()->setMasterVolume(v / 100.f);
                return;
            }
            auto state = m_engine->audio()->channelState(id);
            if (state.locked) return;
            state.volume = v / 100.f;
            m_engine->audio()->setChannelState(id, state);
        });
        col->addWidget(fader, 0, Qt::AlignHCenter);

        auto* btnRow = new QHBoxLayout();
        btnRow->setSpacing(2);
        auto* mute = new QPushButton(QStringLiteral("M"), strip);
        mute->setObjectName(QStringLiteral("muteBtn"));
        mute->setFixedSize(22, 18);
        mute->setCheckable(true);
        mute->setChecked(ch.muted);
        mute->setToolTip(QStringLiteral("Mute"));
        mute->setStyleSheet(QStringLiteral(
            "QPushButton{background:#1A1D22;border:1px solid #3A3D45;color:#A0A8B8;"
            "font-size:9px;font-weight:800;border-radius:3px;}"
            "QPushButton:checked{background:#EF4444;color:white;border-color:#F87171;}"
            "QPushButton:hover{border-color:#8892A4;}"));
        connect(mute, &QPushButton::clicked, this, [this, id = ch.id, master] {
            if (master) {
                m_engine->audio()->setMasterMuted(!m_engine->audio()->masterMuted());
                return;
            }
            auto state = m_engine->audio()->channelState(id);
            state.muted = !state.muted;
            m_engine->audio()->setChannelState(id, state);
        });
        btnRow->addWidget(mute);

        if (!master) {
            auto* solo = new QPushButton(QStringLiteral("S"), strip);
            solo->setObjectName(QStringLiteral("soloBtn"));
            solo->setFixedSize(22, 18);
            solo->setCheckable(true);
            solo->setChecked(ch.solo);
            solo->setToolTip(QStringLiteral("Solo"));
            solo->setStyleSheet(QStringLiteral(
                "QPushButton{background:#1A1D22;border:1px solid #3A3D45;color:#A0A8B8;"
                "font-size:9px;font-weight:800;border-radius:3px;}"
                "QPushButton:checked{background:#FBBF24;color:#111;border-color:#FDE68A;}"
                "QPushButton:hover{border-color:#8892A4;}"));
            connect(solo, &QPushButton::clicked, this, [this, id = ch.id] {
                auto state = m_engine->audio()->channelState(id);
                state.solo = !state.solo;
                m_engine->audio()->setChannelState(id, state);
            });
            btnRow->addWidget(solo);

            auto* mon = new QPushButton(QStringLiteral("H"), strip);
            mon->setObjectName(QStringLiteral("monBtn"));
            mon->setFixedSize(22, 18);
            mon->setCheckable(true);
            mon->setChecked(ch.monitoring != AudioMonitoringType::None);
            mon->setToolTip(QStringLiteral("Headphones / monitor"));
            mon->setStyleSheet(QStringLiteral(
                "QPushButton{background:#1A1D22;border:1px solid #3A3D45;color:#A0A8B8;"
                "font-size:9px;font-weight:800;border-radius:3px;}"
                "QPushButton:checked{background:#3A6AFF;color:white;border-color:#7AB8FF;}"
                "QPushButton:hover{border-color:#8892A4;}"));
            connect(mon, &QPushButton::clicked, this, [this, id = ch.id] {
                auto state = m_engine->audio()->channelState(id);
                if (state.monitoring == AudioMonitoringType::None)
                    state.monitoring = AudioMonitoringType::MonitorAndOutput;
                else if (state.monitoring == AudioMonitoringType::MonitorAndOutput)
                    state.monitoring = AudioMonitoringType::MonitorOnly;
                else
                    state.monitoring = AudioMonitoringType::None;
                m_engine->audio()->setChannelState(id, state);
                // Rebuild to refresh check state styling
                m_stripIds.clear();
                rebuildStrips();
            });
            btnRow->addWidget(mon);
        }
        col->addLayout(btnRow);
        m_row->addWidget(strip);
    }
    m_row->addStretch();
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
    for (int i = 0; i < channels.size() && i < m_row->count(); ++i) {
        auto* item = m_row->itemAt(i);
        if (!item || !item->widget()) continue;
        auto* strip = item->widget();
        const auto& ch = channels[i];
        if (auto* barL = strip->findChild<QProgressBar*>(ch.id + QStringLiteral("_L")))
            barL->setValue(int(std::clamp(ch.peakL, 0.f, 1.f) * 100));
        if (auto* barR = strip->findChild<QProgressBar*>(ch.id + QStringLiteral("_R")))
            barR->setValue(int(std::clamp(ch.peakR, 0.f, 1.f) * 100));
        if (auto* db = strip->findChild<QLabel*>(QStringLiteral("dbLabel"))) {
            const float peak = std::max(ch.peakL, ch.peakR);
            db->setText(formatDb(peakToDb(peak)));
        }
        if (auto* mute = strip->findChild<QPushButton*>(QStringLiteral("muteBtn"))) {
            QSignalBlocker b(mute);
            mute->setChecked(ch.muted);
        }
        if (auto* solo = strip->findChild<QPushButton*>(QStringLiteral("soloBtn"))) {
            QSignalBlocker b(solo);
            solo->setChecked(ch.solo);
        }
        if (auto* mon = strip->findChild<QPushButton*>(QStringLiteral("monBtn"))) {
            QSignalBlocker b(mon);
            mon->setChecked(ch.monitoring != AudioMonitoringType::None);
            mon->setToolTip(ch.monitoring == AudioMonitoringType::MonitorOnly
                                ? QStringLiteral("Monitor Only (muted in stream)")
                                : ch.monitoring == AudioMonitoringType::MonitorAndOutput
                                      ? QStringLiteral("Monitor and Output")
                                      : QStringLiteral("Monitor Off"));
        }
        if (auto* fader = strip->findChild<QSlider*>(QStringLiteral("volSlider"))) {
            if (!fader->isSliderDown()) {
                QSignalBlocker b(fader);
                fader->setValue(int(std::clamp(ch.volume, 0.f, 1.f) * 100));
            }
        }
        if (auto* bal = strip->findChild<QSlider*>(QStringLiteral("balSlider"))) {
            if (!bal->isSliderDown()) {
                QSignalBlocker b(bal);
                bal->setValue(int(ch.pan * 100));
            }
        }
    }
}

} // namespace railshot
