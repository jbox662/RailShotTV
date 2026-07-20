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
#include <QTcpSocket>
#include <QUrl>
#include <functional>

namespace railshot {
namespace {

QString platformSecretId(const QString& platform)
{
    return QStringLiteral("stream/platform/%1").arg(platform);
}

} // namespace

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
        const QString id = QString::fromLatin1(p.id);
        m_urlsByPlatform.insert(id, defaultUrlFor(id));
        if (auto stored = SecretStore::load(platformSecretId(id)))
            m_keysByPlatform.insert(id, *stored);

        auto* b = new QPushButton(QString::fromLatin1(p.name), config);
        b->setFixedSize(72, 64);
        b->setCheckable(true);
        b->setCursor(Qt::PointingHandCursor);
        b->setProperty("platformId", id);
        b->setStyleSheet(QStringLiteral(
            "QPushButton{background:#0F1218;border:2px solid #2A3350;border-radius:8px;"
            "color:%1;font-weight:800;font-size:10px;}"
            "QPushButton:checked{border-color:%1;background:#1A1F2E;}").arg(QString::fromLatin1(p.color)));
        connect(b, &QPushButton::clicked, this, [this, b, id](bool checked) {
            if (!checked) {
                // Keep at least one platform selected.
                bool any = false;
                for (auto* x : m_platformBtns) {
                    if (x != b && x->isChecked()) { any = true; break; }
                }
                if (!any) {
                    b->setChecked(true);
                    return;
                }
            }
            selectPlatform(id, false);
        });
        m_platformBtns.push_back(b);
        platGrid->addWidget(b);
    }
    m_platformBtns.first()->setChecked(true);
    cfg->addLayout(platGrid);

    m_hint = new QLabel(QStringLiteral("Multi-select platforms to stream to several destinations. "
                                       "Each selected platform needs its own stream key."), config);
    m_hint->setWordWrap(true);
    m_hint->setStyleSheet(QStringLiteral("color:#606878; font-size:10px;"));
    cfg->addWidget(m_hint);

    m_title = new QLineEdit(config);
    m_title->setPlaceholderText(QStringLiteral("Stream Title"));
    cfg->addWidget(m_title);

    m_url = new QLineEdit(defaultUrlFor(QStringLiteral("youtube")), config);
    m_url->setPlaceholderText(QStringLiteral("RTMP URL"));
    cfg->addWidget(m_url);

    m_key = new QLineEdit(config);
    m_key->setEchoMode(QLineEdit::Password);
    m_key->setPlaceholderText(QStringLiteral("Stream Key (for focused platform)"));
    m_key->setStyleSheet(QStringLiteral(
        "QLineEdit{font-family:'JetBrains Mono';}"
        "QLineEdit:focus{border-color:#FF5A2C;}"));
    cfg->addWidget(m_key);

    auto* chips = new QLabel(QStringLiteral("Ready  ·  Audio  ·  Scene  ·  Encoder  ·  Network  ·  Key"), config);
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

    selectPlatform(QStringLiteral("youtube"), true);
    showStep(0);
    motion::playModalEnter(this);
}

QString GoLiveDialog::defaultUrlFor(const QString& platform) const
{
    if (platform == QLatin1String("youtube"))
        return QStringLiteral("rtmp://a.rtmp.youtube.com/live2");
    if (platform == QLatin1String("twitch"))
        return QStringLiteral("rtmp://live.twitch.tv/app");
    if (platform == QLatin1String("facebook"))
        return QStringLiteral("rtmps://live-api-s.facebook.com:443/rtmp");
    return {};
}

QStringList GoLiveDialog::selectedPlatforms() const
{
    QStringList out;
    for (auto* b : m_platformBtns) {
        if (b->isChecked())
            out.push_back(b->property("platformId").toString());
    }
    return out;
}

void GoLiveDialog::persistFocusedKey()
{
    if (!m_key) return;
    m_keysByPlatform[m_platform] = m_key->text();
    if (m_url)
        m_urlsByPlatform[m_platform] = m_url->text().trimmed();
}

