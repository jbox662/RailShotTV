#include "ui/widgets/BottomToolbar.h"
#include "core/EngineController.h"
#include "ui/Motion.h"
#include <QHBoxLayout>
#include <QMessageBox>
#include <QStyle>
#include <QPoint>
#include <QPainter>
#include <QPaintEvent>
#include <QRadialGradient>

namespace railshot {

namespace {
/** Stream CTA with painted brand glow (QSS cannot do box-shadow). */
class StreamGlowButton : public QPushButton {
public:
    using QPushButton::QPushButton;
protected:
    void paintEvent(QPaintEvent* e) override
    {
        if (objectName() == QLatin1String("streamButton") && isEnabled()) {
            QPainter p(this);
            p.setRenderHint(QPainter::Antialiasing);
            QRadialGradient g(rect().center(), qMax(width(), height()) * 0.85);
            g.setColorAt(0, QColor(255, 90, 44, 150));
            g.setColorAt(0.5, QColor(255, 90, 44, 70));
            g.setColorAt(1, QColor(255, 90, 44, 0));
            p.setPen(Qt::NoPen);
            p.setBrush(g);
            p.drawEllipse(rect().adjusted(-6, -4, 6, 4));
        }
        QPushButton::paintEvent(e);
    }
};
} // namespace

BottomToolbar::BottomToolbar(EngineController* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setObjectName(QStringLiteral("bottomToolbar"));
    setFixedHeight(36);
    // MUST scope to #bottomToolbar — unscoped rules flatten every child button to the bar fill.
    setStyleSheet(QStringLiteral(
        "QWidget#bottomToolbar {"
        "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #171A1F, stop:1 #0C0E12);"
        "  border-top: 1px solid #3A3D45;"
        "}"
        "QWidget#bottomToolbar QPushButton#toolbarChromeBtn {"
        "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #32363F, stop:1 #1A1E26);"
        "  border: 1px solid #5A5E68;"
        "  border-radius: 3px;"
        "  color: #E0E2E8;"
        "  font-weight: 700;"
        "  font-size: 10px;"
        "  padding: 2px 9px;"
        "  min-height: 22px;"
        "  max-height: 24px;"
        "}"
        "QWidget#bottomToolbar QPushButton#toolbarChromeBtn:hover {"
        "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #3A404C, stop:1 #242830);"
        "  border: 1px solid #4F9EFF;"
        "  color: #FFFFFF;"
        "}"
        "QWidget#bottomToolbar QPushButton#toolbarChromeBtn:pressed {"
        "  background: #12151A;"
        "  border-color: #3A3D45;"
        "}"
        "QWidget#bottomToolbar QPushButton#toolbarChromeBtn:disabled {"
        "  color: #505860;"
        "  border-color: #2A2D35;"
        "  background: #16181E;"
        "}"
        "QWidget#bottomToolbar QPushButton#toolbarChromeBtn:checked {"
        "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #1A2A3A, stop:1 #101820);"
        "  border: 1px solid #4F9EFF;"
        "  color: #7AB8FF;"
        "}"
        "QWidget#bottomToolbar QPushButton#recordButton {"
        "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #32363F, stop:1 #1A1E26);"
        "  border: 1px solid #5A5E68;"
        "  border-radius: 3px;"
        "  color: #E0E2E8;"
        "  font-weight: 800;"
        "  font-size: 10px;"
        "  padding: 2px 9px;"
        "  min-height: 22px;"
        "  max-height: 24px;"
        "}"
        "QWidget#bottomToolbar QPushButton#recordButton:hover {"
        "  border: 1px solid #EF4444;"
        "  color: #FECACA;"
        "}"
        "QWidget#bottomToolbar QPushButton#recordButton[recording=\"true\"] {"
        "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #9F1D1D, stop:1 #5A1010);"
        "  border: 1px solid #EF4444;"
        "  color: #FECACA;"
        "}"
        "QWidget#bottomToolbar QPushButton#streamButton {"
        "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #FF8C42, stop:1 #CC3A18);"
        "  border: 1px solid #FFB08A;"
        "  border-radius: 3px;"
        "  color: #FFFFFF;"
        "  font-weight: 900;"
        "  font-size: 11px;"
        "  padding: 2px 12px;"
        "  min-height: 22px;"
        "  max-height: 24px;"
        "}"
        "QWidget#bottomToolbar QPushButton#streamButton:hover {"
        "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #FFA868, stop:1 #FF5A2C);"
        "  border: 1px solid #FFD0B0;"
        "}"
        "QWidget#bottomToolbar QPushButton#endStreamButton {"
        "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #9F1D1D, stop:1 #5A1010);"
        "  border: 1px solid #EF4444;"
        "  border-radius: 3px;"
        "  color: #FECACA;"
        "  font-weight: 900;"
        "  font-size: 11px;"
        "  padding: 2px 12px;"
        "  min-height: 22px;"
        "  max-height: 24px;"
        "}"
        "QWidget#bottomToolbar QPushButton#addInputBtn {"
        "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #3A6AFF, stop:1 #1A3AFF);"
        "  border: 1px solid #5A8AFF;"
        "  border-radius: 3px;"
        "  color: #FFFFFF;"
        "  font-weight: 800;"
        "  font-size: 10px;"
        "  padding: 2px 10px;"
        "  min-height: 22px;"
        "  max-height: 24px;"
        "}"
        "QWidget#bottomToolbar QPushButton#addInputBtn:hover {"
        "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #5A8AFF, stop:1 #3A6AFF);"
        "  border: 1px solid #8AB4FF;"
        "}"
        "QWidget#bottomToolbar QLabel#statusPill {"
        "  font-family: 'JetBrains Mono','Consolas',monospace;"
        "  font-size: 8px;"
        "  color: #8892A4;"
        "  background: #0A0C0F;"
        "  border: 1px solid #3A3D45;"
        "  border-radius: 2px;"
        "  padding: 2px 8px;"
        "}"));

    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(6, 4, 6, 4);
    row->setSpacing(4);

    auto* add = new QPushButton(QStringLiteral("Add Input  ▾"), this);
    add->setObjectName(QStringLiteral("addInputBtn"));
    add->setCursor(Qt::PointingHandCursor);
    add->setFixedHeight(22);
    connect(add, &QPushButton::clicked, this, &BottomToolbar::addInputRequested);
    row->addWidget(add);

    m_recordBtn = new QPushButton(QStringLiteral("+ Record"), this);
    m_recordBtn->setObjectName(QStringLiteral("recordButton"));
    m_recordBtn->setCursor(Qt::PointingHandCursor);
    m_recordBtn->setFixedHeight(22);
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
    external->setObjectName(QStringLiteral("toolbarChromeBtn"));
    external->setEnabled(false);
    external->setFixedHeight(22);
    row->addWidget(external);

    m_streamBtn = new StreamGlowButton(QStringLiteral("● Stream"), this);
    m_streamBtn->setObjectName(QStringLiteral("streamButton"));
    m_streamBtn->setCursor(Qt::PointingHandCursor);
    m_streamBtn->setMinimumWidth(96);
    m_streamBtn->setFixedHeight(22);
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
    multi->setObjectName(QStringLiteral("toolbarChromeBtn"));
    multi->setCursor(Qt::PointingHandCursor);
    multi->setFixedHeight(22);
    connect(multi, &QPushButton::clicked, this, &BottomToolbar::multiCorderRequested);
    row->addWidget(multi);
    m_multiBtn = multi;

    auto* playlist = new QPushButton(QStringLiteral("PlayList"), this);
    playlist->setObjectName(QStringLiteral("toolbarChromeBtn"));
    playlist->setCursor(Qt::PointingHandCursor);
    playlist->setFixedHeight(22);
    connect(playlist, &QPushButton::clicked, this, &BottomToolbar::playListRequested);
    row->addWidget(playlist);
    m_playlistBtn = playlist;

    auto* overlay = new QPushButton(QStringLiteral("Overlay  ▾"), this);
    overlay->setObjectName(QStringLiteral("toolbarChromeBtn"));
    overlay->setCursor(Qt::PointingHandCursor);
    overlay->setFixedHeight(22);
    connect(overlay, &QPushButton::clicked, this, [this, overlay] {
        emit overlayMenuRequested(overlay->mapToGlobal(QPoint(0, overlay->height())));
    });
    row->addWidget(overlay);
    m_overlayBtn = overlay;

    auto* replayBtn = new QPushButton(QStringLiteral("Save Replay"), this);
    replayBtn->setObjectName(QStringLiteral("toolbarChromeBtn"));
    replayBtn->setFixedHeight(22);
    connect(replayBtn, &QPushButton::clicked, this, [this] {
        QString err;
        if (!m_engine->saveReplay(&err))
            QMessageBox::warning(this, QStringLiteral("Replay"), err);
        else
            QMessageBox::information(this, QStringLiteral("Replay"), QStringLiteral("Replay MKV saved."));
    });
    row->addWidget(replayBtn);

    auto* vcamBtn = new QPushButton(QStringLiteral("Virtual Cam"), this);
    vcamBtn->setObjectName(QStringLiteral("toolbarChromeBtn"));
    vcamBtn->setCheckable(true);
    vcamBtn->setFixedHeight(22);
    connect(vcamBtn, &QPushButton::toggled, this, [this, vcamBtn](bool on) {
        QString err;
        if (!m_engine->setVirtualCameraEnabled(on, &err)) {
            QMessageBox::warning(this, QStringLiteral("Virtual Camera"), err);
            vcamBtn->setChecked(false);
        }
    });
    row->addWidget(vcamBtn);

    row->addStretch();

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
        if (s.recording) {
            m_recordBtn->setText(QStringLiteral("■ Stop Rec %1s").arg(s.recordUptimeSec));
            m_recordBtn->setProperty("recording", true);
        } else {
            m_recordBtn->setText(QStringLiteral("+ Record"));
            m_recordBtn->setProperty("recording", false);
        }
        m_recordBtn->style()->unpolish(m_recordBtn);
        m_recordBtn->style()->polish(m_recordBtn);

        m_statusPill->setText(
            QStringLiteral("<span style='color:#22C55E;font-weight:900;'>1080p60</span>"
                           "  ·  <span style='color:#E0E2E8;'>%1 FPS</span>"
                           "  ·  ENC %2  ·  GPU %3%  ·  CPU %4%")
                .arg(int(s.fpsRender))
                .arg(int(s.fpsEncode))
                .arg(int(s.gpuPercent))
                .arg(int(s.cpuPercent)));
        m_statusPill->setTextFormat(Qt::RichText);
        m_streamBtn->update();
    });

    motion::installPressScale(m_streamBtn);
    motion::installPressScale(m_recordBtn);
    motion::installPressScale(add);
}

void BottomToolbar::setBasicMode(bool on)
{
    if (m_multiBtn) m_multiBtn->setVisible(!on);
    if (m_playlistBtn) m_playlistBtn->setVisible(!on);
}

} // namespace railshot
