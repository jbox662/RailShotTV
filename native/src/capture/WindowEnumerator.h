#pragma once

#include <QString>
#include <QVector>
#include <cstdint>

namespace railshot {

struct WindowInfo {
    quintptr hwnd = 0;
    QString title;
    QString exeName;
};

/// Enumerates visible top-level windows suitable for capture pickers.
class WindowEnumerator {
public:
    static QVector<WindowInfo> listTopLevelWindows();
};

} // namespace railshot
