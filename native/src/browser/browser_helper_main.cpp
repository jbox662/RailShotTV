#include "browser/BrowserIpc.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTextBrowser>
#include <QTimer>
#include <QUrl>
#include <QImage>
#include <QPainter>
#include <QDateTime>
#include <QByteArray>
#include <QEventLoop>
#include <QCoreApplication>
#include <cstring>
#include <functional>
#include <memory>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <objbase.h>
#include <shlwapi.h>
#include <wrl.h>
#include <wrl/event.h>

#if defined(RAILSHOT_HAS_WEBVIEW2) && RAILSHOT_HAS_WEBVIEW2
#include <WebView2.h>
#pragma comment(lib, "Shlwapi.lib")
#endif
#endif

using railshot::browser_ipc::FrameHeader;
using railshot::browser_ipc::bufferBytes;
using railshot::browser_ipc::kMagic;

namespace {
#if defined(_WIN32) && defined(RAILSHOT_HAS_WEBVIEW2) && RAILSHOT_HAS_WEBVIEW2
using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;
#endif

struct SharedState {
    void* mapping = nullptr;
    void* view = nullptr;
    void* mutex = nullptr;
    int width = 1280;
    int height = 720;
};

bool openShared(SharedState& s, const QString& shmName, QString* error)
{
#ifdef _WIN32
    const std::wstring mapName = shmName.toStdWString();
    const std::wstring mtxName = (shmName + QStringLiteral("_mtx")).toStdWString();
    const DWORD bytes = static_cast<DWORD>(bufferBytes(s.width, s.height));
    s.mutex = OpenMutexW(SYNCHRONIZE | MUTEX_MODIFY_STATE, FALSE, mtxName.c_str());
    if (!s.mutex)
        s.mutex = CreateMutexW(nullptr, FALSE, mtxName.c_str());
    s.mapping = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, mapName.c_str());
    if (!s.mapping)
        s.mapping = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, bytes, mapName.c_str());
    if (!s.mapping) {
        if (error) *error = QStringLiteral("OpenFileMapping failed");
        return false;
    }
    s.view = MapViewOfFile(s.mapping, FILE_MAP_ALL_ACCESS, 0, 0, bytes);
    if (!s.view) {
        if (error) *error = QStringLiteral("MapViewOfFile failed");
        return false;
    }
    return true;
#else
    Q_UNUSED(s);
    Q_UNUSED(shmName);
    if (error) *error = QStringLiteral("Windows only");
    return false;
#endif
}

void closeShared(SharedState& s)
{
#ifdef _WIN32
    if (s.view) UnmapViewOfFile(s.view);
    if (s.mapping) CloseHandle(s.mapping);
    if (s.mutex) CloseHandle(s.mutex);
    s.view = s.mapping = s.mutex = nullptr;
#endif
}

