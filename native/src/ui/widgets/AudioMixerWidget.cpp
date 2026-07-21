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
#include <QCursor>
#include <QAction>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <cmath>
#include <algorithm>

namespace railshot {

namespace {
float peakToDb(float peak)
{
    if (peak <= 0.000001f) return -60.f;
    return std::max(-60.f, 20.f * std::log10(peak));
}
float volumeToDb(float vol)
{
    if (vol <= 0.000001f) return -60.f;
    return std::max(-60.f, 20.f * std::log10(std::clamp(vol, 0.f, 20.f)));
}
QString formatDb(float db)
{
    if (db <= -59.5f) return QStringLiteral("-inf dB");
    return QStringLiteral("%1 dB").arg(db, 0, 'f', 1);
}
} // namespace

AudioMixerWidget::AudioMixerWidget(EngineController* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setObjectName(QStringLiteral("audioMixer"));
    setMinimumWidth(280);
    setMinimumHeight(120);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setStyleSheet(QStringLiteral(
        "QWidget#audioMixer {"
        "  background:#0D0F12;"
        "  border-top:2px solid #A855F7;"
        "}"
        "QWidget#audioMixer QLabel#mixerHint {"
        "  color:#6B7280; font-size:9px; font-weight:600;"
        "  background:transparent; border:none;"
        "}"
        "QWidget#audioMixer QPushButton#mixerToolBtn {"
        "  background:#1A1D22; border:1px solid #3A3D45; border-radius:3px;"
        "  color:#C8CAD0; font-size:10px; font-weight:700; padding:3px 10px;"
        "  min-height:22px; max-height:24px;"
        "}"
        "QWidget#audioMixer QPushButton#mixerToolBtn:hover {"
        "  border-color:#4F9EFF; color:#FFFFFF;"
        "}"
        "QWidget#audioMixer QPushButton#mixerToolBtn:checked {"
        "  background:#4C1D95; border-color:#C084FC; color:#F5E8FF;"
        "}"));

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(6, 4, 6, 4);
    outer->setSpacing(4);

    // Toolbar — OBS Options / Adv Audio + global monitor
    auto* tools = new QHBoxLayout();
    tools->setSpacing(6);
    auto* hint = new QLabel(QStringLiteral("Mixer"), this);
    hint->setObjectName(QStringLiteral("mixerHint"));
    tools->addWidget(hint);
    tools->addStretch(1);

    m_monitorBtn = new QPushButton(QStringLiteral("Monitor"), this);
    m_monitorBtn->setObjectName(QStringLiteral("mixerToolBtn"));
    m_monitorBtn->setCheckable(true);
    m_monitorBtn->setChecked(true);
    m_monitorBtn->setCursor(Qt::PointingHandCursor);
    m_monitorBtn->setToolTip(QStringLiteral("Enable headphone / speaker monitoring"));
    connect(m_monitorBtn, &QPushButton::toggled, this, [this](bool on) {
        if (m_engine && m_engine->audio())
            m_engine->audio()->setMonitorEnabled(on);
    });
    tools->addWidget(m_monitorBtn);

    m_advBtn = new QPushButton(QStringLiteral("Advanced Audio…"), this);
    m_advBtn->setObjectName(QStringLiteral("mixerToolBtn"));
    m_advBtn->setCursor(Qt::PointingHandCursor);
    m_advBtn->setToolTip(QStringLiteral("Advanced Audio Properties (balance, sync, tracks, monitoring)"));
    connect(m_advBtn, &QPushButton::clicked, this, &AudioMixerWidget::openAdvAudio);
    tools->addWidget(m_advBtn);
    outer->addLayout(tools);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setStyleSheet(QStringLiteral(
        "QScrollArea{background:transparent; border:none;}"
        "QScrollBar:vertical{width:8px; background:#0A0C0F;}"
        "QScrollBar::handle:vertical{background:#3A3D45; border-radius:3px; min-height:24px;}"));
    auto* host = new QWidget(scroll);
    host->setObjectName(QStringLiteral("mixerHost"));
    host->setStyleSheet(QStringLiteral("QWidget#mixerHost{background:transparent;}"));
    m_list = new QVBoxLayout(host);
    m_list->setContentsMargins(0, 0, 0, 0);
    m_list->setSpacing(2);
    m_list->setAlignment(Qt::AlignTop);
    scroll->setWidget(host);
    outer->addWidget(scroll, 1);

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
    timer->start(50);
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
    while (QLayoutItem* child = m_list->takeAt(0)) {
        if (child->widget()) child->widget()->deleteLater();
        delete child;
    }
    m_stripIds.clear();

    const auto channels = m_engine->audioChannels();
    for (const auto& ch : channels) {
        const bool master = ch.id == QLatin1String("master");
        m_stripIds.append(ch.id);

        auto* row = new QWidget(this);
        row->setObjectName(QStringLiteral("mixRow_") + ch.id);
        row->setProperty("channelId", ch.id);
        row->setFixedHeight(30);
        row->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        row->setContextMenuPolicy(Qt::CustomContextMenu);
        row->setStyleSheet(QStringLiteral(
            "QWidget{background:#12151A; border:1px solid #2A2D35; border-radius:3px;}"));
        connect(row, &QWidget::customContextMenuRequested, this, [this, id = ch.id](const QPoint&) {
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
                menu.addSeparator();
                auto* monOff = menu.addAction(QStringLiteral("Monitor Off"));
                auto* monBoth = menu.addAction(QStringLiteral("Monitor and Output"));
                auto* monOnly = menu.addAction(QStringLiteral("Monitor Only (mute output)"));
                connect(monOff, &QAction::triggered, this, [this, id] {
                    auto s = m_engine->audio()->channelState(id);
                    s.monitoring = AudioMonitoringType::None;
                    m_engine->audio()->setChannelState(id, s);
                });
                connect(monBoth, &QAction::triggered, this, [this, id] {
                    auto s = m_engine->audio()->channelState(id);
                    s.monitoring = AudioMonitoringType::MonitorAndOutput;
                    m_engine->audio()->setChannelState(id, s);
                });
                connect(monOnly, &QAction::triggered, this, [this, id] {
                    auto s = m_engine->audio()->channelState(id);
                    s.monitoring = AudioMonitoringType::MonitorOnly;
                    m_engine->audio()->setChannelState(id, s);
                });
            }
            menu.exec(QCursor::pos());
        });

        auto* lay = new QHBoxLayout(row);
        lay->setContentsMargins(6, 2, 4, 2);
        lay->setSpacing(6);

        auto* name = new QLabel(master ? QStringLiteral("Master") : ch.name, row);
        name->setObjectName(QStringLiteral("stripName"));
        name->setFixedWidth(108);
        name->setToolTip(ch.name);
        const QString nameColor = master ? QStringLiteral("#4ADE80")
                                         : (ch.muted ? QStringLiteral("#EF4444")
                                                     : QStringLiteral("#E5E7EB"));
        name->setStyleSheet(QStringLiteral(
            "font-family:'DM Sans','Segoe UI'; font-size:11px; font-weight:700;"
            "color:%1; background:transparent; border:none;")
                                .arg(nameColor));
        name->setText(name->fontMetrics().elidedText(name->text(), Qt::ElideRight, 108));
        lay->addWidget(name);

        // Compact stereo meters (OBS-like horizontal)
        auto* meterCol = new QVBoxLayout();
        meterCol->setContentsMargins(0, 0, 0, 0);
        meterCol->setSpacing(1);
        auto makeMeter = [&](const QString& suffix) {
            auto* meter = new QProgressBar(row);
            meter->setObjectName(ch.id + suffix);
            meter->setOrientation(Qt::Horizontal);
            meter->setRange(0, 100);
            meter->setValue(0);
            meter->setTextVisible(false);
            meter->setFixedHeight(5);
            meter->setMinimumWidth(72);
            meter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            meter->setStyleSheet(QStringLiteral(
                "QProgressBar{background:#0A0C0F;border:1px solid #1F2937;border-radius:1px;}"
                "QProgressBar::chunk{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,"
                "stop:0 #22C55E, stop:0.55 #84CC16, stop:0.8 #FBBF24, stop:1 #EF4444);}"));
            return meter;
        };
        meterCol->addWidget(makeMeter(QStringLiteral("_L")));
        meterCol->addWidget(makeMeter(QStringLiteral("_R")));
        lay->addLayout(meterCol, 2);

        auto* fader = new QSlider(Qt::Horizontal, row);
        fader->setObjectName(QStringLiteral("volSlider"));
        fader->setRange(0, 100);
        fader->setValue(int(std::clamp(ch.volume, 0.f, 1.f) * 100));
        fader->setFixedHeight(18);
        fader->setMinimumWidth(90);
        fader->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        fader->setEnabled(!ch.locked || master);
        fader->setToolTip(QStringLiteral("Volume"));
        fader->setStyleSheet(QStringLiteral(
            "QSlider::groove:horizontal{background:#1A1D22;height:4px;border:1px solid #3A3D45;border-radius:2px;}"
            "QSlider::sub-page:horizontal{background:#6366F1;border-radius:2px;}"
            "QSlider::handle:horizontal{background:#E5E7EB;width:10px;height:14px;margin:-6px 0;"
            "border:1px solid #9CA3AF;border-radius:2px;}"
            "QSlider::handle:horizontal:hover{background:#FFFFFF;border-color:#A855F7;}"
            "QSlider:disabled{opacity:0.4;}"));
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
        lay->addWidget(fader, 3);

        auto* volDb = new QLabel(formatDb(volumeToDb(ch.volume)), row);
        volDb->setObjectName(QStringLiteral("volDbLabel"));
        volDb->setFixedWidth(58);
        volDb->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        volDb->setStyleSheet(QStringLiteral(
            "font-family:'JetBrains Mono','Consolas',monospace; font-size:9px; color:#9CA3AF;"
            "background:transparent; border:none;"));
        lay->addWidget(volDb);

        auto* peakDb = new QLabel(formatDb(peakToDb(std::max(ch.peakL, ch.peakR))), row);
        peakDb->setObjectName(QStringLiteral("dbLabel"));
        peakDb->setFixedWidth(58);
        peakDb->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        peakDb->setToolTip(QStringLiteral("Peak level"));
        peakDb->setStyleSheet(QStringLiteral(
            "font-family:'JetBrains Mono','Consolas',monospace; font-size:9px; color:#6B7280;"
            "background:transparent; border:none;"));
        lay->addWidget(peakDb);

        auto makeCtrl = [&](const QString& text, const QString& tip) {
            auto* b = new QPushButton(text, row);
            b->setCheckable(true);
            b->setCursor(Qt::PointingHandCursor);
            b->setFixedSize(22, 20);
            b->setToolTip(tip);
            b->setStyleSheet(QStringLiteral(
                "QPushButton{background:#1A1D22;border:1px solid #3A3D45;color:#9CA3AF;"
                "font-size:9px;font-weight:800;border-radius:2px;}"
                "QPushButton:hover{border-color:#6B7280;color:#E5E7EB;}"
                "QPushButton:checked{background:#B91C1C;color:#FFFFFF;border-color:#EF4444;}"));
            return b;
        };

        auto* mute = makeCtrl(QStringLiteral("M"), QStringLiteral("Mute"));
        mute->setObjectName(QStringLiteral("muteBtn"));
        mute->setChecked(ch.muted);
        connect(mute, &QPushButton::clicked, this, [this, id = ch.id, master] {
            if (master) {
                m_engine->audio()->setMasterMuted(!m_engine->audio()->masterMuted());
                return;
            }
            auto state = m_engine->audio()->channelState(id);
            state.muted = !state.muted;
            m_engine->audio()->setChannelState(id, state);
        });
        lay->addWidget(mute);

        if (!master) {
            auto* solo = makeCtrl(QStringLiteral("S"), QStringLiteral("Solo"));
            solo->setObjectName(QStringLiteral("soloBtn"));
            solo->setChecked(ch.solo);
            solo->setStyleSheet(QStringLiteral(
                "QPushButton{background:#1A1D22;border:1px solid #3A3D45;color:#9CA3AF;"
                "font-size:9px;font-weight:800;border-radius:2px;}"
                "QPushButton:hover{border-color:#6B7280;color:#E5E7EB;}"
                "QPushButton:checked{background:#CA8A04;color:#111;border-color:#FBBF24;}"));
            connect(solo, &QPushButton::clicked, this, [this, id = ch.id] {
                auto state = m_engine->audio()->channelState(id);
                state.solo = !state.solo;
                m_engine->audio()->setChannelState(id, state);
            });
            lay->addWidget(solo);

            auto* mon = makeCtrl(QStringLiteral("H"), QStringLiteral("Monitor (cycle)"));
            mon->setObjectName(QStringLiteral("monBtn"));
            mon->setChecked(ch.monitoring != AudioMonitoringType::None);
            mon->setStyleSheet(QStringLiteral(
                "QPushButton{background:#1A1D22;border:1px solid #3A3D45;color:#9CA3AF;"
                "font-size:9px;font-weight:800;border-radius:2px;}"
                "QPushButton:hover{border-color:#6B7280;color:#E5E7EB;}"
                "QPushButton:checked{background:#1D4ED8;color:#FFFFFF;border-color:#60A5FA;}"));
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
            lay->addWidget(mon);
        }

        m_list->addWidget(row);
    }
    m_list->addStretch(1);
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
        if (!item || !item->widget()) continue;
        auto* row = item->widget();
        const auto& ch = channels[i];
        const bool master = ch.id == QLatin1String("master");

