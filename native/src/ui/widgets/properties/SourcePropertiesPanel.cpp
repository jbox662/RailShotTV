#include "ui/widgets/properties/SourcePropertiesPanel.h"
#include "core/EngineController.h"
#include "capture/MediaFoundationCamera.h"
#include "capture/NdiSource.h"
#include "audio/WasapiCapture.h"
#include "audio/AudioTypes.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QFileDialog>
#include <QColorDialog>
#include <QLabel>
#include <QListWidget>
#include <QFontComboBox>
#include <QFrame>
#include <QSignalBlocker>
#include <QSet>
#include <QJsonArray>
#include <QAbstractItemView>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
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

struct WindowEntry {
    quintptr hwnd = 0;
    QString title;
    QString exe;
};

QVector<WindowEntry> enumerateWindows()
{
    QVector<WindowEntry> out;
#ifdef _WIN32
    struct Ctx {
        QVector<WindowEntry>* list;
    } ctx{&out};
    EnumWindows(
        [](HWND hwnd, LPARAM lp) -> BOOL {
            auto* c = reinterpret_cast<Ctx*>(lp);
            if (!IsWindowVisible(hwnd)) return TRUE;
            if (GetWindow(hwnd, GW_OWNER)) return TRUE;
            wchar_t title[512];
            const int n = GetWindowTextW(hwnd, title, 512);
            if (n <= 0) return TRUE;
            WINDOWINFO wi{};
            wi.cbSize = sizeof(wi);
            if (GetWindowInfo(hwnd, &wi) && (wi.dwStyle & WS_DISABLED)) return TRUE;
            WindowEntry e;
            e.hwnd = reinterpret_cast<quintptr>(hwnd);
            e.title = QString::fromWCharArray(title);
            DWORD pid = 0;
            GetWindowThreadProcessId(hwnd, &pid);
            if (pid) {
                HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
                if (h) {
                    wchar_t path[MAX_PATH];
                    DWORD size = MAX_PATH;
                    if (QueryFullProcessImageNameW(h, 0, path, &size)) {
                        e.exe = QString::fromWCharArray(path);
                        const int slash = e.exe.lastIndexOf(QLatin1Char('\\'));
                        if (slash >= 0) e.exe = e.exe.mid(slash + 1);
                    }
                    CloseHandle(h);
                }
            }
            c->list->append(e);
            return TRUE;
        },
        reinterpret_cast<LPARAM>(&ctx));
#endif
    return out;
}

void wireChanged(SourcePropertiesPanel* self, QObject* obj)
{
    if (auto* e = qobject_cast<QLineEdit*>(obj))
        QObject::connect(e, &QLineEdit::editingFinished, self, &SourcePropertiesPanel::settingsEdited);
    else if (auto* p = qobject_cast<QPlainTextEdit*>(obj))
        QObject::connect(p, &QPlainTextEdit::textChanged, self, &SourcePropertiesPanel::settingsEdited);
    else if (auto* c = qobject_cast<QComboBox*>(obj))
        QObject::connect(c, qOverload<int>(&QComboBox::currentIndexChanged), self,
                         [self](int) { emit self->settingsEdited(); });
    else if (auto* s = qobject_cast<QSpinBox*>(obj))
        QObject::connect(s, qOverload<int>(&QSpinBox::valueChanged), self,
                         [self](int) { emit self->settingsEdited(); });
    else if (auto* d = qobject_cast<QDoubleSpinBox*>(obj))
        QObject::connect(d, qOverload<double>(&QDoubleSpinBox::valueChanged), self,
                         [self](double) { emit self->settingsEdited(); });
    else if (auto* k = qobject_cast<QCheckBox*>(obj))
        QObject::connect(k, &QCheckBox::toggled, self, [self](bool) { emit self->settingsEdited(); });
    else if (auto* f = qobject_cast<QFontComboBox*>(obj))
        QObject::connect(f, &QFontComboBox::currentFontChanged, self,
                         [self](const QFont&) { emit self->settingsEdited(); });
}

class DisplayPanel : public SourcePropertiesPanel {
public:
    DisplayPanel(EngineController* engine, QWidget* parent)
        : SourcePropertiesPanel(engine, parent)
    {
        auto* form = new QFormLayout(this);
        m_monitor = new QComboBox(this);
        for (const auto& m : enumerateMonitors())
            m_monitor->addItem(m.second, m.first);
        m_cursor = new QCheckBox(QStringLiteral("Capture cursor"), this);
        m_cursor->setChecked(true);
        m_forceSdr = new QCheckBox(QStringLiteral("Force SDR"), this);
        auto* refresh = new QPushButton(QStringLiteral("Refresh monitors"), this);
        connect(refresh, &QPushButton::clicked, this, [this] {
            const int cur = m_monitor->currentData().toInt();
            m_monitor->clear();
            for (const auto& m : enumerateMonitors())
                m_monitor->addItem(m.second, m.first);
            const int idx = m_monitor->findData(cur);
            if (idx >= 0) m_monitor->setCurrentIndex(idx);
            emit settingsEdited();
        });
        form->addRow(QStringLiteral("Monitor"), m_monitor);
        form->addRow(m_cursor);
        form->addRow(m_forceSdr);
        form->addRow(refresh);
        wireChanged(this, m_monitor);
        wireChanged(this, m_cursor);
        wireChanged(this, m_forceSdr);
    }
    void loadFrom(const SourceItem& src) override
    {
        const int mon = src.settings.value(QStringLiteral("monitorIndex")).toInt(0);
        const int idx = m_monitor->findData(mon);
        m_monitor->setCurrentIndex(idx >= 0 ? idx : 0);
        m_cursor->setChecked(src.settings.value(QStringLiteral("captureCursor")).toBool(true));
        m_forceSdr->setChecked(src.settings.value(QStringLiteral("forceSdr")).toBool(false));
    }
    void applyTo(QJsonObject& s) const override
    {
        s.insert(QStringLiteral("monitorIndex"), m_monitor->currentData().toInt());
        s.insert(QStringLiteral("captureCursor"), m_cursor->isChecked());
        s.insert(QStringLiteral("forceSdr"), m_forceSdr->isChecked());
    }
    void resetDefaults(QJsonObject& s) const override
    {
        s.insert(QStringLiteral("monitorIndex"), 0);
        s.insert(QStringLiteral("captureCursor"), true);
        s.insert(QStringLiteral("forceSdr"), false);
    }

private:
    QComboBox* m_monitor = nullptr;
    QCheckBox* m_cursor = nullptr;
    QCheckBox* m_forceSdr = nullptr;
};

