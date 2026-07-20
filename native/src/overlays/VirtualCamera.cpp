#include "overlays/VirtualCamera.h"
#include "vcam/VCamIpc.h"
#include "core/Logger.h"
#include <QtGlobal>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <cstring>
#include <string>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <d3d11.h>
#include <mfapi.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

#if __has_include(<mfvirtualcamera.h>)
#include <mfvirtualcamera.h>
#define RAILSHOT_HAS_MF_VCAM_HEADER 1
#else
#define RAILSHOT_HAS_MF_VCAM_HEADER 0
#endif
#endif

namespace railshot {

namespace {
#ifdef _WIN32
using vcam_ipc::SharedHeader;
using vcam_ipc::kMapName;
using vcam_ipc::kMtxName;
using vcam_ipc::kMediaSourceClsid;
using vcam_ipc::kFriendlyName;
using vcam_ipc::bufferBytes;

#if RAILSHOT_HAS_MF_VCAM_HEADER
using PFN_MFCreateVirtualCamera = decltype(&MFCreateVirtualCamera);
#endif

QString vcamDllPath()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString local = QDir(appDir).filePath(QStringLiteral("railshot_vcam.dll"));
    if (QFile::exists(local)) return local;
    return QDir(appDir).absoluteFilePath(QStringLiteral("../railshot_vcam.dll"));
}

QString stagedVcamDllPath()
{
    const QString dir = QDir::toNativeSeparators(
        QString::fromLocal8Bit(qgetenv("PROGRAMDATA")) + QStringLiteral("/RailShotTV"));
    QDir().mkpath(dir);
    return dir + QStringLiteral("/railshot_vcam.dll");
}

bool stageAndRegisterVcamDll(QString* error)
{
    const QString src = vcamDllPath();
    if (!QFile::exists(src)) {
        if (error) *error = QStringLiteral("railshot_vcam.dll not found next to RailShotTV");
        return false;
    }
    const QString dst = stagedVcamDllPath();
    if (QFileInfo(src).canonicalFilePath() != QFileInfo(dst).canonicalFilePath()) {
        QFile::remove(dst);
        if (!QFile::copy(src, dst))
            Logger::warn(QStringLiteral("Could not stage vcam DLL to ProgramData; registering in-place"));
    }
    const QString regTarget = QFile::exists(dst) ? dst : src;
    const std::wstring pathW = regTarget.toStdWString();
    HMODULE mod = LoadLibraryW(pathW.c_str());
    if (!mod) {
        if (error) *error = QStringLiteral("LoadLibrary(railshot_vcam.dll) failed");
        return false;
    }
    using DllRegisterServerFn = HRESULT(STDAPICALLTYPE*)();
    auto reg = reinterpret_cast<DllRegisterServerFn>(GetProcAddress(mod, "DllRegisterServer"));
    HRESULT hr = reg ? reg() : E_FAIL;
    FreeLibrary(mod);
    if (FAILED(hr)) {
        if (error) *error = QStringLiteral("DllRegisterServer failed: 0x%1")
                                .arg(quint32(hr), 8, 16, QLatin1Char('0'));
        return false;
    }
    return true;
}

bool startSystemVirtualCamera(void** outCamera, QString* error)
{
#if RAILSHOT_HAS_MF_VCAM_HEADER
    HMODULE sensorgroup = LoadLibraryW(L"mfsensorgroup.dll");
    if (!sensorgroup) {
        if (error) *error = QStringLiteral("mfsensorgroup.dll unavailable (Win11 required for system vcam)");
        return false;
    }
    auto create = reinterpret_cast<PFN_MFCreateVirtualCamera>(
        GetProcAddress(sensorgroup, "MFCreateVirtualCamera"));
    if (!create) {
        FreeLibrary(sensorgroup);
        if (error) *error = QStringLiteral("MFCreateVirtualCamera export missing");
        return false;
    }

    IMFVirtualCamera* cam = nullptr;
    HRESULT hr = create(MFVirtualCameraType_SoftwareCameraSource,
                        MFVirtualCameraLifetime_Session,
                        MFVirtualCameraAccess_CurrentUser,
                        kFriendlyName,
                        kMediaSourceClsid,
                        nullptr,
                        0,
                        &cam);
    FreeLibrary(sensorgroup);
    if (FAILED(hr) || !cam) {
        if (error) *error = QStringLiteral("MFCreateVirtualCamera failed: 0x%1")
                                .arg(quint32(hr), 8, 16, QLatin1Char('0'));
        return false;
    }
    hr = cam->Start(nullptr);
    if (FAILED(hr)) {
        cam->Shutdown();
        cam->Release();
        if (error) *error = QStringLiteral("IMFVirtualCamera::Start failed: 0x%1 "
                                           "(Frame Server may need DLL under ProgramData)")
                                .arg(quint32(hr), 8, 16, QLatin1Char('0'));
        return false;
    }
    *outCamera = cam;
    return true;
#else
    Q_UNUSED(outCamera);
    if (error) *error = QStringLiteral("SDK missing mfvirtualcamera.h");
    return false;
#endif
}

void stopSystemVirtualCamera(void*& camera)
{
#if RAILSHOT_HAS_MF_VCAM_HEADER
    if (!camera) return;
    auto* cam = static_cast<IMFVirtualCamera*>(camera);
    cam->Stop();
    cam->Shutdown();
    cam->Release();
    camera = nullptr;
#else
    camera = nullptr;
#endif
}
#endif
} // namespace