        if (auto* barL = row->findChild<QProgressBar*>(ch.id + QStringLiteral("_L")))
            barL->setValue(int(std::clamp(ch.peakL, 0.f, 1.f) * 100));
        if (auto* barR = row->findChild<QProgressBar*>(ch.id + QStringLiteral("_R")))
            barR->setValue(int(std::clamp(ch.peakR, 0.f, 1.f) * 100));
        if (auto* db = row->findChild<QLabel*>(QStringLiteral("dbLabel")))
            db->setText(formatDb(peakToDb(std::max(ch.peakL, ch.peakR))));
        if (auto* volDb = row->findChild<QLabel*>(QStringLiteral("volDbLabel")))
            volDb->setText(formatDb(volumeToDb(ch.volume)));
        if (auto* name = row->findChild<QLabel*>(QStringLiteral("stripName"))) {
            const QString nameColor = master ? QStringLiteral("#4ADE80")
                                             : (ch.muted ? QStringLiteral("#EF4444")
                                                         : QStringLiteral("#E5E7EB"));
            name->setStyleSheet(QStringLiteral(
                "font-family:'DM Sans','Segoe UI'; font-size:11px; font-weight:700;"
                "color:%1; background:transparent; border:none;")
                                    .arg(nameColor));
        }
        if (auto* mute = row->findChild<QPushButton*>(QStringLiteral("muteBtn"))) {
            QSignalBlocker b(mute);
            mute->setChecked(ch.muted);
        }
        if (auto* solo = row->findChild<QPushButton*>(QStringLiteral("soloBtn"))) {
            QSignalBlocker b(solo);
            solo->setChecked(ch.solo);
        }
        if (auto* mon = row->findChild<QPushButton*>(QStringLiteral("monBtn"))) {
            QSignalBlocker b(mon);
            mon->setChecked(ch.monitoring != AudioMonitoringType::None);
            mon->setToolTip(ch.monitoring == AudioMonitoringType::MonitorOnly
                                ? QStringLiteral("Monitor Only (muted in stream)")
                                : ch.monitoring == AudioMonitoringType::MonitorAndOutput
                                      ? QStringLiteral("Monitor and Output")
                                      : QStringLiteral("Monitor Off — click to cycle"));
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