class CameraPanel : public SourcePropertiesPanel {
public:
    CameraPanel(EngineController* engine, QWidget* parent)
        : SourcePropertiesPanel(engine, parent)
    {
        auto* form = new QFormLayout(this);
        m_device = new QComboBox(this);
        try {
            for (const auto& d : MediaFoundationCamera::enumerateDevices())
                m_device->addItem(d.second, d.first);
        } catch (...) {
        }
        if (m_device->count() == 0)
            m_device->addItem(QStringLiteral("(No cameras)"), QString());
        m_res = new QComboBox(this);
        m_res->addItem(QStringLiteral("Device default"), QString());
        m_res->addItem(QStringLiteral("1280×720"), QStringLiteral("1280x720"));
        m_res->addItem(QStringLiteral("1920×1080"), QStringLiteral("1920x1080"));
        m_fps = new QSpinBox(this);
        m_fps->setRange(0, 120);
        m_fps->setSpecialValueText(QStringLiteral("Default"));
        m_fps->setValue(0);
        form->addRow(QStringLiteral("Device"), m_device);
        form->addRow(QStringLiteral("Resolution"), m_res);
        form->addRow(QStringLiteral("FPS"), m_fps);
        wireChanged(this, m_device);
        wireChanged(this, m_res);
        wireChanged(this, m_fps);
    }
    void loadFrom(const SourceItem& src) override
    {
        const QString id = src.settings.value(QStringLiteral("deviceId")).toString();
        const int idx = m_device->findData(id);
        m_device->setCurrentIndex(idx >= 0 ? idx : 0);
        const int r = m_res->findData(src.settings.value(QStringLiteral("resolution")).toString());
        m_res->setCurrentIndex(r >= 0 ? r : 0);
        m_fps->setValue(src.settings.value(QStringLiteral("fps")).toInt(0));
    }
    void applyTo(QJsonObject& s) const override
    {
        s.insert(QStringLiteral("deviceId"), m_device->currentData().toString());
        s.insert(QStringLiteral("resolution"), m_res->currentData().toString());
        s.insert(QStringLiteral("fps"), m_fps->value());
    }
    void resetDefaults(QJsonObject& s) const override
    {
        s.insert(QStringLiteral("deviceId"), m_device->count() ? m_device->itemData(0).toString() : QString());
        s.insert(QStringLiteral("resolution"), QString());
        s.insert(QStringLiteral("fps"), 0);
    }

private:
    QComboBox* m_device = nullptr;
    QComboBox* m_res = nullptr;
    QSpinBox* m_fps = nullptr;
};

class BrowserPanel : public SourcePropertiesPanel {
public:
    BrowserPanel(EngineController* engine, QWidget* parent)
        : SourcePropertiesPanel(engine, parent)
    {
        auto* form = new QFormLayout(this);
        m_url = new QLineEdit(this);
        m_w = new QSpinBox(this);
        m_w->setRange(64, 7680);
        m_w->setValue(1280);
        m_h = new QSpinBox(this);
        m_h->setRange(64, 4320);
        m_h->setValue(720);
        m_fps = new QSpinBox(this);
        m_fps->setRange(1, 60);
        m_fps->setValue(15);
        m_hw = new QCheckBox(QStringLiteral("Hardware acceleration"), this);
        m_hw->setChecked(true);
        m_controlAudio = new QCheckBox(QStringLiteral("Control audio via OBS"), this);
        m_controlAudio->setChecked(false);
        m_controlAudio->setToolTip(QStringLiteral(
            "When enabled, browser audio is routed through the Audio Mixer (OBS default: off)."));
        auto* refresh = new QPushButton(QStringLiteral("Reload page"), this);
        connect(refresh, &QPushButton::clicked, this, [this] {
            // bump a token so BrowserSource reloads
            emit settingsEdited();
            if (!m_engine) return;
            auto src = m_engine->selectedSource();
            if (!src) return;
            auto s = src->settings;
            applyTo(s);
            s.insert(QStringLiteral("reloadToken"), s.value(QStringLiteral("reloadToken")).toInt() + 1);
            m_engine->updateSourceSettings(src->id, s);
        });
        form->addRow(QStringLiteral("URL"), m_url);
        form->addRow(QStringLiteral("Width"), m_w);
        form->addRow(QStringLiteral("Height"), m_h);
        form->addRow(QStringLiteral("FPS"), m_fps);
        form->addRow(m_hw);
        form->addRow(m_controlAudio);
        form->addRow(refresh);
        wireChanged(this, m_url);
        wireChanged(this, m_w);
        wireChanged(this, m_h);
        wireChanged(this, m_fps);
        wireChanged(this, m_hw);
        wireChanged(this, m_controlAudio);
    }
    void loadFrom(const SourceItem& src) override
    {
        m_url->setText(src.settings.value(QStringLiteral("url")).toString(QStringLiteral("https://example.com")));
        m_w->setValue(src.settings.value(QStringLiteral("width")).toInt(1280));
        m_h->setValue(src.settings.value(QStringLiteral("height")).toInt(720));
        m_fps->setValue(src.settings.value(QStringLiteral("fps")).toInt(15));
        m_hw->setChecked(src.settings.value(QStringLiteral("hwAccel")).toBool(true));
        m_controlAudio->setChecked(src.settings.value(QStringLiteral("controlAudioViaObs")).toBool(false));
    }
    void applyTo(QJsonObject& s) const override
    {
        s.insert(QStringLiteral("url"), m_url->text().trimmed());
        s.insert(QStringLiteral("width"), m_w->value());
        s.insert(QStringLiteral("height"), m_h->value());
        s.insert(QStringLiteral("fps"), m_fps->value());
        s.insert(QStringLiteral("hwAccel"), m_hw->isChecked());
        s.insert(QStringLiteral("controlAudioViaObs"), m_controlAudio->isChecked());
    }
    void resetDefaults(QJsonObject& s) const override
    {
        s.insert(QStringLiteral("url"), QStringLiteral("https://example.com"));
        s.insert(QStringLiteral("width"), 1280);
        s.insert(QStringLiteral("height"), 720);
        s.insert(QStringLiteral("fps"), 15);
        s.insert(QStringLiteral("hwAccel"), true);
        s.insert(QStringLiteral("controlAudioViaObs"), false);
    }

private:
    QLineEdit* m_url = nullptr;
    QSpinBox* m_w = nullptr;
    QSpinBox* m_h = nullptr;
    QSpinBox* m_fps = nullptr;
    QCheckBox* m_hw = nullptr;
    QCheckBox* m_controlAudio = nullptr;
};

