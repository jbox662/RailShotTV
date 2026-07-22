# OBS Feature Parity Matrix (RailShot)

Living SoT for OBS Studio frontend → RailShot Chromatic Command.
Status: `done` | `partial` | `todo` | `wont` (documented engine limit).

Last updated: Wave 17 (Apply LUT PNG filter).

## Design rules

- Behavior/layout from OBS (`obs-studio-master/frontend/`)
- Visuals: Chromatic Command (`docs/design/ORIGINAL_UI_RESTORE_SPEC.md`)
- Engine: proprietary Qt/D3D (no libobs)
- Game Capture = WGC/window (`wont` for DLL graphics-hook until proprietary path exists)

## Widgets (`frontend/widgets`)

| OBS | Role | Status | RailShot |
|-----|------|--------|----------|
| OBSBasic / OBSBasic_* | Main shell | partial | `MainWindow`, `DashboardPage`, `TopMenuBar` (File/Edit/View/Docks menus) |
| OBSBasicControls | Stream/Record/Replay/VCam/Studio | partial | `BottomToolbar` Controls core + Swap (Studio on); tools via Docks |
| OBSBasicStatusBar | FPS/CPU/dropped/bitrate/timers | partial | `ObsStatusBarWidget` (+ reconnect) |
| OBSBasicPreview | Interactive preview | partial | `PreviewWidget` + Display ▾ (scale/lock/projectors) |
| OBSBasic_StudioMode | Preview/Program | partial | Dual monitors; Studio Mode toggle + Swap |
| OBSBasic_Scenes | Scenes dock | partial | `SceneListWidget` + full toolbar |
| OBSBasic_SceneItems | Sources dock | partial | `InputTilesWidget` (OBS context menu: rename, filters, transform, order, duplicate, copy/paste filters, Set Color) |
| AudioMixer | Mixer dock | partial | `AudioMixerWidget` + Adv Audio (OBS audio-capable sources only) |
| OBSBasic_Transitions | Transitions | partial | `TransitionPanel` (Wipe L/R/U/D; Merge ≠ Fade; Stinger + POT) |
| Groups / Scene | Scene-as-source + Groups | partial | `SourceType::Scene` / `Group`; nested compose + `childIds` |
| OBSBasicStats | Stats dock | partial | `ObsStatsDock` + Analytics page |
| OBSProjector / Multiview | Projectors | partial | `ProjectorWindow` + `MultiviewWindow` |
| OBSBasic_ContextToolbar | Source context toolbar | partial | `SourceContextToolbar` |
| OBSBasic_Hotkeys | Hotkeys | partial | `HotkeyDispatcher` + Settings labels + Shortcuts overlay |
| OBSBasic_Recording/_Streaming/_ReplayBuffer/_VirtualCam | Outputs | partial | `OutputHub` + Settings Stream/Output/Advanced |
| OBSBasic_Screenshots | Screenshots | partial | Preview/Program PNG via View menu + F12 |
| OBSBasic_Projectors | Projector windows | partial | `ProjectorWindow` + View menu |
| OBSBasic_Docks | Dock layout | partial | Nested QMainWindow docks |
| OBSBasic_Profiles / _SceneCollections | Profiles/collections | partial | `ProfilesDialog` / `SceneCollectionsDialog` |
| OBSBasic_Clipboard | Copy/paste transforms | partial | `TransformDialog` clipboard + Filters clipboard |
| Undo / Redo | Edit history | partial | `UndoStack` + Edit menu + Ctrl+Z/Y (project snapshots) |
| OBSBasic_Dropfiles | Drag-drop media | partial | Preview drop → Image/Media/Browser |
| Media Source (network) | Local file / Input URL | partial | `MediaSource` + Properties: Local File off → rtsp/http/hls + FFmpeg options |
| OBSBasic_Browser | Extra browsers | partial | `ExtraBrowserPanel` docks (Docks → Add Browser Panel) |
| ColorSelect | Color tags | partial | Sources list tint + Set Color menu + Properties swatches |

## Dialogs

| OBS | Status | RailShot |
|-----|--------|----------|
| OBSBasicSourceSelect | partial | Sources **+** type menu → Create/Select → Properties |
| OBSBasicProperties | partial | `SourcePropertiesDialog` + live source preview |
| OBSBasicFilters | partial | `FiltersDialog` (Color Correction, Chroma/Color/Luma Key, Image Mask, Apply LUT, Blur, Crop/Pad, Scroll, Sharpen) |
| OBSBasicTransform | partial | `TransformDialog` |
| OBSBasicInteraction | partial | `InteractDialog` |
| OBSBasicAdvAudio | partial | `AdvAudioDialog` (+ Gain, Noise Gate, Compressor, 3-Band EQ, Limiter) |
| OBSBasicVCamConfig | partial | `VCamConfigDialog` (View → Virtual Camera…) |
| OBSRemux / MissingFiles / LogViewer | partial | `RemuxDialog` / `MissingFilesDialog` / `LogViewerDialog` |
| NameDialog | partial | QInputDialog |

## Wave progress

- [x] Wave 0 — this matrix
- [x] Wave 1 — Controls polish, StatusBar, Scenes toolbar, Context toolbar, Filters + Transform
- [x] Wave 2 — Adv Audio + mixer depth (balance, monitor modes, sync, tracks, lock)
- [x] Wave 3 — Preview power (lock, scale, projectors, interact)
- [x] Wave 4 — Profiles / collections / remux / logs
- [x] Wave 5 — Output/service polish (Settings Stream/Output/Advanced, rate control, reconnect, delay, auto-remux, filename tokens)
- [x] Wave 6 — Extra browsers, dropfiles, hotkey UI, stats dock
- [x] Wave 7 — Screenshots, Missing Files, VCam config, Multiview
- [x] Wave 8 — Sources context menu, duplicate, color tags, Filters stack (Color Correction + reorder + copy/paste)
- [x] Wave 9 — Replay Start/Stop, Studio Mode Swap + hotkeys, Wipe direction + Merge≠Fade
- [x] Wave 10 — OBS chrome cleanup (File/Edit/View/Docks menus; Controls core; Scenes/Preview overlap fixes)
- [x] Wave 11 — Filters Crop/Pad + Scroll + Sharpen; Adv Audio Gain / Noise Gate / Compressor
- [x] Wave 12 — Scene-as-source, Groups, Stinger transition (media + point-of-take)
- [x] Wave 13 — Adv Audio 3-Band EQ + Limiter
- [x] Wave 14 — Color Key + Luma Key; Chroma Key color type / smoothness
- [x] Wave 15 — Undo/Redo (project snapshots, coalesced edits, Edit menu + hotkeys)
- [x] Wave 16 — Image Mask (Mask Alpha: path, opacity, invert)
- [x] Wave 17 — Apply LUT (OBS-style 512×512 PNG; path + amount; `.cube` deferred)
