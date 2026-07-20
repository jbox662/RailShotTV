#include "compositor/PreviewSurface.h"
#include "compositor/D3D11Device.h"
#include <QPainter>
#include <QPaintEvent>

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

namespace railshot {

PreviewSurface::PreviewSurface(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_NoSystemBackground);
    setMinimumSize(160, 90);
}

PreviewSurface::~PreviewSurface()
{
    releaseSwapChain();
}

void PreviewSurface::setDevice(D3D11Device* device)
{
    m_device = device;
    releaseSwapChain();
}

void PreviewSurface::setLabel(const QString& label, const QColor& color)
{
    m_label = label;
    m_labelColor = color;
    update();
}

void PreviewSurface::setEmptyMessage(const QString& msg)
{
    m_empty = msg;
    update();
}

void PreviewSurface::releaseSwapChain()
{
#ifdef _WIN32
    if (m_rtv) { m_rtv->Release(); m_rtv = nullptr; }
    if (m_swap) { m_swap->Release(); m_swap = nullptr; }
#endif
}

bool PreviewSurface::ensureSwapChain()
{
#ifdef _WIN32
    if (m_swap || !m_device || !m_device->device()) return m_swap != nullptr;
    HWND hwnd = reinterpret_cast<HWND>(winId());
    ComPtr<IDXGIDevice> dxgiDevice;
    m_device->device()->QueryInterface(IID_PPV_ARGS(&dxgiDevice));
    ComPtr<IDXGIAdapter> adapter;
    dxgiDevice->GetAdapter(&adapter);
    ComPtr<IDXGIFactory2> factory;
    adapter->GetParent(IID_PPV_ARGS(&factory));

    DXGI_SWAP_CHAIN_DESC1 scd{};
    scd.Width = static_cast<UINT>(qMax(1, width()));
    scd.Height = static_cast<UINT>(qMax(1, height()));
    scd.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    scd.SampleDesc.Count = 1;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.BufferCount = 2;
    scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    scd.Scaling = DXGI_SCALING_STRETCH;

    if (FAILED(factory->CreateSwapChainForHwnd(m_device->device(), hwnd, &scd, nullptr, nullptr, &m_swap)))
        return false;
    ComPtr<ID3D11Texture2D> back;
    m_swap->GetBuffer(0, IID_PPV_ARGS(&back));
    m_device->device()->CreateRenderTargetView(back.Get(), nullptr, &m_rtv);
    return true;
#else
    return false;
#endif
}

void PreviewSurface::presentTexture(ID3D11Texture2D* texture)
{
#ifdef _WIN32
    if (!texture || !m_device) return;
    if (!ensureSwapChain()) return;
    ComPtr<ID3D11Texture2D> back;
    m_swap->GetBuffer(0, IID_PPV_ARGS(&back));
    m_device->context()->CopyResource(back.Get(), texture);
    m_swap->Present(1, 0);
    m_hasFrame = true;
#else
    Q_UNUSED(texture);
#endif
}

void PreviewSurface::paintEvent(QPaintEvent*)
{
    // When native present is active we still overlay labels via a secondary pass.
    // If no frame yet, fill with dark background message using GDI-compatible paint.
    if (m_hasFrame) return;
    QPainter p(this);
    p.fillRect(rect(), QColor(8, 10, 13));
    p.setPen(QColor(58, 69, 96));
    p.drawText(rect(), Qt::AlignCenter, m_empty);
    if (!m_label.isEmpty()) {
        p.fillRect(6, 6, 70, 16, m_labelColor);
        p.setPen(Qt::black);
        p.drawText(6, 6, 70, 16, Qt::AlignCenter, m_label);
    }
}

void PreviewSurface::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    releaseSwapChain();
}

} // namespace railshot
