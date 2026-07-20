#include "capture/MediaFoundationCamera.h"
#include "core/Logger.h"
#include "platform/windows/ComInitializer.h"
#include <QElapsedTimer>
#include <QVector>
#include <QPair>
#include <cstring>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <d3d11.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;
#endif

namespace railshot {

MediaFoundationCamera::MediaFoundationCamera(QString id, QString name, QString deviceId)
    : m_id(std::move(id))
    , m_name(std::move(name))
    , m_deviceId(std::move(deviceId))
{
}

MediaFoundationCamera::~MediaFoundationCamera()
{
    stop();
}

QVector<QPair<QString, QString>> MediaFoundationCamera::enumerateDevices()
{
    QVector<QPair<QString, QString>> devices;
#ifdef _WIN32
    ComInitializer com;
    if (!com.ok()) {
        devices.append({QStringLiteral("default"), QStringLiteral("Default Camera")});
        return devices;
    }

    // MFEnumDeviceSources requires an active MFStartup ref — without it this AVs on many systems.
    const HRESULT startupHr = MFStartup(MF_VERSION);
    if (FAILED(startupHr)) {
        Logger::warn(QStringLiteral("MFStartup failed during camera enumerate: 0x%1")
                         .arg(quint32(startupHr), 8, 16, QChar('0')));
        devices.append({QStringLiteral("default"), QStringLiteral("Default Camera")});
        return devices;
    }

    IMFAttributes* attrs = nullptr;
    if (SUCCEEDED(MFCreateAttributes(&attrs, 1)) && attrs) {
        attrs->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
        IMFActivate** activates = nullptr;
        UINT32 count = 0;
        if (SUCCEEDED(MFEnumDeviceSources(attrs, &activates, &count)) && activates) {
            for (UINT32 i = 0; i < count; ++i) {
                if (!activates[i]) continue;
                WCHAR* name = nullptr;
                UINT32 nameLen = 0;
                WCHAR* symlink = nullptr;
                UINT32 symlinkLen = 0;
                activates[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &name, &nameLen);
                activates[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK,
                                                 &symlink, &symlinkLen);
                devices.append({QString::fromWCharArray(symlink ? symlink : L""),
                                QString::fromWCharArray(name ? name : L"Camera")});
                if (name) CoTaskMemFree(name);
                if (symlink) CoTaskMemFree(symlink);
                activates[i]->Release();
            }
            CoTaskMemFree(activates);
        }
        attrs->Release();
    }

    MFShutdown();
#endif
    if (devices.isEmpty())
        devices.append({QStringLiteral("default"), QStringLiteral("Default Camera")});
    return devices;
}

bool MediaFoundationCamera::start(ID3D11Device* device, QString* error)
{
    if (m_running.load()) return true;
    m_device = device;
#ifdef _WIN32
    ComInitializer com;
    if (!com.ok()) {
        if (error) *error = QStringLiteral("COM init failed");
        return false;
    }
    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) {
        if (error) *error = QStringLiteral("MFStartup failed");
        return false;
    }

    if (m_device) {
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = static_cast<UINT>(m_width);
        desc.Height = static_cast<UINT>(m_height);
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        desc.CPUAccessFlags = 0;
        ComPtr<ID3D11Texture2D> tex;
        if (SUCCEEDED(m_device->CreateTexture2D(&desc, nullptr, &tex))) {
            std::lock_guard lock(m_frameMutex);
            if (m_texture) m_texture->Release();
            m_texture = tex.Detach();
        }
    }

    m_running = true;
    m_thread = std::thread([this] { captureLoop(); });
    Logger::info(QStringLiteral("Camera started: %1").arg(m_name));
    return true;
#else
    Q_UNUSED(error);
    return false;
#endif
}

void MediaFoundationCamera::stop()
{
    if (!m_running.exchange(false)) return;
    if (m_thread.joinable()) m_thread.join();
#ifdef _WIN32
    {
        std::lock_guard lock(m_frameMutex);
        if (m_texture) { m_texture->Release(); m_texture = nullptr; }
    }
    MFShutdown();
#endif
    Logger::info(QStringLiteral("Camera stopped: %1").arg(m_name));
}

bool MediaFoundationCamera::acquireLatest(VideoFrame& out)
{
    std::lock_guard lock(m_frameMutex);
    if (!m_texture) return false;
    out.texture = m_texture;
    out.ptsUs = m_ptsUs;
    out.width = m_width;
    out.height = m_height;
    out.sourceId = m_id;
    return true;
}

