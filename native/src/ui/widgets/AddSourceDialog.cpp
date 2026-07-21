#include "ui/widgets/AddSourceDialog.h"
#include "core/EngineController.h"
#include "core/Project.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <QRadioButton>
#include <QButtonGroup>
#include <QMenu>
#include <QSet>
#include <QWidgetAction>
#include <QFrame>

namespace railshot {

namespace {

struct TypeEntry {
    SourceType type;
    const char* label;
};

const TypeEntry kMenuTypes[] = {
    {SourceType::Camera, "Video Capture Device"},
    {SourceType::Display, "Display Capture"},
    {SourceType::Window, "Window Capture"},
    {SourceType::Game, "Game Capture"},
    {SourceType::AudioInput, "Audio Input Capture"},
    {SourceType::AudioOutput, "Audio Output Capture"},
    {SourceType::Image, "Image"},
    {SourceType::Media, "Media Source"},
    {SourceType::Text, "Text (GDI+)"},
    {SourceType::Browser, "Browser"},
    {SourceType::Color, "Color Source"},
    {SourceType::Scoreboard, "Scoreboard"},
    {SourceType::LowerThird, "Lower Third"},
    {SourceType::Ndi, "NDI Source"},
    {SourceType::Alert, "Alert / Stinger"},
};

QJsonObject defaultSettingsFor(SourceType type)
{
    QJsonObject s;
    switch (type) {
    case SourceType::Camera:
        s.insert(QStringLiteral("deviceId"), QString());
        break;
    case SourceType::Display:
        s.insert(QStringLiteral("monitor"), 0);
        break;
    case SourceType::Browser:
        s.insert(QStringLiteral("url"), QStringLiteral("https://obsproject.com"));
        s.insert(QStringLiteral("width"), 1280);
        s.insert(QStringLiteral("height"), 720);
        s.insert(QStringLiteral("fps"), 30);
        break;
    case SourceType::Text:
        s.insert(QStringLiteral("text"), QStringLiteral("Text"));
        break;
    case SourceType::Color:
        s.insert(QStringLiteral("color"), QStringLiteral("#4F9EFF"));
        break;
    case SourceType::Media:
        s.insert(QStringLiteral("loop"), true);
        break;
    default:
        break;
    }
    return s;
}

QString uniqueName(EngineController* engine, const QString& base)
{
    if (!engine) return base;
    const auto p = engine->projectSnapshot();
    auto used = [&](const QString& n) {
        for (const auto& sc : p.scenes)
            for (const auto& src : sc.sources)
                if (src.name.compare(n, Qt::CaseInsensitive) == 0)
                    return true;
        return false;
    };
    if (!used(base)) return base;
    for (int i = 2; i < 1000; ++i) {
        const QString n = QStringLiteral("%1 %2").arg(base).arg(i);
        if (!used(n)) return n;
    }
    return base + QStringLiteral(" 2");
}

} // namespace

QString AddSourceDialog::defaultNameForType(SourceType type)
{
    for (const auto& e : kMenuTypes) {
        if (e.type == type)
            return QString::fromUtf8(e.label);
    }
    return sourceTypeToString(type);
}

SourceType showAddSourceTypeMenu(QWidget* parent, const QPoint& globalPos)
{
    QMenu menu(parent);
    menu.setStyleSheet(QStringLiteral(
        "QMenu{background:#1A1D24; border:1px solid #4A4D55; color:#E0E2E8; padding:4px 0;}"
        "QMenu::item{padding:8px 28px 8px 16px;}"
        "QMenu::item:selected{background:#102438; color:#FFFFFF;}"
        "QMenu::separator{height:1px; background:#2A2D35; margin:4px 8px;}"));

    auto* title = new QLabel(QStringLiteral("Add Source"), &menu);
    title->setStyleSheet(QStringLiteral(
        "color:#8892A4; font-size:10px; font-weight:800; letter-spacing:1px; padding:6px 16px 4px;"));
    auto* titleAct = new QWidgetAction(&menu);
    titleAct->setDefaultWidget(title);
    menu.addAction(titleAct);
    menu.addSeparator();

    SourceType chosen = SourceType::Unknown;
    for (const auto& e : kMenuTypes) {
        QAction* act = menu.addAction(QString::fromUtf8(e.label));
        act->setData(static_cast<int>(e.type));
    }
    if (QAction* picked = menu.exec(globalPos)) {
        if (picked->data().isValid())
            chosen = static_cast<SourceType>(picked->data().toInt());
    }
    return chosen;
}

AddSourceDialog::AddSourceDialog(EngineController* engine, SourceType type, QWidget* parent)
    : QDialog(parent), m_engine(engine), m_type(type)
{
    m_result.type = type;
    const QString typeLabel = defaultNameForType(type);
    setWindowTitle(QStringLiteral("Create/Select Source"));
    setObjectName(QStringLiteral("addSourceDialog"));
    setModal(true);
    setFixedWidth(420);
    setStyleSheet(QStringLiteral(
        "QDialog#addSourceDialog{background:#1A1D24; border:1px solid #4A4D55;}"
        "QLabel{color:#D0D2D8; background:transparent;}"
        "QLineEdit{background:#0A0C10; border:1px solid #4A4D55; color:#E0E2E8;"
        "  border-radius:3px; padding:6px;}"
        "QListWidget{background:#0A0C10; border:1px solid #4A4D55; color:#E0E2E8; outline:none;}"
        "QListWidget::item{padding:6px 10px;}"
        "QListWidget::item:selected{background:#102438;}"
        "QRadioButton{color:#E0E2E8; spacing:8px;}"
        "QPushButton{min-width:88px; padding:6px 14px; border-radius:3px;}"
        "QPushButton#okBtn{background:#3A6AFF; color:white; border:1px solid #5A8AFF; font-weight:700;}"
        "QPushButton#cancelBtn{background:transparent; color:#A0A8B8; border:1px solid #4A4D55;}"));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 14, 16, 14);
    root->setSpacing(10);