void publishFrame(SharedState& s, const QImage& img, quint64 index, quint32 status, const QString& err)
{
#ifdef _WIN32
    if (!s.view) return;
    if (s.mutex) WaitForSingleObject(static_cast<HANDLE>(s.mutex), 50);
    auto* hdr = static_cast<FrameHeader*>(s.view);
    hdr->magic = kMagic;
    hdr->width = static_cast<quint32>(s.width);
    hdr->height = static_cast<quint32>(s.height);
    hdr->stride = static_cast<quint32>(s.width * 4);
    hdr->bytesPerPixel = 4;
    hdr->frameIndex = index;
    hdr->ptsUs = QDateTime::currentMSecsSinceEpoch() * 1000;
    hdr->status = status;
    std::memset(hdr->error, 0, sizeof(hdr->error));
    if (!err.isEmpty()) {
        const QByteArray utf = err.toUtf8();
        std::memcpy(hdr->error, utf.constData(), size_t(qMin(127, utf.size())));
    }
    uint8_t* dst = reinterpret_cast<uint8_t*>(hdr + 1);
    QImage converted = img;
    if (converted.format() != QImage::Format_ARGB32)
        converted = converted.convertToFormat(QImage::Format_ARGB32);
    if (converted.width() != s.width || converted.height() != s.height)
        converted = converted.scaled(s.width, s.height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    for (int y = 0; y < s.height; ++y) {
        std::memcpy(dst + y * s.width * 4, converted.constScanLine(y), size_t(s.width * 4));
    }
    if (s.mutex) ReleaseMutex(static_cast<HANDLE>(s.mutex));
#else
    Q_UNUSED(s); Q_UNUSED(img); Q_UNUSED(index); Q_UNUSED(status); Q_UNUSED(err);
#endif
}

QImage renderHtml(const QString& html, int w, int h)
{
    QTextBrowser browser;
    browser.setFixedSize(w, h);
    browser.setFrameShape(QFrame::NoFrame);
    browser.setStyleSheet(QStringLiteral("background: transparent; color: white;"));
    browser.setAttribute(Qt::WA_TranslucentBackground, true);
    browser.document()->setDocumentMargin(12);
    browser.setHtml(html);
    browser.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    browser.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QImage img(w, h, QImage::Format_ARGB32);
    img.fill(Qt::transparent);
    QPainter p(&img);
    browser.render(&p);
    p.end();
    return img;
}

QString wrapPage(const QString& bodyHtml, const QString& note)
{
    return QStringLiteral(
               "<html><body style='margin:0;background:rgba(10,12,16,0.65);color:#F8F8FF;"
               "font-family:Segoe UI,sans-serif;'>"
               "<div style='padding:16px;'>%1"
               "<div style='margin-top:12px;color:#A0A8B8;font-size:12px;'>%2</div>"
               "</div></body></html>")
        .arg(bodyHtml, note);
}

#if defined(_WIN32) && defined(RAILSHOT_HAS_WEBVIEW2) && RAILSHOT_HAS_WEBVIEW2
QImage captureHwnd(HWND hwnd, int w, int h)
{
    QImage img(w, h, QImage::Format_ARGB32);
    img.fill(Qt::transparent);
    if (!hwnd) return img;

    HDC hdcWindow = GetDC(hwnd);
    if (!hdcWindow) return img;
    HDC hdcMem = CreateCompatibleDC(hdcWindow);
    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    void* bits = nullptr;
    HBITMAP hbm = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!hbm || !bits) {
        if (hbm) DeleteObject(hbm);
        DeleteDC(hdcMem);
        ReleaseDC(hwnd, hdcWindow);
        return img;
    }
    HGDIOBJ old = SelectObject(hdcMem, hbm);
    BitBlt(hdcMem, 0, 0, w, h, hdcWindow, 0, 0, SRCCOPY);
    std::memcpy(img.bits(), bits, size_t(w) * size_t(h) * 4);
    SelectObject(hdcMem, old);
    DeleteObject(hbm);
    DeleteDC(hdcMem);
    ReleaseDC(hwnd, hdcWindow);
    return img;
}

struct WebViewHost {
    HWND hwnd = nullptr;
    ComPtr<ICoreWebView2Controller> controller;
    ComPtr<ICoreWebView2> webview;
    bool ready = false;
    QString lastError;

    static LRESULT CALLBACK WndProc(HWND h, UINT msg, WPARAM w, LPARAM l)
    {
        if (msg == WM_DESTROY) return 0;
        return DefWindowProcW(h, msg, w, l);
    }

