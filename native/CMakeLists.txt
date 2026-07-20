cmake_minimum_required(VERSION 3.21)

project(RailShotTV
    VERSION 0.1.0
    DESCRIPTION "RailShotTV — proprietary Windows livestreaming broadcast engine"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

option(RAILSHOT_BUILD_TESTS "Build unit and integration tests" ON)
option(RAILSHOT_ENABLE_FFMPEG "Link against LGPL FFmpeg libraries" ON)
option(RAILSHOT_ENABLE_HW_ENCODERS "Enable vendor hardware encoder backends" ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Network
    Concurrent
    Svg
)

include(RailShotDependencies)

# ── Core domain + engine libraries ──────────────────────────────────────────
add_library(railshot_core STATIC
    src/core/Types.h
    src/core/Types.cpp
    src/core/Project.h
    src/core/Project.cpp
    src/core/SceneGraph.h
    src/core/SceneGraph.cpp
    src/core/SettingsStore.h
    src/core/SettingsStore.cpp
    src/core/SecretStore.h
    src/core/SecretStore.cpp
    src/core/JsonSchema.h
    src/core/JsonSchema.cpp
    src/core/Logger.h
    src/core/Logger.cpp
)
target_include_directories(railshot_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(railshot_core PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Network
)

# ── Platform (Windows) ──────────────────────────────────────────────────────
add_library(railshot_platform STATIC
    src/platform/windows/WinCrashHandler.h
    src/platform/windows/WinCrashHandler.cpp
    src/platform/windows/DxgiHelpers.h
    src/platform/windows/DxgiHelpers.cpp
    src/platform/windows/ComInitializer.h
    src/platform/windows/ComInitializer.cpp
)
target_include_directories(railshot_platform PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(railshot_platform PUBLIC
    railshot_core
    d3d11
    dxgi
    dxguid
    mf
    mfplat
    mfreadwrite
    mfuuid
    wmcodecdspuuid
    ole32
    oleaut32
    uuid
    winmm
    ws2_32
    crypt32
    advapi32
)

# ── Capture ─────────────────────────────────────────────────────────────────
add_library(railshot_capture STATIC
    src/capture/IVideoSource.h
    src/capture/FrameBus.h
    src/capture/FrameBus.cpp
    src/capture/MediaFoundationCamera.h
    src/capture/MediaFoundationCamera.cpp
    src/capture/WindowsGraphicsCapture.h
    src/capture/WindowsGraphicsCapture.cpp
    src/capture/DesktopDuplicationCapture.h
    src/capture/DesktopDuplicationCapture.cpp
    src/capture/WindowEnumerator.h
    src/capture/WindowEnumerator.cpp
    src/capture/ImageSource.h
    src/capture/ImageSource.cpp
    src/capture/TextSource.h
    src/capture/TextSource.cpp
    src/capture/ColorSource.h
    src/capture/ColorSource.cpp
    src/capture/MediaSource.h
    src/capture/MediaSource.cpp
    src/capture/NdiSource.h
    src/capture/NdiSource.cpp
    src/capture/OverlayRenderer.h
    src/capture/OverlayRenderer.cpp
    src/capture/OverlaySource.h
    src/capture/OverlaySource.cpp
    src/capture/BrowserSource.h
    src/capture/BrowserSource.cpp
    src/capture/AudioSource.h
    src/capture/AudioSource.cpp
    src/browser/BrowserIpc.h
    src/capture/CaptureManager.h
    src/capture/CaptureManager.cpp
)
target_link_libraries(railshot_capture PUBLIC
    railshot_platform
    Qt6::Gui
    Qt6::Network
)
if(WIN32)
    target_link_libraries(railshot_capture PUBLIC
        windowsapp
        dwmapi
        gdi32
        user32
    )
endif()
railshot_link_ffmpeg(railshot_capture)

# ── Compositor ──────────────────────────────────────────────────────────────
add_library(railshot_compositor STATIC
    src/compositor/D3D11Device.h
    src/compositor/D3D11Device.cpp
    src/compositor/D3D11Compositor.h
    src/compositor/D3D11Compositor.cpp
    src/compositor/PreviewSurface.h
    src/compositor/PreviewSurface.cpp
    src/compositor/TransitionEngine.h
    src/compositor/TransitionEngine.cpp
    src/compositor/Shaders.h
)
target_link_libraries(railshot_compositor PUBLIC
    railshot_capture
    Qt6::Widgets
)

# ── Audio ───────────────────────────────────────────────────────────────────
add_library(railshot_audio STATIC
    src/audio/AudioTypes.h
    src/audio/WasapiCapture.h
    src/audio/WasapiCapture.cpp
    src/audio/AudioGraph.h
    src/audio/AudioGraph.cpp
    src/audio/AudioMeter.h
    src/audio/AudioMeter.cpp
    src/audio/AudioClock.h
    src/audio/AudioClock.cpp
    src/audio/AudioMonitor.h
    src/audio/AudioMonitor.cpp
)
target_link_libraries(railshot_audio PUBLIC
    railshot_platform
)
# Capture uses AudioGraph for AudioInput/Output sources (declared after audio target).
target_link_libraries(railshot_capture PUBLIC railshot_audio)

# ── Encoding / Streaming / Recording ────────────────────────────────────────
add_library(railshot_encoding STATIC
    src/encoding/IEncoder.h
    src/encoding/MfH264Encoder.h
    src/encoding/MfH264Encoder.cpp
    src/encoding/SoftwareEncoder.h
    src/encoding/SoftwareEncoder.cpp
    src/encoding/EncoderFactory.h
    src/encoding/EncoderFactory.cpp
    src/encoding/AacEncoder.h
    src/encoding/AacEncoder.cpp
)
target_link_libraries(railshot_encoding PUBLIC
    railshot_platform
    railshot_audio
)

add_library(railshot_streaming STATIC
    src/streaming/RtmpOutput.h
    src/streaming/RtmpOutput.cpp
    src/streaming/StreamSession.h
    src/streaming/StreamSession.cpp
)
target_link_libraries(railshot_streaming PUBLIC
    railshot_encoding
)

add_library(railshot_recording STATIC
    src/recording/MkvRecorder.h
    src/recording/MkvRecorder.cpp
    src/recording/RecordingSession.h
    src/recording/RecordingSession.cpp
    src/recording/IsoRecordManager.h
    src/recording/IsoRecordManager.cpp
)
target_link_libraries(railshot_recording PUBLIC
    railshot_encoding
)

add_library(railshot_output STATIC
    src/output/OutputHub.h
    src/output/OutputHub.cpp
)
target_link_libraries(railshot_output PUBLIC
    railshot_encoding
    railshot_recording
    railshot_streaming
    railshot_core
    Qt6::Gui
)

# ── Telemetry ───────────────────────────────────────────────────────────────
add_library(railshot_telemetry STATIC
    src/telemetry/HealthTelemetry.h
    src/telemetry/HealthTelemetry.cpp
)
target_link_libraries(railshot_telemetry PUBLIC
    railshot_core
)

# ── Engine controller (wires all subsystems) ────────────────────────────────
add_library(railshot_engine STATIC
    src/core/EngineController.h
    src/core/EngineController.cpp
)
target_link_libraries(railshot_engine PUBLIC
    railshot_core
    railshot_capture
    railshot_compositor
    railshot_audio
    railshot_encoding
    railshot_streaming
    railshot_recording
    railshot_output
    railshot_telemetry
)
railshot_link_ffmpeg(railshot_encoding)
railshot_link_ffmpeg(railshot_streaming)
railshot_link_ffmpeg(railshot_recording)
railshot_link_ffmpeg(railshot_output)

# ── Creator features ────────────────────────────────────────────────────────
add_library(railshot_scoreboard STATIC
    src/scoreboard/ScoreboardModel.h
    src/scoreboard/ScoreboardModel.cpp
)
target_link_libraries(railshot_scoreboard PUBLIC railshot_core)

add_library(railshot_schedule STATIC
    src/schedule/ScheduleModel.h
    src/schedule/ScheduleModel.cpp
)
target_link_libraries(railshot_schedule PUBLIC railshot_core)

add_library(railshot_chat STATIC
    src/chat/ChatService.h
    src/chat/ChatService.cpp
    src/chat/PlatformAdapters.h
    src/chat/PlatformAdapters.cpp
)
target_link_libraries(railshot_chat PUBLIC railshot_core Qt6::Network)

add_library(railshot_overlays STATIC
    src/overlays/ReplayBuffer.h
    src/overlays/ReplayBuffer.cpp
    src/overlays/VirtualCamera.h
    src/overlays/VirtualCamera.cpp
)
target_link_libraries(railshot_overlays PUBLIC
    railshot_encoding
    railshot_recording
    railshot_platform
    Qt6::Gui
)
railshot_link_ffmpeg(railshot_overlays)

# Engine needs creator services after their libraries exist
target_link_libraries(railshot_engine PUBLIC
    railshot_scoreboard
    railshot_chat
    railshot_overlays
)

# ── UI ──────────────────────────────────────────────────────────────────────
add_library(railshot_ui STATIC
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
    src/ui/Theme.h
    src/ui/Theme.cpp
    src/ui/Motion.h
    src/ui/Motion.cpp
    src/ui/widgets/PreviewWidget.h
    src/ui/widgets/PreviewWidget.cpp
    src/ui/widgets/SceneListWidget.h
    src/ui/widgets/SceneListWidget.cpp
    src/ui/widgets/InputTilesWidget.h
    src/ui/widgets/InputTilesWidget.cpp
    src/ui/widgets/AudioMixerWidget.h
    src/ui/widgets/AudioMixerWidget.cpp
    src/ui/widgets/ScoreboardControlsWidget.h
    src/ui/widgets/ScoreboardControlsWidget.cpp
    src/ui/widgets/TransitionPanel.h
    src/ui/widgets/TransitionPanel.cpp
    src/ui/widgets/TopMenuBar.h
    src/ui/widgets/TopMenuBar.cpp
    src/ui/widgets/BottomToolbar.h
    src/ui/widgets/BottomToolbar.cpp
    src/ui/widgets/GoLiveDialog.h
    src/ui/widgets/GoLiveDialog.cpp
    src/ui/widgets/SidebarRail.h
    src/ui/widgets/SidebarRail.cpp
    src/ui/widgets/SourcePropertiesWidget.h
    src/ui/widgets/SourcePropertiesWidget.cpp
    src/ui/widgets/SourcePropertiesDialog.h
    src/ui/widgets/SourcePropertiesDialog.cpp
    src/ui/widgets/AddSourceDialog.h
    src/ui/widgets/AddSourceDialog.cpp
    src/ui/widgets/properties/SourcePropertiesPanel.h
    src/ui/widgets/properties/SourcePropertiesPanel.cpp
    src/ui/widgets/StreamStatusWidget.h
    src/ui/widgets/StreamStatusWidget.cpp
    src/ui/widgets/OverlayLibraryWidget.h
    src/ui/widgets/OverlayLibraryWidget.cpp
    src/ui/widgets/MultiCorderPanel.h
    src/ui/widgets/MultiCorderPanel.cpp
    src/ui/widgets/PlayListPanel.h
    src/ui/widgets/PlayListPanel.cpp
    src/ui/widgets/ShortcutsOverlay.h
    src/ui/widgets/ShortcutsOverlay.cpp
    src/ui/HotkeyDispatcher.h
    src/ui/HotkeyDispatcher.cpp
    src/ui/pages/DashboardPage.h
    src/ui/pages/DashboardPage.cpp
    src/ui/pages/SettingsPage.h
    src/ui/pages/SettingsPage.cpp
    src/ui/pages/SchedulePage.h
    src/ui/pages/SchedulePage.cpp
    src/ui/pages/ChatPage.h
    src/ui/pages/ChatPage.cpp
    src/ui/pages/AnalyticsPage.h
    src/ui/pages/AnalyticsPage.cpp
    src/ui/pages/SceneEditorPage.h
    src/ui/pages/SceneEditorPage.cpp
    resources/styles/railshot.qss
    resources/railshot.qrc
)
target_link_libraries(railshot_ui PUBLIC
    railshot_engine
    railshot_scoreboard
    railshot_schedule
    railshot_chat
    railshot_overlays
    Qt6::Widgets
    Qt6::Svg
    Qt6::Concurrent
)

# ── Application executable ──────────────────────────────────────────────────
qt_add_executable(RailShotTV
    src/app/main.cpp
    src/app/Application.h
    src/app/Application.cpp
    resources/windows/railshot.rc
)

qt_add_executable(railshot_browser_helper
    src/browser/browser_helper_main.cpp
    src/browser/BrowserIpc.h
)
target_include_directories(railshot_browser_helper PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(railshot_browser_helper PRIVATE Qt6::Widgets Qt6::Network Qt6::Gui)
if(RAILSHOT_WEBVIEW2_FOUND)
    target_include_directories(railshot_browser_helper PRIVATE "${RAILSHOT_WEBVIEW2_ROOT}/include")
    target_compile_definitions(railshot_browser_helper PRIVATE RAILSHOT_HAS_WEBVIEW2=1)
    target_link_libraries(railshot_browser_helper PRIVATE
        "${RAILSHOT_WEBVIEW2_ROOT}/x64/WebView2LoaderStatic.lib"
        version
        shlwapi
    )
else()
    target_compile_definitions(railshot_browser_helper PRIVATE RAILSHOT_HAS_WEBVIEW2=0)
endif()
set_target_properties(railshot_browser_helper PROPERTIES
    WIN32_EXECUTABLE ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Win11 MF virtual camera media source (consumes Local\RailShotTV_VirtualCamera)
add_library(railshot_vcam SHARED
    src/vcam/railshot_vcam.cpp
    src/vcam/railshot_vcam.def
    src/vcam/VCamIpc.h
)
target_include_directories(railshot_vcam PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(railshot_vcam PRIVATE mfplat mfuuid ole32 oleaut32 advapi32)
set_target_properties(railshot_vcam PROPERTIES
    OUTPUT_NAME "railshot_vcam"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

target_link_libraries(RailShotTV PRIVATE
    railshot_ui
    railshot_platform
)
add_dependencies(RailShotTV railshot_browser_helper railshot_vcam)
add_custom_command(TARGET RailShotTV POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:railshot_browser_helper>
            $<TARGET_FILE_DIR:RailShotTV>/railshot_browser_helper.exe
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:railshot_vcam>
            $<TARGET_FILE_DIR:RailShotTV>/railshot_vcam.dll
    COMMENT "Copy browser helper + vcam DLL next to RailShotTV"
)

set_target_properties(RailShotTV PROPERTIES
    WIN32_EXECUTABLE ON
    OUTPUT_NAME "RailShotTV"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_compile_definitions(RailShotTV PRIVATE
    RAILSHOT_VERSION="${PROJECT_VERSION}"
    RAILSHOT_APP_NAME="RailShotTV"
)

if(MSVC)
    target_compile_options(RailShotTV PRIVATE /W4 /permissive- /Zc:__cplusplus)
    target_compile_definitions(RailShotTV PRIVATE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
endif()

# Deployment helper
if(Qt6_FOUND)
    qt_generate_deploy_app_script(
        TARGET RailShotTV
        OUTPUT_SCRIPT railshot_deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
endif()

# ── Tests ───────────────────────────────────────────────────────────────────
if(RAILSHOT_BUILD_TESTS)
    enable_testing()
    find_package(Qt6 REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
endif()

# ── Packaging ───────────────────────────────────────────────────────────────
include(RailShotPackaging)

message(STATUS "RailShotTV ${PROJECT_VERSION} configured")
message(STATUS "  Tests: ${RAILSHOT_BUILD_TESTS}")
message(STATUS "  FFmpeg: ${RAILSHOT_ENABLE_FFMPEG}")
message(STATUS "  HW Encoders: ${RAILSHOT_ENABLE_HW_ENCODERS}")