class TextPanel : public SourcePropertiesPanel {
public:
    TextPanel(EngineController* engine, QWidget* parent)
        : SourcePropertiesPanel(engine, parent)
    {
        auto* lay = new QVBoxLayout(this);
        m_text = new QPlainTextEdit(this);
        m_font = new QFontComboBox(this);
        m_size = new QSpinBox(this);
        m_size->setRange(8, 256);
        m_size->setValue(48);
        m_color = new QLineEdit(QStringLiteral("#FFFFFF"), this);
        m_outline = new QCheckBox(QStringLiteral("Outline"), this);
        m_wrap = new QCheckBox(QStringLiteral("Word wrap"), this);
        m_align = new QComboBox(this);
        m_align->addItem(QStringLiteral("Left"), QStringLiteral("left"));
        m_align->addItem(QStringLiteral("Center"), QStringLiteral("center"));
        m_align->addItem(QStringLiteral("Right"), QStringLiteral("right"));
        auto* pick = new QPushButton(QStringLiteral("Pick colour…"), this);
        connect(pick, &QPushButton::clicked, this, [this] {
            const QColor c = QColorDialog::getColor(QColor(m_color->text()), this);
            if (c.isValid()) {
                m_color->setText(c.name(QColor::HexRgb));
                emit settingsEdited();
            }
        });
        auto* form = new QFormLayout();
        form->addRow(QStringLiteral("Font"), m_font);
        form->addRow(QStringLiteral("Size"), m_size);
        form->addRow(QStringLiteral("Colour"), m_color);
        form->addRow(pick);
        form->addRow(QStringLiteral("Align"), m_align);
        form->addRow(m_outline);
        form->addRow(m_wrap);
        lay->addWidget(new QLabel(QStringLiteral("Text"), this));
        lay->addWidget(m_text, 1);
        lay->addLayout(form);
        wireChanged(this, m_text);
        wireChanged(this, m_font);
        wireChanged(this, m_size);
        wireChanged(this, m_color);
        wireChanged(this, m_outline);
        wireChanged(this, m_wrap);
        wireChanged(this, m_align);
    }
    void loadFrom(const SourceItem& src) override
    {
        m_text->setPlainText(src.settings.value(QStringLiteral("text")).toString(src.name));
        m_font->setCurrentFont(QFont(src.settings.value(QStringLiteral("fontFamily")).toString(QStringLiteral("Segoe UI"))));
        m_size->setValue(src.settings.value(QStringLiteral("fontSize")).toInt(48));
        m_color->setText(src.settings.value(QStringLiteral("textColor")).toString(QStringLiteral("#FFFFFF")));
        m_outline->setChecked(src.settings.value(QStringLiteral("outline")).toBool(false));
        m_wrap->setChecked(src.settings.value(QStringLiteral("wrap")).toBool(true));
        const int a = m_align->findData(src.settings.value(QStringLiteral("align")).toString(QStringLiteral("left")));
        m_align->setCurrentIndex(a >= 0 ? a : 0);
    }
    void applyTo(QJsonObject& s) const override
    {
        s.insert(QStringLiteral("text"), m_text->toPlainText());
        s.insert(QStringLiteral("fontFamily"), m_font->currentFont().family());
        s.insert(QStringLiteral("fontSize"), m_size->value());
        s.insert(QStringLiteral("textColor"), m_color->text().trimmed());
        s.insert(QStringLiteral("outline"), m_outline->isChecked());
        s.insert(QStringLiteral("wrap"), m_wrap->isChecked());
        s.insert(QStringLiteral("align"), m_align->currentData().toString());
    }
    void resetDefaults(QJsonObject& s) const override
    {
        s.insert(QStringLiteral("text"), QStringLiteral("RailShotTV"));
        s.insert(QStringLiteral("fontFamily"), QStringLiteral("Segoe UI"));
        s.insert(QStringLiteral("fontSize"), 48);
        s.insert(QStringLiteral("textColor"), QStringLiteral("#FFFFFF"));
        s.insert(QStringLiteral("outline"), false);
        s.insert(QStringLiteral("wrap"), true);
        s.insert(QStringLiteral("align"), QStringLiteral("left"));
    }

private:
    QPlainTextEdit* m_text = nullptr;
    QFontComboBox* m_font = nullptr;
    QSpinBox* m_size = nullptr;
    QLineEdit* m_color = nullptr;
    QCheckBox* m_outline = nullptr;
    QCheckBox* m_wrap = nullptr;
    QComboBox* m_align = nullptr;
};

