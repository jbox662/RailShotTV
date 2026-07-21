#pragma once

#include <QString>
#include <QStringList>
#include <QList>
#include <QUrl>

namespace railshot {
class EngineController;

/// Import dropped local files / URLs as Image, Media, or Browser sources.
/// Returns number of sources created; optional human summary.
int importDroppedUrls(EngineController* engine, const QList<QUrl>& urls, QString* summary = nullptr);

} // namespace railshot
