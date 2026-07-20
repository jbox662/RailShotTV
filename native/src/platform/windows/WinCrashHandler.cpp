#include "platform/windows/WinCrashHandler.h"
#include "core/Logger.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <dbghelp.h>
#include <QDir>
#include <QDateTime>
#pragma comment(lib, "Dbghelp.lib")

namespace {
QString g_dumpDir;

LONG WINAPI railshotUnhandledException(EXCEPTION_POINTERS* info)
{
    if (g_dumpDir.isEmpty())
        return EXCEPTION_CONTINUE_SEARCH;

    QDir().mkpath(g_dumpDir);
    const QString path = g_dumpDir + QStringLiteral("/crash-%1.dmp")
                             .arg(QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd-HHmmss")));

    HANDLE file = CreateFileW(reinterpret_cast<LPCWSTR>(path.utf16()),
                              GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION mei{};
        mei.ThreadId = GetCurrentThreadId();
        mei.ExceptionPointers = info;
        mei.ClientPointers = FALSE;
        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file,
                          MiniDumpWithDataSegs, &mei, nullptr, nullptr);
        CloseHandle(file);
        railshot::Logger::error(QStringLiteral("Crash dump written: %1").arg(path));
    }
    return EXCEPTION_EXECUTE_HANDLER;
}
} // namespace
#endif

namespace railshot {

void WinCrashHandler::install(const QString& dumpDirectory)
{
#ifdef _WIN32
    g_dumpDir = dumpDirectory;
    QDir().mkpath(dumpDirectory);
    SetUnhandledExceptionFilter(railshotUnhandledException);
    Logger::info(QStringLiteral("Crash handler installed → %1").arg(dumpDirectory));
#else
    Q_UNUSED(dumpDirectory);
#endif
}

} // namespace railshot
