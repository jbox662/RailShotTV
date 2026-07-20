# RailShotTV Native Engine

Production Windows broadcast application (Qt 6 Widgets + D3D11).

## Layout
```
src/
  app/           entry + Application
  core/          domain, persistence, EngineController
  capture/       cameras, desktop duplication, image/text/browser/overlays
  browser/       isolated helper IPC (WebView2 when available)
  vcam/          Win11 MF virtual camera media source DLL
  compositor/    D3D11 preview/program + transitions
  audio/         WASAPI capture/mix/monitor/clock
  encoding/      MF H.264 + software + AAC
  streaming/     RTMP output + session
  recording/     crash-safe recorder
  telemetry/     health metrics
  scoreboard/ schedule/ chat/ overlays/
  ui/            Dashboard shell + pages
  platform/windows/
tests/
packaging/windows/
```

Optional deps:
- FFmpeg LGPL: `../scripts/install-ffmpeg.ps1`
- WebView2 SDK: `../scripts/install-webview2.ps1` (JS browser overlays)
- Virtual cam register: `../scripts/register-vcam.ps1` (Win11; UI also auto-registers)

See `docs/build/BUILD.md` at repo root.
