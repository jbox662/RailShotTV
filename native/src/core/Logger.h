#pragma once

#include <QString>
#include <QMutex>
#include <functional>

namespace railshot {

enum class LogLevel { Debug, Info, Warn, Error };

class Logger {
public:
    static void setLevel(LogLevel level);
    static void setSink(std::function<void(LogLevel, const QString&)> sink);
    static void debug(const QString& msg);
    static void info(const QString& msg);
    static void warn(const QString& msg);
    static void error(const QString& msg);
    static void log(LogLevel level, const QString& msg);
    static QString logFilePath();
    static void initFileLogging(const QString& directory);

private:
    static QMutex& mutex();
    static LogLevel& levelRef();
    static std::function<void(LogLevel, const QString&)>& sinkRef();
};

} // namespace railshot
