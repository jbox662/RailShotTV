# RailShotTV

**Billiards-first Windows livestreaming broadcast engine.**

This repository contains:

1. **`native/`** — proprietary Qt 6 / C++ / Direct3D 11 production engine (the real app)
2. **`client/`** — React UX prototype used as the visual/interaction reference

## Status

Native engine foundation is in place:

- Scene / source project model with versioned JSON + Credential Manager secrets
- D3D11 preview/program compositor with transitions
- WASAPI mic + desktop loopback mixer with meters
- Media Foundation / software encoder path, RTMP session with reconnect, crash-safe recording
- Dashboard shell (Preview | GO | Program), scoreboard, schedule, chat, analytics, settings
- Unit/integration tests + GitHub Actions CI skeleton
- Licensing docs for proprietary + LGPL FFmpeg / Qt compliance

## Quick start (native)

See [docs/build/BUILD.md](docs/build/BUILD.md).

```powershell
cd native
$env:QT6_DIR = "C:\Qt\6.7.3\msvc2019_64"
cmake --preset windows-msvc-debug
cmake --build --preset debug
```

## Quick start (prototype UI reference)

```powershell
pnpm install
pnpm dev
```

## Docs
- Architecture: [docs/architecture/OVERVIEW.md](docs/architecture/OVERVIEW.md)
- Licensing: [docs/licensing/THIRD_PARTY_NOTICES.md](docs/licensing/THIRD_PARTY_NOTICES.md)
- Release gates: [docs/release/RELEASE.md](docs/release/RELEASE.md)
- Design brief: [ideas.md](ideas.md)

## License

Proprietary — see [LICENSE](LICENSE) and [docs/licensing/EULA.txt](docs/licensing/EULA.txt).
