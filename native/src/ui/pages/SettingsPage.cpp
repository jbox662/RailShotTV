#include "ui/pages/SettingsPage.h"
#include "ui/Theme.h"
#include "core/EngineController.h"
#include "core/SecretStore.h"
#include "core/RecordingPath.h"
#include "compositor/D3D11Compositor.h"
#include "overlays/ReplayBuffer.h"
#include "ui/HotkeyDispatcher.h"
#include "audio/WasapiCapture.h"
#include "audio/AudioGraph.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QSpinBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QKeySequenceEdit>
#include <QJsonObject>
#include <QHash>
#include <QCheckBox>
#include <QStackedWidget>
#include <QButtonGroup>
#include <QFrame>
#include <QScrollArea>
#include <QMessageBox>
#include <memory>
#include <QStyle>

namespace railshot {

SettingsPage::SettingsPage(EngineController* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setObjectName(QStringLiteral("settingsPage"));
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    root->addWidget(theme::makePageHeader(QStringLiteral("Settings"), theme::PanelAccent::Brand, this));

    auto* body = new QHBoxLayout();
    body->setContentsMargins(0, 0, 0, 0);
    body->setSpacing(0);

    auto* rail = new QFrame(this);
    rail->setObjectName(QStringLiteral("settingsRail"));
    rail->setFixedWidth(168);
    auto* railLay = new QVBoxLayout(rail);
    railLay->setContentsMargins(0, 8, 0, 8);
    railLay->setSpacing(0);

    auto* stack = new QStackedWidget(this);
    stack->setObjectName(QStringLiteral("settingsStack"));
    stack->setStyleSheet(QStringLiteral("QStackedWidget#settingsStack{background:#141928; border:none;}"));
    auto* tabGroup = new QButtonGroup(this);
    tabGroup->setExclusive(true);

    bool* dirty = new bool(false);
    auto* saveBtn = new QPushButton(QStringLiteral("SAVE SETTINGS"), this);
    saveBtn->setEnabled(false);
    auto markDirty = [=] {
        *dirty = true;
        saveBtn->setEnabled(true);
        saveBtn->setObjectName(QStringLiteral("saveSettingsDirty"));
        saveBtn->style()->unpolish(saveBtn);
        saveBtn->style()->polish(saveBtn);
    };

    const QStringList tabNames = {
        QStringLiteral("General"), QStringLiteral("Stream"), QStringLiteral("Output"),
        QStringLiteral("Video"), QStringLiteral("Audio"), QStringLiteral("Hotkeys"),
        QStringLiteral("Advanced"), QStringLiteral("Plugins")};

    auto addScrollPage = [&](QWidget* inner) {
        auto* scroll = new QScrollArea(stack);
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setStyleSheet(QStringLiteral("QScrollArea{background:#141928; border:none;}"));
        auto* host = new QWidget;
        host->setMaximumWidth(640);
        auto* hl = new QVBoxLayout(host);
        hl->setContentsMargins(24, 20, 24, 20);
        hl->addWidget(inner);
        hl->addStretch();
        scroll->setWidget(host);
        stack->addWidget(scroll);
    };

    // General
    QComboBox* generalTheme = nullptr;
    QComboBox* generalLang = nullptr;
    QCheckBox* generalConfirmQuit = nullptr;
    QCheckBox* generalRestoreProject = nullptr;
    {
        auto* w = new QWidget;
        auto* f = new QFormLayout(w);
        generalTheme = new QComboBox(w);
        generalTheme->addItems({QStringLiteral("Dark"), QStringLiteral("Light"), QStringLiteral("System")});
        generalLang = new QComboBox(w);
        generalLang->addItems({QStringLiteral("English"), QStringLiteral("Español"), QStringLiteral("Français")});
        generalConfirmQuit = new QCheckBox(QStringLiteral("Confirm before quitting while live"), w);
        generalConfirmQuit->setChecked(true);
        generalRestoreProject = new QCheckBox(QStringLiteral("Restore last project on startup"), w);
        generalRestoreProject->setChecked(true);
        const auto ui0 = engine->settings()->uiState();
        if (ui0.contains(QStringLiteral("theme")))
            generalTheme->setCurrentText(ui0.value(QStringLiteral("theme")).toString());
        if (ui0.contains(QStringLiteral("language")))
            generalLang->setCurrentText(ui0.value(QStringLiteral("language")).toString());
        if (ui0.contains(QStringLiteral("confirmQuitWhileLive")))
            generalConfirmQuit->setChecked(ui0.value(QStringLiteral("confirmQuitWhileLive")).toBool());
        if (ui0.contains(QStringLiteral("restoreLastProject")))
            generalRestoreProject->setChecked(ui0.value(QStringLiteral("restoreLastProject")).toBool());
        f->addRow(QStringLiteral("Theme"), generalTheme);
        f->addRow(QStringLiteral("Language"), generalLang);
        f->addRow(generalConfirmQuit);
        f->addRow(generalRestoreProject);
        connect(generalTheme, &QComboBox::currentTextChanged, this, [markDirty](const QString&) { markDirty(); });
        connect(generalLang, &QComboBox::currentTextChanged, this, [markDirty](const QString&) { markDirty(); });
        connect(generalConfirmQuit, &QCheckBox::toggled, this, [markDirty](bool) { markDirty(); });
        connect(generalRestoreProject, &QCheckBox::toggled, this, [markDirty](bool) { markDirty(); });
        addScrollPage(w);
    }

    // Stream
    {
        auto* w = new QWidget;
        auto* f = new QFormLayout(w);
        const auto ui0 = engine->settings()->uiState();
        auto* plat = new QComboBox(w);
        plat->addItem(QStringLiteral("YouTube"), QStringLiteral("youtube"));
        plat->addItem(QStringLiteral("Twitch"), QStringLiteral("twitch"));
        plat->addItem(QStringLiteral("Facebook"), QStringLiteral("facebook"));
        plat->addItem(QStringLiteral("Custom"), QStringLiteral("custom"));
        const QString savedPlat = ui0.value(QStringLiteral("streamPlatformId")).toString(
            ui0.value(QStringLiteral("streamPlatform")).toString(QStringLiteral("YouTube")).toLower());
        {
            const int idx = plat->findData(savedPlat.contains(QLatin1String("twitch")) ? QStringLiteral("twitch")
                              : savedPlat.contains(QLatin1String("facebook")) ? QStringLiteral("facebook")
                              : savedPlat.contains(QLatin1String("custom")) ? QStringLiteral("custom")
                              : QStringLiteral("youtube"));
            plat->setCurrentIndex(idx >= 0 ? idx : 0);
        }

        auto defaultUrl = [](const QString& id) -> QString {
            if (id == QLatin1String("youtube"))
                return QStringLiteral("rtmp://a.rtmp.youtube.com/live2");
            if (id == QLatin1String("twitch"))
                return QStringLiteral("rtmp://live.twitch.tv/app");
            if (id == QLatin1String("facebook"))
                return QStringLiteral("rtmps://live-api-s.facebook.com:443/rtmp/");
            return {};
        };

        auto* url = new QLineEdit(w);
        auto* key = new QLineEdit(w);
        key->setEchoMode(QLineEdit::Password);
        key->setPlaceholderText(QStringLiteral("Stream key (stored in Credential Manager)"));

        auto loadPlatformFields = [=](const QString& id) {
            const QString secretId = QStringLiteral("stream/platform/%1").arg(id);
            const QString urlKey = QStringLiteral("streamUrl/%1").arg(id);
            const QString savedUrl = ui0.value(urlKey).toString();
            url->setText(savedUrl.isEmpty() ? defaultUrl(id) : savedUrl);
            if (auto stored = SecretStore::load(secretId))
                key->setText(*stored);
            else
                key->clear();
        };
        loadPlatformFields(plat->currentData().toString());

        auto profile = engine->settings()->outputProfile();
        auto* enc = new QComboBox(w);
        enc->addItem(QStringLiteral("Auto (prefer hardware)"), QStringLiteral("auto"));
        enc->addItem(QStringLiteral("Hardware (Media Foundation)"), QStringLiteral("mf"));
        enc->addItem(QStringLiteral("Software"), QStringLiteral("software"));
        {
            QString pref = profile.encoderPreference.toLower();
            if (pref == QLatin1String("x264") || pref == QLatin1String("nvenc")
                || pref == QLatin1String("amf") || pref == QLatin1String("qsv"))
                pref = (pref == QLatin1String("x264")) ? QStringLiteral("software") : QStringLiteral("auto");
            const int idx = enc->findData(pref);
            enc->setCurrentIndex(idx >= 0 ? idx : 0);
        }
        auto* rate = new QComboBox(w);
        rate->addItems({QStringLiteral("CBR"), QStringLiteral("VBR"), QStringLiteral("CQP")});
        rate->setCurrentText(profile.rateControl.isEmpty() ? QStringLiteral("CBR") : profile.rateControl);
        auto* bitrate = new QSpinBox(w);
        bitrate->setRange(500, 50000);
        bitrate->setValue(profile.videoBitrateKbps);
        bitrate->setSuffix(QStringLiteral(" kbps"));
        auto* audioBr = new QSpinBox(w);
        audioBr->setRange(64, 320);
        audioBr->setValue(profile.audioBitrateKbps > 0 ? profile.audioBitrateKbps : 160);
        audioBr->setSuffix(QStringLiteral(" kbps"));
        auto* kf = new QSpinBox(w);
        kf->setRange(1, 10);
        kf->setValue(profile.keyframeIntervalSec > 0 ? profile.keyframeIntervalSec : 2);
        kf->setSuffix(QStringLiteral(" s"));
        auto* note = new QLabel(
            QStringLiteral("Encoder labels: vendor names (NVENC/AMF/QSV) currently map to Auto → MF hardware, then software."),
            w);
        note->setWordWrap(true);
        note->setStyleSheet(QStringLiteral("color:#606878; font-size:10px;"));

        f->addRow(QStringLiteral("Service"), plat);
        f->addRow(QStringLiteral("Server"), url);
        f->addRow(QStringLiteral("Stream Key"), key);
        f->addRow(QStringLiteral("Encoder"), enc);
        f->addRow(QStringLiteral("Rate control"), rate);
        f->addRow(QStringLiteral("Video bitrate"), bitrate);
        f->addRow(QStringLiteral("Audio bitrate"), audioBr);
        f->addRow(QStringLiteral("Keyframe interval"), kf);
        f->addRow(note);
        connect(plat, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int) {
            loadPlatformFields(plat->currentData().toString());
            markDirty();
        });
        connect(url, &QLineEdit::textChanged, this, [markDirty](const QString&) { markDirty(); });
        connect(key, &QLineEdit::textChanged, this, [markDirty](const QString&) { markDirty(); });
        connect(enc, &QComboBox::currentTextChanged, this, [markDirty](const QString&) { markDirty(); });
        connect(rate, &QComboBox::currentTextChanged, this, [markDirty](const QString&) { markDirty(); });
        connect(bitrate, QOverload<int>::of(&QSpinBox::valueChanged), this, [markDirty](int) { markDirty(); });
        connect(audioBr, QOverload<int>::of(&QSpinBox::valueChanged), this, [markDirty](int) { markDirty(); });
        connect(kf, QOverload<int>::of(&QSpinBox::valueChanged), this, [markDirty](int) { markDirty(); });
        addScrollPage(w);
        m_streamEnc = enc;
        m_streamBitrate = bitrate;
        m_streamAudioBitrate = audioBr;
        m_streamRate = rate;
        m_streamKeyframe = kf;
        m_streamPlatform = plat;
        m_streamUrl = url;
        m_streamKey = key;
    }

