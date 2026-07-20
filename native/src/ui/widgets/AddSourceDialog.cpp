#include "ui/widgets/AddSourceDialog.h"
#include "core/EngineController.h"
#include "capture/MediaFoundationCamera.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QStackedWidget>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QDialogButtonBox>
#include <QLabel>

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
} // namespace

AddSourceDialog::AddSourceDialog(EngineController* engine, QWidget* parent)
    : QDialog(parent), m_engine(engine)
{
    setWindowTitle(QStringLiteral("Add Input"));
    resize(420, 360);
    auto* root = new QVBoxLayout(this);
    auto* form = new QFormLayout();
    m_type = new QComboBox(this);
    m_type->addItem(QStringLiteral("Camera"), int(SourceType::Camera));
    m_type->addItem(QStringLiteral("Display Capture"), int(SourceType::Display));
    m_type->addItem(QStringLiteral("Image"), int(SourceType::Image));
    m_type->addItem(QStringLiteral("Text"), int(SourceType::Text));
    m_type->addItem(QStringLiteral("Browser"), int(SourceType::Browser));
    m_type->addItem(QStringLiteral("Scoreboard"), int(SourceType::Scoreboard));
    m_type->addItem(QStringLiteral("Lower Third"), int(SourceType::LowerThird));
    m_name = new QLineEdit(this);
    form->addRow(QStringLiteral("Type"), m_type);
    form->addRow(QStringLiteral("Name"), m_name);
    root->addLayout(form);

    m_stack = new QStackedWidget(this);
    // Camera
    auto* camPage = new QWidget(m_stack);
    auto* camForm = new QFormLayout(camPage);
    m_camera = new QComboBox(camPage);
    for (const auto& d : MediaFoundationCamera::enumerateDevices())
        m_camera->addItem(d.second, d.first);
    if (m_camera->count() == 0)
        m_camera->addItem(QStringLiteral("(No cameras found)"), QString());
    camForm->addRow(QStringLiteral("Device"), m_camera);
    m_stack->addWidget(camPage);
    // Display
    auto* monPage = new QWidget(m_stack);
    auto* monForm = new QFormLayout(monPage);
    m_monitor = new QComboBox(monPage);
    for (const auto& m : enumerateMonitors())
        m_monitor->addItem(m.second, m.first);
    monForm->addRow(QStringLiteral("Monitor"), m_monitor);
    monForm->addRow(new QLabel(QStringLiteral("Window capture requires WGC (later). Use monitor capture."), monPage));
    m_stack->addWidget(monPage);
    // Image
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
    // Text
    auto* textPage = new QWidget(m_stack);
    auto* textForm = new QVBoxLayout(textPage);
    m_text = new QPlainTextEdit(textPage);
    m_text->setPlainText(QStringLiteral("RailShotTV"));
    textForm->addWidget(new QLabel(QStringLiteral("Text content"), textPage));
    textForm->addWidget(m_text);
    m_stack->addWidget(textPage);
    // Browser
    auto* brPage = new QWidget(m_stack);
    auto* brForm = new QFormLayout(brPage);
    m_browserUrl = new QLineEdit(QStringLiteral("https://example.com"), brPage);
    brForm->addRow(QStringLiteral("URL"), m_browserUrl);
    m_stack->addWidget(brPage);
    // Scoreboard / Lower third — no extra fields
    m_stack->addWidget(new QLabel(QStringLiteral("Uses live Scoreboard model."), m_stack));
    m_stack->addWidget(new QLabel(QStringLiteral("Lower-third overlay source."), m_stack));

    root->addWidget(m_stack, 1);
    connect(m_type, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) { rebuildFields(); });

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &AddSourceDialog::acceptConfigured);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(buttons);
    rebuildFields();
}

void AddSourceDialog::rebuildFields()
{
    const auto type = SourceType(m_type->currentData().toInt());
    if (m_name->text().isEmpty() || m_name->text() == m_type->itemText(m_type->currentIndex()))
        m_name->setText(m_type->currentText());
    switch (type) {
    case SourceType::Camera: m_stack->setCurrentIndex(0); break;
    case SourceType::Display: m_stack->setCurrentIndex(1); break;
    case SourceType::Image: m_stack->setCurrentIndex(2); break;
    case SourceType::Text: m_stack->setCurrentIndex(3); break;
    case SourceType::Browser: m_stack->setCurrentIndex(4); break;
    case SourceType::Scoreboard: m_stack->setCurrentIndex(5); break;
    case SourceType::LowerThird: m_stack->setCurrentIndex(6); break;
    default: m_stack->setCurrentIndex(0); break;
    }
}

void AddSourceDialog::acceptConfigured()
{
    m_result.type = SourceType(m_type->currentData().toInt());
    m_result.name = m_name->text().trimmed();
    if (m_result.name.isEmpty())
        m_result.name = m_type->currentText();
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
    default:
        break;
    }
    m_result.accepted = true;
    accept();
}

} // namespace railshot
