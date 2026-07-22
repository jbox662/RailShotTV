#include "ui/widgets/FiltersDialog.h"
#include "core/EngineController.h"
#include "core/SceneGraph.h"
#include "core/Types.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QCheckBox>
#include <QSlider>
#include <QLabel>
#include <QStackedWidget>
#include <QFormLayout>
#include <QMenu>
#include <QCursor>
#include <QComboBox>
#include <QColorDialog>
#include <QFileDialog>
#include <QLineEdit>
#include <QJsonArray>
#include <QJsonObject>

namespace railshot {

QJsonArray FiltersDialog::s_clipboard{};
bool FiltersDialog::s_hasClipboard = false;

namespace {

QString filterTypeLabel(const QString& type)
{
    if (type == QLatin1String("chroma_key")) return QStringLiteral("Chroma Key");
    if (type == QLatin1String("color_key")) return QStringLiteral("Color Key");
    if (type == QLatin1String("luma_key")) return QStringLiteral("Luma Key");
    if (type == QLatin1String("mask_alpha")) return QStringLiteral("Image Mask/Blend");
    if (type == QLatin1String("apply_lut")) return QStringLiteral("Apply LUT");
    if (type == QLatin1String("blur")) return QStringLiteral("Blur");
    if (type == QLatin1String("color_correction")) return QStringLiteral("Color Correction");
    if (type == QLatin1String("crop_pad")) return QStringLiteral("Crop/Pad");
    if (type == QLatin1String("scroll")) return QStringLiteral("Scroll");
    if (type == QLatin1String("sharpen")) return QStringLiteral("Sharpen");
    return type;
}

QJsonArray remappedFilters(const QJsonArray& in)
{
    QJsonArray out;
    for (const auto& v : in) {
        QJsonObject o = v.toObject();
        o.insert(QStringLiteral("id"), newId(QStringLiteral("flt")));
        out.append(o);
    }
    return out;
}

QString keyTypeToColor(const QString& type, const QString& customHex)
{
    if (type == QLatin1String("blue"))
        return QStringLiteral("#0000FF");
    if (type == QLatin1String("custom") && !customHex.isEmpty())
        return customHex;
    return QStringLiteral("#00FF00");
}

void flattenFiltersToSettings(QJsonObject& settings, const QJsonArray& filters)
{
    int keyMode = 0;
    int sim = 40;
    int smooth = 8;
    QString keyColor = QStringLiteral("#00FF00");
    int lumaMin = 0;
    int lumaMax = 100;
    int blur = 0;
    int bri = 0, con = 0, sat = 0, hue = 0, gamma = 0, colorOpacity = 100;
    int cropL = 0, cropR = 0, cropT = 0, cropB = 0;
    bool pad = false;
    int scrollX = 0, scrollY = 0;
    int sharpen = 0;
    QString maskPath;
    int maskOpacity = 0;
    bool maskInvert = false;
    int maskMode = 0;
    QString lutPath;
    int lutAmount = 0;

    for (const auto& v : filters) {
        const auto o = v.toObject();
        if (!o.value(QStringLiteral("enabled")).toBool(true)) continue;
        const QString type = o.value(QStringLiteral("type")).toString();
        if (type == QLatin1String("chroma_key")) {
            keyMode = 1;
            sim = o.value(QStringLiteral("similarity")).toInt(40);
            smooth = o.value(QStringLiteral("smoothness")).toInt(8);
            keyColor = keyTypeToColor(o.value(QStringLiteral("keyType")).toString(QStringLiteral("green")),
                                      o.value(QStringLiteral("keyColor")).toString());
        } else if (type == QLatin1String("color_key")) {
            keyMode = 2;
            sim = o.value(QStringLiteral("similarity")).toInt(8);
            smooth = o.value(QStringLiteral("smoothness")).toInt(5);
            keyColor = keyTypeToColor(o.value(QStringLiteral("keyType")).toString(QStringLiteral("green")),
                                      o.value(QStringLiteral("keyColor")).toString());
        } else if (type == QLatin1String("luma_key")) {
            keyMode = 3;
            lumaMin = o.value(QStringLiteral("lumaMin")).toInt(0);
            lumaMax = o.value(QStringLiteral("lumaMax")).toInt(100);
            smooth = o.value(QStringLiteral("smoothness")).toInt(5);
        } else if (type == QLatin1String("mask_alpha")) {
            maskPath = o.value(QStringLiteral("path")).toString();
            maskOpacity = o.value(QStringLiteral("opacity")).toInt(100);
            maskInvert = o.value(QStringLiteral("invert")).toBool(false);
            maskMode = o.value(QStringLiteral("mode")).toInt(0);
        } else if (type == QLatin1String("apply_lut")) {
            lutPath = o.value(QStringLiteral("path")).toString();
            lutAmount = o.value(QStringLiteral("amount")).toInt(100);
        } else if (type == QLatin1String("blur")) {
            blur = qMax(blur, o.value(QStringLiteral("amount")).toInt(0));
        } else if (type == QLatin1String("color_correction")) {
            bri = o.value(QStringLiteral("brightness")).toInt(0);
            con = o.value(QStringLiteral("contrast")).toInt(0);
            sat = o.value(QStringLiteral("saturation")).toInt(0);
            hue = o.value(QStringLiteral("hue")).toInt(0);
            gamma = o.value(QStringLiteral("gamma")).toInt(0);
            colorOpacity = o.value(QStringLiteral("opacity")).toInt(100);
        } else if (type == QLatin1String("crop_pad")) {
            cropL = o.value(QStringLiteral("left")).toInt(0);
            cropR = o.value(QStringLiteral("right")).toInt(0);
            cropT = o.value(QStringLiteral("top")).toInt(0);
            cropB = o.value(QStringLiteral("bottom")).toInt(0);
            pad = o.value(QStringLiteral("pad")).toBool(false);
        } else if (type == QLatin1String("scroll")) {
            scrollX = o.value(QStringLiteral("speedX")).toInt(0);
            scrollY = o.value(QStringLiteral("speedY")).toInt(0);
        } else if (type == QLatin1String("sharpen")) {
            sharpen = qMax(sharpen, o.value(QStringLiteral("amount")).toInt(0));
        }
    }

    settings.insert(QStringLiteral("keyMode"), keyMode);
    settings.insert(QStringLiteral("chromaKey"), keyMode == 1);
    settings.insert(QStringLiteral("chromaSimilarity"), sim);
    settings.insert(QStringLiteral("keySmoothness"), smooth);
    settings.insert(QStringLiteral("keyColor"), keyColor);
    settings.insert(QStringLiteral("lumaMin"), lumaMin);
    settings.insert(QStringLiteral("lumaMax"), lumaMax);
    settings.insert(QStringLiteral("maskPath"), maskPath);
    settings.insert(QStringLiteral("maskOpacity"), maskOpacity);
    settings.insert(QStringLiteral("maskInvert"), maskInvert);
    settings.insert(QStringLiteral("maskMode"), maskMode);
    settings.insert(QStringLiteral("lutPath"), lutPath);
    settings.insert(QStringLiteral("lutAmount"), lutAmount);
    settings.insert(QStringLiteral("blur"), blur);
    settings.insert(QStringLiteral("brightness"), bri);
    settings.insert(QStringLiteral("contrast"), con);
    settings.insert(QStringLiteral("saturation"), sat);
    settings.insert(QStringLiteral("hue"), hue);
    settings.insert(QStringLiteral("gamma"), gamma);
    settings.insert(QStringLiteral("colorOpacity"), colorOpacity);
    settings.insert(QStringLiteral("filterCropL"), cropL);
    settings.insert(QStringLiteral("filterCropR"), cropR);
    settings.insert(QStringLiteral("filterCropT"), cropT);
    settings.insert(QStringLiteral("filterCropB"), cropB);
    settings.insert(QStringLiteral("filterPad"), pad);
    settings.insert(QStringLiteral("scrollSpeedX"), scrollX);
    settings.insert(QStringLiteral("scrollSpeedY"), scrollY);
    settings.insert(QStringLiteral("sharpen"), sharpen);
}

void fillKeyTypeCombo(QComboBox* box)
{
    box->clear();
    box->addItem(QStringLiteral("Green"), QStringLiteral("green"));
    box->addItem(QStringLiteral("Blue"), QStringLiteral("blue"));
    box->addItem(QStringLiteral("Custom"), QStringLiteral("custom"));
}

} // namespace

FiltersDialog::FiltersDialog(EngineController* engine, const QString& sourceId, QWidget* parent)
    : QDialog(parent), m_engine(engine), m_sourceId(sourceId)
{
    setWindowTitle(QStringLiteral("Filters"));
    setMinimumSize(580, 440);
    setStyleSheet(QStringLiteral(
        "QDialog{background:#0F1114;}"
        "QLabel{color:#C8CCD4; font-family:'DM Sans';}"
        "QListWidget{background:#0A0C0F; border:1px solid #2A2D35; color:#E0E2E8;}"
        "QListWidget::item:selected{background:#1A2A3A; color:#7AB8FF;}"
        "QPushButton{background:#1A1E26; border:1px solid #5A5E68; border-radius:3px;"
        "  color:#E0E2E8; font-weight:700; padding:4px 10px;}"
        "QPushButton:hover{border-color:#4F9EFF;}"
        "QPushButton:disabled{color:#505860; border-color:#2A2D35;}"
        "QComboBox{background:#12151A; border:1px solid #3A3D45; color:#E0E2E8; padding:2px 6px;}"
        "QCheckBox{color:#E0E2E8;}"));

    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(10, 10, 10, 10);
    root->setSpacing(10);

    auto* left = new QVBoxLayout();
    m_list = new QListWidget(this);
    left->addWidget(m_list, 1);

    auto* tools = new QHBoxLayout();
    auto* addBtn = new QPushButton(QStringLiteral("+"), this);
    auto* remBtn = new QPushButton(QStringLiteral("\u2212"), this);
    m_upBtn = new QPushButton(QStringLiteral("▲"), this);
    m_downBtn = new QPushButton(QStringLiteral("▼"), this);
    addBtn->setFixedWidth(32);
    remBtn->setFixedWidth(32);
    m_upBtn->setFixedWidth(32);
    m_downBtn->setFixedWidth(32);
    tools->addWidget(addBtn);
    tools->addWidget(remBtn);
    tools->addSpacing(6);
    tools->addWidget(m_upBtn);
    tools->addWidget(m_downBtn);
    tools->addStretch();
    left->addLayout(tools);
    root->addLayout(left, 1);

    auto* right = new QVBoxLayout();
    m_hint = new QLabel(QStringLiteral("Select or add a filter"), this);
    right->addWidget(m_hint);

    m_stack = new QStackedWidget(this);
    m_emptyPage = new QWidget(m_stack);
    m_stack->addWidget(m_emptyPage);

    m_chromaPage = new QWidget(m_stack);
    {
        auto* form = new QFormLayout(m_chromaPage);
        m_chromaEnabled = new QCheckBox(QStringLiteral("Enabled"), m_chromaPage);
        m_chromaType = new QComboBox(m_chromaPage);
        fillKeyTypeCombo(m_chromaType);
        m_chromaColorBtn = new QPushButton(QStringLiteral("Pick color…"), m_chromaPage);
        m_chromaSim = new QSlider(Qt::Horizontal, m_chromaPage);
        m_chromaSim->setRange(5, 95);
        m_chromaSmooth = new QSlider(Qt::Horizontal, m_chromaPage);
        m_chromaSmooth->setRange(1, 40);
        form->addRow(m_chromaEnabled);
        form->addRow(QStringLiteral("Key Color Type"), m_chromaType);
        form->addRow(QStringLiteral("Custom Color"), m_chromaColorBtn);
        form->addRow(QStringLiteral("Similarity"), m_chromaSim);
        form->addRow(QStringLiteral("Smoothness"), m_chromaSmooth);
    }
    m_stack->addWidget(m_chromaPage);

    m_colorKeyPage = new QWidget(m_stack);
    {
        auto* form = new QFormLayout(m_colorKeyPage);
        m_colorKeyEnabled = new QCheckBox(QStringLiteral("Enabled"), m_colorKeyPage);
        m_colorKeyType = new QComboBox(m_colorKeyPage);
        fillKeyTypeCombo(m_colorKeyType);
        m_colorKeyColorBtn = new QPushButton(QStringLiteral("Pick color…"), m_colorKeyPage);
        m_colorKeySim = new QSlider(Qt::Horizontal, m_colorKeyPage);
        m_colorKeySim->setRange(1, 40);
        m_colorKeySmooth = new QSlider(Qt::Horizontal, m_colorKeyPage);
        m_colorKeySmooth->setRange(1, 40);
        form->addRow(m_colorKeyEnabled);
        form->addRow(QStringLiteral("Key Color Type"), m_colorKeyType);
        form->addRow(QStringLiteral("Custom Color"), m_colorKeyColorBtn);
        form->addRow(QStringLiteral("Similarity"), m_colorKeySim);
        form->addRow(QStringLiteral("Smoothness"), m_colorKeySmooth);
    }
    m_stack->addWidget(m_colorKeyPage);

    m_lumaPage = new QWidget(m_stack);
    {
        auto* form = new QFormLayout(m_lumaPage);
        m_lumaEnabled = new QCheckBox(QStringLiteral("Enabled"), m_lumaPage);
        m_lumaMin = new QSlider(Qt::Horizontal, m_lumaPage);
        m_lumaMax = new QSlider(Qt::Horizontal, m_lumaPage);
        m_lumaSmooth = new QSlider(Qt::Horizontal, m_lumaPage);
        m_lumaMin->setRange(0, 100);
        m_lumaMax->setRange(0, 100);
        m_lumaSmooth->setRange(0, 40);
        form->addRow(m_lumaEnabled);
        form->addRow(QStringLiteral("Luma Min"), m_lumaMin);
        form->addRow(QStringLiteral("Luma Max"), m_lumaMax);
        form->addRow(QStringLiteral("Smoothness"), m_lumaSmooth);
    }
    m_stack->addWidget(m_lumaPage);

    m_maskPage = new QWidget(m_stack);
    {
        auto* form = new QFormLayout(m_maskPage);
        m_maskEnabled = new QCheckBox(QStringLiteral("Enabled"), m_maskPage);
        m_maskPath = new QLineEdit(m_maskPage);
        m_maskPath->setPlaceholderText(QStringLiteral("PNG / JPG with alpha…"));
        m_maskPath->setReadOnly(true);
        auto* browse = new QPushButton(QStringLiteral("Browse…"), m_maskPage);
        connect(browse, &QPushButton::clicked, this, [this] {
            const QString path = QFileDialog::getOpenFileName(
                this, QStringLiteral("Mask Image"), QString(),
                QStringLiteral("Images (*.png *.jpg *.jpeg *.bmp *.webp);;All (*.*)"));
            if (path.isEmpty()) return;
            m_maskPath->setText(path);
            saveCurrent();
        });
        auto* pathRow = new QWidget(m_maskPage);
        auto* pathLay = new QHBoxLayout(pathRow);
        pathLay->setContentsMargins(0, 0, 0, 0);
        pathLay->addWidget(m_maskPath, 1);
        pathLay->addWidget(browse);
        m_maskOpacity = new QSlider(Qt::Horizontal, m_maskPage);
        m_maskOpacity->setRange(0, 100);
        m_maskOpacity->setValue(100);
        m_maskMode = new QComboBox(m_maskPage);
        m_maskMode->addItem(QStringLiteral("Alpha"), 0);
        m_maskMode->addItem(QStringLiteral("Color"), 1);
        m_maskMode->addItem(QStringLiteral("Multiply"), 2);
        m_maskMode->addItem(QStringLiteral("Addition"), 3);
        m_maskMode->addItem(QStringLiteral("Subtraction"), 4);
        m_maskInvert = new QCheckBox(QStringLiteral("Invert mask"), m_maskPage);
        form->addRow(m_maskEnabled);
        form->addRow(QStringLiteral("Image"), pathRow);
        form->addRow(QStringLiteral("Type"), m_maskMode);
        form->addRow(QStringLiteral("Opacity"), m_maskOpacity);
        form->addRow(m_maskInvert);
    }
    m_stack->addWidget(m_maskPage);

    m_lutPage = new QWidget(m_stack);
    {
        auto* form = new QFormLayout(m_lutPage);
        m_lutEnabled = new QCheckBox(QStringLiteral("Enabled"), m_lutPage);
        m_lutPath = new QLineEdit(m_lutPage);
        m_lutPath->setPlaceholderText(QStringLiteral("512×512 PNG or Adobe .cube…"));
        m_lutPath->setReadOnly(true);
        auto* browse = new QPushButton(QStringLiteral("Browse…"), m_lutPage);
        connect(browse, &QPushButton::clicked, this, [this] {
            const QString path = QFileDialog::getOpenFileName(
                this, QStringLiteral("Apply LUT"), QString(),
                QStringLiteral("LUT (*.cube *.png *.jpg *.jpeg *.bmp *.webp);;All (*.*)"));
            if (path.isEmpty()) return;
            m_lutPath->setText(path);
            saveCurrent();
        });
        auto* pathRow = new QWidget(m_lutPage);
        auto* pathLay = new QHBoxLayout(pathRow);
        pathLay->setContentsMargins(0, 0, 0, 0);
        pathLay->addWidget(m_lutPath, 1);
        pathLay->addWidget(browse);
        m_lutAmount = new QSlider(Qt::Horizontal, m_lutPage);
        m_lutAmount->setRange(0, 100);
        m_lutAmount->setValue(100);
        form->addRow(m_lutEnabled);
        form->addRow(QStringLiteral("LUT Image"), pathRow);
        form->addRow(QStringLiteral("Amount"), m_lutAmount);
        form->addRow(new QLabel(QStringLiteral("PNG 512×512 (8×8 tiles) or 3D .cube (baked)."), m_lutPage));
    }
    m_stack->addWidget(m_lutPage);

    m_blurPage = new QWidget(m_stack);
    {
        auto* form = new QFormLayout(m_blurPage);
        m_blurEnabled = new QCheckBox(QStringLiteral("Enabled"), m_blurPage);
        m_blurAmount = new QSlider(Qt::Horizontal, m_blurPage);
        m_blurAmount->setRange(0, 100);
        form->addRow(m_blurEnabled);
        form->addRow(QStringLiteral("Amount"), m_blurAmount);
    }
    m_stack->addWidget(m_blurPage);

    m_colorPage = new QWidget(m_stack);
    {
        auto* form = new QFormLayout(m_colorPage);
        m_colorEnabled = new QCheckBox(QStringLiteral("Enabled"), m_colorPage);
        m_brightness = new QSlider(Qt::Horizontal, m_colorPage);
        m_brightness->setRange(-100, 100);
        m_contrast = new QSlider(Qt::Horizontal, m_colorPage);
        m_contrast->setRange(-100, 100);
        m_saturation = new QSlider(Qt::Horizontal, m_colorPage);
        m_saturation->setRange(-100, 100);
        m_hue = new QSlider(Qt::Horizontal, m_colorPage);
        m_hue->setRange(-180, 180);
        m_gamma = new QSlider(Qt::Horizontal, m_colorPage);
        m_gamma->setRange(-100, 100);
        m_colorOpacity = new QSlider(Qt::Horizontal, m_colorPage);
        m_colorOpacity->setRange(0, 100);
        m_colorOpacity->setValue(100);
        form->addRow(m_colorEnabled);
        form->addRow(QStringLiteral("Brightness"), m_brightness);
        form->addRow(QStringLiteral("Contrast"), m_contrast);
        form->addRow(QStringLiteral("Saturation"), m_saturation);
        form->addRow(QStringLiteral("Hue Shift"), m_hue);
        form->addRow(QStringLiteral("Gamma"), m_gamma);
        form->addRow(QStringLiteral("Opacity"), m_colorOpacity);
    }
    m_stack->addWidget(m_colorPage);

    m_cropPage = new QWidget(m_stack);
    {
        auto* form = new QFormLayout(m_cropPage);
        m_cropEnabled = new QCheckBox(QStringLiteral("Enabled"), m_cropPage);
        m_cropL = new QSlider(Qt::Horizontal, m_cropPage);
        m_cropR = new QSlider(Qt::Horizontal, m_cropPage);
        m_cropT = new QSlider(Qt::Horizontal, m_cropPage);
        m_cropB = new QSlider(Qt::Horizontal, m_cropPage);
        for (auto* s : {m_cropL, m_cropR, m_cropT, m_cropB})
            s->setRange(0, 50);
        m_cropPad = new QCheckBox(QStringLiteral("Relative / Pad (letterbox)"), m_cropPage);
        form->addRow(m_cropEnabled);
        form->addRow(QStringLiteral("Left"), m_cropL);
        form->addRow(QStringLiteral("Right"), m_cropR);
        form->addRow(QStringLiteral("Top"), m_cropT);
        form->addRow(QStringLiteral("Bottom"), m_cropB);
        form->addRow(m_cropPad);
    }
    m_stack->addWidget(m_cropPage);

    m_scrollPage = new QWidget(m_stack);
    {
        auto* form = new QFormLayout(m_scrollPage);
        m_scrollEnabled = new QCheckBox(QStringLiteral("Enabled"), m_scrollPage);
        m_scrollX = new QSlider(Qt::Horizontal, m_scrollPage);
        m_scrollY = new QSlider(Qt::Horizontal, m_scrollPage);
        m_scrollX->setRange(-100, 100);
        m_scrollY->setRange(-100, 100);
        form->addRow(m_scrollEnabled);
        form->addRow(QStringLiteral("Speed X"), m_scrollX);
        form->addRow(QStringLiteral("Speed Y"), m_scrollY);
    }
    m_stack->addWidget(m_scrollPage);

    m_sharpenPage = new QWidget(m_stack);
    {
        auto* form = new QFormLayout(m_sharpenPage);
        m_sharpenEnabled = new QCheckBox(QStringLiteral("Enabled"), m_sharpenPage);
        m_sharpenAmount = new QSlider(Qt::Horizontal, m_sharpenPage);
        m_sharpenAmount->setRange(0, 100);
        form->addRow(m_sharpenEnabled);
        form->addRow(QStringLiteral("Amount"), m_sharpenAmount);
    }
    m_stack->addWidget(m_sharpenPage);

    right->addWidget(m_stack, 1);
    auto* closeRow = new QHBoxLayout();
    closeRow->addStretch();
    auto* closeBtn = new QPushButton(QStringLiteral("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    closeRow->addWidget(closeBtn);
    right->addLayout(closeRow);
    root->addLayout(right, 2);

    connect(addBtn, &QPushButton::clicked, this, &FiltersDialog::onAddFilter);
    connect(remBtn, &QPushButton::clicked, this, &FiltersDialog::onRemoveFilter);
    connect(m_upBtn, &QPushButton::clicked, this, &FiltersDialog::onMoveUp);
    connect(m_downBtn, &QPushButton::clicked, this, &FiltersDialog::onMoveDown);
    connect(m_list, &QListWidget::currentRowChanged, this, &FiltersDialog::onSelectionChanged);
    connect(m_chromaColorBtn, &QPushButton::clicked, this, [this] {
        m_pickingKeyFor = QStringLiteral("chroma");
        pickKeyColor();
    });
    connect(m_colorKeyColorBtn, &QPushButton::clicked, this, [this] {
        m_pickingKeyFor = QStringLiteral("color_key");
        pickKeyColor();
    });

    for (QCheckBox* c : {m_chromaEnabled, m_colorKeyEnabled, m_lumaEnabled, m_maskEnabled, m_maskInvert,
                         m_lutEnabled, m_blurEnabled, m_colorEnabled,
                         m_cropEnabled, m_cropPad, m_scrollEnabled, m_sharpenEnabled})
        connect(c, &QCheckBox::toggled, this, &FiltersDialog::saveCurrent);
    for (QSlider* s : {m_chromaSim, m_chromaSmooth, m_colorKeySim, m_colorKeySmooth,
                       m_lumaMin, m_lumaMax, m_lumaSmooth, m_maskOpacity, m_lutAmount, m_blurAmount,
                       m_brightness, m_contrast, m_saturation, m_hue, m_gamma, m_colorOpacity,
                       m_cropL, m_cropR, m_cropT, m_cropB,
                       m_scrollX, m_scrollY, m_sharpenAmount})
        connect(s, &QSlider::valueChanged, this, &FiltersDialog::saveCurrent);
    connect(m_chromaType, qOverload<int>(&QComboBox::currentIndexChanged), this, &FiltersDialog::saveCurrent);
    connect(m_colorKeyType, qOverload<int>(&QComboBox::currentIndexChanged), this, &FiltersDialog::saveCurrent);
    connect(m_maskMode, qOverload<int>(&QComboBox::currentIndexChanged), this, &FiltersDialog::saveCurrent);

    updateKeyColorButton();
    reload();
}

void FiltersDialog::copyFilters(const QJsonArray& filters)
{
    s_clipboard = filters;
    s_hasClipboard = !filters.isEmpty();
}

bool FiltersDialog::hasFilterClipboard() { return s_hasClipboard && !s_clipboard.isEmpty(); }
QJsonArray FiltersDialog::filterClipboard() { return s_clipboard; }

void FiltersDialog::pasteOnto(EngineController* engine, const QString& sourceId)
{
    if (!engine || sourceId.isEmpty() || !hasFilterClipboard()) return;
    const QJsonArray pasted = remappedFilters(s_clipboard);
    engine->sceneGraph()->mutate([&](Project& p) {
        if (auto* s = p.findSourceAnywhere(sourceId)) {
            s->settings.insert(QStringLiteral("filters"), pasted);
            flattenFiltersToSettings(s->settings, pasted);
        }
    });
}

void FiltersDialog::pickKeyColor()
{
    QColor cur = m_pickingKeyFor == QLatin1String("color_key") ? m_colorKeyCustomColor : m_chromaCustomColor;
    const QColor c = QColorDialog::getColor(cur, this, QStringLiteral("Key Color"));
    if (!c.isValid()) return;
    if (m_pickingKeyFor == QLatin1String("color_key")) {
        m_colorKeyCustomColor = c;
        if (m_colorKeyType)
            m_colorKeyType->setCurrentIndex(m_colorKeyType->findData(QStringLiteral("custom")));
    } else {
        m_chromaCustomColor = c;
        if (m_chromaType)
            m_chromaType->setCurrentIndex(m_chromaType->findData(QStringLiteral("custom")));
    }
    updateKeyColorButton();
    saveCurrent();
}

void FiltersDialog::updateKeyColorButton()
{
    auto paint = [](QPushButton* btn, const QColor& c) {
        if (!btn) return;
        btn->setStyleSheet(QStringLiteral(
            "QPushButton{background:%1; border:1px solid #5A5E68; border-radius:3px;"
            " color:%2; font-weight:700; padding:4px 10px;}")
                               .arg(c.name(), c.lightness() > 140 ? QStringLiteral("#111") : QStringLiteral("#EEE")));
        btn->setText(c.name().toUpper());
    };
    paint(m_chromaColorBtn, m_chromaCustomColor);
    paint(m_colorKeyColorBtn, m_colorKeyCustomColor);
}

void FiltersDialog::reload()
{
    m_loading = true;
    m_list->clear();
    const auto* src = m_engine->projectSnapshot().findSourceAnywhere(m_sourceId);
    if (!src) {
        m_loading = false;
        return;
    }
    setWindowTitle(QStringLiteral("Filters — %1").arg(src->name));

    QJsonArray filters = src->settings.value(QStringLiteral("filters")).toArray();
    if (filters.isEmpty()) {
        if (src->settings.value(QStringLiteral("chromaKey")).toBool()
            || src->settings.value(QStringLiteral("keyMode")).toInt(0) == 1) {
            QJsonObject f;
            f.insert(QStringLiteral("id"), newId(QStringLiteral("flt")));
            f.insert(QStringLiteral("type"), QStringLiteral("chroma_key"));
            f.insert(QStringLiteral("enabled"), true);
            f.insert(QStringLiteral("similarity"),
                     src->settings.value(QStringLiteral("chromaSimilarity")).toInt(40));
            f.insert(QStringLiteral("smoothness"),
                     src->settings.value(QStringLiteral("keySmoothness")).toInt(8));
            f.insert(QStringLiteral("keyType"), QStringLiteral("green"));
            f.insert(QStringLiteral("keyColor"),
                     src->settings.value(QStringLiteral("keyColor")).toString(QStringLiteral("#00FF00")));
            filters.append(f);
        }
        if (src->settings.value(QStringLiteral("blur")).toInt(0) > 0) {
            QJsonObject f;
            f.insert(QStringLiteral("id"), newId(QStringLiteral("flt")));
            f.insert(QStringLiteral("type"), QStringLiteral("blur"));
            f.insert(QStringLiteral("enabled"), true);
            f.insert(QStringLiteral("amount"), src->settings.value(QStringLiteral("blur")).toInt(0));
            filters.append(f);
        }
        const int bri = src->settings.value(QStringLiteral("brightness")).toInt(0);
        const int con = src->settings.value(QStringLiteral("contrast")).toInt(0);
        const int sat = src->settings.value(QStringLiteral("saturation")).toInt(0);
        const int hue = src->settings.value(QStringLiteral("hue")).toInt(0);
        const int gamma = src->settings.value(QStringLiteral("gamma")).toInt(0);
        const int colorOp = src->settings.value(QStringLiteral("colorOpacity")).toInt(100);
        if (bri != 0 || con != 0 || sat != 0 || hue != 0 || gamma != 0 || colorOp != 100) {
            QJsonObject f;
            f.insert(QStringLiteral("id"), newId(QStringLiteral("flt")));
            f.insert(QStringLiteral("type"), QStringLiteral("color_correction"));
            f.insert(QStringLiteral("enabled"), true);
            f.insert(QStringLiteral("brightness"), bri);
            f.insert(QStringLiteral("contrast"), con);
            f.insert(QStringLiteral("saturation"), sat);
            f.insert(QStringLiteral("hue"), hue);
            f.insert(QStringLiteral("gamma"), gamma);
            f.insert(QStringLiteral("opacity"), colorOp);
            filters.append(f);
        }
        if (!filters.isEmpty()) {
            m_engine->sceneGraph()->mutate([this, filters](Project& p) {
                if (auto* s = p.findSourceAnywhere(m_sourceId))
                    s->settings.insert(QStringLiteral("filters"), filters);
            });
        }
    }

    for (const auto& v : filters) {
        const auto o = v.toObject();
        const QString type = o.value(QStringLiteral("type")).toString();
        const bool en = o.value(QStringLiteral("enabled")).toBool(true);
        auto* item = new QListWidgetItem(
            QStringLiteral("%1%2").arg(en ? QString() : QStringLiteral("[off] "), filterTypeLabel(type)),
            m_list);
        item->setData(Qt::UserRole, o.value(QStringLiteral("id")).toString());
        item->setData(Qt::UserRole + 1, type);
        item->setCheckState(en ? Qt::Checked : Qt::Unchecked);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    }
    m_loading = false;

    disconnect(m_list, &QListWidget::itemChanged, nullptr, nullptr);
    connect(m_list, &QListWidget::itemChanged, this, [this](QListWidgetItem* item) {
        if (m_loading || !item) return;
        const QString id = item->data(Qt::UserRole).toString();
        const bool en = item->checkState() == Qt::Checked;
        m_engine->sceneGraph()->mutate([this, id, en](Project& p) {
            if (auto* s = p.findSourceAnywhere(m_sourceId)) {
                auto arr = s->settings.value(QStringLiteral("filters")).toArray();
                for (int i = 0; i < arr.size(); ++i) {
                    auto o = arr.at(i).toObject();
                    if (o.value(QStringLiteral("id")).toString() != id) continue;
                    o.insert(QStringLiteral("enabled"), en);
                    arr.replace(i, o);
                    break;
                }
                s->settings.insert(QStringLiteral("filters"), arr);
            }
        });
        syncLegacyKeys();
        const int keep = m_list->row(item);
        reload();
        if (keep >= 0 && keep < m_list->count())
            m_list->setCurrentRow(keep);
    });

    if (m_list->count() > 0)
        m_list->setCurrentRow(0);
    else
        onSelectionChanged();
}

void FiltersDialog::onAddFilter()
{
    QMenu menu(this);
    auto* color = menu.addAction(QStringLiteral("Color Correction"));
    auto* chroma = menu.addAction(QStringLiteral("Chroma Key"));
    auto* colorKey = menu.addAction(QStringLiteral("Color Key"));
    auto* luma = menu.addAction(QStringLiteral("Luma Key"));
    auto* mask = menu.addAction(QStringLiteral("Image Mask/Blend"));
    auto* lut = menu.addAction(QStringLiteral("Apply LUT"));
    auto* blur = menu.addAction(QStringLiteral("Blur"));
    menu.addSeparator();
    auto* crop = menu.addAction(QStringLiteral("Crop/Pad"));
    auto* scroll = menu.addAction(QStringLiteral("Scroll"));
    auto* sharpen = menu.addAction(QStringLiteral("Sharpen"));
    auto* chosen = menu.exec(QCursor::pos());
    if (!chosen) return;

    QJsonObject f;
    f.insert(QStringLiteral("id"), newId(QStringLiteral("flt")));
    f.insert(QStringLiteral("enabled"), true);
    if (chosen == color) {
        f.insert(QStringLiteral("type"), QStringLiteral("color_correction"));
        f.insert(QStringLiteral("brightness"), 0);
        f.insert(QStringLiteral("contrast"), 0);
        f.insert(QStringLiteral("saturation"), 0);
        f.insert(QStringLiteral("hue"), 0);
        f.insert(QStringLiteral("gamma"), 0);
        f.insert(QStringLiteral("opacity"), 100);
    } else if (chosen == chroma) {
        f.insert(QStringLiteral("type"), QStringLiteral("chroma_key"));
        f.insert(QStringLiteral("similarity"), 40);
        f.insert(QStringLiteral("smoothness"), 8);
        f.insert(QStringLiteral("keyType"), QStringLiteral("green"));
        f.insert(QStringLiteral("keyColor"), QStringLiteral("#00FF00"));
    } else if (chosen == colorKey) {
        f.insert(QStringLiteral("type"), QStringLiteral("color_key"));
        f.insert(QStringLiteral("similarity"), 8);
        f.insert(QStringLiteral("smoothness"), 5);
        f.insert(QStringLiteral("keyType"), QStringLiteral("green"));
        f.insert(QStringLiteral("keyColor"), QStringLiteral("#00FF00"));
    } else if (chosen == luma) {
        f.insert(QStringLiteral("type"), QStringLiteral("luma_key"));
        f.insert(QStringLiteral("lumaMin"), 0);
        f.insert(QStringLiteral("lumaMax"), 100);
        f.insert(QStringLiteral("smoothness"), 5);
    } else if (chosen == mask) {
        f.insert(QStringLiteral("type"), QStringLiteral("mask_alpha"));
        f.insert(QStringLiteral("path"), QString());
        f.insert(QStringLiteral("opacity"), 100);
        f.insert(QStringLiteral("invert"), false);
        f.insert(QStringLiteral("mode"), 0);
    } else if (chosen == lut) {
        f.insert(QStringLiteral("type"), QStringLiteral("apply_lut"));
        f.insert(QStringLiteral("path"), QString());
        f.insert(QStringLiteral("amount"), 100);
    } else if (chosen == blur) {
        f.insert(QStringLiteral("type"), QStringLiteral("blur"));
        f.insert(QStringLiteral("amount"), 25);
    } else if (chosen == crop) {
        f.insert(QStringLiteral("type"), QStringLiteral("crop_pad"));
        f.insert(QStringLiteral("left"), 0);
        f.insert(QStringLiteral("right"), 0);
        f.insert(QStringLiteral("top"), 0);
        f.insert(QStringLiteral("bottom"), 0);
        f.insert(QStringLiteral("pad"), false);
    } else if (chosen == scroll) {
        f.insert(QStringLiteral("type"), QStringLiteral("scroll"));
        f.insert(QStringLiteral("speedX"), 10);
        f.insert(QStringLiteral("speedY"), 0);
    } else {
        f.insert(QStringLiteral("type"), QStringLiteral("sharpen"));
        f.insert(QStringLiteral("amount"), 35);
    }

    m_engine->sceneGraph()->mutate([this, f](Project& p) {
        if (auto* s = p.findSourceAnywhere(m_sourceId)) {
            auto arr = s->settings.value(QStringLiteral("filters")).toArray();
            arr.append(f);
            s->settings.insert(QStringLiteral("filters"), arr);
        }
    });
    syncLegacyKeys();
    reload();
    m_list->setCurrentRow(m_list->count() - 1);
}

void FiltersDialog::onRemoveFilter()
{
    if (!m_list->currentItem()) return;
    const QString id = m_list->currentItem()->data(Qt::UserRole).toString();
    m_engine->sceneGraph()->mutate([this, id](Project& p) {
        if (auto* s = p.findSourceAnywhere(m_sourceId)) {
            auto arr = s->settings.value(QStringLiteral("filters")).toArray();
            QJsonArray next;
            for (const auto& v : arr) {
                if (v.toObject().value(QStringLiteral("id")).toString() != id)
                    next.append(v);
            }
            s->settings.insert(QStringLiteral("filters"), next);
        }
    });
    syncLegacyKeys();
    reload();
}

void FiltersDialog::onMoveUp() { moveFilter(-1); }
void FiltersDialog::onMoveDown() { moveFilter(1); }

void FiltersDialog::moveFilter(int delta)
{
    const int row = m_list->currentRow();
    if (row < 0) return;
    const int dest = row + delta;
    if (dest < 0 || dest >= m_list->count()) return;
    const QString id = m_list->item(row)->data(Qt::UserRole).toString();

    m_engine->sceneGraph()->mutate([this, id, delta](Project& p) {
        if (auto* s = p.findSourceAnywhere(m_sourceId)) {
            auto arr = s->settings.value(QStringLiteral("filters")).toArray();
            int idx = -1;
            for (int i = 0; i < arr.size(); ++i) {
                if (arr.at(i).toObject().value(QStringLiteral("id")).toString() == id) {
                    idx = i;
                    break;
                }
            }
            if (idx < 0) return;
            const int destIdx = idx + delta;
            if (destIdx < 0 || destIdx >= arr.size()) return;
            const QJsonValue a = arr.at(idx);
            const QJsonValue b = arr.at(destIdx);
            arr.replace(idx, b);
            arr.replace(destIdx, a);
            s->settings.insert(QStringLiteral("filters"), arr);
        }
    });
    reload();
    m_list->setCurrentRow(dest);
}

void FiltersDialog::onSelectionChanged()
{
    m_loading = true;
    const int row = m_list->currentRow();
    m_upBtn->setEnabled(row > 0);
    m_downBtn->setEnabled(row >= 0 && row < m_list->count() - 1);
    if (row < 0) {
        m_stack->setCurrentWidget(m_emptyPage);
        m_hint->setText(QStringLiteral("Select or add a filter"));
        m_loading = false;
        return;
    }
    const QString type = m_list->item(row)->data(Qt::UserRole + 1).toString();
    const QString id = m_list->item(row)->data(Qt::UserRole).toString();
    const auto* src = m_engine->projectSnapshot().findSourceAnywhere(m_sourceId);
    QJsonObject f;
    if (src) {
        for (const auto& v : src->settings.value(QStringLiteral("filters")).toArray()) {
            if (v.toObject().value(QStringLiteral("id")).toString() == id) {
                f = v.toObject();
                break;
            }
        }
    }
    m_hint->setText(filterTypeLabel(type));
    if (type == QLatin1String("chroma_key")) {
        m_chromaEnabled->setChecked(f.value(QStringLiteral("enabled")).toBool(true));
        const QString kt = f.value(QStringLiteral("keyType")).toString(QStringLiteral("green"));
        const int idx = m_chromaType->findData(kt);
        m_chromaType->setCurrentIndex(idx >= 0 ? idx : 0);
        m_chromaCustomColor = QColor(f.value(QStringLiteral("keyColor")).toString(QStringLiteral("#00FF00")));
        if (!m_chromaCustomColor.isValid())
            m_chromaCustomColor = QColor(0, 255, 0);
        m_chromaSim->setValue(f.value(QStringLiteral("similarity")).toInt(40));
        m_chromaSmooth->setValue(f.value(QStringLiteral("smoothness")).toInt(8));
        updateKeyColorButton();
        m_stack->setCurrentWidget(m_chromaPage);
    } else if (type == QLatin1String("color_key")) {
        m_colorKeyEnabled->setChecked(f.value(QStringLiteral("enabled")).toBool(true));
        const QString kt = f.value(QStringLiteral("keyType")).toString(QStringLiteral("green"));
        const int idx = m_colorKeyType->findData(kt);
        m_colorKeyType->setCurrentIndex(idx >= 0 ? idx : 0);
        m_colorKeyCustomColor = QColor(f.value(QStringLiteral("keyColor")).toString(QStringLiteral("#00FF00")));
        if (!m_colorKeyCustomColor.isValid())
            m_colorKeyCustomColor = QColor(0, 255, 0);
        m_colorKeySim->setValue(f.value(QStringLiteral("similarity")).toInt(8));
        m_colorKeySmooth->setValue(f.value(QStringLiteral("smoothness")).toInt(5));
        updateKeyColorButton();
        m_stack->setCurrentWidget(m_colorKeyPage);
    } else if (type == QLatin1String("luma_key")) {
        m_lumaEnabled->setChecked(f.value(QStringLiteral("enabled")).toBool(true));
        m_lumaMin->setValue(f.value(QStringLiteral("lumaMin")).toInt(0));
        m_lumaMax->setValue(f.value(QStringLiteral("lumaMax")).toInt(100));
        m_lumaSmooth->setValue(f.value(QStringLiteral("smoothness")).toInt(5));
        m_stack->setCurrentWidget(m_lumaPage);
    } else if (type == QLatin1String("mask_alpha")) {
        m_maskEnabled->setChecked(f.value(QStringLiteral("enabled")).toBool(true));
        m_maskPath->setText(f.value(QStringLiteral("path")).toString());
        m_maskOpacity->setValue(f.value(QStringLiteral("opacity")).toInt(100));
        m_maskInvert->setChecked(f.value(QStringLiteral("invert")).toBool(false));
        const int mode = f.value(QStringLiteral("mode")).toInt(0);
        const int idx = m_maskMode->findData(mode);
        m_maskMode->setCurrentIndex(idx >= 0 ? idx : 0);
        m_stack->setCurrentWidget(m_maskPage);
    } else if (type == QLatin1String("apply_lut")) {
        m_lutEnabled->setChecked(f.value(QStringLiteral("enabled")).toBool(true));
        m_lutPath->setText(f.value(QStringLiteral("path")).toString());
        m_lutAmount->setValue(f.value(QStringLiteral("amount")).toInt(100));
        m_stack->setCurrentWidget(m_lutPage);
    } else if (type == QLatin1String("blur")) {
        m_blurEnabled->setChecked(f.value(QStringLiteral("enabled")).toBool(true));
        m_blurAmount->setValue(f.value(QStringLiteral("amount")).toInt(0));
        m_stack->setCurrentWidget(m_blurPage);
    } else if (type == QLatin1String("color_correction")) {
        m_colorEnabled->setChecked(f.value(QStringLiteral("enabled")).toBool(true));
        m_brightness->setValue(f.value(QStringLiteral("brightness")).toInt(0));
        m_contrast->setValue(f.value(QStringLiteral("contrast")).toInt(0));
        m_saturation->setValue(f.value(QStringLiteral("saturation")).toInt(0));
        m_hue->setValue(f.value(QStringLiteral("hue")).toInt(0));
        m_gamma->setValue(f.value(QStringLiteral("gamma")).toInt(0));
        m_colorOpacity->setValue(f.value(QStringLiteral("opacity")).toInt(100));
        m_stack->setCurrentWidget(m_colorPage);
    } else if (type == QLatin1String("crop_pad")) {
        m_cropEnabled->setChecked(f.value(QStringLiteral("enabled")).toBool(true));
        m_cropL->setValue(f.value(QStringLiteral("left")).toInt(0));
        m_cropR->setValue(f.value(QStringLiteral("right")).toInt(0));
        m_cropT->setValue(f.value(QStringLiteral("top")).toInt(0));
        m_cropB->setValue(f.value(QStringLiteral("bottom")).toInt(0));
        m_cropPad->setChecked(f.value(QStringLiteral("pad")).toBool(false));
        m_stack->setCurrentWidget(m_cropPage);
    } else if (type == QLatin1String("scroll")) {
        m_scrollEnabled->setChecked(f.value(QStringLiteral("enabled")).toBool(true));
        m_scrollX->setValue(f.value(QStringLiteral("speedX")).toInt(0));
        m_scrollY->setValue(f.value(QStringLiteral("speedY")).toInt(0));
        m_stack->setCurrentWidget(m_scrollPage);
    } else if (type == QLatin1String("sharpen")) {
        m_sharpenEnabled->setChecked(f.value(QStringLiteral("enabled")).toBool(true));
        m_sharpenAmount->setValue(f.value(QStringLiteral("amount")).toInt(0));
        m_stack->setCurrentWidget(m_sharpenPage);
    } else {
        m_stack->setCurrentWidget(m_emptyPage);
    }
    m_loading = false;
}

void FiltersDialog::saveCurrent()
{
    if (m_loading) return;
    const int row = m_list->currentRow();
    if (row < 0) return;
    const QString id = m_list->item(row)->data(Qt::UserRole).toString();
    const QString type = m_list->item(row)->data(Qt::UserRole + 1).toString();

    m_engine->sceneGraph()->mutate([this, id, type](Project& p) {
        if (auto* s = p.findSourceAnywhere(m_sourceId)) {
            auto arr = s->settings.value(QStringLiteral("filters")).toArray();
            for (int i = 0; i < arr.size(); ++i) {
                auto o = arr.at(i).toObject();
                if (o.value(QStringLiteral("id")).toString() != id) continue;
                if (type == QLatin1String("chroma_key")) {
                    o.insert(QStringLiteral("enabled"), m_chromaEnabled->isChecked());
                    o.insert(QStringLiteral("similarity"), m_chromaSim->value());
                    o.insert(QStringLiteral("smoothness"), m_chromaSmooth->value());
                    o.insert(QStringLiteral("keyType"), m_chromaType->currentData().toString());
                    o.insert(QStringLiteral("keyColor"), m_chromaCustomColor.name(QColor::HexRgb));
                } else if (type == QLatin1String("color_key")) {
                    o.insert(QStringLiteral("enabled"), m_colorKeyEnabled->isChecked());
                    o.insert(QStringLiteral("similarity"), m_colorKeySim->value());
                    o.insert(QStringLiteral("smoothness"), m_colorKeySmooth->value());
                    o.insert(QStringLiteral("keyType"), m_colorKeyType->currentData().toString());
                    o.insert(QStringLiteral("keyColor"), m_colorKeyCustomColor.name(QColor::HexRgb));
                } else if (type == QLatin1String("luma_key")) {
                    o.insert(QStringLiteral("enabled"), m_lumaEnabled->isChecked());
                    o.insert(QStringLiteral("lumaMin"), m_lumaMin->value());
                    o.insert(QStringLiteral("lumaMax"), m_lumaMax->value());
                    o.insert(QStringLiteral("smoothness"), m_lumaSmooth->value());
                } else if (type == QLatin1String("mask_alpha")) {
                    o.insert(QStringLiteral("enabled"), m_maskEnabled->isChecked());
                    o.insert(QStringLiteral("path"), m_maskPath->text());
                    o.insert(QStringLiteral("opacity"), m_maskOpacity->value());
                    o.insert(QStringLiteral("invert"), m_maskInvert->isChecked());
                    o.insert(QStringLiteral("mode"), m_maskMode->currentData().toInt());
                } else if (type == QLatin1String("apply_lut")) {
                    o.insert(QStringLiteral("enabled"), m_lutEnabled->isChecked());
                    o.insert(QStringLiteral("path"), m_lutPath->text());
                    o.insert(QStringLiteral("amount"), m_lutAmount->value());
                } else if (type == QLatin1String("blur")) {
                    o.insert(QStringLiteral("enabled"), m_blurEnabled->isChecked());
                    o.insert(QStringLiteral("amount"), m_blurAmount->value());
                } else if (type == QLatin1String("color_correction")) {
                    o.insert(QStringLiteral("enabled"), m_colorEnabled->isChecked());
                    o.insert(QStringLiteral("brightness"), m_brightness->value());
                    o.insert(QStringLiteral("contrast"), m_contrast->value());
                    o.insert(QStringLiteral("saturation"), m_saturation->value());
                    o.insert(QStringLiteral("hue"), m_hue->value());
                    o.insert(QStringLiteral("gamma"), m_gamma->value());
                    o.insert(QStringLiteral("opacity"), m_colorOpacity->value());
                } else if (type == QLatin1String("crop_pad")) {
                    o.insert(QStringLiteral("enabled"), m_cropEnabled->isChecked());
                    o.insert(QStringLiteral("left"), m_cropL->value());
                    o.insert(QStringLiteral("right"), m_cropR->value());
                    o.insert(QStringLiteral("top"), m_cropT->value());
                    o.insert(QStringLiteral("bottom"), m_cropB->value());
                    o.insert(QStringLiteral("pad"), m_cropPad->isChecked());
                } else if (type == QLatin1String("scroll")) {
                    o.insert(QStringLiteral("enabled"), m_scrollEnabled->isChecked());
                    o.insert(QStringLiteral("speedX"), m_scrollX->value());
                    o.insert(QStringLiteral("speedY"), m_scrollY->value());
                } else if (type == QLatin1String("sharpen")) {
                    o.insert(QStringLiteral("enabled"), m_sharpenEnabled->isChecked());
                    o.insert(QStringLiteral("amount"), m_sharpenAmount->value());
                }
                arr.replace(i, o);
                break;
            }
            s->settings.insert(QStringLiteral("filters"), arr);
        }
    });
    syncLegacyKeys();
    const int keep = row;
    reload();
    if (keep >= 0 && keep < m_list->count())
        m_list->setCurrentRow(keep);
}

void FiltersDialog::syncLegacyKeys()
{
    m_engine->sceneGraph()->mutate([this](Project& p) {
        if (auto* s = p.findSourceAnywhere(m_sourceId))
            flattenFiltersToSettings(s->settings, s->settings.value(QStringLiteral("filters")).toArray());
    });
}

} // namespace railshot
