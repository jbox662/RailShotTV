#include "platform/windows/DxgiHelpers.h"
#include "core/Logger.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;
#endif

#include <QVector>

namespace railshot {

bool DxgiHelpers::createDevice(ID3D11Device** device,
                               ID3D11DeviceContext** context,
                               QString* error)
{
#ifdef _WIN32
    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    D3D_FEATURE_LEVEL levels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    D3D_FEATURE_LEVEL got{};
    HRESULT hr = D3D11CreateDevice(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
        levels, ARRAYSIZE(levels), D3D11_SDK_VERSION,
        device, &got, context);
    if (FAILED(hr)) {
        hr = D3D11CreateDevice(
            nullptr, D3D_DRIVER_TYPE_WARP, nullptr, flags,
            levels, ARRAYSIZE(levels), D3D11_SDK_VERSION,
            device, &got, context);
    }
    if (FAILED(hr)) {
        if (error) *error = QStringLiteral("D3D11CreateDevice failed: 0x%1").arg(quint32(hr), 8, 16, QLatin1Char('0'));
        return false;
    }
    Logger::info(QStringLiteral("D3D11 device created (feature level 0x%1)")
                     .arg(quint32(got), 4, 16, QLatin1Char('0')));
    return true;
#else
    Q_UNUSED(device); Q_UNUSED(context);
    if (error) *error = QStringLiteral("D3D11 only available on Windows");
    return false;
#endif
}

QVector<DxgiAdapterInfo> DxgiHelpers::enumerateAdapters()
{
    QVector<DxgiAdapterInfo> out;
#ifdef _WIN32
    ComPtr<IDXGIFactory1> factory;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))))
        return out;
    for (UINT i = 0;; ++i) {
        ComPtr<IDXGIAdapter1> adapter;
        if (factory->EnumAdapters1(i, &adapter) == DXGI_ERROR_NOT_FOUND)
            break;
        DXGI_ADAPTER_DESC1 desc{};
        adapter->GetDesc1(&desc);
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            continue;
        DxgiAdapterInfo info;
        info.name = QString::fromWCharArray(desc.Description);
        info.dedicatedVideoMemory = static_cast<qint64>(desc.DedicatedVideoMemory);
        info.luid = (quint64(desc.AdapterLuid.HighPart) << 32) | desc.AdapterLuid.LowPart;
        out.append(info);
    }
#endif
    return out;
}

} // namespace railshot
