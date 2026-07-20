#pragma once

#include <QtGlobal>
#include <cstddef>

namespace railshot {
namespace browser_ipc {

inline constexpr quint32 kMagic = 0x52534252; // RSBR
inline constexpr wchar_t kDefaultMappingName[] = L"Local\\RailShotTV_BrowserFrame";
inline constexpr wchar_t kDefaultMutexName[] = L"Local\\RailShotTV_BrowserMutex";

#pragma pack(push, 1)
struct FrameHeader {
    quint32 magic = kMagic;
    quint32 width = 0;
    quint32 height = 0;
    quint32 stride = 0;
    quint32 bytesPerPixel = 4;
    quint64 frameIndex = 0;
    qint64 ptsUs = 0;
    quint32 status = 0; // 0=ok, 1=loading, 2=error
    char error[128] = {};
};
#pragma pack(pop)

inline size_t bufferBytes(int width, int height)
{
    return sizeof(FrameHeader) + static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
}

} // namespace browser_ipc
} // namespace railshot
