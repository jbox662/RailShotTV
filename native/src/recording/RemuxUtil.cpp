#include "recording/RemuxUtil.h"
#include "core/Logger.h"
#include <QFileInfo>
#include <QProcess>

namespace railshot {

QString defaultRemuxOutputPath(const QString& inputPath, const QString& containerExt)
{
    const QFileInfo fi(inputPath);
    QString ext = containerExt;
    if (ext.startsWith(QLatin1Char('.')))
        ext = ext.mid(1);
    if (ext.isEmpty())
        ext = QStringLiteral("mp4");
    return fi.absolutePath() + QLatin1Char('/') + fi.completeBaseName()
           + QStringLiteral("_remux.") + ext;
}

bool remuxCopy(const QString& inputPath, const QString& outputPath, QString* error)
{
    if (inputPath.isEmpty() || outputPath.isEmpty()) {
        if (error) *error = QStringLiteral("Remux paths empty");
        return false;
    }
    QProcess proc;
    proc.start(QStringLiteral("ffmpeg"),
               {QStringLiteral("-y"), QStringLiteral("-i"), inputPath,
                QStringLiteral("-c"), QStringLiteral("copy"), outputPath});
    if (!proc.waitForStarted(5000)) {
        if (error)
            *error = QStringLiteral("Could not start ffmpeg (install FFmpeg and add it to PATH)");
        return false;
    }
    if (!proc.waitForFinished(600000)) {
        proc.kill();
        if (error) *error = QStringLiteral("Remux timed out");
        return false;
    }
    if (proc.exitCode() != 0) {
        const QString detail = QString::fromUtf8(proc.readAllStandardError()).trimmed();
        if (error)
            *error = detail.isEmpty()
                         ? QStringLiteral("ffmpeg remux failed (exit %1)").arg(proc.exitCode())
                         : detail.left(400);
        Logger::warn(QStringLiteral("Remux failed: %1").arg(error ? *error : QString()));
        return false;
    }
    Logger::info(QStringLiteral("Remuxed %1 → %2").arg(inputPath, outputPath));
    return true;
}

} // namespace railshot