class PathPanel : public SourcePropertiesPanel {
public:
    PathPanel(EngineController* engine, QWidget* parent, const QString& key, const QString& filter,
              bool /*mediaExtras*/)
        : SourcePropertiesPanel(engine, parent), m_key(key)
    {
        auto* form = new QFormLayout(this);
        m_path = new QLineEdit(this);
        auto* browse = new QPushButton(QStringLiteral("Browse…"), this);
        connect(browse, &QPushButton::clicked, this, [this, filter] {
            const auto p = QFileDialog::getOpenFileName(this, QStringLiteral("Select file"), {}, filter);
            if (!p.isEmpty()) {
                m_path->setText(p);
                emit settingsEdited();
            }
        });
        form->addRow(QStringLiteral("Path"), m_path);
        form->addRow(browse);
        m_unload = new QCheckBox(QStringLiteral("Unload when not showing"), this);
        form->addRow(m_unload);
        wireChanged(this, m_unload);
        wireChanged(this, m_path);
    }
    void loadFrom(const SourceItem& src) override
    {
        m_path->setText(src.settings.value(m_key).toString());
        if (m_unload)
            m_unload->setChecked(src.settings.value(QStringLiteral("unloadWhenHidden")).toBool(false));
    }
    void applyTo(QJsonObject& s) const override
    {
        s.insert(m_key, m_path->text().trimmed());
        if (m_unload)
            s.insert(QStringLiteral("unloadWhenHidden"), m_unload->isChecked());
    }
    void resetDefaults(QJsonObject& s) const override
    {
        s.insert(QStringLiteral("unloadWhenHidden"), false);
    }

private:
    QString m_key;
    QLineEdit* m_path = nullptr;
    QCheckBox* m_unload = nullptr;
};

class SlideshowPanel : public SourcePropertiesPanel {
public:
    SlideshowPanel(EngineController* engine, QWidget* parent)
        : SourcePropertiesPanel(engine, parent)
    {
        auto* form = new QFormLayout(this);
        m_list = new QListWidget(this);
        m_list->setMinimumHeight(120);
        form->addRow(QStringLiteral("Images"), m_list);
        auto* btnRow = new QHBoxLayout();
        auto* addBtn = new QPushButton(QStringLiteral("Add…"), this);
        auto* remBtn = new QPushButton(QStringLiteral("Remove"), this);
        connect(addBtn, &QPushButton::clicked, this, [this] {
            const auto paths = QFileDialog::getOpenFileNames(
                this, QStringLiteral("Add slideshow images"), {},
                QStringLiteral("Images (*.png *.jpg *.jpeg *.bmp *.webp)"));
            for (const auto& p : paths) {
                if (!p.isEmpty())
                    m_list->addItem(p);
            }
            if (!paths.isEmpty())
                emit settingsEdited();
        });
        connect(remBtn, &QPushButton::clicked, this, [this] {
            qDeleteAll(m_list->selectedItems());
            emit settingsEdited();
        });
        btnRow->addWidget(addBtn);
        btnRow->addWidget(remBtn);
        form->addRow(btnRow);
        m_interval = new QSpinBox(this);
        m_interval->setRange(250, 600000);
        m_interval->setSuffix(QStringLiteral(" ms"));
        m_interval->setValue(5000);
        form->addRow(QStringLiteral("Slide Time"), m_interval);
        m_transition = new QComboBox(this);
        m_transition->addItem(QStringLiteral("Cut"), QStringLiteral("cut"));
        m_transition->addItem(QStringLiteral("Fade"), QStringLiteral("fade"));
        m_transition->addItem(QStringLiteral("Swipe"), QStringLiteral("swipe"));
        form->addRow(QStringLiteral("Transition"), m_transition);
        m_transitionMs = new QSpinBox(this);
        m_transitionMs->setRange(0, 10000);
        m_transitionMs->setSuffix(QStringLiteral(" ms"));
        m_transitionMs->setValue(700);
        form->addRow(QStringLiteral("Transition Speed"), m_transitionMs);
        m_swipeDir = new QComboBox(this);
        m_swipeDir->addItem(QStringLiteral("Left → Right"), 0);
        m_swipeDir->addItem(QStringLiteral("Right → Left"), 1);
        m_swipeDir->addItem(QStringLiteral("Top → Bottom"), 2);
        m_swipeDir->addItem(QStringLiteral("Bottom → Top"), 3);
        form->addRow(QStringLiteral("Swipe Direction"), m_swipeDir);
        m_loop = new QCheckBox(QStringLiteral("Loop"), this);
        m_loop->setChecked(true);
        form->addRow(m_loop);
        m_randomize = new QCheckBox(QStringLiteral("Randomize Playback"), this);
        form->addRow(m_randomize);
        auto* stepRow = new QHBoxLayout();
        auto* prevBtn = new QPushButton(QStringLiteral("◀ Prev"), this);
        auto* nextBtn = new QPushButton(QStringLiteral("Next ▶"), this);
        connect(prevBtn, &QPushButton::clicked, this, [this] {
            if (m_engine) m_engine->slideshowStepSelected(-1);
        });
        connect(nextBtn, &QPushButton::clicked, this, [this] {
            if (m_engine) m_engine->slideshowStepSelected(1);
        });
        stepRow->addWidget(prevBtn);
        stepRow->addWidget(nextBtn);
        form->addRow(stepRow);
        wireChanged(this, m_transition);
        wireChanged(this, m_transitionMs);
        wireChanged(this, m_swipeDir);
        wireChanged(this, m_loop);
        wireChanged(this, m_randomize);
        wireChanged(this, m_interval);
    }
    void loadFrom(const SourceItem& src) override
    {
        m_list->clear();
        const auto arr = src.settings.value(QStringLiteral("paths")).toArray();
        for (const auto& v : arr) {
            const QString p = v.toString();
            if (!p.isEmpty())
                m_list->addItem(p);
        }
        if (m_list->count() == 0) {
            const QString one = src.settings.value(QStringLiteral("path")).toString();
            if (!one.isEmpty())
                m_list->addItem(one);
        }
        m_interval->setValue(src.settings.value(QStringLiteral("intervalMs")).toInt(5000));
        const int tIdx = m_transition->findData(
            src.settings.value(QStringLiteral("transition")).toString(QStringLiteral("cut")));
        m_transition->setCurrentIndex(tIdx >= 0 ? tIdx : 0);
        m_transitionMs->setValue(src.settings.value(QStringLiteral("transitionMs")).toInt(700));
        const int sIdx = m_swipeDir->findData(src.settings.value(QStringLiteral("swipeDir")).toInt(0));
        m_swipeDir->setCurrentIndex(sIdx >= 0 ? sIdx : 0);
        m_loop->setChecked(src.settings.value(QStringLiteral("loop")).toBool(true));
        m_randomize->setChecked(src.settings.value(QStringLiteral("randomize")).toBool(false));
    }
    void applyTo(QJsonObject& s) const override
    {
        QJsonArray arr;
        for (int i = 0; i < m_list->count(); ++i)
            arr.append(m_list->item(i)->text());
        s.insert(QStringLiteral("paths"), arr);
        s.insert(QStringLiteral("intervalMs"), m_interval->value());
        s.insert(QStringLiteral("transition"), m_transition->currentData().toString());
        s.insert(QStringLiteral("transitionMs"), m_transitionMs->value());
        s.insert(QStringLiteral("swipeDir"), m_swipeDir->currentData().toInt());
        s.insert(QStringLiteral("loop"), m_loop->isChecked());
        s.insert(QStringLiteral("randomize"), m_randomize->isChecked());
    }
    void resetDefaults(QJsonObject& s) const override
    {
        s.insert(QStringLiteral("paths"), QJsonArray{});
        s.insert(QStringLiteral("intervalMs"), 5000);
        s.insert(QStringLiteral("transition"), QStringLiteral("cut"));
        s.insert(QStringLiteral("transitionMs"), 700);
        s.insert(QStringLiteral("swipeDir"), 0);
        s.insert(QStringLiteral("loop"), true);
        s.insert(QStringLiteral("randomize"), false);
    }

private:
    QListWidget* m_list = nullptr;
    QSpinBox* m_interval = nullptr;
    QComboBox* m_transition = nullptr;
    QSpinBox* m_transitionMs = nullptr;
    QComboBox* m_swipeDir = nullptr;
    QCheckBox* m_loop = nullptr;
    QCheckBox* m_randomize = nullptr;
};

