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
#include <QSignalBlocker>
#include <QColorDialog>
#include <QFrame>
#include <QPixmap>
#include <QJsonObject>
#include <QShowEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QTimer>

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
    {"pool_mosconi", "Match Race", "Broadcast · white scores · Race-to tab · ball rack",
     "standard", "broadcast", "#1B3A6B", "#3D1F5C", "#FFFFFF", "", "8ball"},
    {"pool_clean", "Clean Navy", "Name · score tiles · RACE TO center",
     "standard", "carbon", "#3B6EA5", "#3B6EA5", "#FFFFFF", "#121C30", "8ball"},
    {"pool_slant", "Slant Duel", "Modern translucent wings · race shield",
     "standard", "classic", "#1E3A5F", "#0F766E", "#142830", "#78C8C8", "8ball"},
    {"pool_snooker", "Frame Bar", "Snooker-style names · white score chips",
     "center", "carbon", "#DC2626", "#DC2626", "#FFFFFF", "#1C1E22", "8ball"},
    {"pool_felt", "Felt Rails", "Tournament felt · 8-ball race pill · rack",
     "standard", "railshot", "#15803D", "#B45309", "#F0FDF4", "#0A120E", "8ball"},
    {"pool_neon", "Cue Neon", "Neon felt · glow race · ball rack",
     "wide", "neon", "#FF00AA", "#00F0FF", "#FFFFFF", "#080414", "8ball"},
    {"pool_gold", "Championship", "Gold match bar · race tab · ball rack",
     "wide", "gold", "#B45309", "#1B3A6B", "#FFECB3", "", "8ball"},
    {"pool_corner", "Corner Racks", "Compact felt corner bug",
     "compact", "railshot", "#FF5A2C", "#4F9EFF", "", "", "8ball"},
    {"baseball", "Diamond Bug", "Baseball · bases · count · outs",
     "standard", "broadcast", "#22C55E", "#EF4444", "#FFFFFF", "#0B1220", "baseball"},
    {"baseball_wide", "Diamond Wide", "Baseball · full lower bar",
     "wide", "carbon", "#16A34A", "#DC2626", "#F8FAFC", "#121214", "baseball"},
    {"court", "Court Side", "Basketball · quarter + game clock",
     "wide", "broadcast", "#EA580C", "#2563EB", "#FFFFFF", "#0B1220", "basketball"},
    {"pitch", "Pitch Night", "Soccer · minute badge · scoreline",
     "standard", "broadcast", "#16A34A", "#E2E8F0", "#F8FAFC", "#052E16", "soccer"},
    {"tennis", "Baseline", "Tennis · sets · games · serve dot",
     "compact", "gold", "#F59E0B", "#D97706", "#FFECB3", "#1C160A", "tennis"},
    {"generic_dark", "Generic Dark", "Simple lower third",
     "standard", "railshot", "#FF5A2C", "#4F9EFF", "", "", "generic"},
    {"generic_light", "Generic Light", "Clean light board",
     "standard", "classic", "#C2410C", "#1D4ED8", "#14161C", "#F5F5F8", "generic"},
    {"center", "Center Clash", "Mid-screen duel banner",
     "center", "railshot", "#FF5A2C", "#4F9EFF", "", "", "generic"},
};

constexpr int kPresetCount = int(sizeof(kPresets) / sizeof(kPresets[0]));

