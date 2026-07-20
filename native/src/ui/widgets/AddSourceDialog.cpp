#include "ui/widgets/AddSourceDialog.h"
#include "core/EngineController.h"
#include "capture/MediaFoundationCamera.h"
#include "ui/Motion.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QStackedWidget>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QLabel>
#include <QListWidget>
#include <QFrame>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <dxgi.h>
#pragma comment(lib, "dxgi.lib")
#endif

namespace railshot {

namespace {
QVector<QPair<int, QString>> enumerateMonitors()
{
    QVector<QPair<int, QString>> out;
#ifdef _WIN32
    IDXGIFactory1* factory = nullptr;
    if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&factory))) || !factory)
        return out;
    IDXGIAdapter1* adapter = nullptr;
    int globalIndex = 0;
    for (UINT ai = 0; factory->EnumAdapters1(ai, &adapter) != DXGI_ERROR_NOT_FOUND; ++ai) {
        IDXGIOutput* output = nullptr;
        for (UINT oi = 0; adapter->EnumOutputs(oi, &output) != DXGI_ERROR_NOT_FOUND; ++oi) {
            DXGI_OUTPUT_DESC desc{};
            output->GetDesc(&desc);
            out.append({globalIndex, QStringLiteral("Monitor %1 (%2)")
                                         .arg(globalIndex)
                                         .arg(QString::fromWCharArray(desc.DeviceName))});
            ++globalIndex;
            output->Release();
        }
        adapter->Release();
    }
    factory->Release();
#endif
    if (out.isEmpty())
        out.append({0, QStringLiteral("Monitor 0")});
    return out;
}

struct InputTypeEntry {
    const char* label;
    const char* color;
    SourceType type;
    bool available;
};

const InputTypeEntry kInputTypes[] = {
    {"Camera", "#22C55E", SourceType::Camera, true},
    {"Display Capture", "#4F9EFF", SourceType::Display, true},
    {"Image", "#FBBF24", SourceType::Image, true},
    {"Title / Text", "#EC4899", SourceType::Text, true},
    {"Web Browser", "#3B82F6", SourceType::Browser, true},
    {"Scoreboard", "#A855F7", SourceType::Scoreboard, true},
    {"Lower Third", "#F472B6", SourceType::LowerThird, true},
    {"Colour", "#EF4444", SourceType::Color, true},
    {"Media / Video", "#FB923C", SourceType::Media, true},
    {"NDI / OMT", "#06B6D4", SourceType::Ndi, true},
    {"Alert / Stinger", "#F59E0B", SourceType::Alert, true},
    {"Video (file)", "#94A3B8", SourceType::Media, false},
    {"DVD", "#94A3B8", SourceType::Unknown, false},
    {"List", "#94A3B8", SourceType::Unknown, false},
    {"Stream / SRT", "#94A3B8", SourceType::Unknown, false},
    {"Instant Replay", "#94A3B8", SourceType::Unknown, false},
    {"Image Sequence", "#94A3B8", SourceType::Image, false},
    {"Video Delay", "#94A3B8", SourceType::Unknown, false},
    {"Photos", "#94A3B8", SourceType::Image, false},
    {"PowerPoint", "#94A3B8", SourceType::Unknown, false},
    {"Audio Input", "#94A3B8", SourceType::Unknown, false},
    {"Virtual Set", "#94A3B8", SourceType::Unknown, false},
    {"Video Call", "#94A3B8", SourceType::Unknown, false},
    {"Zoom", "#94A3B8", SourceType::Unknown, false},
    {"Telestrator", "#94A3B8", SourceType::Unknown, false},
    {"Overlay", "#94A3B8", SourceType::Browser, false},
};
} // namespace

