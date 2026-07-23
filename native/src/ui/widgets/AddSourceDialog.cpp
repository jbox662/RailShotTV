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
#include <QScrollArea>
#include <QEventLoop>
#include <QScreen>
#include <QGuiApplication>
#include <QCursor>
#include <QEvent>
#include <QMouseEvent>
#include <QJsonArray>
#include <QJsonObject>

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
    {SourceType::Slideshow, "Image Slide Show"},
    {SourceType::Media, "Media Source"},
    {SourceType::Text, "Text (GDI+)"},
    {SourceType::Browser, "Browser"},
    {SourceType::Color, "Color Source"},
    {SourceType::Scoreboard, "Scoreboard"},
    {SourceType::LowerThird, "Lower Third"},
    {SourceType::Ndi, "NDI Source"},
    {SourceType::Scene, "Scene"},
    {SourceType::Group, "Group"},
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
        s.insert(QStringLiteral("isLocalFile"), true);
        s.insert(QStringLiteral("loop"), true);
        s.insert(QStringLiteral("ffmpegOptions"), QString());
        break;
    case SourceType::Slideshow:
        s.insert(QStringLiteral("paths"), QJsonArray{});
        s.insert(QStringLiteral("intervalMs"), 5000);
        s.insert(QStringLiteral("loop"), true);
        break;
    case SourceType::Scene:
        s.insert(QStringLiteral("sceneId"), QString());
        break;
    case SourceType::Group:
        s.insert(QStringLiteral("childIds"), QJsonArray{});
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
    // Custom popup (not QMenu) so we get scroll + Chromatic styling + correct placement.
    auto* popup = new QFrame(parent, Qt::Popup | Qt::FramelessWindowHint);
    popup->setAttribute(Qt::WA_DeleteOnClose);
    popup->setObjectName(QStringLiteral("addSourceTypePopup"));
    popup->setStyleSheet(QStringLiteral(
        "QFrame#addSourceTypePopup{"
        "  background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #1E2228,stop:1 #12151A);"
        "  border:1px solid #5A5E68; border-radius:6px;"
        "}"
        "QLabel#popupTitle{"
        "  color:#4F9EFF; font-size:10px; font-weight:800; letter-spacing:1.5px;"
        "  padding:10px 14px 6px; background:transparent;"
        "}"
        "QScrollArea{background:transparent; border:none;}"));

    auto* root = new QVBoxLayout(popup);
    root->setContentsMargins(6, 4, 6, 8);
    root->setSpacing(2);

    auto* title = new QLabel(QStringLiteral("ADD SOURCE"), popup);
    title->setObjectName(QStringLiteral("popupTitle"));
    root->addWidget(title);

    auto* rule = new QFrame(popup);
    rule->setFixedHeight(1);
    rule->setStyleSheet(QStringLiteral("background:#3A3D45; border:none; margin:0 8px;"));
    root->addWidget(rule);

    auto* scroll = new QScrollArea(popup);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet(QStringLiteral(
        "QScrollBar:vertical{background:#0A0C0F; width:8px; margin:2px;}"
        "QScrollBar::handle:vertical{background:#4A4D55; border-radius:3px; min-height:24px;}"
        "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}"));

    auto* list = new QWidget;
    list->setObjectName(QStringLiteral("popupList"));
    auto* listLay = new QVBoxLayout(list);
    listLay->setContentsMargins(2, 4, 2, 4);
    listLay->setSpacing(1);

    struct ColoredType {
        SourceType type;
        const char* label;
        const char* color;
    };
    const ColoredType types[] = {
        {SourceType::Camera, "Video Capture Device", "#22C55E"},
        {SourceType::Display, "Display Capture", "#4F9EFF"},
        {SourceType::Window, "Window Capture", "#38BDF8"},
        {SourceType::Game, "Game Capture", "#A3E635"},
        {SourceType::AudioInput, "Audio Input Capture", "#F472B6"},
        {SourceType::AudioOutput, "Audio Output Capture", "#FB7185"},
        {SourceType::Image, "Image", "#FBBF24"},
        {SourceType::Slideshow, "Image Slide Show", "#F59E0B"},
        {SourceType::Media, "Media Source", "#FB923C"},
        {SourceType::Text, "Text (GDI+)", "#EC4899"},
        {SourceType::Browser, "Browser", "#3B82F6"},
        {SourceType::Color, "Color Source", "#EF4444"},
        {SourceType::Scoreboard, "Scoreboard", "#A855F7"},
        {SourceType::LowerThird, "Lower Third", "#F472B6"},
        {SourceType::Ndi, "NDI Source", "#06B6D4"},
        {SourceType::Scene, "Scene", "#94A3B8"},
        {SourceType::Group, "Group", "#64748B"},
        {SourceType::Alert, "Alert / Stinger", "#F59E0B"},
    };

    SourceType chosen = SourceType::Unknown;
    QEventLoop loop;

    for (const auto& e : types) {
        auto* row = new QFrame(list);
        row->setObjectName(QStringLiteral("typeRow"));
        row->setCursor(Qt::PointingHandCursor);
        row->setFixedHeight(34);
        row->setAttribute(Qt::WA_Hover, true);
        row->setStyleSheet(QStringLiteral(
            "QFrame#typeRow{background:transparent; border-radius:4px;}"
            "QFrame#typeRow:hover{background:#102438;}"));
        auto* rowLay = new QHBoxLayout(row);
        rowLay->setContentsMargins(10, 0, 10, 0);
        rowLay->setSpacing(10);
        auto* dot = new QFrame(row);
        dot->setFixedSize(7, 7);
        dot->setStyleSheet(QStringLiteral(
            "background:%1; border-radius:3px; border:none;").arg(QString::fromLatin1(e.color)));
        auto* lab = new QLabel(QString::fromUtf8(e.label), row);
        lab->setStyleSheet(QStringLiteral(
            "background:transparent; color:#E0E2E8; font-size:12px; font-weight:600; border:none;"));
        lab->setAttribute(Qt::WA_TransparentForMouseEvents);
        rowLay->addWidget(dot, 0, Qt::AlignVCenter);
        rowLay->addWidget(lab, 1);
        row->setProperty("sourceType", static_cast<int>(e.type));
        listLay->addWidget(row);
    }
    listLay->addStretch(1);
    scroll->setWidget(list);
    root->addWidget(scroll, 1);

    // Route clicks on type rows (hover via stylesheet; click via filter on popup).
    class ClickFilter : public QObject {
    public:
        SourceType* out = nullptr;
        QEventLoop* loop = nullptr;
        QWidget* popup = nullptr;
        bool eventFilter(QObject* obj, QEvent* ev) override
        {
            if (ev->type() == QEvent::MouseButtonRelease) {
                if (auto* w = qobject_cast<QWidget*>(obj)) {
                    const QVariant v = w->property("sourceType");
                    if (v.isValid() && out && loop && popup) {
                        *out = static_cast<SourceType>(v.toInt());
                        loop->quit();
                        popup->close();
                        return true;
                    }
                }
            }
            return QObject::eventFilter(obj, ev);
        }
    };
    auto* filter = new ClickFilter();
    filter->out = &chosen;
    filter->loop = &loop;
    filter->popup = popup;
    filter->setParent(popup);
    for (auto* child : list->findChildren<QFrame*>(QStringLiteral("typeRow")))
        child->installEventFilter(filter);

    popup->setFixedWidth(268);
    const int rows = int(sizeof(types) / sizeof(types[0]));
    const int idealH = 52 + rows * 34 + 16;
    QScreen* screen = QGuiApplication::screenAt(globalPos);
    if (!screen && parent && parent->window())
        screen = parent->window()->screen();
    if (!screen)
        screen = QGuiApplication::primaryScreen();
    const QRect avail = screen ? screen->availableGeometry() : QRect(0, 0, 1920, 1080);
    const int maxH = qMin(idealH, qMin(440, avail.height() - 32));
    popup->setFixedHeight(maxH);

    int x = globalPos.x();
    int y = globalPos.y() - maxH; // open upward from Sources +
    if (y < avail.top() + 8) {
        // Not enough room above — open downward but clamp to screen.
        y = globalPos.y();
        if (y + maxH > avail.bottom() - 8)
            y = avail.bottom() - maxH - 8;
    }
    if (x + popup->width() > avail.right() - 8)
        x = avail.right() - popup->width() - 8;
    if (x < avail.left() + 8)
        x = avail.left() + 8;
    if (y < avail.top() + 8)
        y = avail.top() + 8;

    popup->move(x, y);
    popup->show();
    popup->raise();

    QObject::connect(popup, &QObject::destroyed, &loop, &QEventLoop::quit);
    loop.exec();
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
