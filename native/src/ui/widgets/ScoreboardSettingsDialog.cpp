#include "ui/widgets/ScoreboardSettingsDialog.h"
#include "core/EngineController.h"
#include "scoreboard/ScoreboardModel.h"
#include "capture/OverlayRenderer.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QButtonGroup>
#include <QAbstractButton>
#include <QDialogButtonBox>
#include <QColorDialog>
#include <QFrame>
#include <QPixmap>
#include <QJsonObject>

namespace railshot {

namespace {

struct BoardPreset {
    const char* id;
    const char* name;
    const char* blurb;
    const char* layout;
    const char* theme;
    const char* colorA;
    const char* colorB;
    const char* textColor; // empty = theme default
    const char* bgColor;
    const char* sport;
};

const BoardPreset kPresets[] = {
    {"railshot", "RailShot Dark", "Lower third · signature orange / blue",
     "standard", "railshot", "#FF5A2C", "#4F9EFF", "", "", "8ball"},
    {"broadcast", "Broadcast Navy", "Full-width bar · deep broadcast look",
     "wide", "broadcast", "#F97316", "#38BDF8", "#F8FAFC", "#081428", "generic"},
    {"neon", "Neon Night", "Glow edges · magenta / cyan clash",
     "standard", "neon", "#FF00AA", "#00F0FF", "#FFFFFF", "#080414", "custom"},
    {"classic", "Clean Light", "Light board · crisp dark type",
     "standard", "classic", "#C2410C", "#1D4ED8", "#14161C", "#F5F5F8", "generic"},
    {"corners", "Corner Badges", "Top corners · compact dual badges",
     "compact", "railshot", "#FF5A2C", "#4F9EFF", "", "", "8ball"},
    {"esports", "Esports Dual", "Wide neon · purple / green",
     "wide", "neon", "#A855F7", "#22C55E", "#F8FAFC", "#0A0614", "custom"},
    {"pool", "Pool Hall", "Felt green · amber challenger",
     "standard", "railshot", "#15803D", "#B45309", "#F0FDF4", "#0C1410", "8ball"},
    {"court", "Court Side", "Basketball · orange / royal",
     "wide", "broadcast", "#EA580C", "#2563EB", "#FFFFFF", "#0B1220", "basketball"},
    {"pitch", "Pitch Night", "Soccer · green / white",
     "standard", "broadcast", "#16A34A", "#E2E8F0", "#F8FAFC", "#052E16", "soccer"},
    {"center", "Center Clash", "Mid-screen banner · duel focus",
     "center", "railshot", "#FF5A2C", "#4F9EFF", "", "", "generic"},
    {"carbon", "Carbon Edge", "Matte dark · steel accents",
     "wide", "carbon", "#94A3B8", "#64748B", "#E2E8F0", "#121214", "generic"},
    {"gold", "Championship Gold", "Premium gold on deep bronze",
     "center", "gold", "#F59E0B", "#D97706", "#FFECB3", "#1C160A", "tennis"},
};

QString mapSportUiToModel(const QString& ui)
{
    if (ui == QLatin1String("Pool")) return QStringLiteral("8ball");
    if (ui == QLatin1String("Basketball")) return QStringLiteral("basketball");
    if (ui == QLatin1String("Soccer")) return QStringLiteral("soccer");
    if (ui == QLatin1String("Tennis")) return QStringLiteral("tennis");
    if (ui == QLatin1String("Custom")) return QStringLiteral("custom");
    return QStringLiteral("generic");
}
QString mapSportModelToUi(const QString& model)
{
    if (model == QLatin1String("8ball")) return QStringLiteral("Pool");
    if (model == QLatin1String("basketball")) return QStringLiteral("Basketball");
    if (model == QLatin1String("soccer")) return QStringLiteral("Soccer");
    if (model == QLatin1String("tennis")) return QStringLiteral("Tennis");
    if (model == QLatin1String("custom")) return QStringLiteral("Custom");
    return QStringLiteral("Generic");
}
QString mapThemeUiToModel(const QString& ui)
{
    if (ui == QLatin1String("Light")) return QStringLiteral("classic");
    if (ui == QLatin1String("Broadcast")) return QStringLiteral("broadcast");
    if (ui == QLatin1String("Neon")) return QStringLiteral("neon");
    if (ui == QLatin1String("Carbon")) return QStringLiteral("carbon");
    if (ui == QLatin1String("Gold")) return QStringLiteral("gold");
    return QStringLiteral("railshot");
}
QString mapThemeModelToUi(const QString& model)
{
    if (model == QLatin1String("classic")) return QStringLiteral("Light");
    if (model == QLatin1String("broadcast")) return QStringLiteral("Broadcast");
    if (model == QLatin1String("neon")) return QStringLiteral("Neon");
    if (model == QLatin1String("carbon")) return QStringLiteral("Carbon");
    if (model == QLatin1String("gold")) return QStringLiteral("Gold");
    return QStringLiteral("Dark");
}
QString mapLayoutUiToModel(const QString& ui)
{
    if (ui == QLatin1String("Center")) return QStringLiteral("center");
    if (ui == QLatin1String("Corner")) return QStringLiteral("compact");
    if (ui == QLatin1String("Full")) return QStringLiteral("wide");
    return QStringLiteral("standard");
}
QString mapLayoutModelToUi(const QString& model)
{
    if (model == QLatin1String("compact")) return QStringLiteral("Corner");
    if (model == QLatin1String("wide")) return QStringLiteral("Full");
    if (model == QLatin1String("center")) return QStringLiteral("Center");
    return QStringLiteral("Lower Third");
}

QString colorCss(const QColor& c)
{
    return c.name(QColor::HexRgb);
}

} // namespace

void ScoreboardSettingsDialog::applyColorButton(QPushButton* btn, const QColor& c)
{
    const QString hex = colorCss(c);
    const QColor contrast = (c.lightness() > 140) ? QColor(20, 22, 28) : QColor(240, 242, 246);
    btn->setText(hex.toUpper());
    btn->setStyleSheet(QStringLiteral(
        "QPushButton{background:%1; border:1px solid #5A5E68; border-radius:4px;"
        "  color:%2; font-family:'JetBrains Mono'; font-size:11px; font-weight:700;"
        "  padding:8px 10px; min-height:28px;}"
        "QPushButton:hover{border-color:#4F9EFF;}")
                           .arg(hex, contrast.name()));
}

QColor ScoreboardSettingsDialog::pickColor(const QColor& current, const QString& title)
{
    const QColor picked = QColorDialog::getColor(current.isValid() ? current : Qt::white, this, title,
                                                 QColorDialog::ShowAlphaChannel);
    return picked.isValid() ? picked : current;
}

void ScoreboardSettingsDialog::refreshPreview()
{
    if (!m_preview || !m_engine)
        return;
    auto* model = m_engine->scoreboard();
    const auto live = model->state();

    QJsonObject st;
    st.insert(QStringLiteral("playerA"), live.playerA);
    st.insert(QStringLiteral("playerB"), live.playerB);
    st.insert(QStringLiteral("scoreA"), live.scoreA);
    st.insert(QStringLiteral("scoreB"), live.scoreB);
    st.insert(QStringLiteral("clockSeconds"), live.clockSeconds);
    st.insert(QStringLiteral("layout"), mapLayoutUiToModel(m_layoutBox->currentText()));
    st.insert(QStringLiteral("theme"), mapThemeUiToModel(m_themeBox->currentText()));
    st.insert(QStringLiteral("colorA"), colorCss(m_colorA));
    st.insert(QStringLiteral("colorB"), colorCss(m_colorB));
    if (m_useCustomText)
        st.insert(QStringLiteral("textColor"), colorCss(m_textColor));
    if (m_useCustomBg)
        st.insert(QStringLiteral("bgColor"), colorCss(m_bgColor));

    OverlayRenderer renderer;
    const QImage img = renderer.renderScoreboard(st, 640, 360);
    m_preview->setPixmap(QPixmap::fromImage(img).scaled(m_preview->size(), Qt::KeepAspectRatio,
                                                        Qt::SmoothTransformation));
}

ScoreboardSettingsDialog::ScoreboardSettingsDialog(EngineController* engine, QWidget* parent)
    : QDialog(parent), m_engine(engine)
{
    setWindowTitle(QStringLiteral("Scoreboard Settings"));
    setMinimumSize(640, 720);
    resize(680, 780);
    setStyleSheet(QStringLiteral(
        "QDialog{background:#0F1114;}"
        "QLabel{color:#C8CCD4; font-family:'DM Sans'; font-size:11px;}"
        "QLabel#sec{color:#22C55E; font-size:10px; font-weight:900; letter-spacing:1px;}"
        "QLabel#hint{color:#7A8290; font-size:10px;}"
        "QComboBox{"
        "  background:#12151A; border:1px solid #3A3D45; border-radius:3px;"
        "  color:#E0E2E8; padding:4px 8px; min-height:24px;}"
        "QComboBox:focus{border-color:#22C55E;}"
        "QScrollArea{border:none; background:transparent;}"
        "QPushButton{background:#1A1E26; border:1px solid #5A5E68; border-radius:3px;"
        "  color:#E0E2E8; font-weight:700; padding:6px 14px;}"
        "QPushButton:hover{border-color:#4F9EFF;}"));

    auto* model = engine->scoreboard();
    const auto st0 = model->state();
    m_colorA = QColor(st0.colorA);
    m_colorB = QColor(st0.colorB);
    if (!m_colorA.isValid()) m_colorA = QColor(QStringLiteral("#FF5A2C"));
    if (!m_colorB.isValid()) m_colorB = QColor(QStringLiteral("#4F9EFF"));
    m_useCustomText = !st0.textColor.isEmpty();
    m_useCustomBg = !st0.bgColor.isEmpty();
    m_textColor = m_useCustomText ? QColor(st0.textColor) : QColor(QStringLiteral("#FFFFFF"));
    m_bgColor = m_useCustomBg ? QColor(st0.bgColor) : QColor(QStringLiteral("#0A0C10"));
    if (!m_textColor.isValid()) m_textColor = QColor(Qt::white);
    if (!m_bgColor.isValid()) m_bgColor = QColor(10, 12, 16);

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto* body = new QWidget(scroll);
    auto* root = new QVBoxLayout(body);
    root->setContentsMargins(16, 16, 16, 12);
    root->setSpacing(12);
    scroll->setWidget(body);
    outer->addWidget(scroll, 1);

    // —— Live preview ——
    auto* prevLab = new QLabel(QStringLiteral("LIVE PREVIEW"), body);
    prevLab->setObjectName(QStringLiteral("sec"));
    root->addWidget(prevLab);

    auto* prevFrame = new QFrame(body);
    prevFrame->setStyleSheet(QStringLiteral(
        "QFrame{background:#080A0C; border:1px solid #2A2E36; border-radius:6px;}"));
    auto* prevLay = new QVBoxLayout(prevFrame);
    prevLay->setContentsMargins(8, 8, 8, 8);
    m_preview = new QLabel(prevFrame);
    m_preview->setMinimumHeight(180);
    m_preview->setAlignment(Qt::AlignCenter);
    m_preview->setStyleSheet(QStringLiteral(
        "background:#1a1a1a; border-radius:4px;"));
    // Checker-ish dark so transparent areas read clearly
    m_preview->setStyleSheet(QStringLiteral(
        "background:qlineargradient(x1:0,y1:0,x2:1,y2:1,"
        "stop:0 #1a1d22, stop:0.5 #12151a, stop:1 #1a1d22); border-radius:4px;"));
    prevLay->addWidget(m_preview);
    root->addWidget(prevFrame);

    // —— Presets ——
    auto* presetLab = new QLabel(QStringLiteral("SCOREBOARD PRESETS"), body);
    presetLab->setObjectName(QStringLiteral("sec"));
    root->addWidget(presetLab);
    auto* presetHint = new QLabel(QStringLiteral("Pick a look — then tweak colors below."), body);
    presetHint->setObjectName(QStringLiteral("hint"));
    root->addWidget(presetHint);

    auto* presetGrid = new QGridLayout();
    presetGrid->setSpacing(8);
    m_presetGroup = new QButtonGroup(this);
    m_presetGroup->setExclusive(true);

    const int cols = 3;
    for (int i = 0; i < int(sizeof(kPresets) / sizeof(kPresets[0])); ++i) {
        const auto& pr = kPresets[i];
        auto* card = new QPushButton(body);
        card->setCheckable(true);
        card->setCursor(Qt::PointingHandCursor);
        card->setProperty("presetIndex", i);
        card->setMinimumHeight(88);

        // Thumbnail
        QJsonObject thumbSt;
        thumbSt.insert(QStringLiteral("playerA"), QStringLiteral("HOME"));
        thumbSt.insert(QStringLiteral("playerB"), QStringLiteral("AWAY"));
        thumbSt.insert(QStringLiteral("scoreA"), 3);
        thumbSt.insert(QStringLiteral("scoreB"), 2);
        thumbSt.insert(QStringLiteral("clockSeconds"), 125);
        thumbSt.insert(QStringLiteral("layout"), QString::fromUtf8(pr.layout));
        thumbSt.insert(QStringLiteral("theme"), QString::fromUtf8(pr.theme));
        thumbSt.insert(QStringLiteral("colorA"), QString::fromUtf8(pr.colorA));
        thumbSt.insert(QStringLiteral("colorB"), QString::fromUtf8(pr.colorB));
        if (pr.textColor[0] != '\0')
            thumbSt.insert(QStringLiteral("textColor"), QString::fromUtf8(pr.textColor));
        if (pr.bgColor[0] != '\0')
            thumbSt.insert(QStringLiteral("bgColor"), QString::fromUtf8(pr.bgColor));
        OverlayRenderer renderer;
        const QImage thumb = renderer.renderScoreboard(thumbSt, 320, 180);
        const QPixmap pm = QPixmap::fromImage(thumb).scaled(200, 72, Qt::KeepAspectRatio,
                                                            Qt::SmoothTransformation);

        card->setIcon(QIcon(pm));
        card->setIconSize(QSize(200, 72));
        card->setText(QStringLiteral("\n%1\n%2")
                          .arg(QString::fromUtf8(pr.name), QString::fromUtf8(pr.blurb)));
        card->setStyleSheet(QStringLiteral(
            "QPushButton{"
            "  background:#14171C; border:1px solid #3A3D45; border-radius:6px;"
            "  color:#A0A8B8; font-size:10px; font-weight:700;"
            "  text-align:center; padding:6px 4px 8px 4px;}"
            "QPushButton:hover{border-color:#4F9EFF; color:#E0E2E8;}"
            "QPushButton:checked{border:2px solid #22C55E; background:#0F1A14; color:#86EFAC;}"));
        m_presetGroup->addButton(card, i);
        presetGrid->addWidget(card, i / cols, i % cols);
    }
    root->addLayout(presetGrid);

    // —— Colors ——
    auto* colorLab = new QLabel(QStringLiteral("COLORS"), body);
    colorLab->setObjectName(QStringLiteral("sec"));
    root->addWidget(colorLab);

    auto* colorGrid = new QGridLayout();
    colorGrid->setHorizontalSpacing(10);
    colorGrid->setVerticalSpacing(8);

    auto addColorRow = [&](int row, const QString& label, QPushButton*& btn) {
        auto* lab = new QLabel(label, body);
        btn = new QPushButton(body);
        btn->setCursor(Qt::PointingHandCursor);
        colorGrid->addWidget(lab, row, 0);
        colorGrid->addWidget(btn, row, 1);
    };
    addColorRow(0, QStringLiteral("Team A"), m_colorABtn);
    addColorRow(1, QStringLiteral("Team B"), m_colorBBtn);
    addColorRow(2, QStringLiteral("Text"), m_textColorBtn);
    addColorRow(3, QStringLiteral("Background"), m_bgColorBtn);
    applyColorButton(m_colorABtn, m_colorA);
    applyColorButton(m_colorBBtn, m_colorB);
    applyColorButton(m_textColorBtn, m_textColor);
    applyColorButton(m_bgColorBtn, m_bgColor);

    auto* resetText = new QPushButton(QStringLiteral("Theme default"), body);
    resetText->setToolTip(QStringLiteral("Clear custom text color"));
    auto* resetBg = new QPushButton(QStringLiteral("Theme default"), body);
    resetBg->setToolTip(QStringLiteral("Clear custom background"));
    colorGrid->addWidget(resetText, 2, 2);
    colorGrid->addWidget(resetBg, 3, 2);
    colorGrid->setColumnStretch(1, 1);
    root->addLayout(colorGrid);

    // —— Layout / Theme ——
    auto* fineLab = new QLabel(QStringLiteral("LAYOUT & THEME"), body);
    fineLab->setObjectName(QStringLiteral("sec"));
    root->addWidget(fineLab);

    auto* fineRow = new QHBoxLayout();
    fineRow->setSpacing(10);
    auto* layoutCol = new QVBoxLayout();
    layoutCol->addWidget(new QLabel(QStringLiteral("Layout"), body));
    m_layoutBox = new QComboBox(body);
    m_layoutBox->addItems({QStringLiteral("Lower Third"), QStringLiteral("Center"),
                           QStringLiteral("Corner"), QStringLiteral("Full")});
    m_layoutBox->setCurrentText(mapLayoutModelToUi(st0.layout));
    layoutCol->addWidget(m_layoutBox);
    auto* themeCol = new QVBoxLayout();
    themeCol->addWidget(new QLabel(QStringLiteral("Theme base"), body));
    m_themeBox = new QComboBox(body);
    m_themeBox->addItems({QStringLiteral("Dark"), QStringLiteral("Light"), QStringLiteral("Broadcast"),
                          QStringLiteral("Neon"), QStringLiteral("Carbon"), QStringLiteral("Gold")});
    m_themeBox->setCurrentText(mapThemeModelToUi(st0.theme));
    themeCol->addWidget(m_themeBox);
    fineRow->addLayout(layoutCol, 1);
    fineRow->addLayout(themeCol, 1);
    root->addLayout(fineRow);

    // —— Sport ——
    auto* sportLab = new QLabel(QStringLiteral("SPORT"), body);
    sportLab->setObjectName(QStringLiteral("sec"));
    root->addWidget(sportLab);

    auto* sportGrid = new QGridLayout();
    sportGrid->setSpacing(4);
    const QStringList sports = {QStringLiteral("Generic"), QStringLiteral("Pool"),
                                QStringLiteral("Basketball"), QStringLiteral("Soccer"),
                                QStringLiteral("Tennis"), QStringLiteral("Custom")};
    m_sportGroup = new QButtonGroup(this);
    m_sportGroup->setExclusive(true);
    const QString sportUi = mapSportModelToUi(st0.sport);
    int si = 0;
    for (const auto& sp : sports) {
        auto* b = new QPushButton(sp, body);
        b->setCheckable(true);
        b->setCursor(Qt::PointingHandCursor);
        b->setStyleSheet(QStringLiteral(
            "QPushButton{background:#1A1D22;border:1px solid #3A3D45;color:#A0A8B8;"
            "font-size:10px;font-weight:700;padding:8px 4px;border-radius:3px;}"
            "QPushButton:checked{border:2px solid #F59E0B;color:#FBBF24;background:#2A2010;}"));
        if (sp == sportUi)
            b->setChecked(true);
        m_sportGroup->addButton(b);
        sportGrid->addWidget(b, si / 3, si % 3);
        ++si;
    }
    if (!m_sportGroup->checkedButton() && !m_sportGroup->buttons().isEmpty())
        m_sportGroup->buttons().first()->setChecked(true);
    root->addLayout(sportGrid);
    root->addStretch(1);

    // —— Footer ——
    auto* footer = new QWidget(this);
    footer->setStyleSheet(QStringLiteral("background:#0C0E11; border-top:1px solid #2A2E36;"));
    auto* footLay = new QHBoxLayout(footer);
    footLay->setContentsMargins(16, 10, 16, 10);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, footer);
    footLay->addStretch(1);
    footLay->addWidget(buttons);
    outer->addWidget(footer);

