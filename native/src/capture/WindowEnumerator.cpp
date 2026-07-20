#include "capture/WindowEnumerator.h"

#include <algorithm>
#include <string>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <dwmapi.h>
#endif

namespace railshot {

#ifdef _WIN32
namespace {

bool isCloaked(HWND hwnd)
{
    BOOL cloaked = FALSE;
    if (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked))))
        return cloaked != FALSE;
    return false;
}

QString windowTitle(HWND hwnd)
{
    const int len = GetWindowTextLengthW(hwnd);
    if (len <= 0)
        return {};
    std::wstring buf(static_cast<size_t>(len) + 1, L'\0');
    const int written = GetWindowTextW(hwnd, buf.data(), len + 1);
    if (written <= 0)
        return {};
    buf.resize(static_cast<size_t>(written));
    return QString::fromWCharArray(buf.data(), written);
}

QString processExeName(DWORD pid)
{
    HANDLE proc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!proc)
        return {};
    wchar_t path[MAX_PATH] = {};
    DWORD size = MAX_PATH;
    QString name;
    if (QueryFullProcessImageNameW(proc, 0, path, &size) && size > 0) {
        const QString full = QString::fromWCharArray(path, static_cast<int>(size));
        const int slash = std::max(full.lastIndexOf(QLatin1Char('\\')), full.lastIndexOf(QLatin1Char('/')));
        name = slash >= 0 ? full.mid(slash + 1) : full;
    }
    CloseHandle(proc);
    return name;
}

struct EnumState {
    QVector<WindowInfo>* out = nullptr;
};

BOOL CALLBACK enumProc(HWND hwnd, LPARAM lParam)
{
    auto* state = reinterpret_cast<EnumState*>(lParam);
    if (!state || !state->out)
        return TRUE;
    if (!IsWindowVisible(hwnd) || !IsWindow(hwnd))
        return TRUE;
    if (GetWindow(hwnd, GW_OWNER) != nullptr)
        return TRUE;

    const LONG_PTR exStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    if (exStyle & WS_EX_TOOLWINDOW)
        return TRUE;
    if (isCloaked(hwnd))
        return TRUE;

    RECT rc{};
    if (!GetClientRect(hwnd, &rc) || (rc.right - rc.left) < 2 || (rc.bottom - rc.top) < 2)
        return TRUE;

    const QString title = windowTitle(hwnd);
    if (title.isEmpty())
        return TRUE;

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);

    WindowInfo info;
    info.hwnd = reinterpret_cast<quintptr>(hwnd);
    info.title = title;
    info.exeName = processExeName(pid);
    state->out->push_back(std::move(info));
    return TRUE;
}

} // namespace
#endif

QVector<WindowInfo> WindowEnumerator::listTopLevelWindows()
{
    QVector<WindowInfo> out;
#ifdef _WIN32
    EnumState state{&out};
    EnumWindows(enumProc, reinterpret_cast<LPARAM>(&state));
#endif
    return out;
}

} // namespace railshot