    // Output
    {
        auto* w = new QWidget;
        auto* f = new QFormLayout(w);
        auto profile = engine->settings()->outputProfile();
        const auto ui0 = engine->settings()->uiState();
        auto* width = new QSpinBox(w); width->setRange(640, 3840); width->setValue(profile.width);
        auto* height = new QSpinBox(w); height->setRange(360, 2160); height->setValue(profile.height);
        auto* dir = new QLineEdit(engine->settings()->recordingDirectory(), w);
        auto* browse = new QPushButton(QStringLiteral("Browse…"), w);
        auto* pattern = new QLineEdit(
            ui0.value(QStringLiteral("recordingFilenamePattern")).toString(defaultRecordingFilenamePattern()), w);
        pattern->setPlaceholderText(defaultRecordingFilenamePattern());
        auto* patternHint = new QLabel(
            QStringLiteral("Tokens: %CCYY %YY %MM %DD %HH %mm %ss. Always written as MKV (crash-safe)."), w);
        patternHint->setWordWrap(true);
        patternHint->setStyleSheet(QStringLiteral("color:#606878; font-size:10px;"));
        auto* fmt = new QLabel(QStringLiteral("MKV (recording) → optional MP4 remux"), w);
        fmt->setStyleSheet(QStringLiteral("color:#A8B0C0;"));
        auto* autoRemux = new QCheckBox(QStringLiteral("Automatically remux to MP4 when recording stops"), w);
        autoRemux->setChecked(ui0.value(QStringLiteral("autoRemuxToMp4")).toBool(false));
        const int replayDefault = ui0.value(QStringLiteral("replaySeconds")).toInt(30);
        if (engine->replayBuffer())
            engine->replayBuffer()->setCapacitySeconds(replayDefault);
        auto* replaySec = new QSpinBox(w);
        replaySec->setRange(5, 300);
        replaySec->setValue(replayDefault);
        replaySec->setSuffix(QStringLiteral(" sec"));
        f->addRow(QStringLiteral("Canvas W"), width);
        f->addRow(QStringLiteral("Canvas H"), height);
        f->addRow(QStringLiteral("Recording path"), dir);
        f->addRow(browse);
        f->addRow(QStringLiteral("Filename"), pattern);
        f->addRow(patternHint);
        f->addRow(QStringLiteral("Container"), fmt);
        f->addRow(autoRemux);
        f->addRow(QStringLiteral("Replay buffer"), replaySec);
        connect(browse, &QPushButton::clicked, this, [=] {
            const auto d = QFileDialog::getExistingDirectory(this, QStringLiteral("Recording Folder"));
            if (!d.isEmpty()) { dir->setText(d); markDirty(); }
        });
        connect(width, QOverload<int>::of(&QSpinBox::valueChanged), this, [markDirty](int) { markDirty(); });
        connect(height, QOverload<int>::of(&QSpinBox::valueChanged), this, [markDirty](int) { markDirty(); });
        connect(dir, &QLineEdit::textChanged, this, [markDirty](const QString&) { markDirty(); });
        connect(pattern, &QLineEdit::textChanged, this, [markDirty](const QString&) { markDirty(); });
        connect(autoRemux, &QCheckBox::toggled, this, [markDirty](bool) { markDirty(); });
        connect(replaySec, QOverload<int>::of(&QSpinBox::valueChanged), this, [markDirty](int) { markDirty(); });
        addScrollPage(w);
        m_outW = width;
        m_outH = height;
        m_outDir = dir;
        m_filenamePattern = pattern;
        m_autoRemux = autoRemux;
        m_replaySec = replaySec;
    }