/// OBS Media Source: Local File checkbox, or network Input URL (rtsp/http/hls/…).
class MediaPanel : public SourcePropertiesPanel {
public:
    MediaPanel(EngineController* engine, QWidget* parent)
        : SourcePropertiesPanel(engine, parent)
    {
        auto* form = new QFormLayout(this);
        m_local = new QCheckBox(QStringLiteral("Local File"), this);
        m_local->setChecked(true);
        m_path = new QLineEdit(this);
        m_path->setPlaceholderText(QStringLiteral("C:\\video.mp4"));
        m_browse = new QPushButton(QStringLiteral("Browse…"), this);
        connect(m_browse, &QPushButton::clicked, this, [this] {
            const auto p = QFileDialog::getOpenFileName(
                this, QStringLiteral("Select media"), {},
                QStringLiteral("Media (*.mp4 *.mov *.mkv *.webm *.avi *.png *.jpg *.jpeg);;All (*.*)"));
            if (!p.isEmpty()) {
                m_path->setText(p);
                emit settingsEdited();
            }
        });
        m_inputLabel = new QLabel(QStringLiteral("Local File"), this);
        form->addRow(m_local);
        form->addRow(m_inputLabel, m_path);
        form->addRow(m_browse);

        m_ffopts = new QLineEdit(this);
        m_ffopts->setPlaceholderText(QStringLiteral("rtsp_transport=tcp rtsp_flags=prefer_tcp"));
        m_ffopts->setToolTip(
            QStringLiteral("FFmpeg options as option=value pairs, space-separated.\n"
                           "Example: rtsp_transport=tcp stimeout=5000000"));
        form->addRow(QStringLiteral("FFmpeg Options"), m_ffopts);

        m_loop = new QCheckBox(QStringLiteral("Loop"), this);
        m_loop->setChecked(true);
        m_restart = new QCheckBox(QStringLiteral("Restart when activated"), this);
        m_endAction = new QComboBox(this);
        m_endAction->addItem(QStringLiteral("None (freeze last frame)"), QStringLiteral("none"));
        m_endAction->addItem(QStringLiteral("Hide source"), QStringLiteral("hide"));
        m_audio = new QCheckBox(QStringLiteral("Audio track"), this);
        m_audio->setChecked(true);
        form->addRow(m_loop);
        form->addRow(m_restart);
        form->addRow(QStringLiteral("End Action"), m_endAction);
        form->addRow(m_audio);

        auto* hint = new QLabel(
            QStringLiteral("Uncheck Local File and paste rtsp://, http(s)://, or HLS (.m3u8) for IP cameras."),
            this);
        hint->setWordWrap(true);
        hint->setStyleSheet(QStringLiteral("color:#8892A4; font-size:11px;"));
        form->addRow(hint);

        connect(m_local, &QCheckBox::toggled, this, [this](bool on) { syncLocalMode(on); emit settingsEdited(); });
        wireChanged(this, m_path);
        wireChanged(this, m_ffopts);
        wireChanged(this, m_loop);
        wireChanged(this, m_restart);
        wireChanged(this, m_endAction);
        wireChanged(this, m_audio);
        syncLocalMode(true);
    }

    void loadFrom(const SourceItem& src) override
    {
        const bool local = src.settings.value(QStringLiteral("isLocalFile")).toBool(true);
        QSignalBlocker b(m_local);
        m_local->setChecked(local);
        syncLocalMode(local);
        m_path->setText(src.settings.value(QStringLiteral("path")).toString());
        m_ffopts->setText(src.settings.value(QStringLiteral("ffmpegOptions")).toString());
        m_loop->setChecked(src.settings.value(QStringLiteral("loop")).toBool(local));
        m_restart->setChecked(src.settings.value(QStringLiteral("restartOnActivate")).toBool(false));
        const int endIdx = m_endAction->findData(
            src.settings.value(QStringLiteral("endAction")).toString(QStringLiteral("none")));
        m_endAction->setCurrentIndex(endIdx >= 0 ? endIdx : 0);
        m_audio->setChecked(src.settings.value(QStringLiteral("audioTrack")).toBool(true));
    }

    void applyTo(QJsonObject& s) const override
    {
        s.insert(QStringLiteral("isLocalFile"), m_local->isChecked());
        s.insert(QStringLiteral("path"), m_path->text().trimmed());
        s.insert(QStringLiteral("ffmpegOptions"), m_ffopts->text().trimmed());
        s.insert(QStringLiteral("loop"), m_loop->isChecked());
        s.insert(QStringLiteral("restartOnActivate"), m_restart->isChecked());
        s.insert(QStringLiteral("endAction"), m_endAction->currentData().toString());
        s.insert(QStringLiteral("audioTrack"), m_audio->isChecked());
    }