void GoLiveDialog::selectPlatform(const QString& id, bool exclusive)
{
    persistFocusedKey();
    m_platform = id;
    if (exclusive) {
        for (auto* x : m_platformBtns)
            x->setChecked(x->property("platformId").toString() == id);
    }
    m_url->setText(m_urlsByPlatform.value(id, defaultUrlFor(id)));
    m_key->setText(m_keysByPlatform.value(id));
    const auto selected = selectedPlatforms();
    if (m_hint) {
        m_hint->setText(selected.size() > 1
                            ? QStringLiteral("Multistreaming to %1 destinations. Switch platforms to edit each key.")
                                  .arg(selected.size())
                            : QStringLiteral("Multi-select platforms to stream to several destinations. "
                                             "Each selected platform needs its own stream key."));
    }
}

QString GoLiveDialog::keyForPlatform(const QString& platform) const
{
    if (platform == m_platform)
        return m_key->text().trimmed();
    return m_keysByPlatform.value(platform).trimmed();
}

void GoLiveDialog::setPrefill(const QString& title, const QString& platform)
{
    if (m_title && !title.isEmpty())
        m_title->setText(title);
    if (platform.isEmpty()) return;
    selectPlatform(platform.toLower(), true);
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
    persistFocusedKey();
    showStep(1);
    m_checkIndex = 0;

    struct Check {
        QString label;
        QString color;
        std::function<QString()> probe;
    };

    const QStringList plats = selectedPlatforms();

    QVector<Check> checks;
    checks.push_back({QStringLiteral("Stream keys"), QStringLiteral("#FBBF24"), [this, plats] {
        for (const auto& p : plats) {
            if (keyForPlatform(p).isEmpty())
                return QStringLiteral("Missing stream key for %1").arg(p);
        }
        return QString();
    }});
    checks.push_back({QStringLiteral("RTMP URL"), QStringLiteral("#22D3EE"), [this, plats] {
        for (const auto& p : plats) {
            const QString url = (p == m_platform)
                                    ? m_url->text().trimmed()
                                    : m_urlsByPlatform.value(p, defaultUrlFor(p)).trimmed();
            if (url.isEmpty())
                return QStringLiteral("RTMP URL empty for %1").arg(p);
        }
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
    checks.push_back({QStringLiteral("Network"), QStringLiteral("#22D3EE"), [this, plats] {
        for (const auto& p : plats) {
            const QString raw = (p == m_platform)
                                    ? m_url->text().trimmed()
                                    : m_urlsByPlatform.value(p, defaultUrlFor(p)).trimmed();
            const QUrl url(raw);
            if (!url.isValid() || url.host().isEmpty())
                return QStringLiteral("RTMP host missing for %1").arg(p);
            QTcpSocket sock;
            sock.connectToHost(url.host(), url.port(1935));
            if (!sock.waitForConnected(1500))
                return QStringLiteral("Cannot reach %1").arg(url.host());
            sock.disconnectFromHost();
        }
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
    persistFocusedKey();
    const QStringList plats = selectedPlatforms();
    QVector<StreamTarget> targets;
    targets.reserve(plats.size());

    m_engine->sceneGraph()->mutate([&](Project& p) {
        for (const auto& plat : plats) {
            StreamTarget target;
            target.id = newId(QStringLiteral("tgt"));
            target.platform = plat;
            target.name = m_title->text().trimmed().isEmpty() ? plat : m_title->text().trimmed();
            target.title = m_title->text().trimmed();
            target.rtmpUrl = m_urlsByPlatform.value(plat, defaultUrlFor(plat)).trimmed();
            if (plat == m_platform)
                target.rtmpUrl = m_url->text().trimmed();
            target.streamKeySecretId = SecretStore::makeStreamKeyId(target.id);
            const QString key = keyForPlatform(plat);
            SecretStore::store(target.streamKeySecretId, key);
            SecretStore::store(platformSecretId(plat), key);
            p.streamTargets.append(target);
            targets.push_back(target);
        }
        p.output = p.output.width > 0 ? p.output : OutputProfile{};
    });

    // Persist last stream title in UI state
    auto ui = m_engine->settings()->uiState();
    ui.insert(QStringLiteral("lastStreamTitle"), m_title->text().trimmed());
    ui.insert(QStringLiteral("lastStreamPlatform"), plats.join(QLatin1Char(',')));
    m_engine->settings()->setUiState(ui);

    QString err;
    if (!m_engine->startStreamingTargets(targets, &err)) {
        QMessageBox::warning(this, QStringLiteral("Go Live"), err);
        showStep(0);
        return;
    }
    showStep(3);
}

} // namespace railshot