QString mapSportUiToModel(const QString& ui)
{
    if (ui == QLatin1String("Pool") || ui == QLatin1String("8-Ball")) return QStringLiteral("8ball");
    if (ui == QLatin1String("9-Ball")) return QStringLiteral("9ball");
    if (ui == QLatin1String("10-Ball")) return QStringLiteral("10ball");
    if (ui == QLatin1String("Baseball")) return QStringLiteral("baseball");
    if (ui == QLatin1String("Basketball")) return QStringLiteral("basketball");
    if (ui == QLatin1String("Soccer")) return QStringLiteral("soccer");
    if (ui == QLatin1String("Tennis")) return QStringLiteral("tennis");
    if (ui == QLatin1String("Custom")) return QStringLiteral("custom");
    return QStringLiteral("generic");
}
QString mapSportModelToUi(const QString& model)
{
    if (model == QLatin1String("9ball")) return QStringLiteral("9-Ball");
    if (model == QLatin1String("10ball")) return QStringLiteral("10-Ball");
    if (model == QLatin1String("8ball") || model == QLatin1String("pool")
        || model == QLatin1String("snooker") || model == QLatin1String("straight")
        || model == QLatin1String("onepocket") || model == QLatin1String("7ball"))
        return QStringLiteral("8-Ball");
    if (model == QLatin1String("baseball")) return QStringLiteral("Baseball");
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

QImage cropOpaque(const QImage& src, int pad = 10)
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

QWidget* makeColorRow(QWidget* parent, const QString& name, QPushButton*& swatch, QLabel*& hexLab)
{
    auto* row = new QWidget(parent);
    auto* lay = new QHBoxLayout(row);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(8);

    auto* nameLab = new QLabel(name, row);
    nameLab->setObjectName(QStringLiteral("body"));
    nameLab->setFixedWidth(82);

    swatch = new QPushButton(row);
    swatch->setFixedSize(36, 28);
    swatch->setCursor(Qt::PointingHandCursor);
    swatch->setFocusPolicy(Qt::NoFocus);

    hexLab = new QLabel(row);
    hexLab->setObjectName(QStringLiteral("hex"));
    hexLab->setTextInteractionFlags(Qt::TextSelectableByMouse);

    lay->addWidget(nameLab);
    lay->addWidget(swatch);
    lay->addWidget(hexLab, 1);
    return row;
}

QPixmap makePresetThumb(const BoardPreset& pr, int width)
{
    width = qMax(200, width);
    QJsonObject thumbSt;
    thumbSt.insert(QStringLiteral("playerA"), QStringLiteral("HOME"));
    thumbSt.insert(QStringLiteral("playerB"), QStringLiteral("AWAY"));
    thumbSt.insert(QStringLiteral("scoreA"), 3);
    thumbSt.insert(QStringLiteral("scoreB"), 2);
    thumbSt.insert(QStringLiteral("raceTo"), 7);
    thumbSt.insert(QStringLiteral("activeSide"), 1);
    thumbSt.insert(QStringLiteral("pocketedMask"), 0);
    thumbSt.insert(QStringLiteral("clockSeconds"), 125);
    thumbSt.insert(QStringLiteral("period"), 2);
    thumbSt.insert(QStringLiteral("balls"), 2);
    thumbSt.insert(QStringLiteral("strikes"), 1);
    thumbSt.insert(QStringLiteral("outs"), 1);
    thumbSt.insert(QStringLiteral("inning"), 3);
    thumbSt.insert(QStringLiteral("topHalf"), false);
    thumbSt.insert(QStringLiteral("onFirst"), true);
    thumbSt.insert(QStringLiteral("onSecond"), true);
    thumbSt.insert(QStringLiteral("onThird"), false);
    thumbSt.insert(QStringLiteral("sport"), QString::fromUtf8(pr.sport));
    thumbSt.insert(QStringLiteral("layout"), QString::fromUtf8(pr.layout));
    thumbSt.insert(QStringLiteral("theme"), QString::fromUtf8(pr.theme));
    thumbSt.insert(QStringLiteral("colorA"), QString::fromUtf8(pr.colorA));
    thumbSt.insert(QStringLiteral("colorB"), QString::fromUtf8(pr.colorB));
    if (pr.textColor[0] != '\0')
        thumbSt.insert(QStringLiteral("textColor"), QString::fromUtf8(pr.textColor));
    if (pr.bgColor[0] != '\0')
        thumbSt.insert(QStringLiteral("bgColor"), QString::fromUtf8(pr.bgColor));

    OverlayRenderer renderer;
    const QImage raw = renderer.renderScoreboard(thumbSt, 960, 540);
    const QImage cropped = cropOpaque(raw, 16);

    const int h = qBound(48, int(width * 0.28), 96);
    QImage plate(width, h, QImage::Format_ARGB32);
    plate.fill(QColor(8, 10, 13));
    QPainter pp(&plate);
    pp.setRenderHint(QPainter::SmoothPixmapTransform);
    const QImage scaled = cropped.scaled(width - 8, h - 8, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    pp.drawImage((width - scaled.width()) / 2, (h - scaled.height()) / 2, scaled);
    pp.setPen(QPen(QColor(42, 45, 53), 1));
    pp.drawRect(0, 0, width - 1, h - 1);
    return QPixmap::fromImage(plate);
}

} // namespace

void ScoreboardSettingsDialog::styleSwatch(QPushButton* btn, const QColor& c)
{
    btn->setStyleSheet(QStringLiteral(
        "QPushButton{background:%1; border:1px solid #6A7384; border-radius:3px;}"
        "QPushButton:hover{border-color:#8AB4FF;}")
                           .arg(colorCss(c)));
}

void ScoreboardSettingsDialog::syncHexLabels()
{
    m_colorAHex->setText(colorCss(m_colorA).toUpper());
    m_colorBHex->setText(colorCss(m_colorB).toUpper());
    m_textHex->setText(m_useCustomText ? colorCss(m_textColor).toUpper()
                                      : QStringLiteral("Theme default"));
    m_bgHex->setText(m_useCustomBg ? colorCss(m_bgColor).toUpper()
                                   : QStringLiteral("Theme default"));
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
    st.insert(QStringLiteral("raceTo"), live.raceTo);
    st.insert(QStringLiteral("activeSide"), live.activeSide);
    st.insert(QStringLiteral("pocketedMask"), live.pocketedMask);
    st.insert(QStringLiteral("clockSeconds"), live.clockSeconds);
    st.insert(QStringLiteral("clockRunning"), live.clockRunning);
    st.insert(QStringLiteral("balls"), live.balls);
    st.insert(QStringLiteral("strikes"), live.strikes);
    st.insert(QStringLiteral("outs"), live.outs);
    st.insert(QStringLiteral("inning"), live.inning);
    st.insert(QStringLiteral("topHalf"), live.topHalf);
    st.insert(QStringLiteral("onFirst"), live.onFirst);
    st.insert(QStringLiteral("onSecond"), live.onSecond);
    st.insert(QStringLiteral("onThird"), live.onThird);
    st.insert(QStringLiteral("period"), live.period);
    QString sportUi = QStringLiteral("Generic");
    if (m_sportGroup && m_sportGroup->checkedButton())
        sportUi = m_sportGroup->checkedButton()->text();
    st.insert(QStringLiteral("sport"), mapSportUiToModel(sportUi));
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

void ScoreboardSettingsDialog::paintPreviewLabel()
{
    if (!m_preview || m_previewImg.isNull())
        return;
    const QImage cropped = cropOpaque(m_previewImg, 12);
    QSize box = m_preview->size();
    if (box.width() < 20 || box.height() < 20)
        box = QSize(320, 106);

    QImage plate(box, QImage::Format_ARGB32);
    plate.fill(QColor(8, 10, 13));
    QPainter p(&plate);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    const QImage scaled = cropped.scaled(box.width() - 6, box.height() - 6, Qt::KeepAspectRatio,
                                         Qt::SmoothTransformation);
    p.drawImage((box.width() - scaled.width()) / 2, (box.height() - scaled.height()) / 2, scaled);
    m_preview->setPixmap(QPixmap::fromImage(plate));
}

void ScoreboardSettingsDialog::refreshPreview()
{
    if (!m_preview || !m_engine || !m_layoutBox || !m_themeBox)
        return;
    OverlayRenderer renderer;
    m_previewImg = renderer.renderScoreboard(previewState(), 960, 540);
    paintPreviewLabel();
}

void ScoreboardSettingsDialog::rescalePresetThumbs()
{
    if (!m_presetScroll)
        return;
    const int w = qMax(220, m_presetScroll->viewport()->width() - 4);
    for (int i = 0; i < m_presetThumbs.size() && i < kPresetCount; ++i) {
        if (i < m_presetCards.size() && m_presetCards[i] && !m_presetCards[i]->isVisible())
            continue;
        const QPixmap pm = makePresetThumb(kPresets[i], w);
        m_presetThumbs[i]->setPixmap(pm);
        m_presetThumbs[i]->setFixedHeight(pm.height());
    }
}

void ScoreboardSettingsDialog::filterPresetsBySport()
{
    QString sportModel = QStringLiteral("generic");
    if (m_sportGroup && m_sportGroup->checkedButton())
        sportModel = mapSportUiToModel(m_sportGroup->checkedButton()->text());

    auto matches = [&](const char* presetSport) -> bool {
        const QString ps = QString::fromUtf8(presetSport);
        if (isPoolSport(sportModel))
            return ps == QLatin1String("8ball") || ps == QLatin1String("pool")
                   || ps == QLatin1String("9ball") || ps == QLatin1String("snooker");
        if (sportModel == QLatin1String("generic") || sportModel == QLatin1String("custom"))
            return ps == QLatin1String("generic") || ps == QLatin1String("custom");
        return ps == sportModel;
    };

    int visible = 0;
    bool selectedStillVisible = false;
    for (int i = 0; i < m_presetCards.size() && i < kPresetCount; ++i) {
        const bool show = matches(kPresets[i].sport);
        m_presetCards[i]->setVisible(show);
        if (show) {
            ++visible;
            if (i == m_selectedPreset)
                selectedStillVisible = true;
        }
    }
    if (!selectedStillVisible) {
        m_selectedPreset = -1;
        for (auto* card : m_presetCards) {
            card->setProperty("selected", false);
            card->setStyleSheet(QStringLiteral(
                "QFrame#presetCard{background:#0E1116; border:1px solid #3A3D45;}"));
        }
    }
    if (m_presetEmpty)
        m_presetEmpty->setVisible(visible == 0);
}

void ScoreboardSettingsDialog::setSelectedPreset(int index)
{
    if (index < 0 || index >= kPresetCount)
        return;
    m_selectedPreset = index;
    for (int i = 0; i < m_presetCards.size(); ++i) {
        const bool on = (i == index);
        m_presetCards[i]->setProperty("selected", on);
        m_presetCards[i]->setStyleSheet(QStringLiteral(
            "QFrame#presetCard{"
            "  background:%1; border:%2;}")
                                            .arg(on ? QStringLiteral("#101820") : QStringLiteral("#0E1116"),
                                                 on ? QStringLiteral("2px solid #4F9EFF")
                                                    : QStringLiteral("1px solid #3A3D45")));
    }

    const auto& pr = kPresets[index];
    m_layoutBox->blockSignals(true);
    m_themeBox->blockSignals(true);
    m_layoutBox->setCurrentText(mapLayoutModelToUi(QString::fromUtf8(pr.layout)));
    m_themeBox->setCurrentText(mapThemeModelToUi(QString::fromUtf8(pr.theme)));
    m_layoutBox->blockSignals(false);
    m_themeBox->blockSignals(false);

    m_colorA = QColor(QString::fromUtf8(pr.colorA));
    m_colorB = QColor(QString::fromUtf8(pr.colorB));
    styleSwatch(m_colorABtn, m_colorA);
    styleSwatch(m_colorBBtn, m_colorB);
    m_useCustomText = pr.textColor[0] != '\0';
    m_useCustomBg = pr.bgColor[0] != '\0';
    if (m_useCustomText) {
        m_textColor = QColor(QString::fromUtf8(pr.textColor));
        styleSwatch(m_textColorBtn, m_textColor);
    }
    if (m_useCustomBg) {
        m_bgColor = QColor(QString::fromUtf8(pr.bgColor));
        styleSwatch(m_bgColorBtn, m_bgColor);
    }
    syncHexLabels();

    const QString sportUi = mapSportModelToUi(QString::fromUtf8(pr.sport));
    for (auto* b : m_sportGroup->buttons()) {
        if (b->text() == sportUi) {
            QSignalBlocker block(m_sportGroup);
            b->setChecked(true);
            break;
        }
    }
    filterPresetsBySport();
    refreshPreview();
}

bool ScoreboardSettingsDialog::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonRelease) {
        auto* frame = qobject_cast<QFrame*>(watched);
        if (frame && frame->objectName() == QLatin1String("presetCard")) {
            const int idx = frame->property("presetIndex").toInt();
            setSelectedPreset(idx);
            return true;
        }
    }
    return QDialog::eventFilter(watched, event);
}

void ScoreboardSettingsDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    QTimer::singleShot(0, this, [this] {
        rescalePresetThumbs();
        refreshPreview();
    });
}

void ScoreboardSettingsDialog::resizeEvent(QResizeEvent* event)
{
    QDialog::resizeEvent(event);
    paintPreviewLabel();
    if (m_presetScroll)
        QTimer::singleShot(0, this, [this] { rescalePresetThumbs(); });
}

ScoreboardSettingsDialog::ScoreboardSettingsDialog(EngineController* engine, QWidget* parent)
    : QDialog(parent), m_engine(engine)
{
    setObjectName(QStringLiteral("scoreboardSettingsDialog"));
    setWindowTitle(QStringLiteral("Scoreboard Settings"));
    setModal(true);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    resize(980, 700);
    setMinimumSize(900, 620);

    setStyleSheet(QStringLiteral(
        "QDialog#scoreboardSettingsDialog { background:#0A0C0F; }"
        "QDialog#scoreboardSettingsDialog QFrame#dialogShell {"
        "  background:#12151A; border:1px solid #5A6478; }"
        "QDialog#scoreboardSettingsDialog QFrame#previewCard {"
        "  background:#0A0C10; border:1px solid #3A3D45; }"
        "QDialog#scoreboardSettingsDialog QFrame#panelCard {"
        "  background:#0E1116; border:1px solid #2A2E36; }"
        "QLabel#sec {"
        "  color:#4F9EFF; font-size:9px; font-weight:900; letter-spacing:1.5px;"
        "  font-family:'DM Sans'; background:transparent; border:none; }"
        "QLabel#hint { color:#6A7384; font-size:10px; font-family:'DM Sans'; }"
        "QLabel#body { color:#C8CCD4; font-size:11px; font-family:'DM Sans'; }"
        "QLabel#hex { color:#A0A8B8; font-size:11px; font-family:'JetBrains Mono'; }"
        "QComboBox {"
        "  background:#0A0C10; border:1px solid #3A3D45; border-radius:3px;"
        "  color:#E0E2E8; padding:5px 10px; min-height:26px; font-family:'DM Sans'; }"
        "QComboBox:focus { border-color:#4F9EFF; }"
        "QComboBox::drop-down { border:none; width:22px; }"
        "QScrollArea { border:none; background:transparent; }"
        "QScrollBar:vertical { background:#0A0C10; width:10px; margin:2px; }"
        "QScrollBar::handle:vertical { background:#3A3D45; border-radius:4px; min-height:28px; }"
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

    auto* header = new QWidget(shell);
    header->setStyleSheet(QStringLiteral(
        "background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #161A22,stop:1 #12151A);"
        "border-bottom:1px solid #2A2D35;"));
    auto* headerLay = new QVBoxLayout(header);
    headerLay->setContentsMargins(16, 10, 16, 10);
    headerLay->setSpacing(2);
    auto* title = new QLabel(QStringLiteral("Scoreboard"), header);
    title->setStyleSheet(QStringLiteral(
        "color:#F0F2F6; font-family:'DM Sans'; font-size:16px; font-weight:800; border:none;"));
    auto* sub = new QLabel(QStringLiteral("Pick a preset, then tune team colors and layout."), header);
    sub->setObjectName(QStringLiteral("hint"));
    headerLay->addWidget(title);
    headerLay->addWidget(sub);
    shellLay->addWidget(header);

    auto* main = new QWidget(shell);
    auto* mainLay = new QHBoxLayout(main);
    mainLay->setContentsMargins(14, 12, 14, 10);
    mainLay->setSpacing(16);

    // LEFT — fixed width
    auto* left = new QWidget(main);
    left->setFixedWidth(340);
    auto* leftLay = new QVBoxLayout(left);
    leftLay->setContentsMargins(0, 0, 0, 0);
    leftLay->setSpacing(8);

    leftLay->addWidget(sectionLabel(QStringLiteral("PREVIEW"), left));
    auto* previewCard = new QFrame(left);
    previewCard->setObjectName(QStringLiteral("previewCard"));
    auto* cardLay = new QVBoxLayout(previewCard);
    cardLay->setContentsMargins(3, 3, 3, 3);
    m_preview = new QLabel(previewCard);
    m_preview->setFixedHeight(112);
    m_preview->setAlignment(Qt::AlignCenter);
    m_preview->setStyleSheet(QStringLiteral("background:#080A0C; border:none;"));
    cardLay->addWidget(m_preview);
    leftLay->addWidget(previewCard);

    auto* colorPanel = new QFrame(left);
    colorPanel->setObjectName(QStringLiteral("panelCard"));
    auto* colorPanelLay = new QVBoxLayout(colorPanel);
    colorPanelLay->setContentsMargins(10, 8, 10, 8);
    colorPanelLay->setSpacing(6);
    colorPanelLay->addWidget(sectionLabel(QStringLiteral("COLORS"), colorPanel));
    colorPanelLay->addWidget(makeColorRow(colorPanel, QStringLiteral("Team A"), m_colorABtn, m_colorAHex));
    colorPanelLay->addWidget(makeColorRow(colorPanel, QStringLiteral("Team B"), m_colorBBtn, m_colorBHex));
    colorPanelLay->addWidget(makeColorRow(colorPanel, QStringLiteral("Text"), m_textColorBtn, m_textHex));
    colorPanelLay->addWidget(makeColorRow(colorPanel, QStringLiteral("Background"), m_bgColorBtn, m_bgHex));
    styleSwatch(m_colorABtn, m_colorA);
    styleSwatch(m_colorBBtn, m_colorB);
    styleSwatch(m_textColorBtn, m_textColor);
    styleSwatch(m_bgColorBtn, m_bgColor);
    syncHexLabels();

    auto* resetRow = new QHBoxLayout();
    resetRow->setSpacing(6);
    auto* resetText = new QPushButton(QStringLiteral("Reset text"), colorPanel);
    auto* resetBg = new QPushButton(QStringLiteral("Reset background"), colorPanel);
    for (auto* b : {resetText, resetBg}) {
        b->setCursor(Qt::PointingHandCursor);
        b->setStyleSheet(QStringLiteral(
            "QPushButton{background:#1A1D22; border:1px solid #3A3D45; border-radius:3px;"
            "  color:#A0A8B8; font-size:10px; font-weight:700; padding:4px 8px;}"
            "QPushButton:hover{border-color:#4F9EFF; color:#E0E2E8;}"));
    }
    resetRow->addWidget(resetText);
    resetRow->addWidget(resetBg);
    resetRow->addStretch(1);
    colorPanelLay->addLayout(resetRow);
    leftLay->addWidget(colorPanel);

    auto* finePanel = new QFrame(left);
    finePanel->setObjectName(QStringLiteral("panelCard"));
    auto* finePanelLay = new QVBoxLayout(finePanel);
    finePanelLay->setContentsMargins(10, 8, 10, 8);
    finePanelLay->setSpacing(6);
    finePanelLay->addWidget(sectionLabel(QStringLiteral("LAYOUT & THEME"), finePanel));
    auto* fineRow = new QHBoxLayout();
    fineRow->setSpacing(8);
    auto* layoutCol = new QVBoxLayout();
    layoutCol->setSpacing(3);
    auto* layoutLab = new QLabel(QStringLiteral("Layout"), finePanel);
    layoutLab->setObjectName(QStringLiteral("body"));
    layoutCol->addWidget(layoutLab);
    m_layoutBox = new QComboBox(finePanel);
    m_layoutBox->addItems({QStringLiteral("Lower Third"), QStringLiteral("Center"),
                           QStringLiteral("Corner"), QStringLiteral("Full")});
    m_layoutBox->setCurrentText(mapLayoutModelToUi(st0.layout));
    layoutCol->addWidget(m_layoutBox);
    auto* themeCol = new QVBoxLayout();
    themeCol->setSpacing(3);
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

    auto* sportPanel = new QFrame(left);
    sportPanel->setObjectName(QStringLiteral("panelCard"));
    auto* sportPanelLay = new QVBoxLayout(sportPanel);
    sportPanelLay->setContentsMargins(10, 8, 10, 8);
    sportPanelLay->setSpacing(6);
    sportPanelLay->addWidget(sectionLabel(QStringLiteral("SPORT"), sportPanel));
    auto* sportGrid = new QGridLayout();
    sportGrid->setSpacing(5);
    const QStringList sports = {QStringLiteral("Generic"), QStringLiteral("8-Ball"),
                                QStringLiteral("9-Ball"), QStringLiteral("10-Ball"),
                                QStringLiteral("Baseball"), QStringLiteral("Basketball"),
                                QStringLiteral("Soccer"), QStringLiteral("Tennis")};
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
            "font-size:10px;font-weight:700;padding:6px 2px;border-radius:3px;}"
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

    // RIGHT — single-column full-width cards
    auto* right = new QWidget(main);
    auto* rightLay = new QVBoxLayout(right);
    rightLay->setContentsMargins(0, 0, 0, 0);
    rightLay->setSpacing(6);

    auto* presetHead = new QHBoxLayout();
    presetHead->addWidget(sectionLabel(QStringLiteral("PRESETS"), right));
    presetHead->addStretch(1);
    auto* presetHint = new QLabel(QStringLiteral("Filtered by sport"), right);
    presetHint->setObjectName(QStringLiteral("hint"));
    presetHead->addWidget(presetHint);
    rightLay->addLayout(presetHead);

    m_presetScroll = new QScrollArea(right);
    m_presetScroll->setWidgetResizable(true);
    m_presetScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_presetScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_presetScroll->setFrameShape(QFrame::NoFrame);
    auto* presetBody = new QWidget();
    auto* presetList = new QVBoxLayout(presetBody);
    presetList->setContentsMargins(0, 0, 14, 0); // gutter for scrollbar
    presetList->setSpacing(8);
    m_presetScroll->setWidget(presetBody);

    m_presetEmpty = new QLabel(QStringLiteral("No presets for this sport yet."), presetBody);
    m_presetEmpty->setObjectName(QStringLiteral("hint"));
    m_presetEmpty->setAlignment(Qt::AlignCenter);
    m_presetEmpty->setVisible(false);
    presetList->addWidget(m_presetEmpty);

    for (int i = 0; i < kPresetCount; ++i) {
        const auto& pr = kPresets[i];
        auto* card = new QFrame(presetBody);
        card->setObjectName(QStringLiteral("presetCard"));
        card->setCursor(Qt::PointingHandCursor);
        card->setToolTip(QString::fromUtf8(pr.blurb));
        card->setProperty("presetIndex", i);
        card->setProperty("presetSport", QString::fromUtf8(pr.sport));
        card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        card->setStyleSheet(QStringLiteral(
            "QFrame#presetCard{background:#0E1116; border:1px solid #3A3D45;}"));
        card->installEventFilter(this);

        auto* cardInner = new QVBoxLayout(card);
        cardInner->setContentsMargins(10, 10, 10, 10);
        cardInner->setSpacing(5);

        auto* thumb = new QLabel(card);
        thumb->setAlignment(Qt::AlignCenter);
        thumb->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        thumb->setScaledContents(false);
        thumb->setFixedHeight(64);
        thumb->setStyleSheet(QStringLiteral("background:#080A0C; border:none;"));

        auto* name = new QLabel(QString::fromUtf8(pr.name), card);
        name->setStyleSheet(QStringLiteral(
            "color:#E8EAEE; font-family:'DM Sans'; font-size:12px; font-weight:800;"
            "background:transparent; border:none;"));
        auto* blurb = new QLabel(QString::fromUtf8(pr.blurb), card);
        blurb->setStyleSheet(QStringLiteral(
            "color:#6A7384; font-family:'DM Sans'; font-size:10px;"
            "background:transparent; border:none;"));
        blurb->setWordWrap(true);

        cardInner->addWidget(thumb);
        cardInner->addWidget(name);
        cardInner->addWidget(blurb);

        m_presetCards.push_back(card);
        m_presetThumbs.push_back(thumb);
        presetList->addWidget(card);
    }
    presetList->addStretch(1);
    rightLay->addWidget(m_presetScroll, 1);

    filterPresetsBySport();

    mainLay->addWidget(left, 0);
    mainLay->addWidget(right, 1);
    shellLay->addWidget(main, 1);

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

    connect(m_colorABtn, &QPushButton::clicked, this, [this] {
        m_colorA = pickColor(m_colorA, QStringLiteral("Team A color"));
        styleSwatch(m_colorABtn, m_colorA);
        syncHexLabels();
        refreshPreview();
    });
    connect(m_colorBBtn, &QPushButton::clicked, this, [this] {
        m_colorB = pickColor(m_colorB, QStringLiteral("Team B color"));
        styleSwatch(m_colorBBtn, m_colorB);
        syncHexLabels();
        refreshPreview();
    });
    connect(m_textColorBtn, &QPushButton::clicked, this, [this] {
        m_textColor = pickColor(m_textColor, QStringLiteral("Text color"));
        m_useCustomText = true;
        styleSwatch(m_textColorBtn, m_textColor);
        syncHexLabels();
        refreshPreview();
    });
    connect(m_bgColorBtn, &QPushButton::clicked, this, [this] {
        m_bgColor = pickColor(m_bgColor, QStringLiteral("Background color"));
        m_useCustomBg = true;
        styleSwatch(m_bgColorBtn, m_bgColor);
        syncHexLabels();
        refreshPreview();
    });
    connect(resetText, &QPushButton::clicked, this, [this] {
        m_useCustomText = false;
        m_textColor = QColor(QStringLiteral("#FFFFFF"));
        styleSwatch(m_textColorBtn, m_textColor);
        syncHexLabels();
        refreshPreview();
    });
    connect(resetBg, &QPushButton::clicked, this, [this] {
        m_useCustomBg = false;
        m_bgColor = QColor(QStringLiteral("#0A0C10"));
        styleSwatch(m_bgColorBtn, m_bgColor);
        syncHexLabels();
        refreshPreview();
    });
    connect(m_layoutBox, &QComboBox::currentTextChanged, this, [this](const QString&) {
        refreshPreview();
    });
    connect(m_themeBox, &QComboBox::currentTextChanged, this, [this](const QString&) {
        refreshPreview();
    });
    connect(m_sportGroup, &QButtonGroup::buttonClicked, this, [this](QAbstractButton*) {
        filterPresetsBySport();
        QTimer::singleShot(0, this, [this] { rescalePresetThumbs(); });
        refreshPreview();
    });

    connect(box, &QDialogButtonBox::accepted, this, [this, model] {
        auto st = model->state();
        QString sportUi = QStringLiteral("Generic");
        if (auto* checked = m_sportGroup->checkedButton())
            sportUi = checked->text();
        st.sport = mapSportUiToModel(sportUi);
        if (isPoolSport(st.sport))
            st.pocketedMask &= (1 << poolObjectBallCount(st.sport)) - 1;
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
}

} // namespace railshot