    bool createWindow(int w, int h)
    {
        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = L"RailShotBrowserHelperWV2";
        RegisterClassExW(&wc);
        hwnd = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
                               L"RailShotBrowserHelperWV2",
                               L"RailShot Browser Helper",
                               WS_POPUP,
                               -32000, -32000, w, h,
                               nullptr, nullptr, wc.hInstance, nullptr);
        return hwnd != nullptr;
    }

    bool start(int w, int h, const QString& url, QString* error)
    {
        if (!createWindow(w, h)) {
            if (error) *error = QStringLiteral("CreateWindow failed");
            return false;
        }

        HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
            nullptr, nullptr, nullptr,
            Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
                [this, w, h, url](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
                    if (FAILED(result) || !env) {
                        lastError = QStringLiteral("CreateCoreWebView2Environment failed: 0x%1")
                                        .arg(quint32(result), 8, 16, QLatin1Char('0'));
                        return result;
                    }
                    return env->CreateCoreWebView2Controller(
                        hwnd,
                        Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                            [this, w, h, url](HRESULT result2, ICoreWebView2Controller* ctrl) -> HRESULT {
                                if (FAILED(result2) || !ctrl) {
                                    lastError = QStringLiteral("CreateCoreWebView2Controller failed: 0x%1")
                                                    .arg(quint32(result2), 8, 16, QLatin1Char('0'));
                                    return result2;
                                }
                                controller = ctrl;
                                controller->put_IsVisible(TRUE);
                                RECT bounds{0, 0, w, h};
                                controller->put_Bounds(bounds);
                                if (ComPtr<ICoreWebView2Controller2> c2;
                                    SUCCEEDED(controller.As(&c2))) {
                                    COREWEBVIEW2_COLOR bg{0, 0, 0, 0};
                                    c2->put_DefaultBackgroundColor(bg);
                                }
                                controller->get_CoreWebView2(&webview);
                                if (!webview) {
                                    lastError = QStringLiteral("get_CoreWebView2 returned null");
                                    return E_FAIL;
                                }
                                const std::wstring nav = url.toStdWString();
                                webview->Navigate(nav.c_str());
                                ready = true;
                                return S_OK;
                            })
                            .Get());
                })
                .Get());

        if (FAILED(hr)) {
            if (error) *error = QStringLiteral("CreateCoreWebView2EnvironmentWithOptions failed: 0x%1")
                                    .arg(quint32(hr), 8, 16, QLatin1Char('0'));
            return false;
        }

        // Pump until ready or timeout (~8s)
        const DWORD startTick = GetTickCount();
        while (!ready && lastError.isEmpty() && (GetTickCount() - startTick) < 8000) {
            MSG msg;
            while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            QCoreApplication::processEvents(QEventLoop::AllEvents, 16);
            Sleep(10);
        }
        if (!ready) {
            if (error) *error = lastError.isEmpty() ? QStringLiteral("WebView2 init timeout") : lastError;
            return false;
        }
        return true;
    }

    QImage grab(int w, int h) const
    {
        return captureHwnd(hwnd, w, h);
    }

    void shutdown()
    {
        if (controller) {
            controller->Close();
            controller.Reset();
        }
        webview.Reset();
        if (hwnd) {
            DestroyWindow(hwnd);
            hwnd = nullptr;
        }
        ready = false;
    }
};
#endif

