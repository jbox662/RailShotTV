#pragma once

#include <QString>

namespace railshot {

class WinCrashHandler {
public:
    /// Install unhandled exception filter + optional minidump directory.
    static void install(const QString& dumpDirectory);
};

} // namespace railshot
