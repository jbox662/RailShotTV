#include "ui/widgets/ScoreboardSettingsDialog.h"
#include "core/EngineController.h"
#include "scoreboard/ScoreboardModel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QButtonGroup>
#include <QAbstractButton>
#include <QDialogButtonBox>

namespace railshot {

namespace {
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
    if (ui == QLatin1String("Team")) return QStringLiteral("broadcast");
    if (ui == QLatin1String("Neon")) return QStringLiteral("neon");
    return QStringLiteral("railshot");
}
QString mapThemeModelToUi(const QString& model)
{
    if (model == QLatin1String("classic")) return QStringLiteral("Light");
    if (model == QLatin1String("broadcast")) return QStringLiteral("Team");
    if (model == QLatin1String("neon")) return QStringLiteral("Neon");
    return QStringLiteral("Dark");
}
QString mapLayoutUiToModel(const QString& ui)
{
    if (ui == QLatin1String("Center")) return QStringLiteral("standard");
    if (ui == QLatin1String("Corner")) return QStringLiteral("compact");
    if (ui == QLatin1String("Full")) return QStringLiteral("wide");
    return QStringLiteral("standard");
}
QString mapLayoutModelToUi(const QString& model)
{
    if (model == QLatin1String("compact")) return QStringLiteral("Corner");
    if (model == QLatin1String("wide")) return QStringLiteral("Full");
    // "standard" covers both Lower Third and Center in the old UI mapping.
    return QStringLiteral("Lower Third");
}
} // namespace

ScoreboardSettingsDialog::ScoreboardSettingsDialog(EngineController* engine, QWidget* parent)
    : QDialog(parent), m_engine(engine)
{
    setWindowTitle(QStringLiteral("Scoreboard Settings"));
    setMinimumWidth(360);
    resize(400, 420);
    setStyleSheet(QStringLiteral(
        "QDialog{background:#0F1114;}"
        "QLabel{color:#C8CCD4; font-family:'DM Sans'; font-size:11px;}"
        "QLabel#sec{color:#22C55E; font-size:10px; font-weight:900; letter-spacing:1px;}"
        "QComboBox{"
        "  background:#12151A; border:1px solid #3A3D45; border-radius:3px;"
        "  color:#E0E2E8; padding:4px 8px; min-height:24px;}"
        "QComboBox:focus{border-color:#22C55E;}"
        "QPushButton{background:#1A1E26; border:1px solid #5A5E68; border-radius:3px;"
        "  color:#E0E2E8; font-weight:700; padding:6px 14px;}"
        "QPushButton:hover{border-color:#4F9EFF;}"));

    auto* model = engine->scoreboard();
    const auto st0 = model->state();

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(14, 14, 14, 14);
    root->setSpacing(10);

    auto* sportLab = new QLabel(QStringLiteral("SPORT"), this);
    sportLab->setObjectName(QStringLiteral("sec"));
    root->addWidget(sportLab);

    auto* sportGrid = new QGridLayout();
    sportGrid->setSpacing(4);
    const QStringList sports = {QStringLiteral("Generic"), QStringLiteral("Pool"),
                                QStringLiteral("Basketball"), QStringLiteral("Soccer"),
                                QStringLiteral("Tennis"), QStringLiteral("Custom")};
    auto* sportGroup = new QButtonGroup(this);
    sportGroup->setExclusive(true);
    const QString sportUi = mapSportModelToUi(st0.sport);
    int si = 0;
    for (const auto& sp : sports) {
        auto* b = new QPushButton(sp, this);
        b->setCheckable(true);
        b->setCursor(Qt::PointingHandCursor);
        b->setStyleSheet(QStringLiteral(
            "QPushButton{background:#1A1D22;border:1px solid #3A3D45;color:#A0A8B8;"
            "font-size:10px;font-weight:700;padding:8px 4px;border-radius:3px;}"
            "QPushButton:checked{border:2px solid #F59E0B;color:#FBBF24;background:#2A2010;}"));
        if (sp == sportUi)
            b->setChecked(true);
        sportGroup->addButton(b);
        sportGrid->addWidget(b, si / 3, si % 3);
        ++si;
    }
    if (!sportGroup->checkedButton() && sportGroup->buttons().size() > 0)
        sportGroup->buttons().first()->setChecked(true);
    root->addLayout(sportGrid);

    auto* layoutLab = new QLabel(QStringLiteral("LAYOUT"), this);
    layoutLab->setObjectName(QStringLiteral("sec"));
    root->addWidget(layoutLab);
    auto* layoutBox = new QComboBox(this);
    layoutBox->addItems({QStringLiteral("Lower Third"), QStringLiteral("Center"),
                         QStringLiteral("Corner"), QStringLiteral("Full")});
    layoutBox->setCurrentText(mapLayoutModelToUi(st0.layout));
    root->addWidget(layoutBox);

    auto* themeLab = new QLabel(QStringLiteral("THEME"), this);
    themeLab->setObjectName(QStringLiteral("sec"));
    root->addWidget(themeLab);
    auto* themeBox = new QComboBox(this);
    themeBox->addItems({QStringLiteral("Dark"), QStringLiteral("Light"), QStringLiteral("Team"),
                        QStringLiteral("Neon")});
    themeBox->setCurrentText(mapThemeModelToUi(st0.theme));
    root->addWidget(themeBox);

    root->addStretch(1);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    root->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, [this, model, sportGroup, layoutBox, themeBox] {
        auto st = model->state();
        QString sportUi = QStringLiteral("Generic");
        if (auto* checked = sportGroup->checkedButton())
            sportUi = checked->text();
        st.sport = mapSportUiToModel(sportUi);
        st.theme = mapThemeUiToModel(themeBox->currentText());
        st.layout = mapLayoutUiToModel(layoutBox->currentText());
        model->setState(st);
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

} // namespace railshot