    void resetDefaults(QJsonObject& s) const override
    {
        s.insert(QStringLiteral("isLocalFile"), true);
        s.insert(QStringLiteral("path"), QString());
        s.insert(QStringLiteral("ffmpegOptions"), QString());
        s.insert(QStringLiteral("loop"), true);
        s.insert(QStringLiteral("restartOnActivate"), false);
        s.insert(QStringLiteral("endAction"), QStringLiteral("none"));
        s.insert(QStringLiteral("audioTrack"), true);
    }

private:
    void syncLocalMode(bool local)
    {
        m_inputLabel->setText(local ? QStringLiteral("Local File") : QStringLiteral("Input"));
        m_path->setPlaceholderText(local
                                       ? QStringLiteral("C:\\video.mp4")
                                       : QStringLiteral("rtsp://user:pass@192.168.1.50/stream1"));
        m_browse->setEnabled(local);
        m_browse->setVisible(local);
        if (!local && m_loop->isChecked()) {
            // Live URLs usually should not loop/seek.
            QSignalBlocker b(m_loop);
            m_loop->setChecked(false);
        }
    }

    QCheckBox* m_local = nullptr;
    QLabel* m_inputLabel = nullptr;
    QLineEdit* m_path = nullptr;
    QPushButton* m_browse = nullptr;
    QLineEdit* m_ffopts = nullptr;
    QCheckBox* m_loop = nullptr;
    QCheckBox* m_restart = nullptr;
    QComboBox* m_endAction = nullptr;
    QCheckBox* m_audio = nullptr;
};

class ColorPanel : public SourcePropertiesPanel {
public:
    ColorPanel(EngineController* engine, QWidget* parent)
        : SourcePropertiesPanel(engine, parent)
    {
        auto* form = new QFormLayout(this);
        m_color = new QLineEdit(QStringLiteral("#1A1A1A"), this);
        m_w = new QSpinBox(this);
        m_w->setRange(16, 7680);
        m_w->setValue(1920);
        m_h = new QSpinBox(this);
        m_h->setRange(16, 4320);
        m_h->setValue(1080);
        auto* pick = new QPushButton(QStringLiteral("Pick…"), this);
        connect(pick, &QPushButton::clicked, this, [this] {
            const QColor c = QColorDialog::getColor(QColor(m_color->text()), this);
            if (c.isValid()) {
                m_color->setText(c.name(QColor::HexRgb));
                emit settingsEdited();
            }
        });
        form->addRow(QStringLiteral("Colour"), m_color);
        form->addRow(pick);
        form->addRow(QStringLiteral("Width"), m_w);
        form->addRow(QStringLiteral("Height"), m_h);
        wireChanged(this, m_color);
        wireChanged(this, m_w);
        wireChanged(this, m_h);
    }
    void loadFrom(const SourceItem& src) override
    {
        m_color->setText(src.settings.value(QStringLiteral("color")).toString(QStringLiteral("#1A1A1A")));
        m_w->setValue(src.settings.value(QStringLiteral("width")).toInt(1920));
        m_h->setValue(src.settings.value(QStringLiteral("height")).toInt(1080));
    }
    void applyTo(QJsonObject& s) const override
    {
        s.insert(QStringLiteral("color"), m_color->text().trimmed());
        s.insert(QStringLiteral("width"), m_w->value());
        s.insert(QStringLiteral("height"), m_h->value());
    }
    void resetDefaults(QJsonObject& s) const override
    {
        s.insert(QStringLiteral("color"), QStringLiteral("#1A1A1A"));
        s.insert(QStringLiteral("width"), 1920);
        s.insert(QStringLiteral("height"), 1080);
    }

private:
    QLineEdit* m_color = nullptr;
    QSpinBox* m_w = nullptr;
    QSpinBox* m_h = nullptr;
};

class NdiPanel : public SourcePropertiesPanel {
public:
    NdiPanel(EngineController* engine, QWidget* parent)
        : SourcePropertiesPanel(engine, parent)
    {
        auto* lay = new QVBoxLayout(this);
        m_list = new QListWidget(this);
        m_bw = new QComboBox(this);
        m_bw->addItem(QStringLiteral("Highest"), QStringLiteral("highest"));
        m_bw->addItem(QStringLiteral("Lowest"), QStringLiteral("lowest"));
        auto* refresh = new QPushButton(QStringLiteral("Refresh"), this);
        auto doRefresh = [this] {
            m_list->clear();
            const auto found = NdiSource::discoverSources(1200);
            if (found.isEmpty()) {
                m_list->addItem(QStringLiteral("(No NDI sources)"));
                return;
            }
            for (const auto& n : found)
                m_list->addItem(n);
        };
        connect(refresh, &QPushButton::clicked, this, [this, doRefresh] {
            doRefresh();
            emit settingsEdited();
        });
        connect(m_list, &QListWidget::currentRowChanged, this, [this](int) { emit settingsEdited(); });
        lay->addWidget(new QLabel(QStringLiteral("NDI source"), this));
        lay->addWidget(m_list, 1);
        auto* form = new QFormLayout();
        form->addRow(QStringLiteral("Bandwidth"), m_bw);
        lay->addLayout(form);
        lay->addWidget(refresh);
        wireChanged(this, m_bw);
        doRefresh();
    }
    void loadFrom(const SourceItem& src) override
    {
        const QString name = src.settings.value(QStringLiteral("ndiName")).toString();
        for (int i = 0; i < m_list->count(); ++i) {
            if (m_list->item(i)->text() == name) {
                m_list->setCurrentRow(i);
                break;
            }
        }
        const int b = m_bw->findData(src.settings.value(QStringLiteral("ndiBandwidth")).toString(QStringLiteral("highest")));
        m_bw->setCurrentIndex(b >= 0 ? b : 0);
    }
    void applyTo(QJsonObject& s) const override
    {
        if (m_list->currentItem() && !m_list->currentItem()->text().startsWith(QLatin1Char('(')))
            s.insert(QStringLiteral("ndiName"), m_list->currentItem()->text());
        s.insert(QStringLiteral("ndiBandwidth"), m_bw->currentData().toString());
    }
    void resetDefaults(QJsonObject& s) const override
    {
        s.insert(QStringLiteral("ndiBandwidth"), QStringLiteral("highest"));
    }

private:
    QListWidget* m_list = nullptr;
    QComboBox* m_bw = nullptr;
};

