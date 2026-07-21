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
#include <QShowEvent>
#include <QResizeEvent>
#include <QPainter>

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
    const char* textColor;
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

/// Crop transparent margins so thumbnails fill the card.
QImage cropOpaque(const QImage& src, int pad = 8)
{
    if (src.isNull())
        return src;
    const QImage img = src.convertToFormat(QImage::Format_ARGB32);
    int minX = img.width(), minY = img.height(), maxX = -1, maxY = -1;
    for (int y = 0; y < img.height(); ++y) {
        const auto* line = reinterpret_cast<const QRgb*>(img.constScanLine(y));
        for (int x = 0; x < img.width(); ++x) {
            if (qAlpha(line[x]) > 24) {
                minX = qMin(minX, x);
                minY = qMin(minY, y);
                maxX = qMax(maxX, x);
                maxY = qMax(maxY, y);
            }
        }
    }
    if (maxX < minX)
        return src;
    minX = qMax(0, minX - pad);
    minY = qMax(0, minY - pad);
    maxX = qMin(img.width() - 1, maxX + pad);
    maxY = qMin(img.height() - 1, maxY + pad);
    return img.copy(minX, minY, maxX - minX + 1, maxY - minY + 1);
}

QLabel* sectionLabel(const QString& text, QWidget* parent)
{
    auto* lab = new QLabel(text, parent);
    lab->setObjectName(QStringLiteral("sec"));
    return lab;
}

} // namespace

void ScoreboardSettingsDialog::applyColorButton(QPushButton* btn, const QColor& c, const QString& label)
{
    const QString hex = colorCss(c);
    const QColor contrast = (c.lightness() > 140) ? QColor(20, 22, 28) : QColor(240, 242, 246);
    btn->setText(QStringLiteral("%1\n%2").arg(label, hex.toUpper()));
    btn->setStyleSheet(QStringLiteral(
        "QPushButton{"
        "  background:%1; border:1px solid #5A6478; border-radius:4px;"
        "  color:%2; font-family:'DM Sans'; font-size:10px; font-weight:800;"
        "  padding:10px 8px; min-height:44px; text-align:center;}"
        "QPushButton:hover{border-color:#8AB4FF;}")
                           .arg(hex, contrast.name()));
}

QColor ScoreboardSettingsDialog::pickColor(const QColor& current, const QString& title)
{
    const QColor picked = QColorDialog::getColor(current.isValid() ? current : Qt::white, this, title,
                                                 QColorDialog::ShowAlphaChannel);
    return picked.isValid() ? picked : current;
}

QJsonObject ScoreboardSettingsDialog::previewState() const
{
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
    return st;
}

void ScoreboardSettingsDialog::refreshPreview()
{
    if (!m_preview || !m_engine || !m_layoutBox || !m_themeBox)
        return;

    OverlayRenderer renderer;
    m_previewImg = renderer.renderScoreboard(previewState(), 960, 540);

    QSize box = m_preview->size();
    if (box.width() < 40 || box.height() < 40)
        box = QSize(520, 200);
    const QPixmap pm = QPixmap::fromImage(m_previewImg).scaled(box, Qt::KeepAspectRatio,
                                                               Qt::SmoothTransformation);
    m_preview->setPixmap(pm);
}

void ScoreboardSettingsDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    refreshPreview();
}

