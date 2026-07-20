#include "ui/widgets/BottomToolbar.h"
#include "core/EngineController.h"
#include <QHBoxLayout>
#include <QMessageBox>
#include <QStyle>

namespace railshot {

BottomToolbar::BottomToolbar(EngineController* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setFixedHeight(40);
    setStyleSheet(QStringLiteral("background:#141619; border-top:1px solid #2A2D35;"));
    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(8, 4, 8, 4);

    auto* add = new QPushButton(QStringLiteral("Add Input"), this);
    connect(add, &QPushButton::clicked, this, &BottomToolbar::addInputRequested);
    row->addWidget(add);

    m_recordBtn = new QPushButton(QStringLiteral("● Record"), this);
    connect(m_recordBtn, &QPushButton::clicked, this, [this] {
        if (m_engine->telemetrySnapshot().recording) {
            m_engine->stopRecording();
            m_recordBtn->setText(QStringLiteral("● Record"));
        } else {
            QString err;
            if (!m_engine->startRecording(&err))
                QMessageBox::warning(this, QStringLiteral("Record"), err);
            else
                m_recordBtn->setText(QStringLiteral("■ Stop Rec"));
        }
    });
    row->addWidget(m_recordBtn);

    m_streamBtn = new QPushButton(QStringLiteral("● Stream"), this);
    m_streamBtn->setObjectName(QStringLiteral("streamButton"));
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

    connect(m_engine, &EngineController::telemetryUpdated, this, [this](const TelemetrySnapshot& s) {
        if (s.streaming) {
            m_streamBtn->setText(QStringLiteral("■ End Stream"));
            m_streamBtn->setObjectName(QStringLiteral("endStreamButton"));
        }
        if (s.recording)
            m_recordBtn->setText(QStringLiteral("■ Stop Rec %1s").arg(s.recordUptimeSec));
    });
}

} // namespace railshot
