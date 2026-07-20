#include "audio/WasapiCapture.h"
#include "core/Logger.h"
#include "platform/windows/ComInitializer.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <functiondiscoverykeys_devpkey.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;
#endif

#include <QElapsedTimer>

namespace railshot {

WasapiCapture::WasapiCapture(AudioDeviceKind kind, QString deviceId, QObject* parent)
    : QObject(parent), m_kind(kind), m_deviceId(std::move(deviceId))
{
}

WasapiCapture::~WasapiCapture()
{
    stop();
}

void WasapiCapture::setCallback(std::function<void(const AudioBuffer&)> cb)
{
    std::lock_guard lock(m_cbMutex);
    m_callback = std::move(cb);
}

QVector<AudioDeviceInfo> WasapiCapture::enumerate(AudioDeviceKind kind)
{
    QVector<AudioDeviceInfo> out;
#ifdef _WIN32
    ComInitializer com;
    ComPtr<IMMDeviceEnumerator> enumerator;
    if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                 IID_PPV_ARGS(&enumerator))))
        return out;
    ComPtr<IMMDeviceCollection> collection;
    const EDataFlow flow = (kind == AudioDeviceKind::Loopback) ? eRender : eCapture;
    if (FAILED(enumerator->EnumAudioEndpoints(flow, DEVICE_STATE_ACTIVE, &collection)))
        return out;
    UINT count = 0;
    collection->GetCount(&count);
    for (UINT i = 0; i < count; ++i) {
        ComPtr<IMMDevice> device;
        collection->Item(i, &device);
        LPWSTR id = nullptr;
        device->GetId(&id);
        ComPtr<IPropertyStore> props;
        device->OpenPropertyStore(STGM_READ, &props);
        PROPVARIANT varName;
        PropVariantInit(&varName);
        props->GetValue(PKEY_Device_FriendlyName, &varName);
        AudioDeviceInfo info;
        info.id = QString::fromWCharArray(id);
        info.name = QString::fromWCharArray(varName.pwszVal);
        info.kind = kind;
        out.append(info);
        PropVariantClear(&varName);
        CoTaskMemFree(id);
    }
#endif
    if (out.isEmpty()) {
        AudioDeviceInfo fallback;
        fallback.id = QStringLiteral("default");
        fallback.name = (kind == AudioDeviceKind::Loopback)
                            ? QStringLiteral("Desktop Audio")
                            : QStringLiteral("Mic/Aux");
        fallback.kind = kind;
        out.append(fallback);
    }
    return out;
}

bool WasapiCapture::start(QString* error)
{
    if (m_running.load()) return true;
    m_running = true;
    m_thread = std::thread([this] { captureLoop(); });
    Logger::info(QStringLiteral("WASAPI %1 capture started")
                     .arg(m_kind == AudioDeviceKind::Loopback ? "loopback" : "mic"));
    Q_UNUSED(error);
    return true;
}

void WasapiCapture::stop()
{
    if (!m_running.exchange(false)) return;
    if (m_thread.joinable()) m_thread.join();
}

void WasapiCapture::captureLoop()
{
#ifdef _WIN32
    ComInitializer com;
    ComPtr<IMMDeviceEnumerator> enumerator;
    if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                 IID_PPV_ARGS(&enumerator)))) {
        emit captureError(QStringLiteral("MMDeviceEnumerator failed"));
        return;
    }

    ComPtr<IMMDevice> device;
    const EDataFlow flow = (m_kind == AudioDeviceKind::Loopback) ? eRender : eCapture;
    if (FAILED(enumerator->GetDefaultAudioEndpoint(flow, eConsole, &device))) {
        emit captureError(QStringLiteral("GetDefaultAudioEndpoint failed"));
        return;
    }

    ComPtr<IAudioClient> client;
    if (FAILED(device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                                 reinterpret_cast<void**>(client.GetAddressOf())))) {
        emit captureError(QStringLiteral("IAudioClient activate failed"));
        return;
    }

    WAVEFORMATEX* mix = nullptr;
    client->GetMixFormat(&mix);
    DWORD flags = 0;
    if (m_kind == AudioDeviceKind::Loopback)
        flags |= AUDCLNT_STREAMFLAGS_LOOPBACK;
    flags |= AUDCLNT_STREAMFLAGS_EVENTCALLBACK;

    const REFERENCE_TIME bufferDuration = 100000; // 10ms
    HRESULT hr = client->Initialize(AUDCLNT_SHAREMODE_SHARED, flags, bufferDuration, 0, mix, nullptr);
    if (FAILED(hr)) {
        emit captureError(QStringLiteral("IAudioClient::Initialize failed"));
        CoTaskMemFree(mix);
        return;
    }

    HANDLE event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    client->SetEventHandle(event);

    ComPtr<IAudioCaptureClient> capture;
    client->GetService(IID_PPV_ARGS(&capture));
    client->Start();

    QElapsedTimer timer;
    timer.start();
    const int channels = mix ? mix->nChannels : 2;
    const int sampleRate = mix ? mix->nSamplesPerSec : kAudioSampleRate;

    while (m_running.load()) {
        WaitForSingleObject(event, 20);
        UINT32 packet = 0;
        capture->GetNextPacketSize(&packet);
        while (packet > 0) {
            BYTE* data = nullptr;
            UINT32 frames = 0;
            DWORD flagsOut = 0;
            if (FAILED(capture->GetBuffer(&data, &frames, &flagsOut, nullptr, nullptr)))
                break;

            AudioBuffer buf;
            buf.channels = channels;
            buf.sampleRate = sampleRate;
            buf.ptsUs = timer.nsecsElapsed() / 1000;
            buf.samples.resize(static_cast<size_t>(frames) * channels);

            const bool isFloat = mix && (mix->wFormatTag == WAVE_FORMAT_IEEE_FLOAT
                || mix->wFormatTag == WAVE_FORMAT_EXTENSIBLE);
            if (isFloat) {
                const float* src = reinterpret_cast<const float*>(data);
                for (size_t i = 0; i < buf.samples.size(); ++i)
                    buf.samples[i] = (flagsOut & AUDCLNT_BUFFERFLAGS_SILENT) ? 0.f : src[i];
            } else {
                // Convert 16-bit PCM
                const int16_t* src = reinterpret_cast<const int16_t*>(data);
                for (size_t i = 0; i < buf.samples.size(); ++i)
                    buf.samples[i] = (flagsOut & AUDCLNT_BUFFERFLAGS_SILENT)
                                         ? 0.f
                                         : src[i] / 32768.f;
            }

            {
                std::lock_guard lock(m_cbMutex);
                if (m_callback) m_callback(buf);
            }

            capture->ReleaseBuffer(frames);
            capture->GetNextPacketSize(&packet);
        }
    }

    client->Stop();
    CloseHandle(event);
    CoTaskMemFree(mix);
#else
    while (m_running.load())
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
#endif
}

} // namespace railshot
