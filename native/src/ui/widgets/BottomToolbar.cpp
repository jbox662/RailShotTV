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
            QRadialGradient g(rect().center(), qMax(width(), height()) * 0.75);
            g.setColorAt(0, QColor(255, 90, 44, 130));
            g.setColorAt(0.55, QColor(255, 90, 44, 55));
            g.setColorAt(1, QColor(255, 90, 44, 0));
            p.setPen(Qt::NoPen);
            p.setBrush(g);
            p.drawEllipse(rect().adjusted(-10, -6, 10, 6));
        }
        QPushButton::paintEvent(e);
    }
};
} // namespace

BottomToolbar::BottomToolbar(EngineController* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setFixedHeight(50);
    setStyleSheet(QStringLiteral(
        "background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #1A1D22, stop:1 #0C0E12);"
        "border-top: 2px solid #4A4D55;"));
    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(10, 7, 10, 7);
    row->setSpacing(6);

    auto* add = new QPushButton(QStringLiteral("Add Input  ▾"), this);
    add->setObjectName(QStringLiteral("toolbarChromeBtn"));
    add->setCursor(Qt::PointingHandCursor);
    add->setMinimumHeight(30);
    connect(add, &QPushButton::clicked, this, &BottomToolbar::addInputRequested);
    row->addWidget(add);

    m_recordBtn = new QPushButton(QStringLiteral("+ Record"), this);
    m_recordBtn->setObjectName(QStringLiteral("recordButton"));
    m_recordBtn->setCursor(Qt::PointingHandCursor);
    m_recordBtn->setMinimumHeight(30);
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
    external->setMinimumHeight(30);
    row->addWidget(external);

    m_streamBtn = new StreamGlowButton(QStringLiteral("● Stream"), this);
    m_streamBtn->setObjectName(QStringLiteral("streamButton"));
    m_streamBtn->setCursor(Qt::PointingHandCursor);
    m_streamBtn->setMinimumWidth(120);
    m_streamBtn->setMinimumHeight(30);
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
    multi->setMinimumHeight(30);
    connect(multi, &QPushButton::clicked, this, &BottomToolbar::multiCorderRequested);
    row->addWidget(multi);
    m_multiBtn = multi;

    auto* playlist = new QPushButton(QStringLiteral("PlayList"), this);
    playlist->setObjectName(QStringLiteral("toolbarChromeBtn"));
    playlist->setCursor(Qt::PointingHandCursor);
    playlist->setMinimumHeight(30);
    connect(playlist, &QPushButton::clicked, this, &BottomToolbar::playListRequested);
    row->addWidget(playlist);
    m_playlistBtn = playlist;

    auto* overlay = new QPushButton(QStringLiteral("Overlay  ▾"), this);
    overlay->setObjectName(QStringLiteral("toolbarChromeBtn"));
    overlay->setCursor(Qt::PointingHandCursor);
    overlay->setMinimumHeight(30);
    connect(overlay, &QPushButton::clicked, this, [this, overlay] {
        emit overlayMenuRequested(overlay->mapToGlobal(QPoint(0, overlay->height())));
    });
    row->addWidget(overlay);
    m_overlayBtn = overlay;

    auto* replayBtn = new QPushButton(QStringLiteral("Save Replay"), this);
    replayBtn->setObjectName(QStringLiteral("toolbarChromeBtn"));
    replayBtn->setMinimumHeight(30);
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
    vcamBtn->setMinimumHeight(30);
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
        if (s.recording)
            m_recordBtn->setText(QStringLiteral("■ Stop Rec %1s").arg(s.recordUptimeSec));
        else
            m_recordBtn->setText(QStringLiteral("+ Record"));

        m_statusPill->setText(
            QStringLiteral("<span style='color:#22C55E;font-weight:800;'>1080p60</span>"
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
}

void BottomToolbar::setBasicMode(bool on)
{
    if (m_multiBtn) m_multiBtn->setVisible(!on);
    if (m_playlistBtn) m_playlistBtn->setVisible(!on);
}

} // namespace railshot
