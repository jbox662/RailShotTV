# FFmpeg Build Policy (Proprietary / LGPL)

RailShotTV ships **only** LGPL FFmpeg builds.

## Configure flags (required)

```bash
./configure \
  --disable-static --enable-shared \
  --disable-gpl --disable-nonfree \
  --enable-avcodec --enable-avformat --enable-avutil \
  --enable-swresample --enable-swscale --enable-avfilter \
  --enable-libmp3lame  # only if LAME LGPL path reviewed \
  --prefix=/path/to/ffmpeg-lgpl
```

## Forbidden in shipping builds
- `--enable-gpl`
- `--enable-nonfree`
- `--enable-libx264` (GPL)
- Any filter/codec that flips the tree to GPL

## Hardware encoding
Prefer Windows Media Foundation / vendor SDKs **outside** FFmpeg for H.264.
If using FFmpeg NVENC wrappers, legal review is mandatory before release.

## Distribution checklist
1. Dynamic DLLs (`avcodec-*.dll`, etc.) next to `RailShotTV.exe` or in a `ffmpeg/` folder
2. Offer FFmpeg corresponding source download URL in About / Notices
3. Mention FFmpeg LGPLv2.1 in EULA and website download pages
4. Record exact commit/hash of the FFmpeg tree used
