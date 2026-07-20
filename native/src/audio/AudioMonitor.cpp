#include "audio/AudioMonitor.h"
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
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;
#endif

namespace railshot {

AudioMonitor::AudioMonitor(QObject* parent)
    : QObject(parent)
{
}

AudioMonitor::~AudioMonitor()
{
    stop();
}

bool AudioMonitor::start(QString* error)
{
    if (m_running.load()) return true;
    m_running = true;
    m_thread = std::thread([this] { renderLoop(); });
    Q_UNUSED(error);
    Logger::info(QStringLiteral("Audio monitor started"));
    return true;
}

void AudioMonitor::stop()
{
    if (!m_running.exchange(false)) return;
    if (m_thread.joinable()) m_thread.join();
}

void AudioMonitor::push(const AudioBuffer& buffer)
{
    std::lock_guard lock(m_mutex);
    m_queue.push_back(buffer);
    while (m_queue.size() > 32)
        m_queue.pop_front();
}

void AudioMonitor::renderLoop()
{
#ifdef _WIN32
    ComInitializer com;
    ComPtr<IMMDeviceEnumerator> enumerator;
    CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                     IID_PPV_ARGS(&enumerator));
    if (!enumerator) return;
    ComPtr<IMMDevice> device;
    enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
    if (!device) return;
    ComPtr<IAudioClient> client;
    device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                     reinterpret_cast<void**>(client.GetAddressOf()));
    if (!client) return;
    WAVEFORMATEX* mix = nullptr;
    client->GetMixFormat(&mix);
    client->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, 100000, 0, mix, nullptr);
    ComPtr<IAudioRenderClient> render;
    client->GetService(IID_PPV_ARGS(&render));
    client->Start();

    while (m_running.load()) {
        AudioBuffer buf;
        {
            std::lock_guard lock(m_mutex);
            if (!m_queue.empty()) {
                buf = std::move(m_queue.front());
                m_queue.pop_front();
            }
        }
        if (buf.samples.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }
        UINT32 padding = 0, bufferFrames = 0;
        client->GetCurrentPadding(&padding);
        client->GetBufferSize(&bufferFrames);
        const UINT32 available = bufferFrames - padding;
        const UINT32 frames = std::min<UINT32>(available, static_cast<UINT32>(buf.frameCount()));
        if (frames == 0) continue;
        BYTE* data = nullptr;
        if (SUCCEEDED(render->GetBuffer(frames, &data))) {
            float* dst = reinterpret_cast<float*>(data);
            const float vol = m_volume.load();
            const int ch = buf.channels;
            for (UINT32 i = 0; i < frames; ++i) {
                for (int c = 0; c < ch && c < mix->nChannels; ++c)
                    dst[i * mix->nChannels + c] = buf.samples[i * ch + c] * vol;
            }
            render->ReleaseBuffer(frames, 0);
        }
    }
    client->Stop();
    CoTaskMemFree(mix);
#else
    while (m_running.load())
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
#endif
}

} // namespace railshot
