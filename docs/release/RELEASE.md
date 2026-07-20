# Release Gates & Runbook

## Performance / reliability gates (Phase 3)
- Sustained 1080p60 compose on supported hardware
- No UI stalls > 100 ms during steady streaming
- Bounded memory over 8-hour soak (no monotonic growth)
- A/V drift < 50 ms average
- RTMP reconnect recovers within backoff policy
- Recording file playable / recoverable after forced kill
- Zero crashes in 8-hour soak
- Encoder auto-select prefers Media Foundation H.264, falls back to software
- OutputHub copies GPU frames into a texture pool (encode off UI thread)

### Local soak
```powershell
# Short gate (1h). Use -Hours 8 for full gate.
./scripts/soak-test.ps1 -Hours 1
```

## Browser overlays (ADR-004)
Isolated `railshot_browser_helper.exe` prefers **WebView2** (full HTML/CSS/JS) when the
Evergreen WebView2 Runtime and SDK are available (`scripts/install-webview2.ps1`).
If WebView2 init fails, it falls back to QTextBrowser (HTML/CSS subset, no JS).
`BrowserSource` uploads frames into the compositor via `BrowserIpc.h` shared memory.

## Virtual camera
Program frames are published to `Local\RailShotTV_VirtualCamera` with mutex
`Local\RailShotTV_VirtualCamera_mtx`. On Windows 11, enabling Virtual Camera also:
1. Stages/registers `railshot_vcam.dll` (MF media source)
2. Calls `MFCreateVirtualCamera` so apps can enumerate **RailShotTV Virtual Camera**

Manual register: `./scripts/register-vcam.ps1`. Frame Server must be able to load the DLL
(staged under `%ProgramData%\RailShotTV\` by default).

### Virtual cam smoke checklist (Win11)
- [ ] Enable Virtual Camera in RailShotTV toolbar
- [ ] Confirm `%ProgramData%\RailShotTV\railshot_vcam.dll` exists
- [ ] Device appears as **RailShotTV Virtual Camera** in Camera app / Zoom / Teams / Discord
- [ ] Program output updates live; idle pattern shows before first frame
- [ ] Disable Virtual Camera; device stops cleanly

## Failure injection checklist
- Unplug camera / disable audio device
- Change display mode while capturing
- Sleep / resume
- Kill network mid-stream
- Fill disk during recording
- Force encoder open failure (fallback path)

## Packaging
1. Build RelWithDebInfo
2. `windeployqt` / `qt_generate_deploy_app_script`
3. Bundle LGPL FFmpeg DLLs
4. Build WiX / MSIX installer (`native/packaging/windows`)
   - `scripts/stage-deploy.ps1` then `scripts/harvest-wix.ps1`
5. Authenticode sign EXE + MSI
6. Archive PDBs to symbol server
7. Attach SBOM + THIRD_PARTY_NOTICES
8. Publish GitHub Release with checksums

## Signing (placeholder)
Set `RAILSHOT_CERT_THUMBPRINT` and run `scripts/sign-release.ps1` after CI artifacts land.

## Audio notes
Capture paths that are not 48 kHz are linearly resampled in `AudioGraph` before mix.
A/V drift is shown on the Stream Status strip and status bar.