    // Video
    {
        auto* w = new QWidget;
        auto* f = new QFormLayout(w);
        auto profile = engine->settings()->outputProfile();
        auto* res = new QComboBox(w);
        res->addItems({QStringLiteral("1920×1080"), QStringLiteral("1280×720"), QStringLiteral("2560×1440"), QStringLiteral("3840×2160")});
        auto* fps = new QComboBox(w);
        fps->addItems({QStringLiteral("24"), QStringLiteral("30"), QStringLiteral("60"), QStringLiteral("120")});
        fps->setCurrentText(QString::number(profile.fps > 0 ? profile.fps : 60));
        f->addRow(QStringLiteral("Resolution"), res);
        f->addRow(QStringLiteral("FPS"), fps);
        connect(res, &QComboBox::currentTextChanged, this, [markDirty](const QString&) { markDirty(); });
        connect(fps, &QComboBox::currentTextChanged, this, [markDirty](const QString&) { markDirty(); });
        addScrollPage(w);
        m_videoFps = fps;
        m_videoRes = res;
    }

    // Audio
    {
        auto* w = new QWidget;
        auto* f = new QFormLayout(w);
        auto* rate = new QComboBox(w);
        rate->addItems({QStringLiteral("48000")});
        rate->setEnabled(false);
        auto* ch = new QComboBox(w);
        ch->addItems({QStringLiteral("Stereo")});
        ch->setEnabled(false);

        auto fillDevices = [](QComboBox* box, AudioDeviceKind kind, const QString& selected) {
            box->clear();
            box->addItem(QStringLiteral("System default"), QString());
            for (const auto& d : WasapiCapture::enumerate(kind))
                box->addItem(d.name, d.id);
            const int idx = box->findData(selected);
            box->setCurrentIndex(idx >= 0 ? idx : 0);
        };

        auto* desktop = new QComboBox(w);
        auto* mic = new QComboBox(w);
        fillDevices(desktop, AudioDeviceKind::Loopback, engine->settings()->desktopDeviceId());
        fillDevices(mic, AudioDeviceKind::Capture, engine->settings()->micDeviceId());
        connect(desktop, &QComboBox::currentIndexChanged, this, [markDirty](int) { markDirty(); });
        connect(mic, &QComboBox::currentIndexChanged, this, [markDirty](int) { markDirty(); });

        auto* note = new QLabel(QStringLiteral("Changing devices restarts WASAPI capture immediately on Save."), w);
        note->setWordWrap(true);
        note->setStyleSheet(QStringLiteral("color:#606878; font-size:10px;"));
        f->addRow(QStringLiteral("Sample rate"), rate);
        f->addRow(QStringLiteral("Channels"), ch);
        f->addRow(QStringLiteral("Desktop device"), desktop);
        f->addRow(QStringLiteral("Mic device"), mic);
        f->addRow(note);
        m_desktopDevice = desktop;
        m_micDevice = mic;
        addScrollPage(w);
    }