void ScoreboardSettingsDialog::resizeEvent(QResizeEvent* event)
{
    QDialog::resizeEvent(event);
    if (!m_previewImg.isNull() && m_preview) {
        QSize box = m_preview->size();
        if (box.width() >= 40 && box.height() >= 40) {
            m_preview->setPixmap(QPixmap::fromImage(m_previewImg)
                                     .scaled(box, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    }
}

ScoreboardSettingsDialog::ScoreboardSettingsDialog(EngineController* engine, QWidget* parent)
    : QDialog(parent), m_engine(engine)
{
    setObjectName(QStringLiteral("scoreboardSettingsDialog"));
    setWindowTitle(QStringLiteral("Scoreboard Settings"));
    setModal(true);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    resize(920, 640);
    setMinimumSize(820, 560);

    setStyleSheet(QStringLiteral(
        "QDialog#scoreboardSettingsDialog { background:#0A0C0F; }"
        "QDialog#scoreboardSettingsDialog QFrame#dialogShell {"
        "  background:#12151A; border:1px solid #5A6478; }"
        "QDialog#scoreboardSettingsDialog QFrame#previewCard {"
        "  background:#0A0C10; border:1px solid #3A3D45; }"
        "QDialog#scoreboardSettingsDialog QFrame#panelCard {"
        "  background:#0E1116; border:1px solid #2A2E36; border-radius:0px; }"
        "QLabel#sec {"
        "  color:#4F9EFF; font-size:9px; font-weight:900; letter-spacing:1.5px;"
        "  font-family:'DM Sans'; background:transparent; border:none; }"
        "QLabel#hint { color:#6A7384; font-size:10px; font-family:'DM Sans'; }"
        "QLabel#body { color:#C8CCD4; font-size:11px; font-family:'DM Sans'; }"
        "QComboBox {"
        "  background:#0A0C10; border:1px solid #3A3D45; border-radius:3px;"
        "  color:#E0E2E8; padding:5px 10px; min-height:26px; font-family:'DM Sans'; }"
        "QComboBox:focus { border-color:#4F9EFF; }"
        "QComboBox::drop-down { border:none; width:22px; }"
        "QScrollArea { border:none; background:transparent; }"
        "QScrollBar:vertical {"
        "  background:#0A0C10; width:10px; margin:0; }"
        "QScrollBar::handle:vertical {"
        "  background:#3A3D45; border-radius:4px; min-height:28px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height:0; }"
        "QDialogButtonBox QPushButton {"
        "  min-width:88px; min-height:28px; padding:4px 14px;"
        "  background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #3A6AFF,stop:1 #1A3AFF);"
        "  border:1px solid #6B9AFF; border-radius:3px; color:#FFFFFF;"
        "  font-family:'DM Sans'; font-size:11px; font-weight:800; }"
        "QDialogButtonBox QPushButton:hover { border-color:#8AB4FF; }"));

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

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(10, 10, 10, 10);
    root->setSpacing(0);

    auto* shell = new QFrame(this);
    shell->setObjectName(QStringLiteral("dialogShell"));
    auto* shellLay = new QVBoxLayout(shell);
    shellLay->setContentsMargins(0, 0, 0, 0);
    shellLay->setSpacing(0);

    auto* accent = new QFrame(shell);
    accent->setFixedHeight(3);
    accent->setStyleSheet(QStringLiteral(
        "background:qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        "stop:0 #4F9EFF, stop:0.5 #A855F7, stop:1 #FF5A2C); border:none;"));
    shellLay->addWidget(accent);

    // Header strip
    auto* header = new QWidget(shell);
    header->setStyleSheet(QStringLiteral(
        "background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #161A22,stop:1 #12151A);"
        "border-bottom:1px solid #2A2D35;"));
    auto* headerLay = new QVBoxLayout(header);
    headerLay->setContentsMargins(16, 12, 16, 12);
    headerLay->setSpacing(2);
    auto* title = new QLabel(QStringLiteral("Scoreboard"), header);
    title->setStyleSheet(QStringLiteral(
        "color:#F0F2F6; font-family:'DM Sans'; font-size:16px; font-weight:800; border:none;"));
    auto* sub = new QLabel(QStringLiteral("Pick a preset, then tune team colors and layout."), header);
    sub->setObjectName(QStringLiteral("hint"));
    headerLay->addWidget(title);
    headerLay->addWidget(sub);
    shellLay->addWidget(header);

    // Main split: left controls + preview, right presets
    auto* main = new QWidget(shell);
    auto* mainLay = new QHBoxLayout(main);
    mainLay->setContentsMargins(14, 14, 14, 12);
    mainLay->setSpacing(14);

    // ── LEFT column ──────────────────────────────────────────────
    auto* left = new QWidget(main);
    auto* leftLay = new QVBoxLayout(left);
    leftLay->setContentsMargins(0, 0, 0, 0);
    leftLay->setSpacing(10);

    leftLay->addWidget(sectionLabel(QStringLiteral("PREVIEW"), left));

    auto* previewHost = new QWidget(left);
    previewHost->setStyleSheet(QStringLiteral(
        "background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #12151A,stop:1 #0A0C10);"));
    auto* previewHostLay = new QVBoxLayout(previewHost);
    previewHostLay->setContentsMargins(0, 0, 0, 0);

    m_previewCard = new QFrame(previewHost);
    m_previewCard->setObjectName(QStringLiteral("previewCard"));
    auto* cardLay = new QVBoxLayout(m_previewCard);
    cardLay->setContentsMargins(3, 3, 3, 3);
    cardLay->setSpacing(0);
    m_preview = new QLabel(m_previewCard);
    m_preview->setMinimumHeight(168);
    m_preview->setMaximumHeight(200);
    m_preview->setAlignment(Qt::AlignCenter);
    m_preview->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_preview->setStyleSheet(QStringLiteral(
        "background:#080A0C; border:none;"));
    cardLay->addWidget(m_preview);
    previewHostLay->addWidget(m_previewCard);
    leftLay->addWidget(previewHost);

    // Colors — 2×2 compact tiles
    auto* colorPanel = new QFrame(left);
    colorPanel->setObjectName(QStringLiteral("panelCard"));
    auto* colorPanelLay = new QVBoxLayout(colorPanel);
    colorPanelLay->setContentsMargins(10, 10, 10, 10);
    colorPanelLay->setSpacing(8);
    colorPanelLay->addWidget(sectionLabel(QStringLiteral("COLORS"), colorPanel));

    auto* colorGrid = new QGridLayout();
    colorGrid->setHorizontalSpacing(8);
    colorGrid->setVerticalSpacing(8);
    m_colorABtn = new QPushButton(colorPanel);
    m_colorBBtn = new QPushButton(colorPanel);
    m_textColorBtn = new QPushButton(colorPanel);
    m_bgColorBtn = new QPushButton(colorPanel);
    for (auto* b : {m_colorABtn, m_colorBBtn, m_textColorBtn, m_bgColorBtn})
        b->setCursor(Qt::PointingHandCursor);
    applyColorButton(m_colorABtn, m_colorA, QStringLiteral("Team A"));
    applyColorButton(m_colorBBtn, m_colorB, QStringLiteral("Team B"));
    applyColorButton(m_textColorBtn, m_textColor, QStringLiteral("Text"));
    applyColorButton(m_bgColorBtn, m_bgColor, QStringLiteral("Background"));
    colorGrid->addWidget(m_colorABtn, 0, 0);
    colorGrid->addWidget(m_colorBBtn, 0, 1);
    colorGrid->addWidget(m_textColorBtn, 1, 0);
    colorGrid->addWidget(m_bgColorBtn, 1, 1);
    colorPanelLay->addLayout(colorGrid);

    auto* resetRow = new QHBoxLayout();
    resetRow->setSpacing(8);
    auto* resetText = new QPushButton(QStringLiteral("Reset text"), colorPanel);
    auto* resetBg = new QPushButton(QStringLiteral("Reset background"), colorPanel);
    for (auto* b : {resetText, resetBg}) {
        b->setCursor(Qt::PointingHandCursor);
        b->setStyleSheet(QStringLiteral(
            "QPushButton{background:#1A1D22; border:1px solid #3A3D45; border-radius:3px;"
            "  color:#A0A8B8; font-size:10px; font-weight:700; padding:5px 10px;}"
            "QPushButton:hover{border-color:#4F9EFF; color:#E0E2E8;}"));
    }
    resetText->setToolTip(QStringLiteral("Use theme default text color"));
    resetBg->setToolTip(QStringLiteral("Use theme default background"));
    resetRow->addWidget(resetText);
    resetRow->addWidget(resetBg);
    resetRow->addStretch(1);
    colorPanelLay->addLayout(resetRow);
    leftLay->addWidget(colorPanel);

    // Layout + theme
    auto* finePanel = new QFrame(left);
    finePanel->setObjectName(QStringLiteral("panelCard"));
    auto* finePanelLay = new QVBoxLayout(finePanel);
    finePanelLay->setContentsMargins(10, 10, 10, 10);
    finePanelLay->setSpacing(8);
    finePanelLay->addWidget(sectionLabel(QStringLiteral("LAYOUT & THEME"), finePanel));

    auto* fineRow = new QHBoxLayout();
    fineRow->setSpacing(10);
    auto* layoutCol = new QVBoxLayout();
    layoutCol->setSpacing(4);
    auto* layoutLab = new QLabel(QStringLiteral("Layout"), finePanel);
    layoutLab->setObjectName(QStringLiteral("body"));
    layoutCol->addWidget(layoutLab);
    m_layoutBox = new QComboBox(finePanel);
    m_layoutBox->addItems({QStringLiteral("Lower Third"), QStringLiteral("Center"),
                           QStringLiteral("Corner"), QStringLiteral("Full")});
    m_layoutBox->setCurrentText(mapLayoutModelToUi(st0.layout));
    layoutCol->addWidget(m_layoutBox);

    auto* themeCol = new QVBoxLayout();
    themeCol->setSpacing(4);
    auto* themeLab = new QLabel(QStringLiteral("Theme base"), finePanel);
    themeLab->setObjectName(QStringLiteral("body"));
    themeCol->addWidget(themeLab);
    m_themeBox = new QComboBox(finePanel);
    m_themeBox->addItems({QStringLiteral("Dark"), QStringLiteral("Light"), QStringLiteral("Broadcast"),
                          QStringLiteral("Neon"), QStringLiteral("Carbon"), QStringLiteral("Gold")});
    m_themeBox->setCurrentText(mapThemeModelToUi(st0.theme));
    themeCol->addWidget(m_themeBox);
    fineRow->addLayout(layoutCol, 1);
    fineRow->addLayout(themeCol, 1);
    finePanelLay->addLayout(fineRow);
    leftLay->addWidget(finePanel);

    // Sport chips
    auto* sportPanel = new QFrame(left);
    sportPanel->setObjectName(QStringLiteral("panelCard"));
    auto* sportPanelLay = new QVBoxLayout(sportPanel);
    sportPanelLay->setContentsMargins(10, 10, 10, 10);
    sportPanelLay->setSpacing(8);
    sportPanelLay->addWidget(sectionLabel(QStringLiteral("SPORT"), sportPanel));

    auto* sportGrid = new QGridLayout();
    sportGrid->setSpacing(6);
    const QStringList sports = {QStringLiteral("Generic"), QStringLiteral("Pool"),
                                QStringLiteral("Basketball"), QStringLiteral("Soccer"),
                                QStringLiteral("Tennis"), QStringLiteral("Custom")};
    m_sportGroup = new QButtonGroup(this);
    m_sportGroup->setExclusive(true);
    const QString sportUi = mapSportModelToUi(st0.sport);
    int si = 0;
    for (const auto& sp : sports) {
        auto* b = new QPushButton(sp, sportPanel);
        b->setCheckable(true);
        b->setCursor(Qt::PointingHandCursor);
        b->setStyleSheet(QStringLiteral(
            "QPushButton{background:#151820;border:1px solid #3A3D45;color:#A0A8B8;"
            "font-size:10px;font-weight:700;padding:7px 4px;border-radius:3px;}"
            "QPushButton:hover{border-color:#4F9EFF;color:#E0E2E8;}"
            "QPushButton:checked{border:1px solid #F59E0B;color:#FBBF24;background:#241A0A;}"));
        if (sp == sportUi)
            b->setChecked(true);
        m_sportGroup->addButton(b);
        sportGrid->addWidget(b, si / 3, si % 3);
        ++si;
    }
    if (!m_sportGroup->checkedButton() && !m_sportGroup->buttons().isEmpty())
        m_sportGroup->buttons().first()->setChecked(true);
    sportPanelLay->addLayout(sportGrid);
    leftLay->addWidget(sportPanel);
    leftLay->addStretch(1);

    // ── RIGHT column: presets ────────────────────────────────────
    auto* right = new QWidget(main);
    auto* rightLay = new QVBoxLayout(right);
    rightLay->setContentsMargins(0, 0, 0, 0);
    rightLay->setSpacing(8);

    auto* presetHead = new QHBoxLayout();
    presetHead->addWidget(sectionLabel(QStringLiteral("PRESETS"), right));
    presetHead->addStretch(1);
    auto* presetHint = new QLabel(QStringLiteral("Click a look to apply"), right);
    presetHint->setObjectName(QStringLiteral("hint"));
    presetHead->addWidget(presetHint);
    rightLay->addLayout(presetHead);

    auto* presetScroll = new QScrollArea(right);
    presetScroll->setWidgetResizable(true);
    presetScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    presetScroll->setFrameShape(QFrame::NoFrame);
    auto* presetBody = new QWidget(presetScroll);
    presetBody->setStyleSheet(QStringLiteral("background:transparent;"));
    auto* presetGrid = new QGridLayout(presetBody);
    presetGrid->setContentsMargins(0, 0, 4, 0);
    presetGrid->setSpacing(8);
    presetScroll->setWidget(presetBody);

    m_presetGroup = new QButtonGroup(this);
    m_presetGroup->setExclusive(true);

    OverlayRenderer thumbRenderer;
    const int cols = 2;
    for (int i = 0; i < int(sizeof(kPresets) / sizeof(kPresets[0])); ++i) {
        const auto& pr = kPresets[i];

        auto* card = new QPushButton(presetBody);
        card->setCheckable(true);
        card->setCursor(Qt::PointingHandCursor);
        card->setToolTip(QString::fromUtf8(pr.blurb));
        card->setMinimumHeight(118);
        card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

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

        const QImage raw = thumbRenderer.renderScoreboard(thumbSt, 640, 360);
        const QImage cropped = cropOpaque(raw, 12);
        // Composite onto dark plate so transparent areas don't look broken
        QImage plate(220, 72, QImage::Format_ARGB32);
        plate.fill(QColor(12, 14, 18));
        {
            QPainter pp(&plate);
            pp.setRenderHint(QPainter::SmoothPixmapTransform);
            const QImage scaled = cropped.scaled(220, 72, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            pp.drawImage((220 - scaled.width()) / 2, (72 - scaled.height()) / 2, scaled);
        }

        card->setIcon(QIcon(QPixmap::fromImage(plate)));
        card->setIconSize(QSize(220, 72));
        card->setText(QString::fromUtf8(pr.name));
        card->setStyleSheet(QStringLiteral(
            "QPushButton{"
            "  background:#0E1116; border:1px solid #3A3D45;"
            "  color:#C8CCD4; font-family:'DM Sans'; font-size:11px; font-weight:800;"
            "  text-align:center; padding:8px 6px 10px 6px;}"
            "QPushButton:hover{border-color:#4F9EFF; color:#FFFFFF; background:#141820;}"
            "QPushButton:checked{border:2px solid #4F9EFF; background:#101820; color:#FFFFFF;}"));

        m_presetGroup->addButton(card, i);
        presetGrid->addWidget(card, i / cols, i % cols);
    }
    presetGrid->setRowStretch((int(sizeof(kPresets) / sizeof(kPresets[0])) + cols - 1) / cols, 1);
    rightLay->addWidget(presetScroll, 1);

    mainLay->addWidget(left, 5);
    mainLay->addWidget(right, 6);
    shellLay->addWidget(main, 1);

    // Footer — match Source Properties
    auto* foot = new QWidget(shell);
    foot->setStyleSheet(QStringLiteral("background:#0C0E12; border-top:1px solid #3A3D45;"));
    auto* footLay = new QHBoxLayout(foot);
    footLay->setContentsMargins(14, 10, 14, 10);
    auto* footHint = new QLabel(QStringLiteral("Changes apply when you press OK"), foot);
    footHint->setObjectName(QStringLiteral("hint"));
    auto* box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, foot);
    box->button(QDialogButtonBox::Ok)->setText(QStringLiteral("OK"));
    box->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("Cancel"));
    box->button(QDialogButtonBox::Ok)->setDefault(true);
    box->button(QDialogButtonBox::Ok)->setCursor(Qt::PointingHandCursor);
    box->button(QDialogButtonBox::Cancel)->setCursor(Qt::PointingHandCursor);
    box->button(QDialogButtonBox::Cancel)->setStyleSheet(QStringLiteral(
        "QPushButton{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #2A2D35,stop:1 #1A1D22);"
        "border:1px solid #3A3D45;color:#C8CAD0;}"));
    footLay->addWidget(footHint);
    footLay->addStretch(1);
    footLay->addWidget(box);
    shellLay->addWidget(foot);

    root->addWidget(shell, 1);

    // Signals
    auto applyPreset = [this](int index) {
        if (index < 0 || index >= int(sizeof(kPresets) / sizeof(kPresets[0])))
            return;
        const auto& pr = kPresets[index];
        m_layoutBox->blockSignals(true);
        m_themeBox->blockSignals(true);
        m_layoutBox->setCurrentText(mapLayoutModelToUi(QString::fromUtf8(pr.layout)));
        m_themeBox->setCurrentText(mapThemeModelToUi(QString::fromUtf8(pr.theme)));
        m_layoutBox->blockSignals(false);
        m_themeBox->blockSignals(false);
        m_colorA = QColor(QString::fromUtf8(pr.colorA));
        m_colorB = QColor(QString::fromUtf8(pr.colorB));
        applyColorButton(m_colorABtn, m_colorA, QStringLiteral("Team A"));
        applyColorButton(m_colorBBtn, m_colorB, QStringLiteral("Team B"));
        m_useCustomText = pr.textColor[0] != '\0';
        m_useCustomBg = pr.bgColor[0] != '\0';
        if (m_useCustomText) {
            m_textColor = QColor(QString::fromUtf8(pr.textColor));
            applyColorButton(m_textColorBtn, m_textColor, QStringLiteral("Text"));
        }
        if (m_useCustomBg) {
            m_bgColor = QColor(QString::fromUtf8(pr.bgColor));
            applyColorButton(m_bgColorBtn, m_bgColor, QStringLiteral("Background"));
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
        applyColorButton(m_colorABtn, m_colorA, QStringLiteral("Team A"));
        refreshPreview();
    });
    connect(m_colorBBtn, &QPushButton::clicked, this, [this] {
        m_colorB = pickColor(m_colorB, QStringLiteral("Team B color"));
        applyColorButton(m_colorBBtn, m_colorB, QStringLiteral("Team B"));
        refreshPreview();
    });
    connect(m_textColorBtn, &QPushButton::clicked, this, [this] {
        m_textColor = pickColor(m_textColor, QStringLiteral("Text color"));
        m_useCustomText = true;
        applyColorButton(m_textColorBtn, m_textColor, QStringLiteral("Text"));
        refreshPreview();
    });
    connect(m_bgColorBtn, &QPushButton::clicked, this, [this] {
        m_bgColor = pickColor(m_bgColor, QStringLiteral("Background color"));
        m_useCustomBg = true;
        applyColorButton(m_bgColorBtn, m_bgColor, QStringLiteral("Background"));
        refreshPreview();
    });
    connect(resetText, &QPushButton::clicked, this, [this] {
        m_useCustomText = false;
        m_textColor = QColor(QStringLiteral("#FFFFFF"));
        applyColorButton(m_textColorBtn, m_textColor, QStringLiteral("Text"));
        refreshPreview();
    });
    connect(resetBg, &QPushButton::clicked, this, [this] {
        m_useCustomBg = false;
        m_bgColor = QColor(QStringLiteral("#0A0C10"));
        applyColorButton(m_bgColorBtn, m_bgColor, QStringLiteral("Background"));
        refreshPreview();
    });

    connect(m_layoutBox, &QComboBox::currentTextChanged, this, [this](const QString&) {
        refreshPreview();
    });
    connect(m_themeBox, &QComboBox::currentTextChanged, this, [this](const QString&) {
        refreshPreview();
    });

    connect(box, &QDialogButtonBox::accepted, this, [this, model] {
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
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);

    refreshPreview();
}

} // namespace railshot
