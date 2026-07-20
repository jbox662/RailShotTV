#pragma once

#include "core/Types.h"
#include <QObject>
#include <QImage>

namespace railshot {

class OverlayRenderer : public QObject {
    Q_OBJECT
public:
    explicit OverlayRenderer(QObject* parent = nullptr);
    QImage renderScoreboard(const QJsonObject& state, int width, int height) const;
    QImage renderLowerThird(const QString& title, const QString& subtitle, int width, int height) const;
    QImage renderAlert(const QString& title, const QString& body, int width, int height) const;
};

} // namespace railshot
