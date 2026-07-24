#include "capture/BrowserSource.h"
#include "browser/BrowserIpc.h"
#include "core/Logger.h"
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QDateTime>
#include <cstring>
#include <vector>
#include <algorithm>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <d3d11.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;
#endif

namespace railshot {

BrowserSource::BrowserSource(QString id, QString name, QJsonObject settings)
    : m_id(std::move(id))
    , m_name(std::move(name))
    , m_settings(std::move(settings))
{
    m_width = m_settings.value(QStringLiteral("width")).toInt(1280);
    m_height = m_settings.value(QStringLiteral("height")).toInt(720);
    m_refreshToken = m_settings.value(QStringLiteral("refreshToken")).toInt(0);
    m_reloadToken = m_settings.value(QStringLiteral("reloadToken")).toInt(0);
}

BrowserSource::~BrowserSource()
{
    stop();
}

QString BrowserSource::mappingName() const
{
#ifdef _WIN32
    // Include PID so a zombie helper from a previous run cannot hand us a smaller mapping.
    return QStringLiteral("Local\\RailShotTV_Browser_%1_%2")
        .arg(quint64(GetCurrentProcessId()))
        .arg(m_id);
#else
    return QStringLiteral("Local\\RailShotTV_Browser_%1").arg(m_id);
#endif
}

QString BrowserSource::helperExecutable() const
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString local = QDir(appDir).filePath(QStringLiteral("railshot_browser_helper.exe"));
    if (QFileInfo::exists(local))
        return local;
    // Dev builds may place helper next to tests / alternate bin dirs
    const QString alt = QDir(appDir).absoluteFilePath(QStringLiteral("../tests/RelWithDebInfo/railshot_browser_helper.exe"));
    if (QFileInfo::exists(alt))
        return QFileInfo(alt).absoluteFilePath();
    return local;
}

bool BrowserSource::openSharedMemory(QString* error)
{
#ifdef _WIN32
    closeSharedMemory();
    m_width = std::clamp(m_width, 64, 7680);
    m_height = std::clamp(m_height, 64, 4320);
    const std::wstring mapName = mappingName().toStdWString();
    const std::wstring mtxName = (mappingName() + QStringLiteral("_mtx")).toStdWString();
    const size_t bytes = browser_ipc::bufferBytes(m_width, m_height);
    if (bytes > size_t(0x7fffffff)) {
        if (error) *error = QStringLiteral("Browser frame buffer too large");
        return false;
    }
    const DWORD dwBytes = static_cast<DWORD>(bytes);

    m_mutexHandle = CreateMutexW(nullptr, FALSE, mtxName.c_str());
    SetLastError(0);
    m_mapping = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, dwBytes, mapName.c_str());
    if (!m_mapping) {
        if (error) *error = QStringLiteral("Browser shared mapping failed");
        return false;
    }
    // If an older mapping with this name already exists, CreateFileMapping ignores our size.
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(m_mapping);
        m_mapping = nullptr;
        // Unique fallback name for this attempt.
        const QString alt = mappingName() + QStringLiteral("_%1").arg(QDateTime::currentMSecsSinceEpoch());
        const std::wstring altMap = alt.toStdWString();
        const std::wstring altMtx = (alt + QStringLiteral("_mtx")).toStdWString();
        if (m_mutexHandle) {
            CloseHandle(m_mutexHandle);
            m_mutexHandle = nullptr;
        }
        m_mutexHandle = CreateMutexW(nullptr, FALSE, altMtx.c_str());
        m_mapping = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, dwBytes, altMap.c_str());
        if (!m_mapping) {
            if (error) *error = QStringLiteral("Browser shared mapping failed (alt)");
            return false;
        }
        // Helper still needs the name we actually opened — stash via settings token is awkward;
        // instead rewrite mapping by storing the live name in a member.
        m_liveMappingName = alt;
    } else {
        m_liveMappingName = mappingName();
    }
    m_view = MapViewOfFile(m_mapping, FILE_MAP_ALL_ACCESS, 0, 0, dwBytes);
    if (!m_view) {
        if (error) *error = QStringLiteral("Browser MapViewOfFile failed");
        closeSharedMemory();
        return false;
    }
    m_mappedBytes = bytes;
    auto* hdr = static_cast<browser_ipc::FrameHeader*>(m_view);
    *hdr = browser_ipc::FrameHeader{};
    hdr->width = static_cast<quint32>(m_width);
    hdr->height = static_cast<quint32>(m_height);
    hdr->stride = static_cast<quint32>(m_width * 4);
    hdr->status = 1;
    return true;