class OverlayNotePanel : public SourcePropertiesPanel {
public:
    OverlayNotePanel(EngineController* engine, QWidget* parent, const QString& note)
        : SourcePropertiesPanel(engine, parent)
    {
        auto* lay = new QVBoxLayout(this);
        lay->setContentsMargins(4, 4, 4, 4);
        auto* card = new QFrame(this);
        card->setObjectName(QStringLiteral("propsInfoCard"));
        auto* cardLay = new QVBoxLayout(card);
        cardLay->setContentsMargins(12, 12, 12, 12);
        cardLay->setSpacing(6);
        auto* title = new QLabel(QStringLiteral("Controlled from dock"), card);
        title->setStyleSheet(QStringLiteral(
            "color:#7AB8FF; font-size:10px; font-weight:800; letter-spacing:0.5px;"
            "background:transparent; border:none;"));
        auto* lab = new QLabel(note, card);
        lab->setWordWrap(true);
        lab->setStyleSheet(QStringLiteral(
            "color:#C8CAD0; font-size:12px; line-height:1.35; background:transparent; border:none;"));
        cardLay->addWidget(title);
        cardLay->addWidget(lab);
        lay->addWidget(card);
        lay->addStretch();
    }
    void loadFrom(const SourceItem&) override {}
    void applyTo(QJsonObject&) const override {}
    void resetDefaults(QJsonObject&) const override {}
};

class WindowOrGamePanel : public SourcePropertiesPanel {
public:
    WindowOrGamePanel(EngineController* engine, QWidget* parent, bool gameFilter)
        : SourcePropertiesPanel(engine, parent), m_game(gameFilter)
    {
        auto* lay = new QVBoxLayout(this);
        m_list = new QListWidget(this);
        auto* refresh = new QPushButton(QStringLiteral("Refresh"), this);
        connect(refresh, &QPushButton::clicked, this, [this] {
            rebuildList();
            emit settingsEdited();
        });
        connect(m_list, &QListWidget::currentRowChanged, this, [this](int) { emit settingsEdited(); });
        lay->addWidget(new QLabel(m_game ? QStringLiteral("Game / fullscreen window (WGC — no hook injection)")
                                         : QStringLiteral("Window"),
                                  this));
        lay->addWidget(m_list, 1);
        lay->addWidget(refresh);
        rebuildList();
    }
    void rebuildList()
    {
        m_list->clear();
        m_entries = enumerateWindows();
        for (const auto& e : m_entries) {
            QString label = e.title;
            if (!e.exe.isEmpty())
                label += QStringLiteral("  [%1]").arg(e.exe);
            auto* item = new QListWidgetItem(label, m_list);
            item->setData(Qt::UserRole, QVariant::fromValue(e.hwnd));
            item->setData(Qt::UserRole + 1, e.title);
            item->setData(Qt::UserRole + 2, e.exe);
            if (m_game) {
                // Prefer larger / likely game titles — keep all but sort exe-bearing first
                Q_UNUSED(e);
            }
        }
    }
    void loadFrom(const SourceItem& src) override
    {
        const quintptr hwnd = src.settings.value(QStringLiteral("hwnd")).toVariant().toULongLong();
        const QString title = src.settings.value(QStringLiteral("windowTitle")).toString();
        for (int i = 0; i < m_list->count(); ++i) {
            auto* it = m_list->item(i);
            if ((hwnd && it->data(Qt::UserRole).toULongLong() == hwnd)
                || (!title.isEmpty() && it->data(Qt::UserRole + 1).toString() == title)) {
                m_list->setCurrentRow(i);
                return;
            }
        }
    }
    void applyTo(QJsonObject& s) const override
    {
        if (!m_list->currentItem()) return;
        s.insert(QStringLiteral("hwnd"), static_cast<double>(m_list->currentItem()->data(Qt::UserRole).toULongLong()));
        s.insert(QStringLiteral("windowTitle"), m_list->currentItem()->data(Qt::UserRole + 1).toString());
        s.insert(QStringLiteral("exe"), m_list->currentItem()->data(Qt::UserRole + 2).toString());
    }
    void resetDefaults(QJsonObject& s) const override
    {
        s.remove(QStringLiteral("hwnd"));
        s.remove(QStringLiteral("windowTitle"));
        s.remove(QStringLiteral("exe"));
    }

private:
    bool m_game = false;
    QListWidget* m_list = nullptr;
    QVector<WindowEntry> m_entries;
};

class AudioDevicePanel : public SourcePropertiesPanel {
public:
    AudioDevicePanel(EngineController* engine, QWidget* parent, AudioDeviceKind kind)
        : SourcePropertiesPanel(engine, parent), m_kind(kind)
    {
        auto* form = new QFormLayout(this);
        m_device = new QComboBox(this);
        for (const auto& d : WasapiCapture::enumerate(kind))
            m_device->addItem(d.name, d.id);
        if (m_device->count() == 0)
            m_device->addItem(QStringLiteral("(Default device)"), QString());
        form->addRow(QStringLiteral("Device"), m_device);
        wireChanged(this, m_device);
    }
    void loadFrom(const SourceItem& src) override
    {
        const QString id = src.settings.value(QStringLiteral("deviceId")).toString();
        const int idx = m_device->findData(id);
        m_device->setCurrentIndex(idx >= 0 ? idx : 0);
    }
    void applyTo(QJsonObject& s) const override
    {
        s.insert(QStringLiteral("deviceId"), m_device->currentData().toString());
        s.insert(QStringLiteral("audioKind"),
                 m_kind == AudioDeviceKind::Capture ? QStringLiteral("input") : QStringLiteral("output"));
    }
    void resetDefaults(QJsonObject& s) const override
    {
        s.insert(QStringLiteral("deviceId"), QString());
    }

private:
    AudioDeviceKind m_kind;
    QComboBox* m_device = nullptr;
};

