# OBS Feature Parity Matrix (RailShot)

Living SoT for OBS Studio frontend → RailShot Chromatic Command.
Status: `done` | `partial` | `todo` | `wont` (documented engine limit).

Last updated: Wave 6 (Extra browsers, dropfiles, hotkey UI, stats dock).

## Design rules

- Behavior/layout from OBS (`obs-studio-master/frontend/`)
- Visuals: Chromatic Command (`docs/design/ORIGINAL_UI_RESTORE_SPEC.md`)
- Engine: proprietary Qt/D3D (no libobs)
- Game Capture = WGC/window (`wont` for DLL graphics-hook until proprietary path exists)

## Widgets (`frontend/widgets`)

| OBS | Role | Status | RailShot |
|-----|------|--------|----------|
| OBSBasic / OBSBasic_* | Main shell | partial | `MainWindow`, `DashboardPage`, `TopMenuBar` |
| OBSBasicControls | Stream/Record/Replay/VCam/Studio | partial | `BottomToolbar` + Go Live |
| OBSBasicStatusBar | FPS/CPU/dropped/bitrate/timers | partial | `ObsStatusBarWidget` (+ reconnect) |
| OBSBasicPreview | Interactive preview | partial | `PreviewWidget` + scale/lock/projectors |
| OBSBasic_StudioMode | Preview/Program | partial | Dual monitors; Studio Mode toggle |
| OBSBasic_Scenes | Scenes dock | partial | `SceneListWidget` + full toolbar |
| OBSBasic_SceneItems | Sources dock | partial | `InputTilesWidget` |
| AudioMixer | Mixer dock | partial | `AudioMixerWidget` + Adv Audio |
| OBSBasic_Transitions | Transitions | partial | `TransitionPanel` |
| OBSBasicStats | Stats dock | partial | `ObsStatsDock` + Analytics page |
| OBSProjector / Multiview | Projectors | partial | `ProjectorWindow` (Preview/Program; Multiview later) |
| OBSBasic_ContextToolbar | Source context toolbar | partial | `SourceContextToolbar` |
| OBSBasic_Hotkeys | Hotkeys | partial | `HotkeyDispatcher` + Settings labels + Shortcuts overlay |
| OBSBasic_Recording/_Streaming/_ReplayBuffer/_VirtualCam | Outputs | partial | `OutputHub` + Settings Stream/Output/Advanced |
| OBSBasic_Screenshots | Screenshots | todo | — |
| OBSBasic_Projectors | Projector windows | partial | `ProjectorWindow` + View menu |
| OBSBasic_Docks | Dock layout | partial | Nested QMainWindow docks |
| OBSBasic_Profiles / _SceneCollections | Profiles/collections | partial | `ProfilesDialog` / `SceneCollectionsDialog` |
| OBSBasic_Clipboard | Copy/paste transforms | partial | `TransformDialog` clipboard |
| OBSBasic_Dropfiles | Drag-drop media | partial | Preview drop → Image/Media/Browser |
| OBSBasic_Browser | Extra browsers | partial | `ExtraBrowserPanel` docks (Docks → Add Browser Panel) |
| ColorSelect | Color tags | partial | Properties swatches |

## Dialogs

| OBS | Status | RailShot |
|-----|--------|----------|
| OBSBasicSourceSelect | partial | `AddSourceDialog` |
| OBSBasicProperties | partial | `SourcePropertiesDialog` |
| OBSBasicFilters | partial | `FiltersDialog` |
| OBSBasicTransform | partial | `TransformDialog` |
| OBSBasicInteraction | partial | `InteractDialog` |
| OBSBasicAdvAudio | partial | `AdvAudioDialog` |
| OBSBasicVCamConfig | todo | External/VCam toggle |
| OBSRemux / MissingFiles / LogViewer | partial | `RemuxDialog` / auto-remux / `LogViewerDialog` (MissingFiles later) |
| NameDialog | partial | QInputDialog |

## Wave progress

- [x] Wave 0 — this matrix
- [x] Wave 1 — Controls polish, StatusBar, Scenes toolbar, Context toolbar, Filters + Transform
- [x] Wave 2 — Adv Audio + mixer depth (balance, monitor modes, sync, tracks, lock)
- [x] Wave 3 — Preview power (lock, scale, projectors, interact)
- [x] Wave 4 — Profiles / collections / remux / logs
- [x] Wave 5 — Output/service polish (Settings Stream/Output/Advanced, rate control, reconnect, delay, auto-remux, filename tokens)
- [x] Wave 6 — Extra browsers, dropfiles, hotkey UI, stats dock