    auto* heading = new QLabel(typeLabel, this);
    heading->setStyleSheet(QStringLiteral(
        "font-size:15px; font-weight:800; color:#F0F0F0; letter-spacing:0.5px;"));
    root->addWidget(heading);

    m_createNew = new QRadioButton(QStringLiteral("Create new"), this);
    m_addExisting = new QRadioButton(QStringLiteral("Add Existing"), this);
    m_createNew->setChecked(true);
    auto* group = new QButtonGroup(this);
    group->addButton(m_createNew);
    group->addButton(m_addExisting);
    root->addWidget(m_createNew);

    auto* nameRow = new QHBoxLayout();
    nameRow->addSpacing(22);
    m_name = new QLineEdit(uniqueName(engine, typeLabel), this);
    m_name->setPlaceholderText(QStringLiteral("Source name"));
    nameRow->addWidget(m_name, 1);
    root->addLayout(nameRow);

    root->addWidget(m_addExisting);
    m_existing = new QListWidget(this);
    m_existing->setMinimumHeight(140);
    m_existing->setEnabled(false);
    root->addWidget(m_existing, 1);

    m_hint = new QLabel(
        QStringLiteral("Properties open after create so you can pick a device, file, or URL."),
        this);
    m_hint->setWordWrap(true);
    m_hint->setStyleSheet(QStringLiteral("color:#606878; font-size:10px;"));
    root->addWidget(m_hint);

    auto* buttons = new QHBoxLayout();
    buttons->addStretch();
    auto* cancel = new QPushButton(QStringLiteral("Cancel"), this);
    cancel->setObjectName(QStringLiteral("cancelBtn"));
    auto* ok = new QPushButton(QStringLiteral("OK"), this);
    ok->setObjectName(QStringLiteral("okBtn"));
    ok->setDefault(true);
    buttons->addWidget(cancel);
    buttons->addWidget(ok);
    root->addLayout(buttons);

    connect(m_createNew, &QRadioButton::toggled, this, &AddSourceDialog::syncModeUi);
    connect(m_addExisting, &QRadioButton::toggled, this, &AddSourceDialog::syncModeUi);
    connect(m_existing, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem*) {
        if (m_addExisting->isChecked())
            acceptChoice();
    });
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(ok, &QPushButton::clicked, this, &AddSourceDialog::acceptChoice);

    refreshExisting();
    syncModeUi();
    m_name->selectAll();
    m_name->setFocus();
}

void AddSourceDialog::refreshExisting()
{
    m_existing->clear();
    if (!m_engine) return;
    const auto p = m_engine->projectSnapshot();
    QSet<QString> seen;
    for (const auto& sc : p.scenes) {
        for (const auto& src : sc.sources) {
            if (src.type != m_type) continue;
            if (seen.contains(src.id)) continue;
            seen.insert(src.id);
            auto* item = new QListWidgetItem(src.name, m_existing);
            item->setData(Qt::UserRole, src.id);
            item->setData(Qt::UserRole + 1, src.settings);
            item->setToolTip(QStringLiteral("From scene: %1").arg(sc.name));
        }
    }
    if (m_existing->count() == 0) {
        m_addExisting->setEnabled(false);
        m_addExisting->setToolTip(QStringLiteral("No existing sources of this type yet"));
    }
}

void AddSourceDialog::syncModeUi()
{
    const bool create = m_createNew->isChecked();
    m_name->setEnabled(create);
    m_existing->setEnabled(!create);
    if (!create && m_existing->count() > 0 && !m_existing->currentItem())
        m_existing->setCurrentRow(0);
}

void AddSourceDialog::acceptChoice()
{
    m_result = {};
    m_result.type = m_type;
    m_result.openProperties = true;

    if (m_createNew->isChecked()) {
        QString name = m_name->text().trimmed();
        if (name.isEmpty())
            name = uniqueName(m_engine, defaultNameForType(m_type));
        m_result.name = name;
        m_result.settings = defaultSettingsFor(m_type);
        m_result.accepted = true;
        accept();
        return;
    }

    auto* item = m_existing->currentItem();
    if (!item) {
        m_hint->setText(QStringLiteral("Select an existing source, or switch to Create new."));
        m_hint->setStyleSheet(QStringLiteral("color:#F87171; font-size:10px;"));
        return;
    }
    m_result.name = item->text();
    m_result.settings = item->data(Qt::UserRole + 1).toJsonObject();
    m_result.openProperties = false; // already configured
    m_result.accepted = true;
    accept();
}

} // namespace railshot
