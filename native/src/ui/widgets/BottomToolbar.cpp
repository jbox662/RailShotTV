#include "ui/widgets/BottomToolbar.h"
#include "core/EngineController.h"
#include <QHBoxLayout>
#include <QMessageBox>
#include <QStyle>
#include <QLabel>

namespace railshot {

BottomToolbar::BottomToolbar(EngineController* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setFixedHeight(44);
    setStyleSheet(QStringLiteral(
        "background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #141619, stop:1 #0F1114);"
        "border-top: 1px solid #2A2D35;"));
    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(10, 6, 10, 6);
    row->setSpacing(6);

    auto* add = new QPushButton(QStringLiteral("Add Input"), this);
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
        } else {
            QString err;
            if (!m_engine->startRecording(&err))
                QMessageBox::warning(this, QStringLiteral("Record"), err);
            else
                m_recordBtn->setText(QStringLiteral("■ Stop Rec"));
        }
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

    auto* mixerHint = new QPushButton(QStringLiteral("Audio Mixer"), this);
    mixerHint->setObjectName(QStringLiteral("mixerToggle"));
    mixerHint->setEnabled(false);
    row->addWidget(mixerHint);

    connect(m_engine, &EngineController::telemetryUpdated, this, [this](const TelemetrySnapshot& s) {
        if (s.streaming) {
            m_streamBtn->setText(QStringLiteral("■ End Stream"));
            m_streamBtn->setObjectName(QStringLiteral("endStreamButton"));
            m_streamBtn->style()->unpolish(m_streamBtn);
            m_streamBtn->style()->polish(m_streamBtn);
        }
        if (s.recording)
            m_recordBtn->setText(QStringLiteral("■ Stop Rec %1s").arg(s.recordUptimeSec));
    });
}

} // namespace railshot
