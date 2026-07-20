#include "core/Logger.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <iostream>

namespace railshot {

QMutex& Logger::mutex()
{
    static QMutex m;
    return m;
}

LogLevel& Logger::levelRef()
{
    static LogLevel l = LogLevel::Info;
    return l;
}

std::function<void(LogLevel, const QString&)>& Logger::sinkRef()
{
    static std::function<void(LogLevel, const QString&)> s;
    return s;
}

static QString& filePathRef()
{
    static QString p;
    return p;
}

void Logger::setLevel(LogLevel level)
{
    QMutexLocker lock(&mutex());
    levelRef() = level;
}

void Logger::setSink(std::function<void(LogLevel, const QString&)> sink)
{
    QMutexLocker lock(&mutex());
    sinkRef() = std::move(sink);
}

QString Logger::logFilePath()
{
    QMutexLocker lock(&mutex());
    return filePathRef();
}

void Logger::initFileLogging(const QString& directory)
{
    QDir().mkpath(directory);
    const QString path = directory + QStringLiteral("/railshot-%1.log")
                             .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss")));
    {
        QMutexLocker lock(&mutex());
        filePathRef() = path;
    }
    info(QStringLiteral("Logging to %1").arg(path));
}

static const char* levelName(LogLevel l)
{
    switch (l) {
    case LogLevel::Debug: return "DBG";
    case LogLevel::Info: return "INF";
    case LogLevel::Warn: return "WRN";
    case LogLevel::Error: return "ERR";
    }
    return "???";
}

void Logger::log(LogLevel level, const QString& msg)
{
    QMutexLocker lock(&mutex());
    if (static_cast<int>(level) < static_cast<int>(levelRef()))
        return;

    const QString line = QStringLiteral("[%1] [%2] %3")
                             .arg(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs),
                                  QLatin1String(levelName(level)),
                                  msg);

    if (sinkRef())
        sinkRef()(level, line);
    else
        std::cerr << line.toStdString() << std::endl;

    if (!filePathRef().isEmpty()) {
        QFile f(filePathRef());
        if (f.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            QTextStream ts(&f);
            ts << line << '\n';
        }
    }
}

void Logger::debug(const QString& msg) { log(LogLevel::Debug, msg); }
void Logger::info(const QString& msg) { log(LogLevel::Info, msg); }
void Logger::warn(const QString& msg) { log(LogLevel::Warn, msg); }
void Logger::error(const QString& msg) { log(LogLevel::Error, msg); }

} // namespace railshot
