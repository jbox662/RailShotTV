#include "encoding/EncoderFactory.h"
#include "encoding/MfH264Encoder.h"
#include "encoding/SoftwareEncoder.h"
#include "encoding/AacEncoder.h"
#include "core/Logger.h"

namespace railshot {

QStringList EncoderFactory::availableVideoEncoders()
{
    return {
        QStringLiteral("auto"),
        QStringLiteral("mf"),
        QStringLiteral("software"),
        QStringLiteral("nvenc"),
        QStringLiteral("amf"),
        QStringLiteral("qsv"),
    };
}

std::unique_ptr<IVideoEncoder> EncoderFactory::createVideo(const OutputProfile& profile, QString* selectedName)
{
    const QString pref = profile.encoderPreference.toLower();

    auto tryMf = [&]() -> std::unique_ptr<IVideoEncoder> {
        auto enc = std::make_unique<MfH264Encoder>();
        QString err;
        if (enc->open(profile, &err)) {
            if (selectedName) *selectedName = enc->name();
            Logger::info(QStringLiteral("Selected video encoder: %1").arg(enc->name()));
            return enc;
        }
        Logger::warn(QStringLiteral("MF encoder unavailable: %1").arg(err));
        return nullptr;
    };

    auto trySoft = [&]() -> std::unique_ptr<IVideoEncoder> {
        auto enc = std::make_unique<SoftwareEncoder>();
        QString err;
        if (enc->open(profile, &err)) {
            if (selectedName) *selectedName = enc->name();
            Logger::info(QStringLiteral("Selected video encoder: %1").arg(enc->name()));
            return enc;
        }
        Logger::warn(QStringLiteral("Software encoder unavailable: %1").arg(err));
        return nullptr;
    };

    if (pref == QLatin1String("software"))
        return trySoft();
    if (pref == QLatin1String("mf")) {
        if (auto enc = tryMf()) return enc;
        return trySoft();
    }

    // auto / nvenc / amf / qsv → prefer MF hardware path, then software fallback
    if (auto enc = tryMf()) return enc;
    return trySoft();
}

std::unique_ptr<IAudioEncoder> EncoderFactory::createAudio(const OutputProfile& profile)
{
    auto enc = std::make_unique<AacEncoder>();
    QString err;
    if (!enc->open(profile, &err)) {
        Logger::warn(QStringLiteral("AAC encoder unavailable: %1").arg(err));
        return nullptr;
    }
    return enc;
}

} // namespace railshot