class SceneAsSourcePanel : public SourcePropertiesPanel {
public:
    SceneAsSourcePanel(EngineController* engine, QWidget* parent)
        : SourcePropertiesPanel(engine, parent)
    {
        auto* form = new QFormLayout(this);
        m_scene = new QComboBox(this);
        m_scene->addItem(QStringLiteral("(Select scene)"), QString());
        if (engine) {
            const auto p = engine->projectSnapshot();
            for (const auto& sc : p.scenes)
                m_scene->addItem(sc.name, sc.id);
        }
        form->addRow(QStringLiteral("Scene"), m_scene);
        auto* hint = new QLabel(
            QStringLiteral("Embeds another scene as a layer. Self-embedding is blocked."), this);
        hint->setWordWrap(true);
        form->addRow(QString(), hint);
        wireChanged(this, m_scene);
    }
    void loadFrom(const SourceItem& src) override
    {
        const QString id = src.settings.value(QStringLiteral("sceneId")).toString();
        const int idx = m_scene->findData(id);
        m_scene->setCurrentIndex(idx >= 0 ? idx : 0);
    }
    void applyTo(QJsonObject& s) const override
    {
        s.insert(QStringLiteral("sceneId"), m_scene->currentData().toString());
    }
    void resetDefaults(QJsonObject& s) const override
    {
        s.insert(QStringLiteral("sceneId"), QString());
    }

private:
    QComboBox* m_scene = nullptr;
};

class GroupPanel : public SourcePropertiesPanel {
public:
    GroupPanel(EngineController* engine, QWidget* parent)
        : SourcePropertiesPanel(engine, parent)
    {
        auto* lay = new QVBoxLayout(this);
        lay->addWidget(new QLabel(QStringLiteral("Group members (same scene)"), this));
        m_list = new QListWidget(this);
        m_list->setSelectionMode(QAbstractItemView::NoSelection);
        lay->addWidget(m_list, 1);
        auto* hint = new QLabel(
            QStringLiteral("Check sources to nest under this group. Grouped items are skipped at the root pass."),
            this);
        hint->setWordWrap(true);
        lay->addWidget(hint);
        connect(m_list, &QListWidget::itemChanged, this, [this](QListWidgetItem*) { emit settingsEdited(); });
    }
    void loadFrom(const SourceItem& src) override
    {
        m_hostId = src.id;
        m_list->clear();
        if (!m_engine) return;
        const auto p = m_engine->projectSnapshot();
        const SceneItem* hostScene = nullptr;
        for (const auto& sc : p.scenes) {
            for (const auto& s : sc.sources) {
                if (s.id == src.id) {
                    hostScene = &sc;
                    break;
                }
            }
            if (hostScene) break;
        }
        QSet<QString> selected;
        const auto arr = src.settings.value(QStringLiteral("childIds")).toArray();
        for (const auto& v : arr)
            selected.insert(v.toString());
        if (!hostScene) return;
        for (const auto& s : hostScene->sources) {
            if (s.id == src.id) continue;
            if (s.type == SourceType::Group) continue;
            auto* item = new QListWidgetItem(s.name, m_list);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setData(Qt::UserRole, s.id);
            item->setCheckState(selected.contains(s.id) ? Qt::Checked : Qt::Unchecked);
        }
    }
    void applyTo(QJsonObject& s) const override
    {
        QJsonArray ids;
        for (int i = 0; i < m_list->count(); ++i) {
            auto* it = m_list->item(i);
            if (it->checkState() == Qt::Checked)
                ids.append(it->data(Qt::UserRole).toString());
        }
        s.insert(QStringLiteral("childIds"), ids);
    }
    void resetDefaults(QJsonObject& s) const override
    {
        s.insert(QStringLiteral("childIds"), QJsonArray{});
    }

private:
    QString m_hostId;
    QListWidget* m_list = nullptr;
};

} // namespace

SourcePropertiesPanel* createSourcePropertiesPanel(SourceType type, EngineController* engine, QWidget* parent)
{
    switch (type) {
    case SourceType::Display:
        return new DisplayPanel(engine, parent);
    case SourceType::Camera:
        return new CameraPanel(engine, parent);
    case SourceType::Browser:
        return new BrowserPanel(engine, parent);
    case SourceType::Text:
        return new TextPanel(engine, parent);
    case SourceType::Image:
        return new PathPanel(engine, parent, QStringLiteral("path"),
                             QStringLiteral("Images (*.png *.jpg *.jpeg *.bmp *.webp)"), false);
    case SourceType::Slideshow:
        return new SlideshowPanel(engine, parent);
    case SourceType::Media:
        return new MediaPanel(engine, parent);
    case SourceType::Color:
        return new ColorPanel(engine, parent);
    case SourceType::Ndi:
        return new NdiPanel(engine, parent);
    case SourceType::Window:
        return new WindowOrGamePanel(engine, parent, false);
    case SourceType::Game:
        return new WindowOrGamePanel(engine, parent, true);
    case SourceType::AudioInput:
        return new AudioDevicePanel(engine, parent, AudioDeviceKind::Capture);
    case SourceType::AudioOutput:
        return new AudioDevicePanel(engine, parent, AudioDeviceKind::Loopback);
    case SourceType::Scene:
        return new SceneAsSourcePanel(engine, parent);
    case SourceType::Group:
        return new GroupPanel(engine, parent);
    case SourceType::Scoreboard:
        return new OverlayNotePanel(engine, parent,
                                    QStringLiteral("Live scores come from the Scoreboard dock. Use Push on the dock to sync."));
    case SourceType::LowerThird:
        return new OverlayNotePanel(engine, parent,
                                    QStringLiteral("Lower-third title/subtitle live in General settings JSON; edit via Overlay presets."));
    case SourceType::Alert:
        return new OverlayNotePanel(engine, parent, QStringLiteral("Alert overlay — trigger from Overlay menu."));
    default:
        return new OverlayNotePanel(engine, parent, QStringLiteral("No type-specific settings."));
    }
}

} // namespace railshot
