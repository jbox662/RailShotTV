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
    /// Main → helper: 0=none, 1=soft reload (OBS Refresh / ReloadIgnoreCache).
    quint32 command = 0;
    /// Helper → main: echoes last handled command id/counter.
    quint32 commandAck = 0;
    char error[128] = {};
};
#pragma pack(pop)

inline constexpr quint32 kCmdNone = 0;
inline constexpr quint32 kCmdReload = 1;

inline size_t bufferBytes(int width, int height)
{
    return sizeof(FrameHeader) + static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
}

} // namespace browser_ipc
} // namespace railshot
