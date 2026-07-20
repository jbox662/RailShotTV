#include "ui/widgets/GoLiveDialog.h"
#include "core/EngineController.h"
#include "core/SecretStore.h"
#include "core/SettingsStore.h"
#include "encoding/EncoderFactory.h"
#include "audio/AudioGraph.h"
#include "ui/Motion.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QMessageBox>
#include <QTimer>
#include <QFrame>
#include <functional>

namespace railshot {

GoLiveDialog::GoLiveDialog(EngineController* engine, QWidget* parent)
    : QDialog(parent), m_engine(engine)
{
    setWindowTitle(QStringLiteral("Go Live"));
    setObjectName(QStringLiteral("goLiveDialog"));
    setFixedWidth(520);
    setStyleSheet(QStringLiteral(
        "QDialog#goLiveDialog{background:#12151C;border:2px solid #FF5A2C88;border-radius:10px;}"
        "QLabel{color:#E0E2E8;background:transparent;}"
        "QLineEdit{background:#0A0C10;border:1px solid #4A4D55;border-radius:4px;"
        "  color:#E0E2E8;padding:8px;font-size:12px;}"
        "QLineEdit:focus{border:2px solid #FF5A2C;}"));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* accent = new QFrame(this);
    accent->setFixedHeight(4);
    accent->setStyleSheet(QStringLiteral(
        "background:qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        "stop:0 #FF5A2C, stop:0.45 #FF8C42, stop:1 #A855F7); border:none;"));
    root->addWidget(accent);

    m_stack = new QStackedWidget(this);
    root->addWidget(m_stack);

    // ── Step 0: config ────────────────────────────────────────────────────
    auto* config = new QWidget(m_stack);
    auto* cfg = new QVBoxLayout(config);
    cfg->setContentsMargins(28, 24, 28, 24);
    cfg->setSpacing(12);

    auto* title = new QLabel(QStringLiteral("GO LIVE"), config);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet(QStringLiteral(
        "font-family:'Bebas Neue'; font-size:32px; letter-spacing:3px; color:#FF5A2C;"));
    cfg->addWidget(title);

    auto* platGrid = new QHBoxLayout();
    platGrid->setSpacing(8);
    struct Plat { const char* name; const char* id; const char* color; };
    const Plat plats[] = {
        {"YouTube", "youtube", "#FF0000"},
        {"Twitch", "twitch", "#9146FF"},
        {"Facebook", "facebook", "#1877F2"},
        {"Custom", "custom", "#FF5A2C"},
    };
    for (const auto& p : plats) {
        auto* b = new QPushButton(QString::fromLatin1(p.name), config);
        b->setFixedSize(72, 64);
        b->setCheckable(true);
        b->setCursor(Qt::PointingHandCursor);
        b->setProperty("platformId", QString::fromLatin1(p.id));
        b->setStyleSheet(QStringLiteral(
            "QPushButton{background:#0F1218;border:2px solid #2A3350;border-radius:8px;"
            "color:%1;font-weight:800;font-size:10px;}"
            "QPushButton:checked{border-color:%1;background:#1A1F2E;}").arg(QString::fromLatin1(p.color)));
        connect(b, &QPushButton::clicked, this, [this, b, id = QString::fromLatin1(p.id)] {
            m_platform = id;
            for (auto* x : m_platformBtns) x->setChecked(x == b);
            if (id == QLatin1String("youtube"))
                m_url->setText(QStringLiteral("rtmp://a.rtmp.youtube.com/live2"));
            else if (id == QLatin1String("twitch"))
                m_url->setText(QStringLiteral("rtmp://live.twitch.tv/app"));
            else if (id == QLatin1String("facebook"))
                m_url->setText(QStringLiteral("rtmps://live-api-s.facebook.com:443/rtmp"));
            else
                m_url->clear();
        });
        m_platformBtns.push_back(b);
        platGrid->addWidget(b);
    }
    m_platformBtns.first()->setChecked(true);
    cfg->addLayout(platGrid);

    m_title = new QLineEdit(config);
    m_title->setPlaceholderText(QStringLiteral("Stream Title"));
    cfg->addWidget(m_title);

    m_url = new QLineEdit(QStringLiteral("rtmp://a.rtmp.youtube.com/live2"), config);
    m_url->setPlaceholderText(QStringLiteral("RTMP URL"));
    cfg->addWidget(m_url);

    m_key = new QLineEdit(config);
    m_key->setEchoMode(QLineEdit::Password);
    m_key->setPlaceholderText(QStringLiteral("Stream Key"));
    m_key->setStyleSheet(QStringLiteral(
        "QLineEdit{font-family:'JetBrains Mono';}"
        "QLineEdit:focus{border-color:#FF5A2C;}"));
    cfg->addWidget(m_key);

    auto* chips = new QLabel(QStringLiteral("Ready  ·  Audio  ·  Scene  ·  Encoder  ·  Network"), config);
    chips->setAlignment(Qt::AlignCenter);
    chips->setStyleSheet(QStringLiteral("color:#606878; font-size:10px; letter-spacing:0.5px;"));
    cfg->addWidget(chips);

    auto* goBtn = new QPushButton(QStringLiteral("RUN CHECKS & GO LIVE"), config);
    goBtn->setFixedHeight(42);
    goBtn->setCursor(Qt::PointingHandCursor);
    goBtn->setStyleSheet(QStringLiteral(
        "QPushButton{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #FF5A2C,stop:1 #FF8C42);"
        "color:white;font-weight:800;font-size:13px;letter-spacing:1px;border:none;border-radius:6px;}"
        "QPushButton:hover{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #FF6B3D,stop:1 #FFA05A);}"));
    connect(goBtn, &QPushButton::clicked, this, &GoLiveDialog::runChecks);
    cfg->addWidget(goBtn);

    auto* cancelRow = new QHBoxLayout();
    auto* cancel = new QPushButton(QStringLiteral("Cancel"), config);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    cancelRow->addStretch();
    cancelRow->addWidget(cancel);
    cancelRow->addStretch();
    cfg->addLayout(cancelRow);
    m_stack->addWidget(config);

    // ── Step 1: checking ──────────────────────────────────────────────────
    auto* checking = new QWidget(m_stack);
    auto* chk = new QVBoxLayout(checking);
    chk->setContentsMargins(28, 40, 28, 40);
    auto* chkTitle = new QLabel(QStringLiteral("PRE-STREAM CHECKS"), checking);
    chkTitle->setAlignment(Qt::AlignCenter);
    chkTitle->setStyleSheet(QStringLiteral(
        "font-family:'Bebas Neue'; font-size:22px; letter-spacing:2px;"));
    m_checkStatus = new QLabel(QStringLiteral("Starting…"), checking);
    m_checkStatus->setAlignment(Qt::AlignCenter);
    m_checkStatus->setStyleSheet(QStringLiteral("font-size:14px; color:#A855F7; padding-top:24px;"));
    chk->addWidget(chkTitle);
    chk->addWidget(m_checkStatus);
    chk->addStretch();
    m_stack->addWidget(checking);

    // ── Step 2: countdown ─────────────────────────────────────────────────
    auto* countdown = new QWidget(m_stack);
    auto* cd = new QVBoxLayout(countdown);
    cd->setContentsMargins(28, 24, 28, 24);
    auto* going = new QLabel(QStringLiteral("GOING LIVE IN"), countdown);
    going->setAlignment(Qt::AlignCenter);
    going->setStyleSheet(QStringLiteral("color:#808898; font-size:12px; letter-spacing:2px; font-weight:700;"));
    m_countdown = new QLabel(QStringLiteral("3"), countdown);
    m_countdown->setAlignment(Qt::AlignCenter);
    m_countdown->setFixedSize(120, 120);
    m_countdown->setStyleSheet(QStringLiteral(
        "font-family:'Bebas Neue'; font-size:72px; color:#FF5A2C;"
        "border:3px solid #FF5A2C; border-radius:60px;"));
    cd->addStretch();
    cd->addWidget(going);
    cd->addWidget(m_countdown, 0, Qt::AlignHCenter);
    cd->addStretch();
    m_stack->addWidget(countdown);

    // ── Step 3: live ──────────────────────────────────────────────────────
    auto* live = new QWidget(m_stack);
    auto* lv = new QVBoxLayout(live);
    lv->setContentsMargins(28, 40, 28, 40);
    auto* liveLab = new QLabel(QStringLiteral("YOU ARE LIVE"), live);
    liveLab->setAlignment(Qt::AlignCenter);
    liveLab->setStyleSheet(QStringLiteral(
        "font-family:'Bebas Neue'; font-size:36px; color:#FF5A2C; letter-spacing:2px;"));
    auto* pulse = new QLabel(QStringLiteral("● ON AIR"), live);
    pulse->setObjectName(QStringLiteral("goLiveOnAir"));
    pulse->setAlignment(Qt::AlignCenter);
    pulse->setStyleSheet(QStringLiteral("color:#EF4444; font-weight:800; font-size:14px; padding-top:12px;"));
    auto* done = new QPushButton(QStringLiteral("Close"), live);
    connect(done, &QPushButton::clicked, this, &QDialog::accept);
    lv->addStretch();
    lv->addWidget(liveLab);
    lv->addWidget(pulse);
    lv->addStretch();
    lv->addWidget(done, 0, Qt::AlignHCenter);
    m_stack->addWidget(live);

    showStep(0);
    motion::playModalEnter(this);
}

void GoLiveDialog::setPrefill(const QString& title, const QString& platform)
{
    if (m_title && !title.isEmpty())
        m_title->setText(title);
    if (platform.isEmpty()) return;
    m_platform = platform.toLower();
    for (auto* b : m_platformBtns) {
        const bool match = b->property("platformId").toString() == m_platform;
        b->setChecked(match);
        if (match)
            emit b->clicked(true);
    }
}

void GoLiveDialog::showStep(int step)
{
    m_stack->setCurrentIndex(step);
    if (auto* onAir = findChild<QLabel*>(QStringLiteral("goLiveOnAir"))) {
        if (step == 3)
            motion::pulseOpacity(onAir, 1800);
        else
            motion::stopPulse(onAir);
    }
}

void GoLiveDialog::runChecks()
{
    showStep(1);
    m_checkIndex = 0;

    struct Check {
        QString label;
        QString color;
        std::function<QString()> probe; // empty = pass
    };

    QVector<Check> checks;
    checks.push_back({QStringLiteral("Stream key"), QStringLiteral("#FBBF24"), [this] {
        if (m_key->text().trimmed().isEmpty())
            return QStringLiteral("Enter a stream key before going live");
        return QString();
    }});
    checks.push_back({QStringLiteral("RTMP URL"), QStringLiteral("#22D3EE"), [this] {
        if (m_url->text().trimmed().isEmpty())
            return QStringLiteral("RTMP URL is empty");
        return QString();
    }});
    checks.push_back({QStringLiteral("Active scene"), QStringLiteral("#4F9EFF"), [this] {
        const auto p = m_engine->projectSnapshot();
        const auto* scene = p.findScene(p.programSceneId.isEmpty() ? p.activeSceneId : p.programSceneId);
        if (!scene)
            return QStringLiteral("No program scene");
        bool any = false;
        for (const auto& s : scene->sources) {
            if (s.visible) { any = true; break; }
        }
        if (!any)
            return QStringLiteral("Program scene has no visible sources");
        return QString();
    }});
    checks.push_back({QStringLiteral("Audio devices"), QStringLiteral("#A855F7"), [this] {
        if (!m_engine->audio())
            return QStringLiteral("Audio graph unavailable");
        return QString();
    }});
    checks.push_back({QStringLiteral("Video encoder"), QStringLiteral("#22C55E"), [this] {
        OutputProfile profile = m_engine->projectSnapshot().output;
        if (profile.width <= 0)
            profile = m_engine->settings()->outputProfile();
        QString name;
        auto enc = EncoderFactory::createVideo(profile, &name);
        if (!enc)
            return QStringLiteral("No video encoder available");
        QString err;
        if (!enc->open(profile, &err))
            return err.isEmpty() ? QStringLiteral("Encoder failed to open") : err;
        enc->close();
        return QString();
    }});

    auto* t = new QTimer(this);
    connect(t, &QTimer::timeout, this, [this, t, checks]() mutable {
        if (m_checkIndex >= checks.size()) {
            t->stop();
            t->deleteLater();
            startCountdown();
            return;
        }
        const auto& c = checks[m_checkIndex];
        const QString fail = c.probe();
        if (!fail.isEmpty()) {
            t->stop();
            t->deleteLater();
            m_checkStatus->setText(QStringLiteral("✗  %1 — %2").arg(c.label, fail));
            m_checkStatus->setStyleSheet(QStringLiteral("font-size:14px; color:#FF5A2C; padding-top:24px;"));
            QTimer::singleShot(1600, this, [this] { showStep(0); });
            return;
        }
        m_checkStatus->setText(QStringLiteral("✓  %1").arg(c.label));
        m_checkStatus->setStyleSheet(QStringLiteral("font-size:14px; color:%1; padding-top:24px;").arg(c.color));
        ++m_checkIndex;
    });
    t->start(280);
}

void GoLiveDialog::startCountdown()
{
    showStep(2);
    int* n = new int(3);
    m_countdown->setText(QString::number(*n));
    auto* t = new QTimer(this);
    connect(t, &QTimer::timeout, this, [this, t, n] {
        --(*n);
        if (*n <= 0) {
            t->stop();
            t->deleteLater();
            delete n;
            goLiveNow();
            return;
        }
        m_countdown->setText(QString::number(*n));
    });
    t->start(700);
}

void GoLiveDialog::goLiveNow()
{
    StreamTarget target;
    target.id = newId(QStringLiteral("tgt"));
    target.platform = m_platform;
    target.name = m_platform;
    target.rtmpUrl = m_url->text().trimmed();
    target.streamKeySecretId = SecretStore::makeStreamKeyId(target.id);
    if (!m_key->text().isEmpty())
        SecretStore::store(target.streamKeySecretId, m_key->text());

    m_engine->sceneGraph()->mutate([&](Project& p) {
        p.streamTargets.append(target);
        p.output = p.output.width > 0 ? p.output : OutputProfile{};
    });

    QString err;
    if (!m_engine->startStreaming(target.id, &err)) {
        QMessageBox::warning(this, QStringLiteral("Go Live"), err);
        showStep(0);
        return;
    }
    showStep(3);
}

} // namespace railshot