AddSourceDialog::AddSourceDialog(EngineController* engine, QWidget* parent)
    : QDialog(parent), m_engine(engine)
{
    setWindowTitle(QStringLiteral("Input Select"));
    resize(780, 520);
    setMaximumHeight(int(0.88 * 900));
    setStyleSheet(QStringLiteral(
        "QDialog{background:#141618;border:1px solid #3A3D45;}"
        "QListWidget{background:#0F1114;border:none;outline:none;color:#C8CAD0;}"
        "QListWidget::item{padding:8px 12px;border-left:3px solid transparent;}"
        "QListWidget::item:selected{background:#1A3A6A;}"
        "QLineEdit,QPlainTextEdit,QComboBox{background:#0A0C10;border:1px solid #3A3D45;"
        "  color:#D0D2D8;border-radius:3px;padding:6px;}"
        "QLabel{color:#D0D2D8;background:transparent;}"));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* header = new QLabel(QStringLiteral("  Input Select"), this);
    header->setFixedHeight(36);
    header->setStyleSheet(QStringLiteral(
        "background:#0F1114; border-bottom:1px solid #2A2D35; font-weight:700; font-size:13px; padding-left:12px;"));
    root->addWidget(header);

    auto* body = new QHBoxLayout();
    body->setContentsMargins(0, 0, 0, 0);
    body->setSpacing(0);

    m_typeList = new QListWidget(this);
    m_typeList->setFixedWidth(210);
    for (const auto& e : kInputTypes) {
        auto* item = new QListWidgetItem(QString::fromLatin1(e.label), m_typeList);
        item->setData(Qt::UserRole, int(e.type));
        item->setData(Qt::UserRole + 1, e.available);
        item->setData(Qt::UserRole + 2, QString::fromLatin1(e.color));
        item->setForeground(QColor(e.available ? QStringLiteral("#C8CAD0") : QStringLiteral("#505860")));
        if (!e.available)
            item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
    }
    connect(m_typeList, &QListWidget::currentRowChanged, this, &AddSourceDialog::selectType);
    body->addWidget(m_typeList);

    auto* right = new QWidget(this);
    right->setStyleSheet(QStringLiteral("background:#141618;"));
    auto* rightLay = new QVBoxLayout(right);
    rightLay->setContentsMargins(20, 16, 20, 12);
    m_typeTitle = new QLabel(QStringLiteral("Camera"), right);
    m_typeTitle->setStyleSheet(QStringLiteral("font-size:16px; font-weight:800; color:#22C55E;"));
    rightLay->addWidget(m_typeTitle);

    auto* nameForm = new QFormLayout();
    m_name = new QLineEdit(right);
    nameForm->addRow(QStringLiteral("Name"), m_name);
    rightLay->addLayout(nameForm);

    m_stack = new QStackedWidget(right);

    auto* camPage = new QWidget(m_stack);
    auto* camForm = new QFormLayout(camPage);
    m_camera = new QComboBox(camPage);
    for (const auto& d : MediaFoundationCamera::enumerateDevices())
        m_camera->addItem(d.second, d.first);
    if (m_camera->count() == 0)
        m_camera->addItem(QStringLiteral("(No cameras found)"), QString());
    camForm->addRow(QStringLiteral("Device"), m_camera);
    m_stack->addWidget(camPage);

    auto* monPage = new QWidget(m_stack);
    auto* monForm = new QFormLayout(monPage);
    m_monitor = new QComboBox(monPage);
    for (const auto& m : enumerateMonitors())
        m_monitor->addItem(m.second, m.first);
    monForm->addRow(QStringLiteral("Monitor"), m_monitor);
    m_stack->addWidget(monPage);

    auto* imgPage = new QWidget(m_stack);
    auto* imgForm = new QFormLayout(imgPage);
    m_imagePath = new QLineEdit(imgPage);
    auto* browse = new QPushButton(QStringLiteral("Browse…"), imgPage);
    connect(browse, &QPushButton::clicked, this, [this] {
        const auto path = QFileDialog::getOpenFileName(this, QStringLiteral("Image"),
                                                       {}, QStringLiteral("Images (*.png *.jpg *.jpeg *.bmp *.webp)"));
        if (!path.isEmpty()) m_imagePath->setText(path);
    });
    imgForm->addRow(QStringLiteral("Path"), m_imagePath);
    imgForm->addRow(browse);
    m_stack->addWidget(imgPage);

    auto* textPage = new QWidget(m_stack);
    auto* textForm = new QVBoxLayout(textPage);
    m_text = new QPlainTextEdit(textPage);
    m_text->setPlainText(QStringLiteral("RailShotTV"));
    textForm->addWidget(new QLabel(QStringLiteral("Text content"), textPage));
    textForm->addWidget(m_text);
    m_stack->addWidget(textPage);

    auto* brPage = new QWidget(m_stack);
    auto* brForm = new QFormLayout(brPage);
    m_browserUrl = new QLineEdit(QStringLiteral("https://example.com"), brPage);
    brForm->addRow(QStringLiteral("URL"), m_browserUrl);
    m_stack->addWidget(brPage);

    m_stack->addWidget(new QLabel(QStringLiteral("Uses the live Scoreboard model."), m_stack));
    m_stack->addWidget(new QLabel(QStringLiteral("Lower-third overlay source."), m_stack));

    auto* colorPage = new QWidget(m_stack);
    auto* colorForm = new QFormLayout(colorPage);
    m_colorHex = new QLineEdit(QStringLiteral("#1A1A1A"), colorPage);
    colorForm->addRow(QStringLiteral("Colour"), m_colorHex);
    m_stack->addWidget(colorPage);

    m_stack->addWidget(new QLabel(QStringLiteral("Media source — configure path after add (engine Media type)."), m_stack));
    m_stack->addWidget(new QLabel(QStringLiteral("NDI source — discovery UI coming soon."), m_stack));
    m_stack->addWidget(new QLabel(QStringLiteral("Alert / stinger overlay source."), m_stack));
    m_stack->addWidget(new QLabel(QStringLiteral("This input type is listed for catalogue parity and is not available yet."), m_stack));

    rightLay->addWidget(m_stack, 1);
    body->addWidget(right, 1);
    root->addLayout(body, 1);

    auto* footer = new QFrame(this);
    footer->setFixedHeight(48);
    footer->setStyleSheet(QStringLiteral("background:#0F1114; border-top:1px solid #2A2D35;"));
    auto* foot = new QHBoxLayout(footer);
    foot->setContentsMargins(16, 8, 16, 8);
    auto* cancel = new QPushButton(QStringLiteral("Cancel"), footer);
    auto* ok = new QPushButton(QStringLiteral("OK"), footer);
    ok->setStyleSheet(QStringLiteral(
        "QPushButton{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #1A6AFF,stop:1 #1050CC);"
        "color:white;font-weight:700;border:1px solid #3A6AFF;border-radius:4px;padding:6px 22px;}"));
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(ok, &QPushButton::clicked, this, &AddSourceDialog::acceptConfigured);
    foot->addStretch();
    foot->addWidget(cancel);
    foot->addWidget(ok);
    root->addWidget(footer);

    m_typeList->setCurrentRow(0);
    rebuildFields();
    motion::playModalEnter(this);
}

