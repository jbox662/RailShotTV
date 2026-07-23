#include "ui/widgets/SourcePropertiesWidget.h"
#include "ui/widgets/properties/SourcePropertiesPanel.h"
#include "core/EngineController.h"
#include "core/SceneGraph.h"
#include "audio/AudioGraph.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QTabWidget>
#include <QSlider>
#include <QScrollArea>
#include <QColorDialog>
#include <QStackedWidget>
#include <cmath>
#include <algorithm>

namespace railshot {

SourcePropertiesWidget::SourcePropertiesWidget(EngineController* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setObjectName(QStringLiteral("inputSettingsDrawer"));
    setFixedWidth(460);
    setStyleSheet(QStringLiteral(
        "QWidget#inputSettingsDrawer {"
        "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #16181E, stop:1 #0F1114);"
        "  border-left: 3px solid #4F9EFF;"
        "}"
        "QTabBar::tab { background:#12151A; color:#8892A4; padding:9px 12px; font-size:10px; font-weight:800;"
        "  border:1px solid #2A2D35; border-bottom:2px solid transparent; }"
        "QTabBar::tab:selected { color:#7AB8FF; border-bottom:2px solid #4F9EFF; background:#0C1830; }"
        "QLineEdit, QDoubleSpinBox { background:#0A0C10; border:1px solid #4A4D55; color:#E0E2E8;"
        "  border-radius:3px; padding:5px 7px; font-size:11px; }"
        "QLineEdit:focus, QDoubleSpinBox:focus { border:2px solid #4F9EFF; }"
        "QLabel#sectionTitle { color:#4F9EFF; font-size:9px; font-weight:900; letter-spacing:1.5px; }"));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* header = new QWidget(this);
    m_header = header;
    header->setFixedHeight(42);
    header->setStyleSheet(QStringLiteral(
        "background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 rgba(79,158,255,0.28),stop:0.55 transparent);"
        "border-bottom:2px solid #3A3D45; border-left:3px solid #4F9EFF;"));
    auto* headerLay = new QHBoxLayout(header);
    headerLay->setContentsMargins(14, 0, 10, 0);
    m_title = new QLabel(QStringLiteral("INPUT SETTINGS"), header);
    m_title->setObjectName(QStringLiteral("panelTitleBlue"));
    m_closeBtn = new QPushButton(QStringLiteral("✕"), header);
    m_closeBtn->setFixedSize(28, 24);
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    connect(m_closeBtn, &QPushButton::clicked, this, &SourcePropertiesWidget::closeRequested);
    headerLay->addWidget(m_title);
    headerLay->addStretch();
    headerLay->addWidget(m_closeBtn);
    root->addWidget(header);

    m_empty = new QLabel(QStringLiteral("Select a source to edit its settings"), this);
    m_empty->setAlignment(Qt::AlignCenter);
    m_empty->setStyleSheet(QStringLiteral("color:#606878; padding:40px;"));
    root->addWidget(m_empty);

    m_formHost = new QWidget(this);
    auto* formLay = new QVBoxLayout(m_formHost);
    formLay->setContentsMargins(0, 0, 0, 0);
    formLay->setSpacing(0);

    m_tabs = new QTabWidget(m_formHost);
    m_tabs->setDocumentMode(true);

    // Source (type-specific) — leading tab
    m_sourceTab = new QWidget(m_tabs);
    m_sourceTabLay = new QVBoxLayout(m_sourceTab);
    m_sourceTabLay->setContentsMargins(14, 12, 14, 12);
    m_tabs->addTab(m_sourceTab, QStringLiteral("Source"));

    auto makeSpin = [this](double minV, double maxV, double step) {
        auto* s = new QDoubleSpinBox(m_formHost);
        s->setRange(minV, maxV);
        s->setSingleStep(step);
        s->setDecimals(3);
        return s;
    };

    // General
    auto* general = new QWidget(m_tabs);
    auto* gForm = new QFormLayout(general);
    gForm->setContentsMargins(14, 12, 14, 12);
    m_name = new QLineEdit(general);
    m_visible = new QCheckBox(QStringLiteral("Visible"), general);
    m_locked = new QCheckBox(QStringLiteral("Locked"), general);
    gForm->addRow(QStringLiteral("Name"), m_name);
    gForm->addRow(m_visible);
    gForm->addRow(m_locked);
    auto* swatchRow = new QHBoxLayout();
    const QStringList colors = {QStringLiteral("#1A1A1A"), QStringLiteral("#EF4444"), QStringLiteral("#22C55E"),
                                QStringLiteral("#FBBF24"), QStringLiteral("#EC4899"), QStringLiteral("#3B82F6"),
                                QStringLiteral("#A855F7")};
    for (const auto& c : colors) {
        auto* b = new QPushButton(general);
        b->setFixedSize(22, 22);
        b->setStyleSheet(QStringLiteral("background:%1; border:1px solid #3A3D45; border-radius:3px;").arg(c));
        connect(b, &QPushButton::clicked, this, [this, c] {
            if (!m_engine) return;
            auto src = m_engine->selectedSource();
            if (!src) return;
            auto settings = src->settings;
            settings.insert(QStringLiteral("categoryColor"), c);
            m_engine->updateSourceSettings(src->id, settings);
            m_engine->sceneGraph()->mutate([&](Project& p) {
                if (auto* s = p.findSourceAnywhere(src->id))
                    s->colorHex = c;
            });
        });
        swatchRow->addWidget(b);
    }
    swatchRow->addStretch();
    gForm->addRow(QStringLiteral("Category"), swatchRow);
    m_tabs->addTab(general, QStringLiteral("General"));

    // Position
    auto* position = new QWidget(m_tabs);
    auto* pForm = new QFormLayout(position);
    pForm->setContentsMargins(14, 12, 14, 12);
    m_x = makeSpin(-2.0, 2.0, 0.01);
    m_y = makeSpin(-2.0, 2.0, 0.01);
    m_w = makeSpin(0.01, 2.0, 0.01);
    m_h = makeSpin(0.01, 2.0, 0.01);
    m_rot = makeSpin(-360.0, 360.0, 1.0);
    m_rot->setDecimals(1);
    m_opacity = makeSpin(0.0, 1.0, 0.05);
    m_opacity->setDecimals(2);
    m_cropL = makeSpin(0.0, 0.49, 0.01);
    m_cropR = makeSpin(0.0, 0.49, 0.01);
    m_cropT = makeSpin(0.0, 0.49, 0.01);
    m_cropB = makeSpin(0.0, 0.49, 0.01);
    pForm->addRow(QStringLiteral("X"), m_x);
    pForm->addRow(QStringLiteral("Y"), m_y);
    pForm->addRow(QStringLiteral("W"), m_w);
    pForm->addRow(QStringLiteral("H"), m_h);
    pForm->addRow(QStringLiteral("Rot"), m_rot);
    pForm->addRow(QStringLiteral("Opacity"), m_opacity);
    pForm->addRow(QStringLiteral("Crop L"), m_cropL);
    pForm->addRow(QStringLiteral("Crop R"), m_cropR);
    pForm->addRow(QStringLiteral("Crop T"), m_cropT);
    pForm->addRow(QStringLiteral("Crop B"), m_cropB);
    auto* zRow = new QHBoxLayout();
    auto* zUp = new QPushButton(QStringLiteral("Z ↑"), position);
    auto* zDown = new QPushButton(QStringLiteral("Z ↓"), position);
    auto* resetPos = new QPushButton(QStringLiteral("Reset"), position);
    zRow->addWidget(zUp);
    zRow->addWidget(zDown);
    zRow->addWidget(resetPos);
    pForm->addRow(zRow);
    m_tabs->addTab(position, QStringLiteral("Position"));

    // Colour
    auto* colour = new QWidget(m_tabs);
    auto* cForm = new QFormLayout(colour);
    cForm->setContentsMargins(14, 12, 14, 12);
    auto makeSlider = [](QWidget* parent) {
        auto* s = new QSlider(Qt::Horizontal, parent);
        s->setRange(-100, 100);
        s->setValue(0);
        return s;
    };
    m_brightness = makeSlider(colour);
    m_contrast = makeSlider(colour);
    m_saturation = makeSlider(colour);
    cForm->addRow(QStringLiteral("Brightness"), m_brightness);
    cForm->addRow(QStringLiteral("Contrast"), m_contrast);
    cForm->addRow(QStringLiteral("Saturation"), m_saturation);
    auto* resetCol = new QPushButton(QStringLiteral("Reset Colour"), colour);
    connect(resetCol, &QPushButton::clicked, this, [this] {
        m_brightness->setValue(0);
        m_contrast->setValue(0);
        m_saturation->setValue(0);
    });
    cForm->addRow(resetCol);
    auto* colourNote = new QLabel(QStringLiteral("Colour adjust applies live on Program/Preview."), colour);
    colourNote->setWordWrap(true);
    colourNote->setStyleSheet(QStringLiteral("color:#606878; font-size:10px;"));
    cForm->addRow(colourNote);
    m_tabs->addTab(colour, QStringLiteral("Colour"));

    // Effects — chroma key
    auto* effects = new QWidget(m_tabs);
    auto* eForm = new QFormLayout(effects);
    eForm->setContentsMargins(14, 12, 14, 12);
    m_chromaKey = new QCheckBox(QStringLiteral("Chroma key (green)"), effects);
    m_chromaSim = new QSlider(Qt::Horizontal, effects);
    m_chromaSim->setRange(5, 95);
    m_chromaSim->setValue(40);
    m_blur = new QSlider(Qt::Horizontal, effects);
    m_blur->setRange(0, 100);
    m_blur->setValue(0);
    eForm->addRow(m_chromaKey);
    eForm->addRow(QStringLiteral("Similarity"), m_chromaSim);
    eForm->addRow(QStringLiteral("Blur"), m_blur);
    auto* effectsNote = new QLabel(QStringLiteral("Chroma + blur + sharpen + pixelate are live via Filters."), effects);
    effectsNote->setWordWrap(true);
    effectsNote->setStyleSheet(QStringLiteral("color:#606878; font-size:10px;"));
    eForm->addRow(effectsNote);
    m_tabs->addTab(effects, QStringLiteral("Effects"));

    // Layers
    auto* layers = new QLabel(QStringLiteral("Scene layers — use ↑/↓ on input tiles or Z buttons."), m_tabs);
    layers->setWordWrap(true);
    layers->setStyleSheet(QStringLiteral("color:#808898; padding:16px;"));
    m_tabs->addTab(layers, QStringLiteral("Layers"));

    // Audio — only for OBS_SOURCE_AUDIO types (hidden for Image/Text/etc.)
    m_audioTab = new QWidget(m_tabs);
    auto* aForm = new QFormLayout(m_audioTab);
    aForm->setContentsMargins(14, 12, 14, 12);
    m_volume = new QSlider(Qt::Horizontal, m_audioTab);
    m_volume->setRange(0, 200);
    m_volume->setValue(100);
    m_audioMute = new QCheckBox(QStringLiteral("Mute"), m_audioTab);
    aForm->addRow(QStringLiteral("Volume %"), m_volume);
    aForm->addRow(m_audioMute);
    auto* audioNote = new QLabel(
        QStringLiteral("Matches OBS: only sources that produce audio appear in the mixer. "
                       "Browser needs “Control audio via OBS” enabled."),
        m_audioTab);
    audioNote->setWordWrap(true);
    audioNote->setStyleSheet(QStringLiteral("color:#606878; font-size:10px;"));
    aForm->addRow(audioNote);
    m_audioTabIndex = m_tabs->addTab(m_audioTab, QStringLiteral("Audio"));

    formLay->addWidget(m_tabs, 1);

    auto* footer = new QWidget(m_formHost);
    m_footer = footer;
    footer->setFixedHeight(48);
    footer->setStyleSheet(QStringLiteral(
        "background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #16181E,stop:1 #0F1114);"
        "border-top:2px solid #3A3D45;"));
    auto* footLay = new QHBoxLayout(footer);
    footLay->setContentsMargins(14, 8, 14, 8);
    auto* cancel = new QPushButton(QStringLiteral("Cancel"), footer);
    auto* applyBtn = new QPushButton(QStringLiteral("Apply"), footer);
    applyBtn->setObjectName(QStringLiteral("primaryButton"));
    applyBtn->setStyleSheet(QStringLiteral(
        "QPushButton{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #3A6AFF,stop:1 #1050CC);"
        "color:white;font-weight:900;border:2px solid #5A8AFF;border-radius:4px;padding:7px 20px;}"
        "QPushButton:hover{border-color:#8AB4FF;}"));
    connect(cancel, &QPushButton::clicked, this, &SourcePropertiesWidget::closeRequested);
    connect(applyBtn, &QPushButton::clicked, this, &SourcePropertiesWidget::applyAll);
    footLay->addStretch();
    footLay->addWidget(cancel);
    footLay->addWidget(applyBtn);
    formLay->addWidget(footer);

    root->addWidget(m_formHost, 1);

    connect(m_name, &QLineEdit::editingFinished, this, [this] {
        if (m_block || !m_engine) return;
        m_engine->setSourceName(m_engine->selectedSourceId(), m_name->text());
    });
    connect(m_visible, &QCheckBox::toggled, this, [this](bool on) {
        if (m_block || !m_engine) return;
        m_engine->setSourceVisible(m_engine->selectedSourceId(), on);
    });
    connect(m_locked, &QCheckBox::toggled, this, [this](bool on) {
        if (m_block || !m_engine) return;
        m_engine->setSourceLocked(m_engine->selectedSourceId(), on);
    });
    auto onSpin = [this](double) { applyTransformFromUi(); };
    for (auto* s : {m_x, m_y, m_w, m_h, m_rot, m_opacity, m_cropL, m_cropR, m_cropT, m_cropB})
        connect(s, qOverload<double>(&QDoubleSpinBox::valueChanged), this, onSpin);
    auto pushColour = [this] {
        if (m_block || !m_engine) return;
        auto src = m_engine->selectedSource();
        if (!src) return;
        auto settings = src->settings;
        settings.insert(QStringLiteral("brightness"), m_brightness->value());
        settings.insert(QStringLiteral("contrast"), m_contrast->value());
        settings.insert(QStringLiteral("saturation"), m_saturation->value());
        if (m_chromaKey)
            settings.insert(QStringLiteral("chromaKey"), m_chromaKey->isChecked());
        if (m_chromaSim)
            settings.insert(QStringLiteral("chromaSimilarity"), m_chromaSim->value());
        if (m_blur)
            settings.insert(QStringLiteral("blur"), m_blur->value());
        m_engine->updateSourceSettings(src->id, settings);
    };
    for (auto* s : {m_brightness, m_contrast, m_saturation, m_chromaSim, m_blur})
        if (s) connect(s, &QSlider::valueChanged, this, [pushColour](int) { pushColour(); });
    if (m_chromaKey)
        connect(m_chromaKey, &QCheckBox::toggled, this, [pushColour](bool) { pushColour(); });
    connect(zUp, &QPushButton::clicked, this, [this] {
        if (m_engine) m_engine->moveSourceZOrder(m_engine->selectedSourceId(), 1);
    });
    connect(zDown, &QPushButton::clicked, this, [this] {
        if (m_engine) m_engine->moveSourceZOrder(m_engine->selectedSourceId(), -1);
    });
    connect(resetPos, &QPushButton::clicked, this, [this] {
        if (m_block || !m_engine) return;
        Transform t;
        m_engine->updateSourceTransform(m_engine->selectedSourceId(), t);
        rebuild();
    });

    connect(engine, &EngineController::selectedSourceChanged, this, [this](const QString&) { rebuild(); });
    connect(engine->sceneGraph(), &SceneGraph::projectChanged, this, [this] { rebuild(); });
    connect(engine, &EngineController::showHideFadeChanged, this, [this] { rebuild(); });
    rebuild();
}

void SourcePropertiesWidget::setDialogMode(bool on)
{
    m_dialogMode = on;
    if (on) {
        setMinimumWidth(420);
        setMaximumWidth(16777215);
        setObjectName(QStringLiteral("inputSettingsDialogBody"));
        setStyleSheet(QStringLiteral(
            "QWidget#inputSettingsDialogBody {"
            "  background:#12151A; border:none;"
            "}"
            "QWidget#inputSettingsDialogBody QTabWidget::pane {"
            "  background:#0E1014;"
            "  border:1px solid #2A2D35;"
            "  border-top:1px solid #3A3D45;"
            "  top:-1px;"
            "  margin:0 10px 10px 10px;"
            "}"
            "QTabBar::tab {"
            "  background:#16181E; color:#8892A4; padding:8px 14px; font-size:10px; font-weight:800;"
            "  border:1px solid #2A2D35; border-bottom:none; margin-right:2px;"
            "}"
            "QTabBar::tab:selected {"
            "  color:#E8F0FF; border:1px solid #4F9EFF; border-bottom:1px solid #0E1014;"
            "  background:#0C1830;"
            "}"
            "QTabBar::tab:hover:!selected { color:#C8CAD0; border-color:#3A3D45; }"
            "QLineEdit, QDoubleSpinBox, QSpinBox, QComboBox {"
            "  background:#0A0C10; border:1px solid #4A4D55; color:#E0E2E8;"
            "  border-radius:3px; padding:5px 7px; font-size:11px;"
            "}"
            "QLineEdit:focus, QDoubleSpinBox:focus, QSpinBox:focus, QComboBox:focus {"
            "  border:1px solid #4F9EFF;"
            "}"
            "QLabel#sectionTitle { color:#4F9EFF; font-size:9px; font-weight:900; letter-spacing:1.5px; }"
            "QCheckBox { color:#C8CAD0; spacing:6px; }"
            "QFrame#propsInfoCard {"
            "  background:#0C1420; border:1px solid #2A3A55; border-left:3px solid #4F9EFF;"
            "}"));
        if (m_header) {
            m_header->setStyleSheet(QStringLiteral(
                "background:qlineargradient(x1:0,y1:0,x2:1,y2:0,"
                "stop:0 rgba(79,158,255,0.28),stop:0.45 rgba(79,158,255,0.08),stop:1 transparent);"
                "border-bottom:1px solid #3A3D45;"
                "border-left:3px solid #4F9EFF;"));
        }
        if (m_closeBtn) m_closeBtn->hide();
        if (m_footer) m_footer->hide();
    } else {
        setFixedWidth(460);
        if (m_closeBtn) m_closeBtn->show();
        if (m_footer) m_footer->show();
    }
}

void SourcePropertiesWidget::applyAndClose()
{
    applyAll();
}

void SourcePropertiesWidget::resetTypeSpecificDefaults()
{
    if (!m_engine || !m_sourcePanel) return;
    auto src = m_engine->selectedSource();
    if (!src) return;
    auto settings = src->settings;
    m_sourcePanel->resetDefaults(settings);
    m_engine->updateSourceSettings(src->id, settings);
    rebuild();
}

void SourcePropertiesWidget::applyTransformFromUi()
{
    if (m_block || !m_engine) return;
    auto src = m_engine->selectedSource();
    if (!src) return;
    Transform t = src->transform;
    t.x = m_x->value();
    t.y = m_y->value();
    t.w = m_w->value();
    t.h = m_h->value();
    t.rotation = m_rot->value();
    t.opacity = m_opacity->value();
    t.cropLeft = m_cropL->value();
    t.cropRight = m_cropR->value();
    t.cropTop = m_cropT->value();
    t.cropBottom = m_cropB->value();
    m_engine->updateSourceTransform(src->id, t);
}

void SourcePropertiesWidget::applyAll()
{
    applyTransformFromUi();
    if (!m_engine) return;
    auto src = m_engine->selectedSource();
    if (!src) return;
    auto settings = src->settings;
    if (m_sourcePanel)
        m_sourcePanel->applyTo(settings);
    settings.insert(QStringLiteral("brightness"), m_brightness->value());
    settings.insert(QStringLiteral("contrast"), m_contrast->value());
    settings.insert(QStringLiteral("saturation"), m_saturation->value());
    settings.insert(QStringLiteral("chromaKey"), m_chromaKey && m_chromaKey->isChecked());
    settings.insert(QStringLiteral("chromaSimilarity"), m_chromaSim ? m_chromaSim->value() : 40);
    settings.insert(QStringLiteral("blur"), m_blur ? m_blur->value() : 0);
    // Only persist audio keys for types that can appear in the OBS mixer.
    if (sourceTypeSupportsAudio(src->type)) {
        settings.insert(QStringLiteral("audioVolume"), m_volume->value());
        settings.insert(QStringLiteral("audioMute"), m_audioMute->isChecked());
    } else {
        settings.remove(QStringLiteral("audioVolume"));
        settings.remove(QStringLiteral("audioMute"));
    }
    m_engine->updateSourceSettings(src->id, settings);
    emit closeRequested();
}

void SourcePropertiesWidget::rebuild()
{
    m_block = true;
    const auto src = m_engine ? m_engine->selectedSource() : std::nullopt;
    const bool has = src.has_value();
    m_empty->setVisible(!has);
    m_formHost->setVisible(has);
    if (has) {
        m_title->setText(m_dialogMode
                             ? QStringLiteral("PROPERTIES — %1").arg(src->name.toUpper())
                             : QStringLiteral("INPUT SETTINGS — %1").arg(src->name.toUpper()));
        m_name->setText(src->name);
        m_visible->setChecked(m_engine->sourceVisibilityTarget(src->id));
        m_locked->setChecked(src->locked);
        m_x->setValue(src->transform.x);
        m_y->setValue(src->transform.y);
        m_w->setValue(src->transform.w);
        m_h->setValue(src->transform.h);
        m_rot->setValue(src->transform.rotation);
        m_opacity->setValue(src->transform.opacity);
        m_cropL->setValue(src->transform.cropLeft);
        m_cropR->setValue(src->transform.cropRight);
        m_cropT->setValue(src->transform.cropTop);
        m_cropB->setValue(src->transform.cropBottom);
        m_brightness->setValue(src->settings.value(QStringLiteral("brightness")).toInt());
        m_contrast->setValue(src->settings.value(QStringLiteral("contrast")).toInt());
        m_saturation->setValue(src->settings.value(QStringLiteral("saturation")).toInt());
        if (m_chromaKey)
            m_chromaKey->setChecked(src->settings.value(QStringLiteral("chromaKey")).toBool());
        if (m_chromaSim)
            m_chromaSim->setValue(src->settings.value(QStringLiteral("chromaSimilarity")).toInt(40));
        if (m_blur)
            m_blur->setValue(src->settings.value(QStringLiteral("blur")).toInt(0));
        m_volume->setValue(src->settings.value(QStringLiteral("audioVolume")).toInt(100));
        m_audioMute->setChecked(src->settings.value(QStringLiteral("audioMute")).toBool());
        // Prefer live mixer state when this source already has a graph channel
        if (m_engine && m_engine->audio()) {
            const auto live = m_engine->audio()->channelState(src->id);
            if (live.id == src->id) {
                m_volume->setValue(int(std::lround(std::clamp(live.volume, 0.f, 20.f) * 100.f)));
                m_audioMute->setChecked(live.muted);
            }
        }

        if (m_audioTabIndex >= 0) {
            const bool showAudio = sourceTypeSupportsAudio(src->type);
            m_tabs->setTabVisible(m_audioTabIndex, showAudio);
            // Browser volume controls only apply when routed through OBS.
            const bool browserControlled = src->type != SourceType::Browser
                || src->settings.value(QStringLiteral("controlAudioViaObs")).toBool(false);
            m_volume->setEnabled(browserControlled);
            m_audioMute->setEnabled(browserControlled);
        }

        if (!m_sourcePanel || m_panelType != src->type) {
            if (m_sourcePanel) {
                m_sourceTabLay->removeWidget(m_sourcePanel);
                m_sourcePanel->deleteLater();
                m_sourcePanel = nullptr;
            }
            // Drop leftover stretch spacers from previous panel mounts.
            while (m_sourceTabLay->count() > 0) {
                QLayoutItem* item = m_sourceTabLay->takeAt(0);
                if (!item) break;
                if (QWidget* w = item->widget()) {
                    w->deleteLater();
                }
                delete item;
            }
            m_panelType = src->type;
            m_sourcePanel = createSourcePropertiesPanel(src->type, m_engine, m_sourceTab);
            if (m_sourcePanel) {
                // Stretch 0 on the form; spacer fills leftover height (OBS preview lives in the dialog).
                m_sourceTabLay->addWidget(m_sourcePanel, 0);
                m_sourceTabLay->addStretch(1);
                connect(m_sourcePanel, &SourcePropertiesPanel::settingsEdited, this, [this] {
                    if (m_block || !m_engine || !m_sourcePanel) return;
                    auto cur = m_engine->selectedSource();
                    if (!cur) return;
                    auto settings = cur->settings;
                    m_sourcePanel->applyTo(settings);
                    m_engine->updateSourceSettings(cur->id, settings);
                });
            }
        }
        if (m_sourcePanel)
            m_sourcePanel->loadFrom(*src);
    } else {
        m_title->setText(QStringLiteral("INPUT SETTINGS"));
        if (m_sourcePanel) {
            m_sourceTabLay->removeWidget(m_sourcePanel);
            m_sourcePanel->deleteLater();
            m_sourcePanel = nullptr;
            m_panelType = SourceType::Unknown;
        }
    }
    m_block = false;
}

} // namespace railshot
