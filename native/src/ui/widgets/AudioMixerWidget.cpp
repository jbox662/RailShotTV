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
#include <QScrollArea>
#include <QFrame>

namespace railshot {

AudioMixerWidget::AudioMixerWidget(EngineController* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setObjectName(QStringLiteral("audioMixer"));
    setMinimumWidth(200);
    setMinimumHeight(140);
    setStyleSheet(QStringLiteral(
        "QWidget#audioMixer{background:#1A1D21;border:none;}"));

    auto* outer = new QHBoxLayout(this);
    outer->setContentsMargins(4, 4, 4, 4);
    outer->setSpacing(4);

    auto* outputsLab = new QLabel(QStringLiteral("O\nU\nT"), this);
    outputsLab->setAlignment(Qt::AlignCenter);
    outputsLab->setFixedWidth(14);
    outputsLab->setStyleSheet(QStringLiteral(
        "color:#6B7280; font-size:8px; font-weight:600; letter-spacing:0px;"
        "background:transparent;"));
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

    auto* inputsLab = new QLabel(QStringLiteral("I\nN"), this);
    inputsLab->setAlignment(Qt::AlignCenter);
    inputsLab->setFixedWidth(14);
    inputsLab->setStyleSheet(QStringLiteral(
        "color:#6B7280; font-size:8px; font-weight:600; letter-spacing:0px;"
        "background:transparent;"));
    outer->addWidget(inputsLab);

    auto* side = new QVBoxLayout();
    side->setSpacing(2);
    auto* title = new QLabel(QStringLiteral("MIX"), this);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet(QStringLiteral(
        "color:#8B919C; font-size:9px; font-weight:600; background:transparent;"));
    m_monitorBtn = new QPushButton(QStringLiteral("MON"), this);
    m_monitorBtn->setCheckable(true);
    m_monitorBtn->setChecked(true);
    m_monitorBtn->setFixedHeight(18);
    m_monitorBtn->setToolTip(QStringLiteral("Headphone / speaker monitor"));
    m_monitorBtn->setStyleSheet(QStringLiteral(
        "QPushButton{background:#2A2E36;border:1px solid #3A3F48;color:#A0A8B8;"
        "font-size:9px;border-radius:2px;}"
        "QPushButton:checked{background:#343944;color:#E5E7EB;border-color:#4A5058;}"
        "QPushButton:hover{border-color:#5A6068;}"));
    connect(m_monitorBtn, &QPushButton::toggled, this, [this](bool on) {
        if (m_engine && m_engine->audio())
            m_engine->audio()->setMonitorEnabled(on);
    });
    side->addWidget(title);
    side->addWidget(m_monitorBtn);
    side->addStretch();
    outer->addLayout(side);

    auto* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &AudioMixerWidget::refreshMeters);
    timer->start(60);
    refreshMeters();
}

