#include "ui/widgets/AudioMixerWidget.h"
#include "core/EngineController.h"
#include "audio/AudioGraph.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QProgressBar>

namespace railshot {

AudioMixerWidget::AudioMixerWidget(EngineController* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setMinimumHeight(160);
    setMaximumHeight(220);
    setStyleSheet(QStringLiteral(
        "background:#0D0F12;"
        "border-top: 2px solid #A855F7;"
        "border-left: 1px solid #1A1D24;"));
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(6, 6, 6, 4);
    outer->setSpacing(2);

    auto* top = new QHBoxLayout();
    auto* title = new QLabel(QStringLiteral("MIX"), this);
    title->setStyleSheet(QStringLiteral(
        "color:#A855F7; font-weight:800; font-size:11px; letter-spacing:1.5px; background:transparent;"));
    m_monitorBtn = new QPushButton(QStringLiteral("MON"), this);
    m_monitorBtn->setCheckable(true);
    m_monitorBtn->setChecked(true);
    m_monitorBtn->setFixedHeight(18);
    m_monitorBtn->setToolTip(QStringLiteral("Headphone / speaker monitor"));
    connect(m_monitorBtn, &QPushButton::toggled, this, [this](bool on) {
        if (m_engine && m_engine->audio())
            m_engine->audio()->setMonitorEnabled(on);
    });
    top->addWidget(title);
    top->addStretch();
    top->addWidget(m_monitorBtn);
    outer->addLayout(top);

    m_row = new QHBoxLayout();
    m_row->setContentsMargins(0, 0, 0, 0);
    m_row->setSpacing(2);
    outer->addLayout(m_row);

    auto* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &AudioMixerWidget::refreshMeters);
    timer->start(100);
    refreshMeters();
}

void AudioMixerWidget::refreshMeters()
{
    const auto channels = m_engine->audioChannels();
    if (m_row->count() != channels.size() + 1) {
        while (QLayoutItem* child = m_row->takeAt(0)) {
            if (child->widget()) child->widget()->deleteLater();
            delete child;
        }
        for (const auto& ch : channels) {
            auto* strip = new QWidget(this);
            strip->setFixedWidth(48);
            auto* col = new QVBoxLayout(strip);
            col->setContentsMargins(2, 2, 2, 2);
            auto* meter = new QProgressBar(strip);
            meter->setOrientation(Qt::Vertical);
            meter->setRange(0, 100);
            meter->setValue(int(ch.peakL * 100));
            meter->setObjectName(ch.id);
            meter->setTextVisible(false);
            meter->setFixedHeight(90);
            col->addWidget(meter, 0, Qt::AlignHCenter);

            auto* fader = new QSlider(Qt::Vertical, strip);
            fader->setRange(0, 100);
            fader->setValue(int(ch.volume * 100));
            fader->setFixedHeight(90);
            connect(fader, &QSlider::valueChanged, this, [this, id = ch.id](int v) {
                auto state = m_engine->audio()->channelState(id);
                state.volume = v / 100.f;
                m_engine->audio()->setChannelState(id, state);
            });
            col->addWidget(fader, 0, Qt::AlignHCenter);

            auto* name = new QLabel(ch.name.left(8), strip);
            name->setAlignment(Qt::AlignCenter);
            name->setStyleSheet(QStringLiteral("font-size:7px;color:#808898;"));
            col->addWidget(name);

            auto* btnRow = new QHBoxLayout();
            btnRow->setSpacing(1);
            auto* mute = new QPushButton(ch.muted ? QStringLiteral("M") : QStringLiteral("m"), strip);
            mute->setFixedHeight(14);
            mute->setCheckable(true);
            mute->setChecked(ch.muted);
            mute->setStyleSheet(QStringLiteral("QPushButton:checked{background:#EF4444;color:white;}"));
            connect(mute, &QPushButton::clicked, this, [this, id = ch.id] {
                auto state = m_engine->audio()->channelState(id);
                state.muted = !state.muted;
                m_engine->audio()->setChannelState(id, state);
            });
            auto* solo = new QPushButton(QStringLiteral("S"), strip);
            solo->setFixedHeight(14);
            solo->setCheckable(true);
            solo->setChecked(ch.solo);
            solo->setStyleSheet(QStringLiteral("QPushButton:checked{background:#F59E0B;color:black;}"));
            connect(solo, &QPushButton::clicked, this, [this, id = ch.id] {
                auto state = m_engine->audio()->channelState(id);
                state.solo = !state.solo;
                m_engine->audio()->setChannelState(id, state);
            });
            btnRow->addWidget(mute);
            btnRow->addWidget(solo);
            col->addLayout(btnRow);
            m_row->addWidget(strip);
        }
        m_row->addStretch();
    } else {
        for (int i = 0; i < channels.size(); ++i) {
            auto* strip = m_row->itemAt(i)->widget();
            if (!strip) continue;
            if (auto* meter = strip->findChild<QProgressBar*>()) {
                const int peak = int(channels[i].peakL * 100);
                meter->setValue(peak);
                QString color = QStringLiteral("#10B981");
                if (peak > 90) color = QStringLiteral("#EF4444");
                else if (peak > 75) color = QStringLiteral("#F59E0B");
                meter->setStyleSheet(QStringLiteral("QProgressBar::chunk{background:%1;}").arg(color));
            }
        }
    }
}

} // namespace railshot