    // —— Signals ——
    auto applyPreset = [this](int index) {
        if (index < 0 || index >= int(sizeof(kPresets) / sizeof(kPresets[0])))
            return;
        const auto& pr = kPresets[index];
        m_layoutBox->setCurrentText(mapLayoutModelToUi(QString::fromUtf8(pr.layout)));
        m_themeBox->setCurrentText(mapThemeModelToUi(QString::fromUtf8(pr.theme)));
        m_colorA = QColor(QString::fromUtf8(pr.colorA));
        m_colorB = QColor(QString::fromUtf8(pr.colorB));
        applyColorButton(m_colorABtn, m_colorA);
        applyColorButton(m_colorBBtn, m_colorB);
        m_useCustomText = pr.textColor[0] != '\0';
        m_useCustomBg = pr.bgColor[0] != '\0';
        if (m_useCustomText) {
            m_textColor = QColor(QString::fromUtf8(pr.textColor));
            applyColorButton(m_textColorBtn, m_textColor);
        }
        if (m_useCustomBg) {
            m_bgColor = QColor(QString::fromUtf8(pr.bgColor));
            applyColorButton(m_bgColorBtn, m_bgColor);
        }
        const QString sportUi = mapSportModelToUi(QString::fromUtf8(pr.sport));
        for (auto* b : m_sportGroup->buttons()) {
            if (b->text() == sportUi) {
                b->setChecked(true);
                break;
            }
        }
        refreshPreview();
    };