#else
    Q_UNUSED(error);
    return false;
#endif
}

void BrowserSource::closeSharedMemory()
{
#ifdef _WIN32
    if (m_view) {
        UnmapViewOfFile(m_view);
        m_view = nullptr;
    }
    if (m_mapping) {
        CloseHandle(m_mapping);
        m_mapping = nullptr;
    }
    if (m_mutexHandle) {
        CloseHandle(m_mutexHandle);
        m_mutexHandle = nullptr;
    }
    m_mappedBytes = 0;
    m_liveMappingName.clear();
#endif
}

bool BrowserSource::ensureHelper(QString* error)
{
    if (m_helper.state() != QProcess::NotRunning)
        return true;

    const QString exe = helperExecutable();
    if (!QFileInfo::exists(exe)) {
        if (error) {
            *error = QStringLiteral("railshot_browser_helper.exe not found next to RailShotTV "
                                    "(build the helper target)");
        }
        return false;
    }

    const QString url = m_settings.value(QStringLiteral("url")).toString(
        QStringLiteral("https://example.com"));
    const QString shm = m_liveMappingName.isEmpty() ? mappingName() : m_liveMappingName;
    QStringList args;
    args << QStringLiteral("--url") << url
         << QStringLiteral("--width") << QString::number(m_width)
         << QStringLiteral("--height") << QString::number(m_height)
         << QStringLiteral("--shm") << shm
         << QStringLiteral("--fps") << QString::number(m_settings.value(QStringLiteral("fps")).toInt(15));

    m_helper.setProgram(exe);
    m_helper.setArguments(args);
    m_helper.setProcessChannelMode(QProcess::MergedChannels);
    m_helper.start();
    if (!m_helper.waitForStarted(5000)) {
        if (error) *error = QStringLiteral("Failed to start browser helper: %1").arg(m_helper.errorString());
        return false;
    }
    Logger::info(QStringLiteral("Browser helper started for %1 → %2").arg(m_id, url));
    return true;
}

