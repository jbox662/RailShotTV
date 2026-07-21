#pragma once

#include <QString>

namespace railshot {

/// Lossless remux via ffmpeg on PATH (`-c copy`). Blocking.
bool remuxCopy(const QString& inputPath, const QString& outputPath, QString* error = nullptr);

/// Derive `foo_remux.mp4` next to an MKV (or replace extension).
QString defaultRemuxOutputPath(const QString& inputPath,
                               const QString& containerExt = QStringLiteral("mp4"));

} // namespace railshot