    // Hotkeys
    {
        auto* w = new QWidget;
        auto* f = new QFormLayout(w);
        f->addRow(new QLabel(QStringLiteral("Click a field and press a key combination. Duplicates are highlighted on Save."), w));
        auto edits = std::make_shared<QHash<QString, QKeySequenceEdit*>>();
        QJsonObject keys = engine->settings()->hotkeys();
        for (const QString& action : HotkeyDispatcher::orderedActions()) {
            auto* edit = new QKeySequenceEdit(HotkeyDispatcher::sequenceFor(keys.value(action).toString()), w);
            edits->insert(action, edit);
            f->addRow(HotkeyDispatcher::labelForAction(action), edit);
            connect(edit, &QKeySequenceEdit::keySequenceChanged, this, [markDirty](const QKeySequence&) { markDirty(); });
        }
        m_hotkeyEdits = edits;
        addScrollPage(w);
    }

    // Advanced
    {
        auto* w = new QWidget;
        auto* f = new QFormLayout(w);
        const auto ui0 = engine->settings()->uiState();
        auto* prio = new QComboBox(w);
        prio->addItems({QStringLiteral("Normal"), QStringLiteral("Above normal"), QStringLiteral("High")});
        auto* net = new QCheckBox(QStringLiteral("Network optimize (saved preference)"), w);
        auto* lowLat = new QCheckBox(QStringLiteral("Low latency mode (saved preference)"), w);
        auto* reconnect = new QCheckBox(QStringLiteral("Automatically reconnect on stream failure"), w);
        reconnect->setChecked(ui0.value(QStringLiteral("reconnectEnabled")).toBool(true));
        auto* reconnectMax = new QSpinBox(w);
        reconnectMax->setRange(0, 100);
        reconnectMax->setSpecialValueText(QStringLiteral("Unlimited"));
        reconnectMax->setValue(ui0.value(QStringLiteral("reconnectMaxAttempts")).toInt(0));
        reconnectMax->setToolTip(QStringLiteral("0 = keep trying with backoff"));
        auto* delay = new QSpinBox(w);
        delay->setRange(0, 600);
        delay->setValue(ui0.value(QStringLiteral("streamDelaySec")).toInt(0));
        delay->setSuffix(QStringLiteral(" sec"));
        auto* bind = new QLineEdit(w);
        bind->setPlaceholderText(QStringLiteral("0.0.0.0"));
        auto* note = new QLabel(
            QStringLiteral("Reconnect and stream delay apply on the next Go Live. "
                           "Priority / bind IP apply on next launch when process hooks land."),
            w);
        note->setWordWrap(true);
        note->setStyleSheet(QStringLiteral("color:#606878; font-size:10px;"));
        f->addRow(QStringLiteral("Process priority"), prio);
        f->addRow(net);
        f->addRow(lowLat);
        f->addRow(reconnect);
        f->addRow(QStringLiteral("Reconnect attempts"), reconnectMax);
        f->addRow(QStringLiteral("Stream delay"), delay);
        f->addRow(QStringLiteral("Bind IP"), bind);
        f->addRow(note);
        connect(prio, &QComboBox::currentTextChanged, this, [markDirty](const QString&) { markDirty(); });
        connect(net, &QCheckBox::toggled, this, [markDirty](bool) { markDirty(); });
        connect(lowLat, &QCheckBox::toggled, this, [markDirty](bool) { markDirty(); });
        connect(reconnect, &QCheckBox::toggled, this, [markDirty](bool) { markDirty(); });
        connect(reconnectMax, QOverload<int>::of(&QSpinBox::valueChanged), this, [markDirty](int) { markDirty(); });
        connect(delay, QOverload<int>::of(&QSpinBox::valueChanged), this, [markDirty](int) { markDirty(); });
        addScrollPage(w);
        m_reconnectEnabled = reconnect;
        m_reconnectMax = reconnectMax;
        m_streamDelay = delay;
    }

