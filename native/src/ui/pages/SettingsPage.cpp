#include "ui/pages/SettingsPage.h"
#include "core/EngineController.h"
#include "compositor/D3D11Compositor.h"
#include "overlays/ReplayBuffer.h"
#include "ui/HotkeyDispatcher.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QSpinBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QTabWidget>
#include <QKeySequenceEdit>
#include <QJsonObject>
#include <QHash>
#include <memory>

namespace railshot {

SettingsPage::SettingsPage(EngineController* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    auto* root = new QVBoxLayout(this);
    auto* tabs = new QTabWidget(this);
    root->addWidget(tabs);

    auto* output = new QWidget(tabs);
    auto* form = new QFormLayout(output);
    auto profile = engine->settings()->outputProfile();
    auto* w = new QSpinBox(output); w->setRange(640, 3840); w->setValue(profile.width);
    auto* h = new QSpinBox(output); h->setRange(360, 2160); h->setValue(profile.height);
    auto* bitrate = new QSpinBox(output); bitrate->setRange(500, 50000); bitrate->setValue(profile.videoBitrateKbps);
    auto* enc = new QComboBox(output);
    enc->addItems({QStringLiteral("auto"), QStringLiteral("mf"), QStringLiteral("software"),
                   QStringLiteral("nvenc"), QStringLiteral("amf"), QStringLiteral("qsv")});
    enc->setCurrentText(profile.encoderPreference);
    form->addRow(QStringLiteral("Width"), w);
    form->addRow(QStringLiteral("Height"), h);
    form->addRow(QStringLiteral("Video Bitrate (kbps)"), bitrate);
    form->addRow(QStringLiteral("Encoder"), enc);
    auto* save = new QPushButton(QStringLiteral("Save Output Profile"), output);
    connect(save, &QPushButton::clicked, this, [this, w, h, bitrate, enc] {
        OutputProfile p = m_engine->settings()->outputProfile();
        p.width = w->value();
        p.height = h->value();
        p.videoBitrateKbps = bitrate->value();
        p.encoderPreference = enc->currentText();
        m_engine->settings()->setOutputProfile(p);
        m_engine->sceneGraph()->mutate([&](Project& proj) { proj.output = p; });
        if (m_engine->compositor())
            m_engine->compositor()->resize(p.width, p.height);
    });
    form->addRow(save);
    tabs->addTab(output, QStringLiteral("Output"));

    auto* rec = new QWidget(tabs);
    auto* rform = new QFormLayout(rec);
    auto* dir = new QLineEdit(engine->settings()->recordingDirectory(), rec);
    auto* browse = new QPushButton(QStringLiteral("Browse…"), rec);
    connect(browse, &QPushButton::clicked, this, [this, dir] {
        const auto d = QFileDialog::getExistingDirectory(this, QStringLiteral("Recording Folder"));
        if (!d.isEmpty()) {
            dir->setText(d);
            m_engine->settings()->setRecordingDirectory(d);
        }
    });
    const int replayDefault = engine->settings()->uiState().value(QStringLiteral("replaySeconds")).toInt(30);
    if (engine->replayBuffer())
        engine->replayBuffer()->setCapacitySeconds(replayDefault);
    auto* replaySec = new QSpinBox(rec);
    replaySec->setRange(5, 300);
    replaySec->setValue(replayDefault);
    replaySec->setSuffix(QStringLiteral(" sec"));
    connect(replaySec, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int v) {
        if (m_engine->replayBuffer())
            m_engine->replayBuffer()->setCapacitySeconds(v);
        auto ui = m_engine->settings()->uiState();
        ui.insert(QStringLiteral("replaySeconds"), v);
        m_engine->settings()->setUiState(ui);
    });
    rform->addRow(QStringLiteral("Directory"), dir);
    rform->addRow(browse);
    rform->addRow(QStringLiteral("Replay buffer"), replaySec);
    rform->addRow(new QLabel(QStringLiteral("Save Replay writes the last N seconds of encoded output."), rec));
    tabs->addTab(rec, QStringLiteral("Recording"));

    auto* stream = new QWidget(tabs);
    auto* sform = new QFormLayout(stream);
    sform->addRow(new QLabel(QStringLiteral("Configure targets via Go Live. Keys live in Windows Credential Manager."), stream));
    tabs->addTab(stream, QStringLiteral("Stream"));

    auto* hotkeysPage = new QWidget(tabs);
    auto* hform = new QFormLayout(hotkeysPage);
    hform->addRow(new QLabel(QStringLiteral("Click a field and press a key combination."), hotkeysPage));

    const QStringList order = {
        QStringLiteral("go"), QStringLiteral("streamToggle"), QStringLiteral("recordToggle"),
        QStringLiteral("saveReplay"), QStringLiteral("muteMic"),
        QStringLiteral("scoreAPlus"), QStringLiteral("scoreAMinus"),
        QStringLiteral("scoreBPlus"), QStringLiteral("scoreBMinus"),
        QStringLiteral("scoreReset"), QStringLiteral("scoreSwap"),
        QStringLiteral("scene1"), QStringLiteral("scene2"), QStringLiteral("scene3"), QStringLiteral("scene4"),
        QStringLiteral("scene5"), QStringLiteral("scene6"), QStringLiteral("scene7"), QStringLiteral("scene8"),
    };

    auto edits = std::make_shared<QHash<QString, QKeySequenceEdit*>>();
    auto persist = [this, edits] {
        QJsonObject o;
        for (auto it = edits->begin(); it != edits->end(); ++it) {
            const QKeySequence seq = it.value()->keySequence();
            if (it.key() == QLatin1String("go")
                && (seq.isEmpty() || seq == QKeySequence(Qt::Key_Space)
                    || seq.toString(QKeySequence::PortableText).compare(QStringLiteral("Space"), Qt::CaseInsensitive) == 0))
                o.insert(it.key(), QStringLiteral("Space"));
            else
                o.insert(it.key(), seq.toString(QKeySequence::NativeText));
        }
        m_engine->settings()->setHotkeys(o);
    };

    QJsonObject keys = engine->settings()->hotkeys();
    for (const QString& action : order) {
        auto* edit = new QKeySequenceEdit(HotkeyDispatcher::sequenceFor(keys.value(action).toString()), hotkeysPage);
        edits->insert(action, edit);
        hform->addRow(action, edit);
        connect(edit, &QKeySequenceEdit::keySequenceChanged, this, persist);
    }
    auto* resetKeys = new QPushButton(QStringLiteral("Reset Defaults"), hotkeysPage);
    connect(resetKeys, &QPushButton::clicked, this, [this, edits] {
        m_engine->settings()->setHotkeys({});
        const auto keys = m_engine->settings()->hotkeys();
        for (auto it = edits->begin(); it != edits->end(); ++it)
            it.value()->setKeySequence(HotkeyDispatcher::sequenceFor(keys.value(it.key()).toString()));
    });
    hform->addRow(resetKeys);
    tabs->addTab(hotkeysPage, QStringLiteral("Hotkeys"));
}

} // namespace railshot
