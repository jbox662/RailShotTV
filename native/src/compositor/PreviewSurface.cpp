#include "compositor/PreviewSurface.h"
#include "compositor/D3D11Device.h"
#include <QPainter>
#include <QPaintEvent>
#include <cmath>
#include <algorithm>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;
#endif

namespace railshot {

namespace {

#ifdef _WIN32
void clearRect(ID3D11DeviceContext1* ctx, ID3D11RenderTargetView* rtv, const float color[4],
               int x0, int y0, int x1, int y1, int maxW, int maxH)
{
    if (!ctx || !rtv) return;
    x0 = std::max(0, std::min(x0, maxW));
    y0 = std::max(0, std::min(y0, maxH));
    x1 = std::max(0, std::min(x1, maxW));
    y1 = std::max(0, std::min(y1, maxH));
    if (x1 <= x0 || y1 <= y0) return;
    D3D11_RECT r{x0, y0, x1, y1};
    ctx->ClearView(rtv, color, &r, 1);
}

void fillHandle(ID3D11DeviceContext1* ctx, ID3D11RenderTargetView* rtv, const float color[4],
                float cx, float cy, int hs, int maxW, int maxH)
{
    const int x0 = int(std::lround(cx)) - hs;
    const int y0 = int(std::lround(cy)) - hs;
    clearRect(ctx, rtv, color, x0, y0, x0 + hs * 2, y0 + hs * 2, maxW, maxH);
}
#endif

} // namespace

PreviewSurface::PreviewSurface(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_NoSystemBackground);
    setMinimumSize(160, 90);
    setFocusPolicy(Qt::StrongFocus);
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

void PreviewSurface::setEditChrome(const PreviewEditChrome& chrome)
{
    m_chrome = chrome;
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

void PreviewSurface::drawEditChrome()
{
#ifdef _WIN32
    if (!m_chrome.visible || !m_rtv || !m_device || m_swapW == 0 || m_swapH == 0)
        return;

    ComPtr<ID3D11DeviceContext1> ctx1;
    if (FAILED(m_device->context()->QueryInterface(IID_PPV_ARGS(&ctx1))) || !ctx1)
        return;

    const int W = int(m_swapW);
    const int H = int(m_swapH);
    const float accent[4] = {
        m_chrome.color.redF(),
        m_chrome.color.greenF(),
        m_chrome.color.blueF(),
        1.0f
    };
    const float cropCol[4] = {0.2f, 0.95f, 0.45f, 1.0f};
    const float* lineCol = (m_chrome.cropping || m_chrome.cropLeft > 0.001 || m_chrome.cropRight > 0.001
                            || m_chrome.cropTop > 0.001 || m_chrome.cropBottom > 0.001)
                               ? cropCol
                               : accent;

    // Axis-aligned bounds in canvas pixels (rotation drawn as AABB + rotate knob).
    const QRectF nr = m_chrome.rect.normalized();
    const int x0 = int(std::lround(nr.left() * W));
    const int y0 = int(std::lround(nr.top() * H));
    const int x1 = int(std::lround(nr.right() * W));
    const int y1 = int(std::lround(nr.bottom() * H));
    if (x1 <= x0 + 2 || y1 <= y0 + 2)
        return;

    constexpr int kLine = 2;
    clearRect(ctx1.Get(), m_rtv, lineCol, x0, y0, x1, y0 + kLine, W, H);
    clearRect(ctx1.Get(), m_rtv, lineCol, x0, y1 - kLine, x1, y1, W, H);
    clearRect(ctx1.Get(), m_rtv, lineCol, x0, y0, x0 + kLine, y1, W, H);
    clearRect(ctx1.Get(), m_rtv, lineCol, x1 - kLine, y0, x1, y1, W, H);

    // Crop edge emphasis (thicker inner marks)
    if (m_chrome.cropLeft > 0.001)
        clearRect(ctx1.Get(), m_rtv, cropCol, x0, y0, x0 + 4, y1, W, H);
    if (m_chrome.cropRight > 0.001)
        clearRect(ctx1.Get(), m_rtv, cropCol, x1 - 4, y0, x1, y1, W, H);
    if (m_chrome.cropTop > 0.001)
        clearRect(ctx1.Get(), m_rtv, cropCol, x0, y0, x1, y0 + 4, W, H);
    if (m_chrome.cropBottom > 0.001)
        clearRect(ctx1.Get(), m_rtv, cropCol, x0, y1 - 4, x1, y1, W, H);

    constexpr int kHs = 5;
    const float cx = (x0 + x1) * 0.5f;
    const float cy = (y0 + y1) * 0.5f;
    fillHandle(ctx1.Get(), m_rtv, accent, float(x0), float(y0), kHs, W, H);
    fillHandle(ctx1.Get(), m_rtv, accent, cx, float(y0), kHs, W, H);
    fillHandle(ctx1.Get(), m_rtv, accent, float(x1), float(y0), kHs, W, H);
    fillHandle(ctx1.Get(), m_rtv, accent, float(x0), cy, kHs, W, H);
    fillHandle(ctx1.Get(), m_rtv, accent, float(x1), cy, kHs, W, H);
    fillHandle(ctx1.Get(), m_rtv, accent, float(x0), float(y1), kHs, W, H);
    fillHandle(ctx1.Get(), m_rtv, accent, cx, float(y1), kHs, W, H);
    fillHandle(ctx1.Get(), m_rtv, accent, float(x1), float(y1), kHs, W, H);

    // Rotate knob above top-center
    const float rotY = float(y0) - 28.0f;
    clearRect(ctx1.Get(), m_rtv, accent, int(cx) - 1, int(rotY) + 6, int(cx) + 1, y0, W, H);
    fillHandle(ctx1.Get(), m_rtv, accent, cx, rotY, kHs + 1, W, H);
#else
    (void)0;
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

    m_device->context()->CopyResource(back.Get(), texture);
    drawEditChrome();
    if (FAILED(m_swap->Present(1, 0)))
        return;
    if (!m_hasFrame) {
        m_hasFrame = true;
        update();
    }
#else
    Q_UNUSED(texture);
#endif
}

void PreviewSurface::paintEvent(QPaintEvent*)
{
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
}

} // namespace railshot
