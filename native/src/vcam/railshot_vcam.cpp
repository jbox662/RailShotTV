// RailShotTV Win11 virtual camera media source.
// Reads BGRA frames from Local\RailShotTV_VirtualCamera shared memory (VCamIpc.h).
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mferror.h>
#include <mfobjects.h>
#include <atomic>
#include <cstring>
#include <new>

#include "vcam/VCamIpc.h"

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "advapi32.lib")

// {8F3C2E1A-9B4D-4F6A-A7C8-1D2E3F4A5B6C}
static const GUID CLSID_RailShotVCamMediaSource =
{ 0x8f3c2e1a, 0x9b4d, 0x4f6a, { 0xa7, 0xc8, 0x1d, 0x2e, 0x3f, 0x4a, 0x5b, 0x6c } };

namespace {

HINSTANCE g_module = nullptr;
std::atomic<long> g_locks{0};

template <typename T>
class ComBase : public T {
public:
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_ref; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        const long v = --m_ref;
        if (v == 0) delete this;
        return ULONG(v);
    }
protected:
    virtual ~ComBase() = default;
    std::atomic<long> m_ref{1};
};

HRESULT makeRgb32Type(UINT32 width, UINT32 height, UINT32 fps, IMFMediaType** out)
{
    IMFMediaType* type = nullptr;
    HRESULT hr = MFCreateMediaType(&type);
    if (FAILED(hr)) return hr;
    hr = type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    if (SUCCEEDED(hr)) hr = type->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
    if (SUCCEEDED(hr)) hr = MFSetAttributeSize(type, MF_MT_FRAME_SIZE, width, height);
    if (SUCCEEDED(hr)) hr = MFSetAttributeRatio(type, MF_MT_FRAME_RATE, fps, 1);
    if (SUCCEEDED(hr)) hr = type->SetUINT32(MF_MT_DEFAULT_STRIDE, LONG(width * 4));
    if (SUCCEEDED(hr)) hr = type->SetUINT32(MF_MT_SAMPLE_SIZE, width * height * 4);
    if (SUCCEEDED(hr)) hr = type->SetUINT32(MF_MT_FIXED_SIZE_SAMPLES, TRUE);
    if (SUCCEEDED(hr)) hr = type->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
    if (SUCCEEDED(hr)) hr = type->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    if (FAILED(hr)) {
        type->Release();
        return hr;
    }
    *out = type;
    return S_OK;
}

class MediaStream;