void AudioMixerWidget::refreshMeters()
{
    const auto channels = m_engine->audioChannels();
    if (m_row->count() != channels.size()) {
        while (QLayoutItem* child = m_row->takeAt(0)) {
            if (child->widget()) child->widget()->deleteLater();
            delete child;
        }
        for (int ci = 0; ci < channels.size(); ++ci) {
            const auto& ch = channels[ci];
            const bool master = ci == 0 || ch.name.contains(QStringLiteral("Master"), Qt::CaseInsensitive);
            auto* strip = new QWidget(this);
            strip->setFixedWidth(master ? 68 : 56);
            strip->setStyleSheet(QStringLiteral(
                "QWidget{background:#1A1D21; border:none;}"));
            auto* col = new QVBoxLayout(strip);
            col->setContentsMargins(2, 2, 2, 2);
            col->setSpacing(2);

            auto* badge = new QLabel(master ? QStringLiteral("Master") : ch.name.left(10), strip);
            badge->setAlignment(Qt::AlignCenter);
            badge->setStyleSheet(QStringLiteral(
                "font-size:9px; font-weight:600; color:%1; background:transparent;"
                "padding:1px 0px;")
                                     .arg(ch.muted ? QStringLiteral("#EF4444")
                                                   : (master ? QStringLiteral("#D0D4DC")
                                                             : QStringLiteral("#A0A8B8"))));
            col->addWidget(badge);

            auto* meterRow = new QHBoxLayout();
            meterRow->setSpacing(1);
            meterRow->setContentsMargins(0, 0, 0, 0);
            auto makeBar = [&](float peak) {
                auto* meter = new QProgressBar(strip);
                meter->setOrientation(Qt::Vertical);
                meter->setRange(0, 100);
                meter->setValue(int(peak * 100));
                meter->setTextVisible(false);
                meter->setFixedSize(7, 78);
                meter->setStyleSheet(QStringLiteral(
                    "QProgressBar{background:#0E1014;border:1px solid #2A2E36;border-radius:0px;}"
                    "QProgressBar::chunk{background:#4ADE80;}"));
                return meter;
            };
            auto* barL = makeBar(ch.peakL);
            barL->setObjectName(ch.id + QStringLiteral("_L"));
            auto* barR = makeBar(ch.peakR);
            barR->setObjectName(ch.id + QStringLiteral("_R"));
            meterRow->addStretch();
            meterRow->addWidget(barL);
            meterRow->addWidget(barR);
            meterRow->addStretch();
            col->addLayout(meterRow);

            auto* db = new QLabel(QStringLiteral("0.0"), strip);
            db->setAlignment(Qt::AlignCenter);
            db->setObjectName(QStringLiteral("mono"));
            db->setStyleSheet(QStringLiteral("font-size:8px; color:#6B7280; background:transparent;"));
            col->addWidget(db);

            auto* fader = new QSlider(Qt::Vertical, strip);
            fader->setRange(0, 100);
            fader->setValue(int(ch.volume * 100));
            fader->setFixedHeight(70);
            fader->setStyleSheet(QStringLiteral(
                "QSlider::groove:vertical{background:#2A2E36;width:4px;border-radius:2px;}"
                "QSlider::handle:vertical{background:#C8CCD4;height:10px;width:12px;"
                "margin:-2px -4px;border-radius:2px;border:1px solid #3A3F48;}"
                "QSlider::handle:vertical:hover{background:#E5E7EB;}"));
            connect(fader, &QSlider::valueChanged, this, [this, id = ch.id](int v) {
                auto state = m_engine->audio()->channelState(id);
                state.volume = v / 100.f;
                m_engine->audio()->setChannelState(id, state);
            });
            col->addWidget(fader, 0, Qt::AlignHCenter);

            auto* btnRow = new QHBoxLayout();
            btnRow->setSpacing(2);
            const QString chipStyle = QStringLiteral(
                "QPushButton{background:#2A2E36;border:1px solid #3A3F48;color:#A0A8B8;"
                "font-size:9px;border-radius:2px;min-width:18px;}"
                "QPushButton:hover{border-color:#4A5058;color:#E5E7EB;}");
            auto* mute = new QPushButton(QStringLiteral("M"), strip);
            mute->setFixedHeight(16);
            mute->setCheckable(true);
            mute->setChecked(ch.muted);
            mute->setStyleSheet(chipStyle + QStringLiteral(
                "QPushButton:checked{background:#7F1D1D;color:#FECACA;border-color:#991B1B;}"));
            connect(mute, &QPushButton::clicked, this, [this, id = ch.id] {
                auto state = m_engine->audio()->channelState(id);
                state.muted = !state.muted;
                m_engine->audio()->setChannelState(id, state);
            });
            auto* solo = new QPushButton(QStringLiteral("S"), strip);
            solo->setFixedHeight(16);
            solo->setCheckable(true);
            solo->setChecked(ch.solo);
            solo->setStyleSheet(chipStyle + QStringLiteral(
                "QPushButton:checked{background:#78350F;color:#FDE68A;border-color:#92400E;}"));
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
            const auto& ch = channels[i];
            if (auto* barL = strip->findChild<QProgressBar*>(ch.id + QStringLiteral("_L")))
                barL->setValue(int(ch.peakL * 100));
            if (auto* barR = strip->findChild<QProgressBar*>(ch.id + QStringLiteral("_R")))
                barR->setValue(int(ch.peakR * 100));
        }
    }
}

} // namespace railshot
