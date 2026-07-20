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
    setMinimumWidth(280);
    setMaximumWidth(360);
    setMinimumHeight(160);
    setStyleSheet(QStringLiteral(
        "QWidget#audioMixer {"
        "  background:#0D0F12;"
        "  border-top: 2px solid #A855F7;"
        "  border-left: 1px solid #1A1D24;"
        "}"));

    auto* outer = new QHBoxLayout(this);
    outer->setContentsMargins(4, 4, 4, 4);
    outer->setSpacing(4);

    auto* outputsLab = new QLabel(QStringLiteral("O\nU\nT\nP\nU\nT\nS"), this);
    outputsLab->setAlignment(Qt::AlignCenter);
    outputsLab->setFixedWidth(16);
    outputsLab->setStyleSheet(QStringLiteral(
        "color:#3A6AFF; font-size:8px; font-weight:700; letter-spacing:1px;"
        "background:rgba(26,58,255,0.08); border-radius:2px;"));
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
    m_row->setSpacing(4);
    scroll->setWidget(host);
    outer->addWidget(scroll, 1);

    auto* inputsLab = new QLabel(QStringLiteral("I\nN\nP\nU\nT\nS"), this);
    inputsLab->setAlignment(Qt::AlignCenter);
    inputsLab->setFixedWidth(16);
    inputsLab->setStyleSheet(QStringLiteral(
        "color:#3A6AFF; font-size:8px; font-weight:700; letter-spacing:1px;"
        "background:rgba(26,58,255,0.08); border-radius:2px;"));
    outer->addWidget(inputsLab);

    auto* side = new QVBoxLayout();
    side->setSpacing(2);
    auto* title = new QLabel(QStringLiteral("MIX"), this);
    title->setObjectName(QStringLiteral("panelTitleViolet"));
    title->setAlignment(Qt::AlignCenter);
    m_monitorBtn = new QPushButton(QStringLiteral("MON"), this);
    m_monitorBtn->setCheckable(true);
    m_monitorBtn->setChecked(true);
    m_monitorBtn->setFixedHeight(18);
    m_monitorBtn->setToolTip(QStringLiteral("Headphone / speaker monitor"));
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
            strip->setFixedWidth(master ? 68 : 58);
            auto* col = new QVBoxLayout(strip);
            col->setContentsMargins(2, 2, 2, 2);
            col->setSpacing(2);

            auto* badge = new QLabel(master ? QStringLiteral("MASTER") : ch.name.left(8), strip);
            badge->setAlignment(Qt::AlignCenter);
            if (master) {
                badge->setStyleSheet(QStringLiteral(
                    "font-size:7px; font-weight:800; color:#04140A;"
                    "background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #22C55E,stop:1 #16A34A);"
                    "border-radius:3px; padding:2px 4px;"));
            } else {
                badge->setStyleSheet(QStringLiteral(
                    "font-size:7px; font-weight:800; color:%1; background:transparent;")
                                         .arg(ch.muted ? QStringLiteral("#EF4444")
                                                       : QStringLiteral("#808898")));
            }
            col->addWidget(badge);

            auto* meterRow = new QHBoxLayout();
            meterRow->setSpacing(2);
            meterRow->setContentsMargins(0, 0, 0, 0);
            auto makeBar = [&](float peak) {
                auto* meter = new QProgressBar(strip);
                meter->setOrientation(Qt::Vertical);
                meter->setRange(0, 100);
                meter->setValue(int(peak * 100));
                meter->setTextVisible(false);
                meter->setFixedSize(8, 80);
                meter->setStyleSheet(QStringLiteral(
                    "QProgressBar{background:#0A0E1A;border:1px solid rgba(255,255,255,0.06);border-radius:1px;}"
                    "QProgressBar::chunk{background:qlineargradient(x1:0,y1:1,x2:0,y2:0,"
                    "stop:0 %1, stop:0.4 #84CC16, stop:0.7 #FBBF24, stop:1 #EF4444);}").arg(ch.id.contains(QLatin1String("mic"), Qt::CaseInsensitive)
                        ? QStringLiteral("#22C55E") : QStringLiteral("#4F9EFF")));
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
            db->setStyleSheet(QStringLiteral("font-size:8px; color:#606878;"));
            col->addWidget(db);

            auto* fader = new QSlider(Qt::Vertical, strip);
            fader->setRange(0, 100);
            fader->setValue(int(ch.volume * 100));
            fader->setFixedHeight(70);
            connect(fader, &QSlider::valueChanged, this, [this, id = ch.id](int v) {
                auto state = m_engine->audio()->channelState(id);
                state.volume = v / 100.f;
                m_engine->audio()->setChannelState(id, state);
            });
            col->addWidget(fader, 0, Qt::AlignHCenter);

            auto* btnRow = new QHBoxLayout();
            btnRow->setSpacing(2);
            auto* mute = new QPushButton(ch.muted ? QStringLiteral("M") : QStringLiteral("m"), strip);
            mute->setFixedHeight(16);
            mute->setCheckable(true);
            mute->setChecked(ch.muted);
            mute->setStyleSheet(QStringLiteral("QPushButton:checked{background:#EF4444;color:white;}"));
            connect(mute, &QPushButton::clicked, this, [this, id = ch.id] {
                auto state = m_engine->audio()->channelState(id);
                state.muted = !state.muted;
                m_engine->audio()->setChannelState(id, state);
            });
            auto* solo = new QPushButton(QStringLiteral("S"), strip);
            solo->setFixedHeight(16);
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
            const auto& ch = channels[i];
            if (auto* barL = strip->findChild<QProgressBar*>(ch.id + QStringLiteral("_L")))
                barL->setValue(int(ch.peakL * 100));
            if (auto* barR = strip->findChild<QProgressBar*>(ch.id + QStringLiteral("_R")))
                barR->setValue(int(ch.peakR * 100));
        }
    }
}

} // namespace railshot