class MediaSource final : public ComBase<IMFMediaSource> {
public:
    static HRESULT create(IMFMediaSource** out)
    {
        auto* src = new (std::nothrow) MediaSource();
        if (!src) return E_OUTOFMEMORY;
        HRESULT hr = src->initialize();
        if (FAILED(hr)) {
            src->Release();
            return hr;
        }
        *out = src;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override
    {
        if (!ppv) return E_POINTER;
        if (riid == IID_IUnknown || riid == IID_IMFMediaSource || riid == IID_IMFMediaEventGenerator) {
            *ppv = static_cast<IMFMediaSource*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }

    // IMFMediaEventGenerator
    HRESULT STDMETHODCALLTYPE GetEvent(DWORD flags, IMFMediaEvent** event) override
    {
        return m_queue->GetEvent(flags, event);
    }
    HRESULT STDMETHODCALLTYPE BeginGetEvent(IMFAsyncCallback* cb, IUnknown* state) override
    {
        return m_queue->BeginGetEvent(cb, state);
    }
    HRESULT STDMETHODCALLTYPE EndGetEvent(IMFAsyncResult* result, IMFMediaEvent** event) override
    {
        return m_queue->EndGetEvent(result, event);
    }
    HRESULT STDMETHODCALLTYPE QueueEvent(MediaEventType met, REFGUID guid, HRESULT status, const PROPVARIANT* val) override
    {
        return m_queue->QueueEventParamVar(met, guid, status, val);
    }

    HRESULT STDMETHODCALLTYPE GetCharacteristics(DWORD* characteristics) override
    {
        if (!characteristics) return E_POINTER;
        *characteristics = MFMEDIASOURCE_IS_LIVE;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE CreatePresentationDescriptor(IMFPresentationDescriptor** pd) override;
    HRESULT STDMETHODCALLTYPE Start(IMFPresentationDescriptor* pd, const GUID* timeFormat, const PROPVARIANT* start) override;
    HRESULT STDMETHODCALLTYPE Stop() override;
    HRESULT STDMETHODCALLTYPE Pause() override { return MF_E_INVALID_STATE_TRANSITION; }
    HRESULT STDMETHODCALLTYPE Shutdown() override;

    UINT32 width() const { return m_width; }
    UINT32 height() const { return m_height; }
    UINT32 fps() const { return m_fps; }

private:
    friend class MediaStream;
    MediaSource() = default;
    ~MediaSource() override { Shutdown(); }

    HRESULT initialize();

    IMFMediaEventQueue* m_queue = nullptr;
    MediaStream* m_stream = nullptr;
    IMFPresentationDescriptor* m_pd = nullptr;
    std::atomic<bool> m_shutdown{false};
    std::atomic<bool> m_started{false};
    UINT32 m_width = 1920;
    UINT32 m_height = 1080;
    UINT32 m_fps = 30;
};

class MediaStream final : public ComBase<IMFMediaStream> {
public:
    explicit MediaStream(MediaSource* source)
        : m_source(source)
    {
        m_source->AddRef();
        MFCreateEventQueue(&m_queue);
    }

    ~MediaStream() override
    {
        if (m_queue) m_queue->Shutdown();
        if (m_queue) m_queue->Release();
        if (m_type) m_type->Release();
        if (m_source) m_source->Release();
        closeShared();
    }

    HRESULT setType(IMFMediaType* type)
    {
        if (m_type) m_type->Release();
        m_type = type;
        if (m_type) m_type->AddRef();
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override
    {
        if (!ppv) return E_POINTER;
        if (riid == IID_IUnknown || riid == IID_IMFMediaStream || riid == IID_IMFMediaEventGenerator) {
            *ppv = static_cast<IMFMediaStream*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }

    HRESULT STDMETHODCALLTYPE GetEvent(DWORD flags, IMFMediaEvent** event) override { return m_queue->GetEvent(flags, event); }
    HRESULT STDMETHODCALLTYPE BeginGetEvent(IMFAsyncCallback* cb, IUnknown* state) override { return m_queue->BeginGetEvent(cb, state); }
    HRESULT STDMETHODCALLTYPE EndGetEvent(IMFAsyncResult* result, IMFMediaEvent** event) override { return m_queue->EndGetEvent(result, event); }
    HRESULT STDMETHODCALLTYPE QueueEvent(MediaEventType met, REFGUID guid, HRESULT status, const PROPVARIANT* val) override
    {
        return m_queue->QueueEventParamVar(met, guid, status, val);
    }

    HRESULT STDMETHODCALLTYPE GetMediaSource(IMFMediaSource** source) override
    {
        if (!source) return E_POINTER;
        *source = m_source;
        m_source->AddRef();
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetStreamDescriptor(IMFStreamDescriptor** sd) override
    {
        if (!sd) return E_POINTER;
        IMFMediaTypeHandler* handler = nullptr;
        HRESULT hr = MFCreateStreamDescriptor(0, 1, &m_type, sd);
        if (FAILED(hr)) return hr;
        hr = (*sd)->GetMediaTypeHandler(&handler);
        if (SUCCEEDED(hr)) {
            handler->SetCurrentMediaType(m_type);
            handler->Release();
        }
        return hr;
    }

    HRESULT STDMETHODCALLTYPE RequestSample(IUnknown* /*token*/) override
    {
        if (!m_running.load()) return MF_E_INVALIDREQUEST;
        return emitSample();
    }

    void start()
    {
        openShared();
        m_running = true;
        m_time = 0;
    }

    void stop()
    {
        m_running = false;
        closeShared();
    }

private:
    HRESULT openShared()
    {
        closeShared();
        m_mutex = OpenMutexW(SYNCHRONIZE, FALSE, railshot::vcam_ipc::kMtxName);
        m_mapping = OpenFileMappingW(FILE_MAP_READ, FALSE, railshot::vcam_ipc::kMapName);
        if (!m_mapping) return S_FALSE;
        const DWORD bytes = DWORD(railshot::vcam_ipc::bufferBytes(int(m_source->width()), int(m_source->height())));
        m_view = MapViewOfFile(m_mapping, FILE_MAP_READ, 0, 0, bytes);
        return m_view ? S_OK : HRESULT_FROM_WIN32(GetLastError());
    }

    void closeShared()
    {
        if (m_view) { UnmapViewOfFile(m_view); m_view = nullptr; }
        if (m_mapping) { CloseHandle(m_mapping); m_mapping = nullptr; }
        if (m_mutex) { CloseHandle(m_mutex); m_mutex = nullptr; }
    }

    HRESULT emitSample()
    {
        const UINT32 w = m_source->width();
        const UINT32 h = m_source->height();
        const UINT32 stride = w * 4;
        const DWORD bytes = stride * h;

        IMFSample* sample = nullptr;
        IMFMediaBuffer* buffer = nullptr;
        HRESULT hr = MFCreateSample(&sample);
        if (FAILED(hr)) return hr;
        hr = MFCreateMemoryBuffer(bytes, &buffer);
        if (FAILED(hr)) {
            sample->Release();
            return hr;
        }

        BYTE* dst = nullptr;
        DWORD maxLen = 0;
        hr = buffer->Lock(&dst, &maxLen, nullptr);
        if (SUCCEEDED(hr)) {
            bool copied = false;
            if (m_view) {
                if (m_mutex) WaitForSingleObject(m_mutex, 8);
                auto* hdr = static_cast<const railshot::vcam_ipc::SharedHeader*>(m_view);
                if (hdr->magic == railshot::vcam_ipc::kMagic && hdr->width == w && hdr->height == h) {
                    const BYTE* src = reinterpret_cast<const BYTE*>(hdr + 1);
                    std::memcpy(dst, src, bytes);
                    copied = true;
                }
                if (m_mutex) ReleaseMutex(m_mutex);
            }
            if (!copied) {
                // Idle pattern when producer is not publishing yet.
                for (UINT32 y = 0; y < h; ++y) {
                    auto* row = reinterpret_cast<UINT32*>(dst + y * stride);
                    for (UINT32 x = 0; x < w; ++x)
                        row[x] = 0xFF1A1F2B;
                }
            }
            buffer->Unlock();
        }
        buffer->SetCurrentLength(bytes);
        sample->AddBuffer(buffer);
        buffer->Release();

        const LONGLONG duration = 10'000'000ll / LONGLONG(m_source->fps() > 0 ? m_source->fps() : 30);
        sample->SetSampleTime(m_time);
        sample->SetSampleDuration(duration);
        m_time += duration;

        hr = m_queue->QueueEventParamUnk(MEMediaSample, GUID_NULL, S_OK, sample);
        sample->Release();
        return hr;
    }

    MediaSource* m_source = nullptr;
    IMFMediaEventQueue* m_queue = nullptr;
    IMFMediaType* m_type = nullptr;
    std::atomic<bool> m_running{false};
    LONGLONG m_time = 0;
    HANDLE m_mapping = nullptr;
    void* m_view = nullptr;
    HANDLE m_mutex = nullptr;
};

HRESULT MediaSource::initialize()
{
    HRESULT hr = MFCreateEventQueue(&m_queue);
    if (FAILED(hr)) return hr;

    // Prefer dimensions advertised by shared memory if present.
    HANDLE mapping = OpenFileMappingW(FILE_MAP_READ, FALSE, railshot::vcam_ipc::kMapName);
    if (mapping) {
        void* view = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, sizeof(railshot::vcam_ipc::SharedHeader));
        if (view) {
            auto* hdr = static_cast<const railshot::vcam_ipc::SharedHeader*>(view);
            if (hdr->magic == railshot::vcam_ipc::kMagic && hdr->width > 0 && hdr->height > 0) {
                m_width = hdr->width;
                m_height = hdr->height;
            }
            UnmapViewOfFile(view);
        }
        CloseHandle(mapping);
    }

    IMFMediaType* type = nullptr;
    hr = makeRgb32Type(m_width, m_height, m_fps, &type);
    if (FAILED(hr)) return hr;

    m_stream = new (std::nothrow) MediaStream(this);
    if (!m_stream) {
        type->Release();
        return E_OUTOFMEMORY;
    }
    m_stream->setType(type);
    type->Release();

    IMFStreamDescriptor* sd = nullptr;
    hr = m_stream->GetStreamDescriptor(&sd);
    if (FAILED(hr)) return hr;
    hr = MFCreatePresentationDescriptor(1, &sd, &m_pd);
    sd->Release();
    if (FAILED(hr)) return hr;
    return m_pd->SelectStream(0);
}

HRESULT MediaSource::CreatePresentationDescriptor(IMFPresentationDescriptor** pd)
{
    if (!pd) return E_POINTER;
    if (m_shutdown.load()) return MF_E_SHUTDOWN;
    return m_pd->Clone(pd);
}

HRESULT MediaSource::Start(IMFPresentationDescriptor* /*pd*/, const GUID* timeFormat, const PROPVARIANT* /*start*/)
{
    if (m_shutdown.load()) return MF_E_SHUTDOWN;
    if (timeFormat && *timeFormat != GUID_NULL) return MF_E_UNSUPPORTED_TIME_FORMAT;

    m_stream->start();
    m_started = true;

    PROPVARIANT val{};
    PropVariantInit(&val);
    val.vt = VT_UNKNOWN;
    val.punkVal = static_cast<IMFMediaStream*>(m_stream);
    m_stream->AddRef();
    m_queue->QueueEventParamVar(MENewStream, GUID_NULL, S_OK, &val);
    PropVariantClear(&val);

    PROPVARIANT empty{};
    PropVariantInit(&empty);
    m_queue->QueueEventParamVar(MESourceStarted, GUID_NULL, S_OK, &empty);
    m_stream->QueueEvent(MEStreamStarted, GUID_NULL, S_OK, &empty);
    return S_OK;
}

HRESULT MediaSource::Stop()
{
    if (m_shutdown.load()) return MF_E_SHUTDOWN;
    if (!m_started.exchange(false)) return S_OK;
    m_stream->stop();
    PROPVARIANT empty{};
    PropVariantInit(&empty);
    m_stream->QueueEvent(MEStreamStopped, GUID_NULL, S_OK, &empty);
    return m_queue->QueueEventParamVar(MESourceStopped, GUID_NULL, S_OK, &empty);
}

HRESULT MediaSource::Shutdown()
{
    if (m_shutdown.exchange(true)) return S_OK;
    m_started = false;
    if (m_stream) {
        m_stream->stop();
        m_stream->Release();
        m_stream = nullptr;
    }
    if (m_pd) { m_pd->Release(); m_pd = nullptr; }
    if (m_queue) {
        m_queue->Shutdown();
        m_queue->Release();
        m_queue = nullptr;
    }
    return S_OK;
}

class ClassFactory final : public IClassFactory {
public:
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_ref; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        const long v = --m_ref;
        if (v == 0) delete this;
        return ULONG(v);
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override
    {
        if (!ppv) return E_POINTER;
        if (riid == IID_IUnknown || riid == IID_IClassFactory) {
            *ppv = static_cast<IClassFactory*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown* outer, REFIID riid, void** ppv) override
    {
        if (outer) return CLASS_E_NOAGGREGATION;
        IMFMediaSource* src = nullptr;
        HRESULT hr = MediaSource::create(&src);
        if (FAILED(hr)) return hr;
        hr = src->QueryInterface(riid, ppv);
        src->Release();
        return hr;
    }
    HRESULT STDMETHODCALLTYPE LockServer(BOOL lock) override
    {
        if (lock) ++g_locks;
        else --g_locks;
        return S_OK;
    }
private:
    std::atomic<long> m_ref{1};
};

HRESULT setRegSz(HKEY root, const wchar_t* path, const wchar_t* value)
{
    HKEY key = nullptr;
    LONG rc = RegCreateKeyExW(root, path, 0, nullptr, 0, KEY_WRITE, nullptr, &key, nullptr);
    if (rc != ERROR_SUCCESS) return HRESULT_FROM_WIN32(rc);
    rc = RegSetValueExW(key, nullptr, 0, REG_SZ,
                        reinterpret_cast<const BYTE*>(value),
                        DWORD((wcslen(value) + 1) * sizeof(wchar_t)));
    RegCloseKey(key);
    return HRESULT_FROM_WIN32(rc);
}

} // namespace

extern "C" BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH) {
        g_module = instance;
        DisableThreadLibraryCalls(instance);
    }
    return TRUE;
}

STDAPI DllCanUnloadNow()
{
    return g_locks.load() == 0 ? S_OK : S_FALSE;
}

STDAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void** ppv)
{
    if (clsid != CLSID_RailShotVCamMediaSource) return CLASS_E_CLASSNOTAVAILABLE;
    auto* factory = new (std::nothrow) ClassFactory();
    if (!factory) return E_OUTOFMEMORY;
    const HRESULT hr = factory->QueryInterface(riid, ppv);
    factory->Release();
    return hr;
}

STDAPI DllRegisterServer()
{
    wchar_t path[MAX_PATH]{};
    if (!GetModuleFileNameW(g_module, path, MAX_PATH))
        return HRESULT_FROM_WIN32(GetLastError());

    const wchar_t* clsid = railshot::vcam_ipc::kMediaSourceClsid;
    wchar_t keyClsid[128]{};
    wsprintfW(keyClsid, L"Software\\Classes\\CLSID\\%s", clsid);
    HRESULT hr = setRegSz(HKEY_LOCAL_MACHINE, keyClsid, railshot::vcam_ipc::kFriendlyName);
    if (FAILED(hr))
        hr = setRegSz(HKEY_CURRENT_USER, keyClsid, railshot::vcam_ipc::kFriendlyName);

    wchar_t keyInproc[160]{};
    wsprintfW(keyInproc, L"%s\\InProcServer32", keyClsid);
    HRESULT hr2 = setRegSz(HKEY_LOCAL_MACHINE, keyInproc, path);
    if (FAILED(hr2))
        hr2 = setRegSz(HKEY_CURRENT_USER, keyInproc, path);

    wchar_t keyThreading[192]{};
    wsprintfW(keyThreading, L"%s\\InProcServer32", keyClsid);
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, keyThreading, 0, KEY_WRITE, &key) != ERROR_SUCCESS)
        RegOpenKeyExW(HKEY_CURRENT_USER, keyThreading, 0, KEY_WRITE, &key);
    if (key) {
        const wchar_t* model = L"Both";
        RegSetValueExW(key, L"ThreadingModel", 0, REG_SZ,
                       reinterpret_cast<const BYTE*>(model),
                       DWORD((wcslen(model) + 1) * sizeof(wchar_t)));
        RegCloseKey(key);
    }
    return FAILED(hr) ? hr : hr2;
}

STDAPI DllUnregisterServer()
{
    const wchar_t* clsid = railshot::vcam_ipc::kMediaSourceClsid;
    wchar_t keyClsid[128]{};
    wsprintfW(keyClsid, L"Software\\Classes\\CLSID\\%s", clsid);
    RegDeleteTreeW(HKEY_LOCAL_MACHINE, keyClsid);
    RegDeleteTreeW(HKEY_CURRENT_USER, keyClsid);
    return S_OK;
}
