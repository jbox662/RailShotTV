#pragma once

#include <QString>
#include <QDateTime>

namespace railshot {

/// Expand OBS-like recording filename tokens.
/// Supported: %CCYY %YY %MM %DD %HH %mm %ss %%
QString expandRecordingFilename(const QString& pattern,
                                const QDateTime& when = QDateTime::currentDateTime());

QString defaultRecordingFilenamePattern();

/// Build `dir/expanded.mkv` (ensures .mkv suffix on the basename).
QString buildRecordingFilePath(const QString& directory, const QString& pattern,
                               const QDateTime& when = QDateTime::currentDateTime());

} // namespace railshot
