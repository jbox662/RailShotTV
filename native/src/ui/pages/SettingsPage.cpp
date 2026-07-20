#include "ui/pages/SettingsPage.h"
#include "ui/Theme.h"
#include "core/EngineController.h"
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
        auto* plat = new QComboBox(w);
        plat->addItems({QStringLiteral("YouTube"), QStringLiteral("Twitch"), QStringLiteral("Facebook"), QStringLiteral("Custom")});
        auto* key = new QLineEdit(w);
        key->setEchoMode(QLineEdit::Password);
        key->setPlaceholderText(QStringLiteral("Configured via Go Live / Credential Manager"));
        key->setEnabled(false);
        auto profile = engine->settings()->outputProfile();
        auto* enc = new QComboBox(w);
        enc->addItems({QStringLiteral("auto"), QStringLiteral("nvenc"), QStringLiteral("qsv"),
                       QStringLiteral("amf"), QStringLiteral("x264"), QStringLiteral("mf"), QStringLiteral("software")});
        enc->setCurrentText(profile.encoderPreference);
        auto* rate = new QComboBox(w);
        rate->addItems({QStringLiteral("CBR"), QStringLiteral("VBR"), QStringLiteral("CQP")});
        rate->setCurrentText(profile.rateControl.isEmpty() ? QStringLiteral("CBR") : profile.rateControl);
        auto* bitrate = new QSpinBox(w);
        bitrate->setRange(500, 50000);
        bitrate->setValue(profile.videoBitrateKbps);
        auto* kf = new QSpinBox(w);
        kf->setRange(1, 10);
        kf->setValue(profile.keyframeIntervalSec > 0 ? profile.keyframeIntervalSec : 2);
        f->addRow(QStringLiteral("Platform"), plat);
        f->addRow(QStringLiteral("Stream Key"), key);
        f->addRow(QStringLiteral("Encoder"), enc);
        f->addRow(QStringLiteral("Rate control"), rate);
        f->addRow(QStringLiteral("Bitrate (kbps)"), bitrate);
        f->addRow(QStringLiteral("Keyframe (s)"), kf);
        connect(enc, &QComboBox::currentTextChanged, this, [markDirty](const QString&) { markDirty(); });
        connect(rate, &QComboBox::currentTextChanged, this, [markDirty](const QString&) { markDirty(); });
        connect(bitrate, QOverload<int>::of(&QSpinBox::valueChanged), this, [markDirty](int) { markDirty(); });
        connect(kf, QOverload<int>::of(&QSpinBox::valueChanged), this, [markDirty](int) { markDirty(); });
        // stash widgets for save via properties
        enc->setObjectName(QStringLiteral("settingsEncoder"));
        bitrate->setObjectName(QStringLiteral("settingsBitrate"));
        addScrollPage(w);
        m_streamEnc = enc;
        m_streamBitrate = bitrate;
        m_streamRate = rate;
        m_streamKeyframe = kf;
        m_streamPlatform = plat;
    }

    // Output
    {
        auto* w = new QWidget;
        auto* f = new QFormLayout(w);
        auto profile = engine->settings()->outputProfile();
        auto* width = new QSpinBox(w); width->setRange(640, 3840); width->setValue(profile.width);
        auto* height = new QSpinBox(w); height->setRange(360, 2160); height->setValue(profile.height);
        auto* dir = new QLineEdit(engine->settings()->recordingDirectory(), w);
        auto* browse = new QPushButton(QStringLiteral("Browse…"), w);
        auto* fmt = new QComboBox(w);
        fmt->addItems({QStringLiteral("MKV"), QStringLiteral("MP4"), QStringLiteral("MOV"), QStringLiteral("FLV")});
        const int replayDefault = engine->settings()->uiState().value(QStringLiteral("replaySeconds")).toInt(30);
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
        f->addRow(QStringLiteral("Container"), fmt);
        f->addRow(QStringLiteral("Replay buffer"), replaySec);
        connect(browse, &QPushButton::clicked, this, [=] {
            const auto d = QFileDialog::getExistingDirectory(this, QStringLiteral("Recording Folder"));
            if (!d.isEmpty()) { dir->setText(d); markDirty(); }
        });
        connect(width, QOverload<int>::of(&QSpinBox::valueChanged), this, [markDirty](int) { markDirty(); });
        connect(height, QOverload<int>::of(&QSpinBox::valueChanged), this, [markDirty](int) { markDirty(); });
        connect(replaySec, QOverload<int>::of(&QSpinBox::valueChanged), this, [markDirty](int) { markDirty(); });
        addScrollPage(w);
        m_outW = width;
        m_outH = height;
        m_outDir = dir;
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
        f->addRow(new QLabel(QStringLiteral("Click a field and press a key combination."), w));
        const QStringList order = {
            QStringLiteral("go"), QStringLiteral("streamToggle"), QStringLiteral("recordToggle"),
            QStringLiteral("saveReplay"), QStringLiteral("muteMic"),
            QStringLiteral("scoreAPlus"), QStringLiteral("scoreAMinus"),
            QStringLiteral("scoreBPlus"), QStringLiteral("scoreBMinus"),
            QStringLiteral("scoreReset"), QStringLiteral("scoreSwap"),
            QStringLiteral("scene1"), QStringLiteral("scene2"), QStringLiteral("scene3"),
        };
        auto edits = std::make_shared<QHash<QString, QKeySequenceEdit*>>();
        QJsonObject keys = engine->settings()->hotkeys();
        for (const QString& action : order) {
            auto* edit = new QKeySequenceEdit(HotkeyDispatcher::sequenceFor(keys.value(action).toString()), w);
            edits->insert(action, edit);
            f->addRow(action, edit);
            connect(edit, &QKeySequenceEdit::keySequenceChanged, this, [markDirty](const QKeySequence&) { markDirty(); });
        }
        m_hotkeyEdits = edits;
        addScrollPage(w);
    }

    // Advanced
    {
        auto* w = new QWidget;
        auto* f = new QFormLayout(w);
        auto* prio = new QComboBox(w);
        prio->addItems({QStringLiteral("Normal"), QStringLiteral("Above normal"), QStringLiteral("High")});
        auto* net = new QCheckBox(QStringLiteral("Network optimize (saved preference)"), w);
        auto* lowLat = new QCheckBox(QStringLiteral("Low latency mode (saved preference)"), w);
        auto* bind = new QLineEdit(w);
        bind->setPlaceholderText(QStringLiteral("0.0.0.0"));
        auto* note = new QLabel(QStringLiteral("Priority / bind IP apply on next launch when process hooks land."), w);
        note->setWordWrap(true);
        note->setStyleSheet(QStringLiteral("color:#606878; font-size:10px;"));
        f->addRow(QStringLiteral("Process priority"), prio);
        f->addRow(net);
        f->addRow(lowLat);
        f->addRow(QStringLiteral("Bind IP"), bind);
        f->addRow(note);
        connect(prio, &QComboBox::currentTextChanged, this, [markDirty](const QString&) { markDirty(); });
        connect(net, &QCheckBox::toggled, this, [markDirty](bool) { markDirty(); });
        connect(lowLat, &QCheckBox::toggled, this, [markDirty](bool) { markDirty(); });
        addScrollPage(w);
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
        if (m_streamEnc) p.encoderPreference = m_streamEnc->currentText();
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
        if (generalTheme) ui.insert(QStringLiteral("theme"), generalTheme->currentText());
        if (generalLang) ui.insert(QStringLiteral("language"), generalLang->currentText());
        if (generalConfirmQuit) ui.insert(QStringLiteral("confirmQuitWhileLive"), generalConfirmQuit->isChecked());
        if (generalRestoreProject) ui.insert(QStringLiteral("restoreLastProject"), generalRestoreProject->isChecked());
        if (m_streamPlatform) ui.insert(QStringLiteral("streamPlatform"), m_streamPlatform->currentText());
        m_engine->settings()->setUiState(ui);
        if (m_hotkeyEdits) {
            QJsonObject o;
            for (auto it = m_hotkeyEdits->begin(); it != m_hotkeyEdits->end(); ++it) {
                const QKeySequence seq = it.value()->keySequence();
                o.insert(it.key(), seq.toString(QKeySequence::NativeText));
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