int runFallback(QApplication& app, SharedState& shared, const QString& url, int fps)
{
    QNetworkAccessManager nam;
    QString currentHtml = wrapPage(QStringLiteral("<h2>Loading…</h2><p>%1</p>").arg(url.toHtmlEscaped()),
                                   QStringLiteral("Isolated helper · HTML/CSS subset (no JS). "
                                                  "Install WebView2 Runtime + SDK for full overlays."));
    quint64 frame = 0;
    publishFrame(shared, renderHtml(currentHtml, shared.width, shared.height), ++frame, 1, {});

    auto reload = [&] {
        const QUrl qurl = QUrl::fromUserInput(url);
        if (qurl.isLocalFile()) {
            QFile f(qurl.toLocalFile());
            if (f.open(QIODevice::ReadOnly)) {
                currentHtml = QString::fromUtf8(f.readAll());
                publishFrame(shared, renderHtml(currentHtml, shared.width, shared.height), ++frame, 0, {});
            }
            return;
        }
        QNetworkRequest req(qurl);
        req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("RailShotTV-BrowserHelper/0.2"));
        QNetworkReply* reply = nam.get(req);
        QObject::connect(reply, &QNetworkReply::finished, &app, [&, reply] {
            QString html;
            if (reply->error() == QNetworkReply::NoError) {
                html = QString::fromUtf8(reply->readAll());
                if (!html.contains(QLatin1String("<html"), Qt::CaseInsensitive))
                    html = wrapPage(html, QStringLiteral("Fetched document · JS not executed in fallback mode."));
            } else {
                html = wrapPage(QStringLiteral("<h2>Load failed</h2><p>%1</p><p>%2</p>")
                                    .arg(url.toHtmlEscaped(), reply->errorString().toHtmlEscaped()),
                                QStringLiteral("Check URL / network."));
            }
            currentHtml = html;
            publishFrame(shared, renderHtml(currentHtml, shared.width, shared.height), ++frame,
                         reply->error() == QNetworkReply::NoError ? 0 : 2,
                         reply->errorString());
            reply->deleteLater();
        });
    };
    reload();

    QTimer publishTimer;
    QObject::connect(&publishTimer, &QTimer::timeout, &app, [&] {
        publishFrame(shared, renderHtml(currentHtml, shared.width, shared.height), ++frame, 0, {});
    });
    publishTimer.start(1000 / fps);

    QTimer refreshTimer;
    QObject::connect(&refreshTimer, &QTimer::timeout, &app, reload);
    refreshTimer.start(15000);

    return app.exec();
}

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "windows");
#ifdef _WIN32
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
#endif
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("railshot_browser_helper"));

    QCommandLineParser parser;
    parser.addHelpOption();
    QCommandLineOption urlOpt(QStringLiteral("url"), QStringLiteral("Page URL or file path"), QStringLiteral("url"));
    QCommandLineOption wOpt(QStringLiteral("width"), QStringLiteral("Width"), QStringLiteral("w"), QStringLiteral("1280"));
    QCommandLineOption hOpt(QStringLiteral("height"), QStringLiteral("Height"), QStringLiteral("h"), QStringLiteral("720"));
    QCommandLineOption shmOpt(QStringLiteral("shm"), QStringLiteral("Shared mapping name"), QStringLiteral("name"));
    QCommandLineOption fpsOpt(QStringLiteral("fps"), QStringLiteral("Publish FPS"), QStringLiteral("fps"), QStringLiteral("15"));
    parser.addOption(urlOpt);
    parser.addOption(wOpt);
    parser.addOption(hOpt);
    parser.addOption(shmOpt);
    parser.addOption(fpsOpt);
    parser.process(app);

    SharedState shared;
    shared.width = parser.value(wOpt).toInt();
    shared.height = parser.value(hOpt).toInt();
    const QString shm = parser.value(shmOpt);
    const QString url = parser.value(urlOpt);
    const int fps = qMax(1, parser.value(fpsOpt).toInt());
    if (shm.isEmpty() || url.isEmpty())
        return 2;

    QString err;
    if (!openShared(shared, shm, &err))
        return 3;

#if defined(RAILSHOT_HAS_WEBVIEW2) && RAILSHOT_HAS_WEBVIEW2
    {
        WebViewHost host;
        QString wvErr;
        if (host.start(shared.width, shared.height, url, &wvErr)) {
            quint64 frame = 0;
            QTimer publishTimer;
            QObject::connect(&publishTimer, &QTimer::timeout, &app, [&] {
                MSG msg;
                while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
                    TranslateMessage(&msg);
                    DispatchMessageW(&msg);
                }
                publishFrame(shared, host.grab(shared.width, shared.height), ++frame, 0, {});
            });
            publishTimer.start(1000 / fps);
            const int rc = app.exec();
            host.shutdown();
            closeShared(shared);
#ifdef _WIN32
            CoUninitialize();
#endif
            return rc;
        }
        // Fall through to QTextBrowser path if Evergreen runtime missing.
        Q_UNUSED(wvErr);
    }
#endif

    const int rc = runFallback(app, shared, url, fps);
    closeShared(shared);
#ifdef _WIN32
    CoUninitialize();
#endif
    return rc;
}