    connect(m_presetGroup, &QButtonGroup::idClicked, this, applyPreset);

    connect(m_colorABtn, &QPushButton::clicked, this, [this] {
        m_colorA = pickColor(m_colorA, QStringLiteral("Team A color"));
        applyColorButton(m_colorABtn, m_colorA);
        refreshPreview();
    });
    connect(m_colorBBtn, &QPushButton::clicked, this, [this] {
        m_colorB = pickColor(m_colorB, QStringLiteral("Team B color"));
        applyColorButton(m_colorBBtn, m_colorB);
        refreshPreview();
    });
    connect(m_textColorBtn, &QPushButton::clicked, this, [this] {
        m_textColor = pickColor(m_textColor, QStringLiteral("Text color"));
        m_useCustomText = true;
        applyColorButton(m_textColorBtn, m_textColor);
        refreshPreview();
    });
    connect(m_bgColorBtn, &QPushButton::clicked, this, [this] {
        m_bgColor = pickColor(m_bgColor, QStringLiteral("Background color"));
        m_useCustomBg = true;
        applyColorButton(m_bgColorBtn, m_bgColor);
        refreshPreview();
    });
    connect(resetText, &QPushButton::clicked, this, [this] {
        m_useCustomText = false;
        m_textColor = QColor(QStringLiteral("#FFFFFF"));
        applyColorButton(m_textColorBtn, m_textColor);
        refreshPreview();
    });
    connect(resetBg, &QPushButton::clicked, this, [this] {
        m_useCustomBg = false;
        m_bgColor = QColor(QStringLiteral("#0A0C10"));
        applyColorButton(m_bgColorBtn, m_bgColor);
        refreshPreview();
    });

    connect(m_layoutBox, &QComboBox::currentTextChanged, this, [this](const QString&) {
        refreshPreview();
    });
    connect(m_themeBox, &QComboBox::currentTextChanged, this, [this](const QString&) {
        refreshPreview();
    });

    connect(buttons, &QDialogButtonBox::accepted, this, [this, model] {
        auto st = model->state();
        QString sportUi = QStringLiteral("Generic");
        if (auto* checked = m_sportGroup->checkedButton())
            sportUi = checked->text();
        st.sport = mapSportUiToModel(sportUi);
        st.theme = mapThemeUiToModel(m_themeBox->currentText());
        st.layout = mapLayoutUiToModel(m_layoutBox->currentText());
        st.colorA = colorCss(m_colorA);
        st.colorB = colorCss(m_colorB);
        st.textColor = m_useCustomText ? colorCss(m_textColor) : QString();
        st.bgColor = m_useCustomBg ? colorCss(m_bgColor) : QString();
        model->setState(st);
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // Select closest matching preset (optional) then preview
    refreshPreview();
}

} // namespace railshot
