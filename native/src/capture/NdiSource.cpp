#include "capture/NdiSource.h"
#include "core/Logger.h"
#include <QLibrary>
#include <algorithm>
#include <cstring>

#ifdef _WIN32
#include <d3d11.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;
#endif

namespace railshot {
namespace {

// Minimal NDI C API surface (matches NewTek NDI SDK naming).
enum NDIlib_frame_type_e {
    NDIlib_frame_type_none = 0,
    NDIlib_frame_type_video = 1,
    NDIlib_frame_type_audio = 2,
    NDIlib_frame_type_metadata = 3,
    NDIlib_frame_type_error = 4,
    NDIlib_frame_type_status_change = 100
};

enum NDIlib_FourCC_video_type_e {
    NDIlib_FourCC_type_UYVY = 0x59565955,
    NDIlib_FourCC_type_BGRA = 0x41524742,
    NDIlib_FourCC_type_BGRX = 0x58524742,
    NDIlib_FourCC_type_RGBA = 0x41424752,
    NDIlib_FourCC_type_RGBX = 0x58424752
};

struct NDIlib_source_t {
    const char* p_ndi_name;
    const char* p_url_address;
};

struct NDIlib_find_create_t {
    bool show_local_sources;
    const char* p_groups;
    const char* p_extra_ips;
};

struct NDIlib_recv_create_v3_t {
    NDIlib_source_t source_to_connect_to;
    int color_format; // NDIlib_recv_color_format_e
    int bandwidth;
    bool allow_video_fields;
    const char* p_ndi_recv_name;
};

struct NDIlib_video_frame_v2_t {
    int xres, yres;
    int FourCC;
    int frame_rate_N, frame_rate_D;
    float picture_aspect_ratio;
    int frame_format_type;
    int64_t timecode;
    uint8_t* p_data;
    int line_stride_in_bytes;
    const char* p_metadata;
    int64_t timestamp;
};

struct NDIlib_audio_frame_v2_t {
    int sample_rate;
    int no_channels;
    int no_samples;
    int64_t timecode;
    float* p_data;
    int channel_stride_in_bytes;
    const char* p_metadata;
    int64_t timestamp;
};

using NDIlib_initialize_fn = bool (*)();
using NDIlib_destroy_fn = void (*)();
using NDIlib_find_create_v2_fn = void* (*)(const NDIlib_find_create_t*);
using NDIlib_find_destroy_fn = void (*)(void*);
using NDIlib_find_wait_for_sources_fn = bool (*)(void*, uint32_t);
using NDIlib_find_get_current_sources_fn = const NDIlib_source_t* (*)(void*, uint32_t*);
using NDIlib_recv_create_v3_fn = void* (*)(const NDIlib_recv_create_v3_t*);
using NDIlib_recv_destroy_fn = void (*)(void*);
using NDIlib_recv_connect_fn = void (*)(void*, const NDIlib_source_t*);
using NDIlib_recv_capture_v2_fn = NDIlib_frame_type_e (*)(void*, NDIlib_video_frame_v2_t*, NDIlib_audio_frame_v2_t*, void*, uint32_t);
using NDIlib_recv_free_video_v2_fn = void (*)(void*, const NDIlib_video_frame_v2_t*);
using NDIlib_recv_free_audio_v2_fn = void (*)(void*, const NDIlib_audio_frame_v2_t*);

struct NdiApi {
    QLibrary lib;
    NDIlib_initialize_fn initialize = nullptr;
    NDIlib_destroy_fn destroy = nullptr;
    NDIlib_find_create_v2_fn find_create = nullptr;
    NDIlib_find_destroy_fn find_destroy = nullptr;
    NDIlib_find_wait_for_sources_fn find_wait = nullptr;
    NDIlib_find_get_current_sources_fn find_sources = nullptr;
    NDIlib_recv_create_v3_fn recv_create = nullptr;
    NDIlib_recv_destroy_fn recv_destroy = nullptr;
    NDIlib_recv_connect_fn recv_connect = nullptr;
    NDIlib_recv_capture_v2_fn recv_capture = nullptr;
    NDIlib_recv_free_video_v2_fn recv_free_video = nullptr;
    NDIlib_recv_free_audio_v2_fn recv_free_audio = nullptr;
    bool ok = false;
};

NdiApi& ndiApi()
{
    static NdiApi api;
    if (api.ok || api.lib.isLoaded()) return api;
    const QStringList names = {
        QStringLiteral("Processing.NDI.Lib.x64.dll"),
        QStringLiteral("Processing.NDI.Lib.x86.dll"),
        QStringLiteral("libndi.so.5"),
        QStringLiteral("libndi.dylib")
    };
    for (const auto& n : names) {
        api.lib.setFileName(n);
        if (api.lib.load()) break;
    }
    if (!api.lib.isLoaded()) return api;
    api.initialize = reinterpret_cast<NDIlib_initialize_fn>(api.lib.resolve("NDIlib_initialize"));
    api.destroy = reinterpret_cast<NDIlib_destroy_fn>(api.lib.resolve("NDIlib_destroy"));
    api.find_create = reinterpret_cast<NDIlib_find_create_v2_fn>(api.lib.resolve("NDIlib_find_create_v2"));
    api.find_destroy = reinterpret_cast<NDIlib_find_destroy_fn>(api.lib.resolve("NDIlib_find_destroy"));
    api.find_wait = reinterpret_cast<NDIlib_find_wait_for_sources_fn>(api.lib.resolve("NDIlib_find_wait_for_sources"));
    api.find_sources = reinterpret_cast<NDIlib_find_get_current_sources_fn>(api.lib.resolve("NDIlib_find_get_current_sources"));
    api.recv_create = reinterpret_cast<NDIlib_recv_create_v3_fn>(api.lib.resolve("NDIlib_recv_create_v3"));
    api.recv_destroy = reinterpret_cast<NDIlib_recv_destroy_fn>(api.lib.resolve("NDIlib_recv_destroy"));
    api.recv_connect = reinterpret_cast<NDIlib_recv_connect_fn>(api.lib.resolve("NDIlib_recv_connect"));
    api.recv_capture = reinterpret_cast<NDIlib_recv_capture_v2_fn>(api.lib.resolve("NDIlib_recv_capture_v2"));
    api.recv_free_video = reinterpret_cast<NDIlib_recv_free_video_v2_fn>(api.lib.resolve("NDIlib_recv_free_video_v2"));
    api.recv_free_audio = reinterpret_cast<NDIlib_recv_free_audio_v2_fn>(api.lib.resolve("NDIlib_recv_free_audio_v2"));
    if (api.initialize && api.find_create && api.recv_create && api.recv_capture) {
        api.ok = api.initialize();
    }
    return api;
}

} // namespace

NdiSource::NdiSource(QString id, QString name, QString sourceName)
    : m_id(std::move(id)), m_name(std::move(name)), m_sourceName(std::move(sourceName))
{
}

NdiSource::~NdiSource() { stop(); }

void NdiSource::setAudioCallback(std::function<void(const AudioBuffer&)> cb)
{
    std::lock_guard lock(m_audioCbMutex);
    m_audioCb = std::move(cb);
}

QStringList NdiSource::discoverSources(int waitMs)
{
    QStringList out;
    auto& api = ndiApi();
    if (!api.ok) return out;
    NDIlib_find_create_t cfg{};
    cfg.show_local_sources = true;
    void* finder = api.find_create(&cfg);
    if (!finder) return out;
    api.find_wait(finder, static_cast<uint32_t>(waitMs));
    uint32_t n = 0;
    const NDIlib_source_t* sources = api.find_sources(finder, &n);
    for (uint32_t i = 0; i < n; ++i) {
        if (sources[i].p_ndi_name)
            out.push_back(QString::fromUtf8(sources[i].p_ndi_name));
    }
    api.find_destroy(finder);
    return out;
}

bool NdiSource::uploadBgra(const uint8_t* bgra, int stride, int w, int h)
{
#ifdef _WIN32
    if (!m_device || !bgra) return false;
    std::lock_guard lock(m_mutex);
    if (!m_texture || m_width != w || m_height != h) {
        if (m_texture) { m_texture->Release(); m_texture = nullptr; }
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = static_cast<UINT>(w);
        desc.Height = static_cast<UINT>(h);
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        ComPtr<ID3D11Texture2D> tex;
        if (FAILED(m_device->CreateTexture2D(&desc, nullptr, &tex))) return false;
        m_texture = tex.Detach();
        m_width = w;
        m_height = h;
        if (!m_context) m_device->GetImmediateContext(&m_context);
    }
    if (!m_context) return false;
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(m_context->Map(m_texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) return false;
    for (int y = 0; y < h; ++y)
        std::memcpy(static_cast<uint8_t*>(mapped.pData) + y * mapped.RowPitch, bgra + y * stride, size_t(w) * 4);
    m_context->Unmap(m_texture, 0);
    return true;
#else
    Q_UNUSED(bgra); Q_UNUSED(stride); Q_UNUSED(w); Q_UNUSED(h);
    return false;
#endif
}

void NdiSource::receiveLoop()
{
    auto& api = ndiApi();
    if (!api.ok) {
        m_running = false;
        return;
    }
    NDIlib_find_create_t fcfg{};
    fcfg.show_local_sources = true;
    void* finder = api.find_create(&fcfg);
    if (!finder) {
        m_running = false;
        return;
    }
    api.find_wait(finder, 2000);
    uint32_t n = 0;
    const NDIlib_source_t* sources = api.find_sources(finder, &n);
    NDIlib_source_t chosen{};
    bool found = false;
    for (uint32_t i = 0; i < n; ++i) {
        const QString name = QString::fromUtf8(sources[i].p_ndi_name ? sources[i].p_ndi_name : "");
        if (m_sourceName.isEmpty() || name == m_sourceName || name.contains(m_sourceName)) {
            chosen = sources[i];
            found = true;
            break;
        }
    }
    api.find_destroy(finder);
    if (!found) {
        Logger::warn(QStringLiteral("NDI source not found: %1").arg(m_sourceName));
        m_running = false;
        return;
    }

    NDIlib_recv_create_v3_t rcfg{};
    rcfg.source_to_connect_to = chosen;
    rcfg.color_format = 1; // BGRA
    rcfg.bandwidth = 100;  // highest
    rcfg.allow_video_fields = false;
    rcfg.p_ndi_recv_name = "RailShotTV";
    void* recv = api.recv_create(&rcfg);
    if (!recv) {
        m_running = false;
        return;
    }
    if (api.recv_connect)
        api.recv_connect(recv, &chosen);

    while (!m_stop.load()) {
        NDIlib_video_frame_v2_t video{};
        NDIlib_audio_frame_v2_t audio{};
        const auto type = api.recv_capture(recv, &video, &audio, nullptr, 100);
        if (type == NDIlib_frame_type_video && video.p_data) {
            if (video.FourCC == NDIlib_FourCC_type_BGRA || video.FourCC == NDIlib_FourCC_type_BGRX)
                uploadBgra(video.p_data, video.line_stride_in_bytes, video.xres, video.yres);
            api.recv_free_video(recv, &video);
        } else if (type == NDIlib_frame_type_audio && audio.p_data && audio.no_samples > 0) {
            AudioBuffer buf;
            buf.sampleRate = audio.sample_rate > 0 ? audio.sample_rate : kAudioSampleRate;
            buf.channels = (std::max)(1, audio.no_channels);
            buf.samples.resize(size_t(audio.no_samples) * size_t(buf.channels));
            // NDI audio is planar float; interleave for the mixer.
            for (int c = 0; c < buf.channels; ++c) {
                const float* plane = reinterpret_cast<const float*>(
                    reinterpret_cast<const uint8_t*>(audio.p_data) + c * audio.channel_stride_in_bytes);
                for (int i = 0; i < audio.no_samples; ++i)
                    buf.samples[size_t(i) * size_t(buf.channels) + size_t(c)] = plane[i];
            }
            std::function<void(const AudioBuffer&)> cb;
            {
                std::lock_guard lock(m_audioCbMutex);
                cb = m_audioCb;
            }
            if (cb) cb(buf);
            if (api.recv_free_audio)
                api.recv_free_audio(recv, &audio);
        }
    }
    api.recv_destroy(recv);
    m_running = false;
}

bool NdiSource::start(ID3D11Device* device, QString* error)
{
    m_device = device;
    auto& api = ndiApi();
    if (!api.ok) {
        if (error)
            *error = QStringLiteral("NDI runtime not found. Install NDI Tools / Runtime, then retry.");
        return false;
    }
    m_stop = false;
    m_running = true;
    m_thread = std::thread([this] { receiveLoop(); });
    Logger::info(QStringLiteral("NDI receive started: %1").arg(m_sourceName));
    return true;
}

void NdiSource::stop()
{
    m_stop = true;
    if (m_thread.joinable())
        m_thread.join();
    m_running = false;
    std::lock_guard lock(m_mutex);
#ifdef _WIN32
    if (m_texture) { m_texture->Release(); m_texture = nullptr; }
    if (m_context) { m_context->Release(); m_context = nullptr; }
#endif
}

bool NdiSource::acquireLatest(VideoFrame& out)
{
    std::lock_guard lock(m_mutex);
    if (!m_texture) return false;
    out.texture = m_texture;
    out.width = m_width;
    out.height = m_height;
    out.sourceId = m_id;
    out.opaque = true;
    return true;
}

} // namespace railshot