void AddSourceDialog::selectType(int index)
{
    if (index < 0 || index >= int(sizeof(kInputTypes) / sizeof(kInputTypes[0]))) return;
    const auto& e = kInputTypes[index];
    if (!e.available) return;
    m_selectedType = e.type;
    m_typeTitle->setText(QString::fromLatin1(e.label));
    m_typeTitle->setStyleSheet(QStringLiteral("font-size:16px; font-weight:800; color:%1;")
                                   .arg(QString::fromLatin1(e.color)));
    if (auto* item = m_typeList->item(index)) {
        item->setData(Qt::DecorationRole, QColor(QString::fromLatin1(e.color)));
    }
    // Left accent via stylesheet per selection — update item foreground color
    for (int i = 0; i < m_typeList->count(); ++i) {
        auto* it = m_typeList->item(i);
        const QString col = it->data(Qt::UserRole + 2).toString();
        if (i == index)
            it->setForeground(QColor(col));
        else if (it->flags() & Qt::ItemIsEnabled)
            it->setForeground(QColor(QStringLiteral("#C8CAD0")));
    }
    rebuildFields();
}

void AddSourceDialog::rebuildFields()
{
    if (m_name->text().isEmpty() || m_name->text() == m_typeTitle->text())
        m_name->setText(m_typeTitle->text());
    switch (m_selectedType) {
    case SourceType::Camera: m_stack->setCurrentIndex(0); break;
    case SourceType::Display: m_stack->setCurrentIndex(1); break;
    case SourceType::Image: m_stack->setCurrentIndex(2); break;
    case SourceType::Text: m_stack->setCurrentIndex(3); break;
    case SourceType::Browser: m_stack->setCurrentIndex(4); break;
    case SourceType::Scoreboard: m_stack->setCurrentIndex(5); break;
    case SourceType::LowerThird: m_stack->setCurrentIndex(6); break;
    case SourceType::Color: m_stack->setCurrentIndex(7); break;
    case SourceType::Media: m_stack->setCurrentIndex(8); break;
    case SourceType::Ndi: m_stack->setCurrentIndex(9); break;
    case SourceType::Alert: m_stack->setCurrentIndex(10); break;
    default: m_stack->setCurrentIndex(11); break;
    }
}

void AddSourceDialog::acceptConfigured()
{
    if (m_selectedType == SourceType::Unknown) return;
    m_result.type = m_selectedType;
    m_result.name = m_name->text().trimmed();
    if (m_result.name.isEmpty())
        m_result.name = m_typeTitle->text();
    m_result.settings = {};
    switch (m_result.type) {
    case SourceType::Camera:
        m_result.settings.insert(QStringLiteral("deviceId"), m_camera->currentData().toString());
        break;
    case SourceType::Display:
        m_result.settings.insert(QStringLiteral("monitorIndex"), m_monitor->currentData().toInt());
        break;
    case SourceType::Image:
        if (m_imagePath->text().trimmed().isEmpty()) return;
        m_result.settings.insert(QStringLiteral("path"), m_imagePath->text().trimmed());
        break;
    case SourceType::Text:
        m_result.settings.insert(QStringLiteral("text"), m_text->toPlainText());
        break;
    case SourceType::Browser:
        m_result.settings.insert(QStringLiteral("url"), m_browserUrl->text().trimmed());
        m_result.settings.insert(QStringLiteral("width"), 1280);
        m_result.settings.insert(QStringLiteral("height"), 720);
        m_result.settings.insert(QStringLiteral("fps"), 15);
        break;
    case SourceType::Color:
        m_result.settings.insert(QStringLiteral("color"), m_colorHex->text().trimmed());
        break;
    default:
        break;
    }
    m_result.accepted = true;
    accept();
}

} // namespace railshot
