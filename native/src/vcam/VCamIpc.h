#pragma once

#include <cstdint>

namespace railshot {
namespace vcam_ipc {

inline constexpr std::uint32_t kMagic = 0x52535643; // RSVC
inline constexpr wchar_t kMapName[] = L"Local\\RailShotTV_VirtualCamera";
inline constexpr wchar_t kMtxName[] = L"Local\\RailShotTV_VirtualCamera_mtx";

// {8F3C2E1A-9B4D-4F6A-A7C8-1D2E3F4A5B6C}
inline constexpr wchar_t kMediaSourceClsid[] = L"{8F3C2E1A-9B4D-4F6A-A7C8-1D2E3F4A5B6C}";
inline constexpr wchar_t kFriendlyName[] = L"RailShotTV Virtual Camera";

#pragma pack(push, 1)
struct SharedHeader {
    std::uint32_t magic = kMagic;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t stride = 0;
    std::uint64_t frameIndex = 0;
    std::int64_t ptsUs = 0;
};
#pragma pack(pop)

inline std::size_t bufferBytes(int width, int height)
{
    return sizeof(SharedHeader) + static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4;
}

} // namespace vcam_ipc
} // namespace railshot