bool BrowserSource::uploadLatest(QString* error)
{
#ifdef _WIN32
    if (!m_device || !m_view || m_mappedBytes < sizeof(browser_ipc::FrameHeader))
        return false;

    // Copy header + pixels under the IPC mutex so we never CreateTexture from a
    // buffer the helper is mid-overwrite (that tore frames and flickered).
    quint64 idx = 0;
    int w = 0;
    int h = 0;
    int stride = 0;
    quint32 status = 0;
    quint32 commandAck = 0;
    std::vector<uint8_t> pixels;
    bool locked = false;
    if (m_mutexHandle) {
        const DWORD wr = WaitForSingleObject(static_cast<HANDLE>(m_mutexHandle), 50);
        locked = (wr == WAIT_OBJECT_0 || wr == WAIT_ABANDONED);
        if (!locked)
            return m_texture != nullptr; // keep last frame; never memcpy without the lock
    }
    auto* hdr = static_cast<browser_ipc::FrameHeader*>(m_view);
    if (hdr->magic == browser_ipc::kMagic && hdr->width > 0 && hdr->height > 0) {
        idx = hdr->frameIndex;
        w = static_cast<int>(hdr->width);
        h = static_cast<int>(hdr->height);
        stride = static_cast<int>(hdr->stride > 0 ? hdr->stride : hdr->width * 4);
        status = hdr->status;
        commandAck = hdr->commandAck;
        // Reject absurd / torn dimensions before touching pixel memory.
        w = std::clamp(w, 0, 7680);
        h = std::clamp(h, 0, 4320);
        stride = std::clamp(stride, 0, 7680 * 4);
        if (w > 0 && h > 0 && stride >= w * 4 && idx != m_lastFrameIndex) {
            const size_t headerBytes = sizeof(browser_ipc::FrameHeader);
            const size_t need = size_t(h) * size_t(stride);
            if (headerBytes + need <= m_mappedBytes) {
                pixels.resize(need);
                std::memcpy(pixels.data(), reinterpret_cast<const uint8_t*>(hdr + 1), need);
            }
        }
    }
    if (locked && m_mutexHandle)
        ReleaseMutex(static_cast<HANDLE>(m_mutexHandle));

    // Soft-reload freeze (OBS-style): keep last GPU texture until the helper
    // publishes a validated status=0 frame after ack. Timer-based unfreeze was
    // uploading CapturePreview blanks and causing the refresh flicker.
    if (m_textureFrozen) {
        if (m_freezeStartedMs == 0)
            m_freezeStartedMs = QDateTime::currentMSecsSinceEpoch();
        const bool helperOkFrame = commandAck >= m_freezeMinAck
                                   && status == 0
                                   && !pixels.empty();
        if (helperOkFrame) {
            m_textureFrozen = false;
            m_freezeStartedMs = 0;
            // fall through and upload this frame
        } else {
            if (!pixels.empty())
                m_lastFrameIndex = idx; // consume without uploading
            return m_texture != nullptr;
        }
    }

    if (pixels.empty())
        return m_texture != nullptr;

    m_lastFrameIndex = idx;
    m_width = w;
    m_height = h;

    ComPtr<ID3D11DeviceContext> ctx;
    m_device->GetImmediateContext(&ctx);
    if (!ctx) {
        if (error) *error = QStringLiteral("Browser missing D3D context");
        return false;
    }

    // Reuse texture + UpdateSubresource — recreating every frame flickered hard.
    bool needCreate = !m_texture;
    if (m_texture) {
        D3D11_TEXTURE2D_DESC cur{};
        m_texture->GetDesc(&cur);
        if (int(cur.Width) != w || int(cur.Height) != h)
            needCreate = true;
    }
    if (needCreate) {
        if (m_texture) {
            m_texture->Release();
            m_texture = nullptr;
        }
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = static_cast<UINT>(w);
        desc.Height = static_cast<UINT>(h);
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        ComPtr<ID3D11Texture2D> tex;
        if (FAILED(m_device->CreateTexture2D(&desc, nullptr, &tex))) {
            if (error) *error = QStringLiteral("Browser texture create failed");
            return false;
        }
        m_texture = tex.Detach();
    }

    ctx->UpdateSubresource(m_texture, 0, nullptr, pixels.data(), static_cast<UINT>(stride), 0);
    return true;
#else
    Q_UNUSED(error);
    return false;
#endif
}

bool BrowserSource::start(ID3D11Device* device, QString* error)
{
    std::lock_guard lock(m_mutex);
    m_device = device;
    m_width = m_settings.value(QStringLiteral("width")).toInt(m_width);
    m_height = m_settings.value(QStringLiteral("height")).toInt(m_height);
    if (!openSharedMemory(error))
        return false;
    if (!ensureHelper(error)) {
        closeSharedMemory();
        return false;
    }
    // Give helper a brief moment to paint; upload may still be empty.
    QElapsedTimer t;
    t.start();
    while (t.elapsed() < 300) {
        if (uploadLatest(nullptr) && m_texture)
            break;
#ifdef _WIN32
        Sleep(30);
#endif
    }
    if (!m_texture) {
        // Visible placeholder so Preview shows the source even before the helper paints.
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = static_cast<UINT>(m_width);
        desc.Height = static_cast<UINT>(m_height);
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        ComPtr<ID3D11Texture2D> tex;
        if (SUCCEEDED(m_device->CreateTexture2D(&desc, nullptr, &tex))) {
            ComPtr<ID3D11RenderTargetView> rtv;
            if (SUCCEEDED(m_device->CreateRenderTargetView(tex.Get(), nullptr, &rtv))) {
                ComPtr<ID3D11DeviceContext> ctx;
                m_device->GetImmediateContext(&ctx);
                const float clear[4] = {0.f, 0.f, 0.f, 0.f}; // transparent until first helper frame
                ctx->ClearRenderTargetView(rtv.Get(), clear);
            }
            m_texture = tex.Detach();
        }
    }
    m_running = true;
    return true;
}