    // Plugins
    {
        auto* w = new QWidget;
        auto* f = new QVBoxLayout(w);
        f->addWidget(new QLabel(QStringLiteral("Plugins"), w));
        auto* list = new QLabel(QStringLiteral("No plugins installed.\nInstall packages will appear here."), w);
        list->setStyleSheet(QStringLiteral("color:#606878;"));
        f->addWidget(list);
        auto* install = new QPushButton(QStringLiteral("Install…"), w);
        install->setEnabled(false);
        f->addWidget(install);
        f->addStretch();
        addScrollPage(w);
    }

    for (int i = 0; i < tabNames.size(); ++i) {
        auto* b = new QPushButton(tabNames[i], rail);
        b->setObjectName(QStringLiteral("settingsTabBtn"));
        b->setCheckable(true);
        b->setChecked(i == 0);
        tabGroup->addButton(b, i);
        railLay->addWidget(b);
        connect(b, &QPushButton::clicked, this, [stack, i] { stack->setCurrentIndex(i); });
    }
    railLay->addStretch();
    body->addWidget(rail);
    body->addWidget(stack, 1);
    root->addLayout(body, 1);

    auto* footer = new QFrame(this);
    footer->setObjectName(QStringLiteral("settingsFooter"));
    footer->setFixedHeight(48);
    auto* foot = new QHBoxLayout(footer);
    foot->setContentsMargins(16, 8, 16, 8);
    auto* cancel = new QPushButton(QStringLiteral("Cancel"), footer);
    cancel->setObjectName(QStringLiteral("chromeBtn"));
    foot->addStretch();
    foot->addWidget(cancel);
    foot->addWidget(saveBtn);
    root->addWidget(footer);

