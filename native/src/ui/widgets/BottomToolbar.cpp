#include "ui/widgets/BottomToolbar.h"
#include "core/EngineController.h"
#include "ui/Motion.h"
#include <QHBoxLayout>
#include <QMessageBox>
#include <QStyle>
#include <QPoint>

namespace railshot {

BottomToolbar::BottomToolbar(EngineController* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setFixedHeight(44);
    setStyleSheet(QStringLiteral(
        "background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #141619, stop:1 #0F1114);"
        "border-top: 1px solid #2A2D35;"));
    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(8, 4, 8, 4);
    row->setSpacing(6);

    auto* add = new QPushButton(QStringLiteral("Add Input  ▾"), this);
    add->setCursor(Qt::PointingHandCursor);
    connect(add, &QPushButton::clicked, this, &BottomToolbar::addInputRequested);
    row->addWidget(add);

    m_recordBtn = new QPushButton(QStringLiteral("+ Record"), this);
    m_recordBtn->setObjectName(QStringLiteral("recordButton"));
    m_recordBtn->setCursor(Qt::PointingHandCursor);
    connect(m_recordBtn, &QPushButton::clicked, this, [this] {
        if (m_engine->telemetrySnapshot().recording) {
            m_engine->stopRecording();
            m_recordBtn->setText(QStringLiteral("+ Record"));
            m_recordBtn->setProperty("recording", false);
        } else {
            QString err;
            if (!m_engine->startRecording(&err))
                QMessageBox::warning(this, QStringLiteral("Record"), err);
            else {
                m_recordBtn->setText(QStringLiteral("■ Stop Rec"));
                m_recordBtn->setProperty("recording", true);
            }
        }
        m_recordBtn->style()->unpolish(m_recordBtn);
        m_recordBtn->style()->polish(m_recordBtn);
    });
    row->addWidget(m_recordBtn);

    auto* external = new QPushButton(QStringLiteral("External"), this);
    external->setEnabled(false);
    row->addWidget(external);

    m_streamBtn = new QPushButton(QStringLiteral("● Stream"), this);
    m_streamBtn->setObjectName(QStringLiteral("streamButton"));
    m_streamBtn->setCursor(Qt::PointingHandCursor);
    m_streamBtn->setMinimumWidth(110);
    connect(m_streamBtn, &QPushButton::clicked, this, [this] {
        if (m_engine->telemetrySnapshot().streaming) {
            m_engine->stopStreaming();
            m_streamBtn->setText(QStringLiteral("● Stream"));
            m_streamBtn->setObjectName(QStringLiteral("streamButton"));
            m_streamBtn->style()->unpolish(m_streamBtn);
            m_streamBtn->style()->polish(m_streamBtn);
        } else {
            emit goLiveRequested();
        }
    });
    row->addWidget(m_streamBtn);

    auto* multi = new QPushButton(QStringLiteral("MultiCorder"), this);
    multi->setCursor(Qt::PointingHandCursor);
    connect(multi, &QPushButton::clicked, this, &BottomToolbar::multiCorderRequested);
    row->addWidget(multi);
    m_multiBtn = multi;

    auto* playlist = new QPushButton(QStringLiteral("PlayList"), this);
    playlist->setCursor(Qt::PointingHandCursor);
    connect(playlist, &QPushButton::clicked, this, &BottomToolbar::playListRequested);
    row->addWidget(playlist);
    m_playlistBtn = playlist;

    auto* overlay = new QPushButton(QStringLiteral("Overlay  ▾"), this);
    overlay->setCursor(Qt::PointingHandCursor);
    connect(overlay, &QPushButton::clicked, this, [this, overlay] {
        emit overlayMenuRequested(overlay->mapToGlobal(QPoint(0, overlay->height())));
    });
    row->addWidget(overlay);
    m_overlayBtn = overlay;

    auto* replayBtn = new QPushButton(QStringLiteral("Save Replay"), this);
    connect(replayBtn, &QPushButton::clicked, this, [this] {
        QString err;
        if (!m_engine->saveReplay(&err))
            QMessageBox::warning(this, QStringLiteral("Replay"), err);
        else
            QMessageBox::information(this, QStringLiteral("Replay"), QStringLiteral("Replay MKV saved."));
    });
    row->addWidget(replayBtn);

    auto* vcamBtn = new QPushButton(QStringLiteral("Virtual Cam"), this);
    vcamBtn->setCheckable(true);
    connect(vcamBtn, &QPushButton::toggled, this, [this, vcamBtn](bool on) {
        QString err;
        if (!m_engine->setVirtualCameraEnabled(on, &err)) {
            QMessageBox::warning(this, QStringLiteral("Virtual Camera"), err);
            vcamBtn->setChecked(false);
        }
    });
    row->addWidget(vcamBtn);

    row->addStretch();

    m_mixerBtn = new QPushButton(QStringLiteral("Audio Mixer"), this);
    m_mixerBtn->setObjectName(QStringLiteral("mixerToggle"));
    m_mixerBtn->setCheckable(true);
    m_mixerBtn->setCursor(Qt::PointingHandCursor);
    connect(m_mixerBtn, &QPushButton::clicked, this, &BottomToolbar::mixerToggleRequested);
    row->addWidget(m_mixerBtn);

    m_statusPill = new QLabel(QStringLiteral("1080p60  ·  — FPS  ·  GPU —  ·  CPU —"), this);
    m_statusPill->setObjectName(QStringLiteral("statusPill"));
    row->addWidget(m_statusPill);

    connect(m_engine, &EngineController::telemetryUpdated, this, [this](const TelemetrySnapshot& s) {
        if (s.streaming) {
            m_streamBtn->setText(QStringLiteral("■ End Stream"));
            m_streamBtn->setObjectName(QStringLiteral("endStreamButton"));
            m_streamBtn->style()->unpolish(m_streamBtn);
            m_streamBtn->style()->polish(m_streamBtn);
        } else {
            m_streamBtn->setText(QStringLiteral("● Stream"));
            m_streamBtn->setObjectName(QStringLiteral("streamButton"));
            m_streamBtn->style()->unpolish(m_streamBtn);
            m_streamBtn->style()->polish(m_streamBtn);
        }
        if (s.recording)
            m_recordBtn->setText(QStringLiteral("■ Stop Rec %1s").arg(s.recordUptimeSec));
        else
            m_recordBtn->setText(QStringLiteral("+ Record"));

        m_statusPill->setText(QStringLiteral("1080p60  ·  %1 FPS  ·  ENC %2  ·  GPU %3%  ·  CPU %4%")
                                  .arg(int(s.fpsRender))
                                  .arg(int(s.fpsEncode))
                                  .arg(int(s.gpuPercent))
                                  .arg(int(s.cpuPercent)));
    });

    motion::installPressScale(m_streamBtn);
    motion::installPressScale(m_recordBtn);
}

void BottomToolbar::setMixerOpen(bool open)
{
    m_mixerOpen = open;
    if (m_mixerBtn) {
        m_mixerBtn->setChecked(open);
        m_mixerBtn->setProperty("open", open);
        m_mixerBtn->style()->unpolish(m_mixerBtn);
        m_mixerBtn->style()->polish(m_mixerBtn);
    }
}

void BottomToolbar::setBasicMode(bool on)
{
    if (m_multiBtn) m_multiBtn->setVisible(!on);
    if (m_playlistBtn) m_playlistBtn->setVisible(!on);
    // Overlay stays available but catalogue is simplified via Dashboard
}

} // namespace railshot
