#pragma once

#include <QImage>
#include <QString>

namespace railshot {

/// Parse Adobe/.cube 3D LUTs and bake an OBS-style 512×512 (8×8 of 64×64) PNG image.
/// 1D LUTs are not supported in this wave.
class LutCubeLoader {
public:
    /// Returns a null image on failure; sets *errorOut if provided.
    static QImage loadAsObsPng(const QString& path, QString* errorOut = nullptr);
};

} // namespace railshot
