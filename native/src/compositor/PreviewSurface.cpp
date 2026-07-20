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
    if (m_device == device)
        return;
    m_device = device;
    releaseSwapChain();
    m_hasFrame = false;
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
    m_swapW = 0;
    m_swapH = 0;
#endif
}

bool PreviewSurface::ensureSwapChain(unsigned width, unsigned height)
{
#ifdef _WIN32
    if (!m_device || !m_device->device() || width == 0 || height == 0)
        return false;
    if (m_swap && m_swapW == width && m_swapH == height)
        return true;
    releaseSwapChain();

    HWND hwnd = reinterpret_cast<HWND>(winId());
    if (!hwnd || !IsWindow(hwnd))
        return false;

    ComPtr<IDXGIDevice> dxgiDevice;
    if (FAILED(m_device->device()->QueryInterface(IID_PPV_ARGS(&dxgiDevice))))
        return false;
    ComPtr<IDXGIAdapter> adapter;
    if (FAILED(dxgiDevice->GetAdapter(&adapter)))
        return false;
    ComPtr<IDXGIFactory2> factory;
    if (FAILED(adapter->GetParent(IID_PPV_ARGS(&factory))))
        return false;

    // Buffer size matches the compositor canvas; DXGI_SCALING_STRETCH scales to the HWND.
    // CopyResource requires identical dimensions — widget-sized buffers would always fail at 1920x1080.
    DXGI_SWAP_CHAIN_DESC1 scd{};
    scd.Width = static_cast<UINT>(width);
    scd.Height = static_cast<UINT>(height);
    scd.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    scd.SampleDesc.Count = 1;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.BufferCount = 2;
    scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    scd.Scaling = DXGI_SCALING_STRETCH;

    if (FAILED(factory->CreateSwapChainForHwnd(m_device->device(), hwnd, &scd, nullptr, nullptr, &m_swap)))
        return false;
    ComPtr<ID3D11Texture2D> back;
    if (FAILED(m_swap->GetBuffer(0, IID_PPV_ARGS(&back)))) {
        releaseSwapChain();
        return false;
    }
    if (FAILED(m_device->device()->CreateRenderTargetView(back.Get(), nullptr, &m_rtv))) {
        releaseSwapChain();
        return false;
    }
    m_swapW = width;
    m_swapH = height;
    return true;
#else
    Q_UNUSED(width);
    Q_UNUSED(height);
    return false;
#endif
}

void PreviewSurface::presentTexture(ID3D11Texture2D* texture)
{
#ifdef _WIN32
    if (!texture || !m_device) return;

    D3D11_TEXTURE2D_DESC td{};
    texture->GetDesc(&td);
    if (!ensureSwapChain(td.Width, td.Height))
        return;

    ComPtr<ID3D11Texture2D> back;
    if (FAILED(m_swap->GetBuffer(0, IID_PPV_ARGS(&back))))
        return;

    // Same size as compositor target → CopyResource succeeds; Present stretches to the widget.
    m_device->context()->CopyResource(back.Get(), texture);
    if (FAILED(m_swap->Present(1, 0)))
        return;
    if (!m_hasFrame) {
        m_hasFrame = true;
        update(); // drop empty-state QPainter fill
    }
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
    p.setPen(QColor(64, 69, 80));
    QFont iconFont = p.font();
    iconFont.setPointSize(22);
    p.setFont(iconFont);
    p.drawText(rect().adjusted(0, -18, 0, 0), Qt::AlignCenter, QStringLiteral("▣"));
    QFont msgFont = p.font();
    msgFont.setFamily(QStringLiteral("DM Sans"));
    msgFont.setPointSize(10);
    msgFont.setBold(true);
    p.setFont(msgFont);
    p.setPen(QColor(58, 69, 96));
    p.drawText(rect().adjusted(0, 28, 0, 0), Qt::AlignCenter, m_empty);
    if (!m_label.isEmpty()) {
        p.fillRect(6, 6, 70, 16, m_labelColor);
        p.setPen(Qt::black);
        QFont badge = p.font();
        badge.setPointSize(8);
        badge.setBold(true);
        p.setFont(badge);
        p.drawText(6, 6, 70, 16, Qt::AlignCenter, m_label);
    }
}

void PreviewSurface::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    // HWND size changed; keep canvas-sized buffers — DXGI stretch handles the window.
    // Recreate only if the HWND was recreated (winId change); otherwise Present scales.
}

} // namespace railshot