VirtualCamera::VirtualCamera(QObject* parent)
    : QObject(parent)
{
}

VirtualCamera::~VirtualCamera()
{
    stop();
}

bool VirtualCamera::start(int width, int height, QString* error)
{
    if (m_running.load()) return true;
    m_width = width > 0 ? width : 1920;
    m_height = height > 0 ? height : 1080;

#ifdef _WIN32
    const DWORD bytes = static_cast<DWORD>(bufferBytes(m_width, m_height));
    m_mutex = CreateMutexW(nullptr, FALSE, kMtxName);
    m_mapping = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
                                   0, bytes, kMapName);
    if (!m_mapping) {
        if (error) *error = QStringLiteral("CreateFileMapping failed");
        return false;
    }
    m_view = MapViewOfFile(m_mapping, FILE_MAP_ALL_ACCESS, 0, 0, bytes);
    if (!m_view) {
        CloseHandle(m_mapping);
        m_mapping = nullptr;
        if (error) *error = QStringLiteral("MapViewOfFile failed");
        return false;
    }
    auto* hdr = static_cast<SharedHeader*>(m_view);
    *hdr = SharedHeader{};
    hdr->magic = vcam_ipc::kMagic;
    hdr->width = static_cast<quint32>(m_width);
    hdr->height = static_cast<quint32>(m_height);
    hdr->stride = static_cast<quint32>(m_width * 4);
    m_running = true;

    QString regErr;
    if (stageAndRegisterVcamDll(&regErr)) {
        QString camErr;
        if (startSystemVirtualCamera(&m_mfCamera, &camErr)) {
            Logger::info(QStringLiteral("Virtual camera system device started (%1)")
                             .arg(QString::fromWCharArray(kFriendlyName)));
        } else {
            Logger::warn(QStringLiteral("Shared-memory vcam active; system device not started: %1").arg(camErr));
        }
    } else {
        Logger::warn(QStringLiteral("Shared-memory vcam active; DLL register skipped: %1").arg(regErr));
    }

    Logger::info(QStringLiteral("Virtual camera shared buffer ready %1x%2 "
                                 "(Local\\RailShotTV_VirtualCamera + _mtx)")
                     .arg(m_width).arg(m_height));
    emit started();
    return true;
#else
    Q_UNUSED(error);
    return false;
#endif
}

void VirtualCamera::stop()
{
    if (!m_running.exchange(false)) return;
#ifdef _WIN32
    stopSystemVirtualCamera(m_mfCamera);
    if (m_view) {
        UnmapViewOfFile(m_view);
        m_view = nullptr;
    }
    if (m_mapping) {
        CloseHandle(m_mapping);
        m_mapping = nullptr;
    }
    if (m_mutex) {
        CloseHandle(m_mutex);
        m_mutex = nullptr;
    }
#endif
    emit stopped();
    Logger::info(QStringLiteral("Virtual camera stopped"));
}

void VirtualCamera::submitFrame(ID3D11Texture2D* texture)
{
#ifdef _WIN32
    if (!m_running.load() || !texture || !m_view) return;

    ComPtr<ID3D11Device> device;
    texture->GetDevice(&device);
    if (!device) return;
    ComPtr<ID3D11DeviceContext> ctx;
    device->GetImmediateContext(&ctx);

    D3D11_TEXTURE2D_DESC desc{};
    texture->GetDesc(&desc);
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.MiscFlags = 0;

    ComPtr<ID3D11Texture2D> staging;
    if (FAILED(device->CreateTexture2D(&desc, nullptr, &staging)))
        return;
    ctx->CopyResource(staging.Get(), texture);
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(ctx->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped)))
        return;

    if (m_mutex)
        WaitForSingleObject(static_cast<HANDLE>(m_mutex), 16);

    auto* hdr = static_cast<SharedHeader*>(m_view);
    const int w = qMin(m_width, static_cast<int>(desc.Width));
    const int h = qMin(m_height, static_cast<int>(desc.Height));
    uint8_t* dst = static_cast<uint8_t*>(m_view) + sizeof(SharedHeader);
    for (int y = 0; y < h; ++y) {
        memcpy(dst + y * m_width * 4,
               static_cast<const char*>(mapped.pData) + y * mapped.RowPitch,
               static_cast<size_t>(w) * 4);
    }
    ctx->Unmap(staging.Get(), 0);
    hdr->magic = vcam_ipc::kMagic;
    hdr->width = static_cast<quint32>(m_width);
    hdr->height = static_cast<quint32>(m_height);
    hdr->stride = static_cast<quint32>(m_width * 4);
    hdr->frameIndex++;
    LARGE_INTEGER qpc{};
    QueryPerformanceCounter(&qpc);
    hdr->ptsUs = qpc.QuadPart;

    if (m_mutex)
        ReleaseMutex(static_cast<HANDLE>(m_mutex));
#else
    Q_UNUSED(texture);
#endif
}

} // namespace railshot
