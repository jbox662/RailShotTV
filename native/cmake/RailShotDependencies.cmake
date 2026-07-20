# RailShotDependencies.cmake
# Resolves optional third-party dependencies (FFmpeg LGPL, hardware encoder SDKs).

include_guard(GLOBAL)

set(RAILSHOT_FFMPEG_FOUND FALSE)
set(RAILSHOT_NVENC_FOUND FALSE)
set(RAILSHOT_AMF_FOUND FALSE)
set(RAILSHOT_ONEVPL_FOUND FALSE)

if(RAILSHOT_ENABLE_FFMPEG)
    # Prefer an explicit RAILSHOT_FFMPEG_ROOT, then env, then vcpkg, then system.
    if(DEFINED RAILSHOT_FFMPEG_ROOT AND NOT "${RAILSHOT_FFMPEG_ROOT}" STREQUAL "")
        set(FFMPEG_ROOT "${RAILSHOT_FFMPEG_ROOT}")
    elseif(DEFINED ENV{RAILSHOT_FFMPEG_ROOT})
        set(FFMPEG_ROOT "$ENV{RAILSHOT_FFMPEG_ROOT}")
    elseif(DEFINED ENV{VCPKG_ROOT})
        set(FFMPEG_ROOT "$ENV{VCPKG_ROOT}/installed/x64-windows")
    endif()

    find_path(FFMPEG_INCLUDE_DIR
        NAMES libavcodec/avcodec.h
        HINTS ${FFMPEG_ROOT}/include
        PATH_SUFFIXES ffmpeg
    )
    find_library(AVCODEC_LIBRARY NAMES avcodec HINTS ${FFMPEG_ROOT}/lib)
    find_library(AVFORMAT_LIBRARY NAMES avformat HINTS ${FFMPEG_ROOT}/lib)
    find_library(AVUTIL_LIBRARY NAMES avutil HINTS ${FFMPEG_ROOT}/lib)
    find_library(SWRESAMPLE_LIBRARY NAMES swresample HINTS ${FFMPEG_ROOT}/lib)
    find_library(SWSCALE_LIBRARY NAMES swscale HINTS ${FFMPEG_ROOT}/lib)
    find_library(AVFILTER_LIBRARY NAMES avfilter HINTS ${FFMPEG_ROOT}/lib)

    if(FFMPEG_INCLUDE_DIR AND AVCODEC_LIBRARY AND AVFORMAT_LIBRARY AND AVUTIL_LIBRARY)
        set(RAILSHOT_FFMPEG_FOUND TRUE)
        add_library(railshot_ffmpeg INTERFACE)
        target_include_directories(railshot_ffmpeg INTERFACE ${FFMPEG_INCLUDE_DIR})
        target_link_libraries(railshot_ffmpeg INTERFACE
            ${AVCODEC_LIBRARY}
            ${AVFORMAT_LIBRARY}
            ${AVUTIL_LIBRARY}
            ${SWRESAMPLE_LIBRARY}
            ${SWSCALE_LIBRARY}
            ${AVFILTER_LIBRARY}
        )
        target_compile_definitions(railshot_ffmpeg INTERFACE RAILSHOT_HAS_FFMPEG=1)
        message(STATUS "FFmpeg (LGPL) found: ${FFMPEG_INCLUDE_DIR}")
    else()
        message(WARNING "FFmpeg not found — streaming/recording muxers will use stubs until FFmpeg is installed. Set RAILSHOT_FFMPEG_ROOT.")
        add_library(railshot_ffmpeg INTERFACE)
        target_compile_definitions(railshot_ffmpeg INTERFACE RAILSHOT_HAS_FFMPEG=0)
    endif()
else()
    add_library(railshot_ffmpeg INTERFACE)
    target_compile_definitions(railshot_ffmpeg INTERFACE RAILSHOT_HAS_FFMPEG=0)
endif()

if(RAILSHOT_ENABLE_HW_ENCODERS)
    # NVIDIA Video Codec SDK (optional)
    if(DEFINED ENV{NVENCODE_API_PATH})
        find_path(NVENC_INCLUDE_DIR
            NAMES nvEncodeAPI.h
            HINTS "$ENV{NVENCODE_API_PATH}/Interface"
        )
        if(NVENC_INCLUDE_DIR)
            set(RAILSHOT_NVENC_FOUND TRUE)
            message(STATUS "NVENC headers found: ${NVENC_INCLUDE_DIR}")
        endif()
    endif()

    # AMD AMF (optional)
    if(DEFINED ENV{AMF_HOME})
        find_path(AMF_INCLUDE_DIR
            NAMES components/VideoEncoderVCE.h
            HINTS "$ENV{AMF_HOME}/amf/public/include"
        )
        if(AMF_INCLUDE_DIR)
            set(RAILSHOT_AMF_FOUND TRUE)
            message(STATUS "AMF headers found: ${AMF_INCLUDE_DIR}")
        endif()
    endif()

    # Intel oneVPL (optional)
    find_package(VPL QUIET)
    if(VPL_FOUND)
        set(RAILSHOT_ONEVPL_FOUND TRUE)
        message(STATUS "Intel oneVPL found")
    endif()
endif()

# Propagate feature flags to all targets that link railshot_ffmpeg
function(railshot_link_ffmpeg target)
    target_link_libraries(${target} PUBLIC railshot_ffmpeg)
endfunction()

# ── WebView2 SDK (optional, for JS browser overlays) ────────────────────────
set(RAILSHOT_WEBVIEW2_FOUND FALSE)
if(NOT RAILSHOT_WEBVIEW2_ROOT AND DEFINED ENV{RAILSHOT_WEBVIEW2_ROOT})
    set(RAILSHOT_WEBVIEW2_ROOT "$ENV{RAILSHOT_WEBVIEW2_ROOT}")
endif()
if(NOT RAILSHOT_WEBVIEW2_ROOT)
    set(_wv2_default "${CMAKE_SOURCE_DIR}/third_party/webview2/pkg/build/native")
    if(EXISTS "${_wv2_default}/include/WebView2.h")
        set(RAILSHOT_WEBVIEW2_ROOT "${_wv2_default}")
    endif()
endif()
if(RAILSHOT_WEBVIEW2_ROOT AND EXISTS "${RAILSHOT_WEBVIEW2_ROOT}/include/WebView2.h")
    set(RAILSHOT_WEBVIEW2_FOUND TRUE)
    message(STATUS "WebView2 SDK found: ${RAILSHOT_WEBVIEW2_ROOT}")
else()
    message(STATUS "WebView2 SDK not found — browser helper will use QTextBrowser fallback. Run scripts/install-webview2.ps1")
endif()
