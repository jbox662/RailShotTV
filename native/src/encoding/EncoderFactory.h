#pragma once

#include "encoding/IEncoder.h"
#include <memory>

namespace railshot {

class EncoderFactory {
public:
    static std::unique_ptr<IVideoEncoder> createVideo(const OutputProfile& profile, QString* selectedName = nullptr);
    static std::unique_ptr<IAudioEncoder> createAudio(const OutputProfile& profile);
    static QStringList availableVideoEncoders();
};

} // namespace railshot