void MediaFoundationCamera::captureLoop()
{
#ifdef _WIN32
    ComInitializer com;
    QElapsedTimer timer;
    timer.start();

    ComPtr<IMFAttributes> attrs;
    ComPtr<IMFMediaSource> source;
    ComPtr<IMFSourceReader> reader;

    MFCreateAttributes(&attrs, 2);
    if (attrs) {
        attrs->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
        attrs->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);
        if (!m_deviceId.isEmpty() && m_deviceId != QLatin1String("default")) {
            const std::wstring link = m_deviceId.toStdWString();
            attrs->SetString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, link.c_str());
        }
    }

    IMFActivate** activates = nullptr;
    UINT32 count = 0;
    ComPtr<IMFActivate> activate;
    if (attrs && SUCCEEDED(MFEnumDeviceSources(attrs.Get(), &activates, &count)) && count > 0) {
        // Prefer matching symbolic link when provided.
        UINT32 chosen = 0;
        if (!m_deviceId.isEmpty() && m_deviceId != QLatin1String("default")) {
            for (UINT32 i = 0; i < count; ++i) {
                WCHAR* symlink = nullptr;
                UINT32 symlinkLen = 0;
                if (SUCCEEDED(activates[i]->GetAllocatedString(
                        MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, &symlink, &symlinkLen))) {
                    if (QString::fromWCharArray(symlink) == m_deviceId)
                        chosen = i;
                    CoTaskMemFree(symlink);
                }
            }
        }
        activate = activates[chosen];
        for (UINT32 i = 0; i < count; ++i) {
            if (i != chosen)
                activates[i]->Release();
        }
        CoTaskMemFree(activates);
        activate->ActivateObject(IID_PPV_ARGS(&source));
        if (source)
            MFCreateSourceReaderFromMediaSource(source.Get(), attrs.Get(), &reader);
    }

    if (reader) {
        ComPtr<IMFMediaType> type;
        MFCreateMediaType(&type);
        type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
        type->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
        MFSetAttributeSize(type.Get(), MF_MT_FRAME_SIZE, static_cast<UINT32>(m_width), static_cast<UINT32>(m_height));
        reader->SetCurrentMediaType(static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM), nullptr, type.Get());

        // Read back actual negotiated size.
        ComPtr<IMFMediaType> current;
        if (SUCCEEDED(reader->GetCurrentMediaType(static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM), &current))) {
            UINT32 w = 0, h = 0;
            if (SUCCEEDED(MFGetAttributeSize(current.Get(), MF_MT_FRAME_SIZE, &w, &h)) && w > 0 && h > 0) {
                m_width = static_cast<int>(w);
                m_height = static_cast<int>(h);
                // Recreate texture at negotiated size.
                if (m_device) {
                    D3D11_TEXTURE2D_DESC desc{};
                    desc.Width = w;
                    desc.Height = h;
                    desc.MipLevels = 1;
                    desc.ArraySize = 1;
                    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
                    desc.SampleDesc.Count = 1;
                    desc.Usage = D3D11_USAGE_DEFAULT;
                    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
                    ComPtr<ID3D11Texture2D> tex;
                    if (SUCCEEDED(m_device->CreateTexture2D(&desc, nullptr, &tex))) {
                        std::lock_guard lock(m_frameMutex);
                        if (m_texture) m_texture->Release();
                        m_texture = tex.Detach();
                    }
                }
            }
        }
    }

    ComPtr<ID3D11DeviceContext> ctx;
    if (m_device)
        m_device->GetImmediateContext(&ctx);

    while (m_running.load()) {
        m_ptsUs = timer.nsecsElapsed() / 1000;
        if (reader) {
            DWORD streamIndex = 0, flags = 0;
            LONGLONG ts = 0;
            ComPtr<IMFSample> sample;
            const HRESULT hr = reader->ReadSample(
                static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM), 0,
                &streamIndex, &flags, &ts, &sample);
            if (SUCCEEDED(hr) && sample && m_device && m_texture && ctx) {
                ComPtr<IMFMediaBuffer> buffer;
                if (SUCCEEDED(sample->ConvertToContiguousBuffer(&buffer)) && buffer) {
                    BYTE* data = nullptr;
                    DWORD maxLen = 0, curLen = 0;
                    if (SUCCEEDED(buffer->Lock(&data, &maxLen, &curLen)) && data && curLen > 0) {
                        const UINT rowPitch = static_cast<UINT>(m_width) * 4;
                        // RGB32 from MF is typically BGRA tightly packed; copy into GPU texture.
                        ctx->UpdateSubresource(m_texture, 0, nullptr, data, rowPitch, 0);
                        buffer->Unlock();
                        std::lock_guard lock(m_frameMutex);
                        m_ptsUs = ts / 10; // 100-ns → us
                    }
                }
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
#else
    while (m_running.load())
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
#endif
}

} // namespace railshot
