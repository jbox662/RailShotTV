#include "core/RecordingPath.h"
#include <QDir>
#include <QFileInfo>

namespace railshot {

QString defaultRecordingFilenamePattern()
{
    return QStringLiteral("RailShotTV-%CCYY%MM%DD-%HH%mm%ss");
}

QString expandRecordingFilename(const QString& pattern, const QDateTime& when)
{
    QString out;
    out.reserve(pattern.size() + 8);
    const QString p = pattern.isEmpty() ? defaultRecordingFilenamePattern() : pattern;
    for (int i = 0; i < p.size(); ++i) {
        if (p[i] == QLatin1Char('%') && i + 1 < p.size()) {
            if (p[i + 1] == QLatin1Char('%')) {
                out += QLatin1Char('%');
                ++i;
                continue;
            }
            const QString token = p.mid(i, 5); // %CCYY
            if (token == QLatin1String("%CCYY")) {
                out += when.toString(QStringLiteral("yyyy"));
                i += 4;
                continue;
            }
            const QString t4 = p.mid(i, 3); // %YY %MM %DD %HH %mm %ss — actually % + 2 chars
            if (i + 2 < p.size()) {
                const QString t = p.mid(i, 3);
                if (t == QLatin1String("%YY")) {
                    out += when.toString(QStringLiteral("yy"));
                    i += 2;
                    continue;
                }
                if (t == QLatin1String("%MM")) {
                    out += when.toString(QStringLiteral("MM"));
                    i += 2;
                    continue;
                }
                if (t == QLatin1String("%DD")) {
                    out += when.toString(QStringLiteral("dd"));
                    i += 2;
                    continue;
                }
                if (t == QLatin1String("%HH")) {
                    out += when.toString(QStringLiteral("HH"));
                    i += 2;
                    continue;
                }
                if (t == QLatin1String("%mm")) {
                    out += when.toString(QStringLiteral("mm"));
                    i += 2;
                    continue;
                }
                if (t == QLatin1String("%ss")) {
                    out += when.toString(QStringLiteral("ss"));
                    i += 2;
                    continue;
                }
            }
        }
        out += p[i];
    }
    // Sanitize Windows-illegal path chars in the basename only
    out.replace(QLatin1Char('\\'), QLatin1Char('_'));
    out.replace(QLatin1Char('/'), QLatin1Char('_'));
    out.replace(QLatin1Char(':'), QLatin1Char('_'));
    out.replace(QLatin1Char('*'), QLatin1Char('_'));
    out.replace(QLatin1Char('?'), QLatin1Char('_'));
    out.replace(QLatin1Char('"'), QLatin1Char('_'));
    out.replace(QLatin1Char('<'), QLatin1Char('_'));
    out.replace(QLatin1Char('>'), QLatin1Char('_'));
    out.replace(QLatin1Char('|'), QLatin1Char('_'));
    return out;
}

QString buildRecordingFilePath(const QString& directory, const QString& pattern, const QDateTime& when)
{
    QString base = expandRecordingFilename(pattern, when);
    if (!base.endsWith(QLatin1String(".mkv"), Qt::CaseInsensitive))
        base += QStringLiteral(".mkv");
    return QDir(directory).filePath(base);
}

} // namespace railshot