    connect(cancel, &QPushButton::clicked, this, [=] {
        *dirty = false;
        saveBtn->setEnabled(false);
        saveBtn->setObjectName(QString());
        saveBtn->style()->unpolish(saveBtn);
        saveBtn->style()->polish(saveBtn);
    });

    connect(saveBtn, &QPushButton::clicked, this, [=] {
        OutputProfile p = m_engine->settings()->outputProfile();
        if (m_outW) p.width = m_outW->value();
        if (m_outH) p.height = m_outH->value();
        if (m_streamBitrate) p.videoBitrateKbps = m_streamBitrate->value();
        if (m_streamAudioBitrate) p.audioBitrateKbps = m_streamAudioBitrate->value();
        if (m_streamEnc) {
            const QVariant d = m_streamEnc->currentData();
            p.encoderPreference = d.isValid() ? d.toString() : m_streamEnc->currentText();
        }
        if (m_streamRate) p.rateControl = m_streamRate->currentText();
        if (m_streamKeyframe) p.keyframeIntervalSec = m_streamKeyframe->value();
        if (m_videoFps) p.fps = m_videoFps->currentText().toDouble();
        if (m_videoRes) {
            const QString r = m_videoRes->currentText();
            if (r.contains(QLatin1String("720"))) { p.width = 1280; p.height = 720; }
            else if (r.contains(QLatin1String("1440"))) { p.width = 2560; p.height = 1440; }
            else if (r.contains(QLatin1String("2160"))) { p.width = 3840; p.height = 2160; }
            else { p.width = 1920; p.height = 1080; }
        }
        m_engine->settings()->setOutputProfile(p);
        m_engine->sceneGraph()->mutate([&](Project& proj) { proj.output = p; });
        if (m_engine->compositor())
            m_engine->compositor()->resize(p.width, p.height);
        if (m_outDir && !m_outDir->text().isEmpty())
            m_engine->settings()->setRecordingDirectory(m_outDir->text());
        auto ui = m_engine->settings()->uiState();
        if (m_replaySec) {
            if (m_engine->replayBuffer())
                m_engine->replayBuffer()->setCapacitySeconds(m_replaySec->value());
            ui.insert(QStringLiteral("replaySeconds"), m_replaySec->value());
        }
        if (m_filenamePattern)
            ui.insert(QStringLiteral("recordingFilenamePattern"),
                      m_filenamePattern->text().trimmed().isEmpty()
                          ? defaultRecordingFilenamePattern()
                          : m_filenamePattern->text().trimmed());
        if (m_autoRemux)
            ui.insert(QStringLiteral("autoRemuxToMp4"), m_autoRemux->isChecked());
        if (m_reconnectEnabled)
            ui.insert(QStringLiteral("reconnectEnabled"), m_reconnectEnabled->isChecked());
        if (m_reconnectMax)
            ui.insert(QStringLiteral("reconnectMaxAttempts"), m_reconnectMax->value());
        if (m_streamDelay)
            ui.insert(QStringLiteral("streamDelaySec"), m_streamDelay->value());
        if (generalTheme) ui.insert(QStringLiteral("theme"), generalTheme->currentText());
        if (generalLang) ui.insert(QStringLiteral("language"), generalLang->currentText());
        if (generalConfirmQuit) ui.insert(QStringLiteral("confirmQuitWhileLive"), generalConfirmQuit->isChecked());
        if (generalRestoreProject) ui.insert(QStringLiteral("restoreLastProject"), generalRestoreProject->isChecked());
        if (m_streamPlatform) {
            const QString platId = m_streamPlatform->currentData().toString();
            ui.insert(QStringLiteral("streamPlatform"), m_streamPlatform->currentText());
            ui.insert(QStringLiteral("streamPlatformId"), platId);
            if (m_streamUrl)
                ui.insert(QStringLiteral("streamUrl/%1").arg(platId), m_streamUrl->text().trimmed());
            if (m_streamKey && !m_streamKey->text().trimmed().isEmpty()) {
                QString err;
                if (!SecretStore::store(QStringLiteral("stream/platform/%1").arg(platId),
                                        m_streamKey->text().trimmed(), &err)) {
                    QMessageBox::warning(this, QStringLiteral("Stream Key"),
                                         err.isEmpty() ? QStringLiteral("Failed to store stream key") : err);
                }
            }
        }
        m_engine->settings()->setUiState(ui);
        if (m_hotkeyEdits) {
            QJsonObject o;
            QHash<QString, QString> seen; // portable seq -> action
            QStringList conflicts;
            for (auto it = m_hotkeyEdits->begin(); it != m_hotkeyEdits->end(); ++it) {
                const QKeySequence seq = it.value()->keySequence();
                const QString portable = seq.toString(QKeySequence::PortableText);
                o.insert(it.key(), seq.toString(QKeySequence::NativeText));
                if (!portable.isEmpty()) {
                    if (seen.contains(portable))
                        conflicts << QStringLiteral("%1 and %2 share %3")
                                         .arg(HotkeyDispatcher::labelForAction(seen.value(portable)),
                                              HotkeyDispatcher::labelForAction(it.key()),
                                              seq.toString(QKeySequence::NativeText));
                    else
                        seen.insert(portable, it.key());
                }
            }
            if (!conflicts.isEmpty()) {
                QMessageBox::warning(this, QStringLiteral("Hotkeys"),
                                     QStringLiteral("Duplicate bindings:\n%1").arg(conflicts.join(QLatin1Char('\n'))));
            }
            m_engine->settings()->setHotkeys(o);
        }
        if (m_desktopDevice && m_micDevice) {
            const QString desktopId = m_desktopDevice->currentData().toString();
            const QString micId = m_micDevice->currentData().toString();
            m_engine->settings()->setDesktopDeviceId(desktopId);
            m_engine->settings()->setMicDeviceId(micId);
            if (m_engine->audio()) {
                QString err;
                if (!m_engine->audio()->reconfigureDevices(desktopId, micId, &err)) {
                    QMessageBox::warning(this, QStringLiteral("Audio"),
                                         err.isEmpty() ? QStringLiteral("Failed to switch audio devices") : err);
                }
            }
        }
        m_engine->settings()->sync();
        *dirty = false;
        saveBtn->setEnabled(false);
        saveBtn->setObjectName(QString());
        saveBtn->style()->unpolish(saveBtn);
        saveBtn->style()->polish(saveBtn);
    });
}

} // namespace railshot