void BrowserSource::stop()
{
    std::lock_guard lock(m_mutex);
    m_running = false;
    if (m_helper.state() != QProcess::NotRunning) {
        m_helper.terminate();
        if (!m_helper.waitForFinished(2000))
            m_helper.kill();
    }
#ifdef _WIN32
    if (m_texture) {
        m_texture->Release();
        m_texture = nullptr;
    }
#endif
    closeSharedMemory();
}

bool BrowserSource::acquireLatest(VideoFrame& out)
{
    std::lock_guard lock(m_mutex);
    if (!m_running.load()) return false;
    uploadLatest(nullptr);
    if (!m_texture) return false;
    out.texture = m_texture;
    out.width = m_width;
    out.height = m_height;
    out.sourceId = m_id;
    out.opaque = false; // browser overlays composite with alpha (OBS default)
    return true;
}

void BrowserSource::writeReloadCommand()
{
#ifdef _WIN32
    if (!m_view) return;
    bool locked = false;
    if (m_mutexHandle) {
        const DWORD wr = WaitForSingleObject(static_cast<HANDLE>(m_mutexHandle), 50);
        locked = (wr == WAIT_OBJECT_0 || wr == WAIT_ABANDONED);
        if (!locked)
            return;
    }
    auto* hdr = static_cast<browser_ipc::FrameHeader*>(m_view);
    if (hdr->magic == browser_ipc::kMagic || hdr->magic == 0) {
        hdr->magic = browser_ipc::kMagic;
        hdr->command = browser_ipc::kCmdReload;
        // Freeze GPU texture until helper publishes a validated post-reload frame.
        m_textureFrozen = true;
        m_freezeMinAck = hdr->commandAck + 1;
        m_freezeStartedMs = QDateTime::currentMSecsSinceEpoch();
        hdr->status = 3;
    }
    if (locked && m_mutexHandle)
        ReleaseMutex(static_cast<HANDLE>(m_mutexHandle));
#endif
}

void BrowserSource::requestSoftReload()
{
    std::lock_guard lock(m_mutex);
    writeReloadCommand();
}

void BrowserSource::applySettings(const QJsonObject& settings)
{
    std::lock_guard lock(m_mutex);
    const QString oldUrl = m_settings.value(QStringLiteral("url")).toString();
    const int oldW = m_width;
    const int oldH = m_height;
    const int oldRefresh = m_settings.value(QStringLiteral("refreshToken")).toInt(m_refreshToken);
    const int oldReload = m_settings.value(QStringLiteral("reloadToken")).toInt(m_reloadToken);
    m_settings = settings;
    const int wantW = std::clamp(m_settings.value(QStringLiteral("width")).toInt(oldW), 64, 7680);
    const int wantH = std::clamp(m_settings.value(QStringLiteral("height")).toInt(oldH), 64, 4320);
    m_width = wantW;
    m_height = wantH;
    const QString newUrl = m_settings.value(QStringLiteral("url")).toString();
    const int newRefresh = m_settings.value(QStringLiteral("refreshToken")).toInt(0);
    const int newReload = m_settings.value(QStringLiteral("reloadToken")).toInt(0);

    // OBS Refresh / Reload page: soft reload in-process — keep last D3D texture.
    if (m_running.load()
        && (newRefresh != oldRefresh || newReload != oldReload
            || newRefresh != m_refreshToken || newReload != m_reloadToken)) {
        m_refreshToken = newRefresh;
        m_reloadToken = newReload;
        writeReloadCommand();
        return;
    }
    m_refreshToken = newRefresh;
    m_reloadToken = newReload;

    const bool sizeChanged = (wantW != oldW || wantH != oldH);
    // URL or canvas size change: remapping + restart helper.
    if (m_running.load() && (oldUrl != newUrl || sizeChanged)) {
        if (m_helper.state() != QProcess::NotRunning) {
            m_helper.terminate();
            m_helper.waitForFinished(1500);
        }
        if (sizeChanged) {
            QString err;
            if (!openSharedMemory(&err))
                Logger::warn(QStringLiteral("Browser remap failed: %1").arg(err));
        }
        ensureHelper(nullptr);
    }
}

} // namespace railshot
