#include "compositor/D3D11Compositor.h"
#include "compositor/D3D11Device.h"
#include "compositor/Shaders.h"
#include "core/Logger.h"
#include <algorithm>
#include <cmath>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;
#pragma comment(lib, "d3dcompiler.lib")
#endif

namespace railshot {

struct alignas(16) CBData {
    float rect[4];       // 0
    float opacity;       // 16
    float rotation;      // 20
    float cropMin[2];    // 24
    float cropMax[2];    // 32
    float _padCrop[2];   // 40 — HLSL packs next float4 at 48
    float colorMul[4];   // 48
    float colorAdd[4];   // 64
};

D3D11Compositor::D3D11Compositor(D3D11Device* device, QObject* parent)
    : QObject(parent), m_device(device)
{
}

D3D11Compositor::~D3D11Compositor()
{
    shutdown();
}

bool D3D11Compositor::createTargets(QString* error)
{
#ifdef _WIN32
    auto* dev = m_device->device();
    if (!dev) {
        if (error) *error = QStringLiteral("No D3D device");
        return false;
    }
    auto makeTex = [&](ID3D11Texture2D** tex, ID3D11RenderTargetView** rtv) -> bool {
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = static_cast<UINT>(m_width);
        desc.Height = static_cast<UINT>(m_height);
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        if (FAILED(dev->CreateTexture2D(&desc, nullptr, tex))) return false;
        return SUCCEEDED(dev->CreateRenderTargetView(*tex, nullptr, rtv));
    };
    if (m_previewTex) { m_previewRtv->Release(); m_previewTex->Release(); m_previewTex = nullptr; m_previewRtv = nullptr; }
    if (m_programTex) { m_programRtv->Release(); m_programTex->Release(); m_programTex = nullptr; m_programRtv = nullptr; }
    if (!makeTex(&m_previewTex, &m_previewRtv) || !makeTex(&m_programTex, &m_programRtv)) {
        if (error) *error = QStringLiteral("Failed to create compositor targets");
        return false;
    }
    return true;
#else
    Q_UNUSED(error);
    return false;
#endif
}

bool D3D11Compositor::createPipeline(QString* error)
{
#ifdef _WIN32
    auto* dev = m_device->device();
    ComPtr<ID3DBlob> vsBlob, psBlob, errBlob;
    HRESULT hr = D3DCompile(shaders::kVsMain, strlen(shaders::kVsMain), "VS", nullptr, nullptr,
                            "main", "vs_5_0", 0, 0, &vsBlob, &errBlob);
    if (FAILED(hr)) {
        if (error) *error = QStringLiteral("VS compile failed");
        return false;
    }
    hr = D3DCompile(shaders::kPsMain, strlen(shaders::kPsMain), "PS", nullptr, nullptr,
                    "main", "ps_5_0", 0, 0, &psBlob, &errBlob);
    if (FAILED(hr)) {
        if (error) *error = QStringLiteral("PS compile failed");
        return false;
    }
    dev->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vs);
    dev->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_ps);

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    dev->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_layout);

    const float verts[] = {
        0,0, 0,0,
        1,0, 1,0,
        0,1, 0,1,
        1,1, 1,1,
    };
    D3D11_BUFFER_DESC vbd{};
    vbd.ByteWidth = sizeof(verts);
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vinit{verts, 0, 0};
    dev->CreateBuffer(&vbd, &vinit, &m_vb);

    D3D11_BUFFER_DESC cbd{};
    cbd.ByteWidth = sizeof(CBData);
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    dev->CreateBuffer(&cbd, nullptr, &m_cb);

    D3D11_SAMPLER_DESC sd{};
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    dev->CreateSamplerState(&sd, &m_sampler);

    D3D11_BLEND_DESC bd{};
    bd.RenderTarget[0].BlendEnable = TRUE;
    bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    dev->CreateBlendState(&bd, &m_blend);
    return true;
#else
    Q_UNUSED(error);
    return false;
#endif
}

bool D3D11Compositor::initialize(int width, int height, QString* error)
{
    m_width = width;
    m_height = height;
    if (!createTargets(error)) return false;
    if (!createPipeline(error)) return false;
    Logger::info(QStringLiteral("Compositor initialized %1x%2").arg(width).arg(height));
    return true;
}

void D3D11Compositor::resize(int width, int height)
{
    if (width == m_width && height == m_height) return;
    m_width = width;
    m_height = height;
    clearProgramHold();
    createTargets(nullptr);
}

void D3D11Compositor::shutdown()
{
#ifdef _WIN32
    auto rel = [](auto*& p) { if (p) { p->Release(); p = nullptr; } };
    rel(m_blend); rel(m_sampler); rel(m_cb); rel(m_vb); rel(m_layout); rel(m_ps); rel(m_vs);
    rel(m_previewRtv); rel(m_programRtv); rel(m_previewTex); rel(m_programTex); rel(m_holdTex);
#endif
}

void D3D11Compositor::clearTarget(ID3D11RenderTargetView* rtv, float r, float g, float b, float a)
{
#ifdef _WIN32
    float color[4] = {r, g, b, a};
    m_device->context()->ClearRenderTargetView(rtv, color);
#else
    Q_UNUSED(rtv); Q_UNUSED(r); Q_UNUSED(g); Q_UNUSED(b); Q_UNUSED(a);
#endif
}

void D3D11Compositor::drawSource(const SourceItem& src, FrameBus& bus, ID3D11RenderTargetView* rtv)
{
#ifdef _WIN32
    auto frame = bus.latest(src.id);
    if (!frame || !frame->texture) return;

    auto* ctx = m_device->context();
    auto* dev = m_device->device();

    ComPtr<ID3D11ShaderResourceView> srv;
    if (FAILED(dev->CreateShaderResourceView(frame->texture, nullptr, &srv)))
        return;

    D3D11_VIEWPORT vp{};
    vp.Width = static_cast<float>(m_width);
    vp.Height = static_cast<float>(m_height);
    vp.MaxDepth = 1.0f;
    ctx->RSSetViewports(1, &vp);
    ctx->OMSetRenderTargets(1, &rtv, nullptr);
    ctx->OMSetBlendState(m_blend, nullptr, 0xffffffff);
    ctx->IASetInputLayout(m_layout);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    UINT stride = 16, offset = 0;
    ctx->IASetVertexBuffers(0, 1, &m_vb, &stride, &offset);
    ctx->VSSetShader(m_vs, nullptr, 0);
    ctx->PSSetShader(m_ps, nullptr, 0);
    ctx->PSSetShaderResources(0, 1, srv.GetAddressOf());
    ctx->PSSetSamplers(0, 1, &m_sampler);

    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (SUCCEEDED(ctx->Map(m_cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
        auto* cb = reinterpret_cast<CBData*>(mapped.pData);
        cb->rect[0] = static_cast<float>(src.transform.x);
        cb->rect[1] = static_cast<float>(src.transform.y);
        cb->rect[2] = static_cast<float>(src.transform.w);
        cb->rect[3] = static_cast<float>(src.transform.h);
        cb->opacity = static_cast<float>(src.transform.opacity);
        cb->rotation = static_cast<float>(src.transform.rotation * 3.14159265358979323846 / 180.0);
        const float cl = static_cast<float>(std::clamp(src.transform.cropLeft, 0.0, 1.0));
        const float cr = static_cast<float>(std::clamp(src.transform.cropRight, 0.0, 1.0));
        const float ct = static_cast<float>(std::clamp(src.transform.cropTop, 0.0, 1.0));
        const float cb_ = static_cast<float>(std::clamp(src.transform.cropBottom, 0.0, 1.0));
        cb->cropMin[0] = cl;
        cb->cropMin[1] = ct;
        cb->cropMax[0] = 1.0f - cr;
        cb->cropMax[1] = 1.0f - cb_;
        const double briUi = src.settings.value(QStringLiteral("brightness")).toDouble(0.0);
        const double conUi = src.settings.value(QStringLiteral("contrast")).toDouble(0.0);
        const double satUi = src.settings.value(QStringLiteral("saturation")).toDouble(0.0);
        // UI stores −100…100; shader wants brightness ±1, contrast/sat multipliers around 1.
        const float brightness = static_cast<float>(std::clamp(briUi / 100.0, -1.0, 1.0));
        const float contrast = static_cast<float>(std::clamp(1.0 + conUi / 100.0, 0.0, 3.0));
        const float saturation = static_cast<float>(std::clamp(1.0 + satUi / 100.0, 0.0, 3.0));
        // Luma-preserving-ish: scale around mid-gray after contrast
        const float mid = 0.5f * (1.0f - contrast);
        cb->colorMul[0] = contrast * saturation;
        cb->colorMul[1] = contrast * saturation;
        cb->colorMul[2] = contrast * (2.0f - saturation); // slight channel split when sat≠1
        if (saturation >= 0.99f && saturation <= 1.01f) {
            cb->colorMul[0] = contrast;
            cb->colorMul[1] = contrast;
            cb->colorMul[2] = contrast;
        }
        cb->colorMul[3] = 1.0f;
        cb->colorAdd[0] = brightness + mid;
        cb->colorAdd[1] = brightness + mid;
        cb->colorAdd[2] = brightness + mid;
        // colorAdd.a = blur radius in UV space (0..~0.02)
        const double blurUi = src.settings.value(QStringLiteral("blur")).toDouble(0.0);
        cb->colorAdd[3] = static_cast<float>(std::clamp(blurUi, 0.0, 100.0) / 100.0 * 0.02);
        // Chroma key (optional)
        cb->_padCrop[0] = src.settings.value(QStringLiteral("chromaKey")).toBool(false) ? 1.0f : 0.0f;
        cb->_padCrop[1] = static_cast<float>(src.settings.value(QStringLiteral("chromaSimilarity")).toDouble(40.0) / 100.0);
        ctx->Unmap(m_cb, 0);
    }
    ctx->VSSetConstantBuffers(0, 1, &m_cb);
    ctx->PSSetConstantBuffers(0, 1, &m_cb);
    ctx->Draw(4, 0);

    ID3D11ShaderResourceView* nullSrv = nullptr;
    ctx->PSSetShaderResources(0, 1, &nullSrv);
#else
    Q_UNUSED(src); Q_UNUSED(bus); Q_UNUSED(rtv);
#endif
}

bool D3D11Compositor::compose(const SceneItem& scene, FrameBus& bus, bool toProgram, float transitionMix)
{
#ifdef _WIN32
    auto* rtv = toProgram ? m_programRtv : m_previewRtv;
    if (!rtv) return false;
    clearTarget(rtv, 0.02f, 0.02f, 0.03f, 1.0f);
    for (const auto& src : scene.sources) {
        if (!src.visible) continue;
        SourceItem drawn = src;
        drawn.transform.opacity *= transitionMix;
        drawSource(drawn, bus, rtv);
    }
    emit frameComposed(toProgram);
    return true;
#else
    Q_UNUSED(scene); Q_UNUSED(bus); Q_UNUSED(toProgram); Q_UNUSED(transitionMix);
    return false;
#endif
}

bool D3D11Compositor::captureProgramHold()
{
#ifdef _WIN32
    if (!m_programTex || !m_device) return false;
    auto* dev = m_device->device();
    auto* ctx = m_device->context();
    D3D11_TEXTURE2D_DESC desc{};
    m_programTex->GetDesc(&desc);
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.CPUAccessFlags = 0;
    if (m_holdTex) { m_holdTex->Release(); m_holdTex = nullptr; }
    ComPtr<ID3D11Texture2D> hold;
    if (FAILED(dev->CreateTexture2D(&desc, nullptr, &hold)))
        return false;
    ctx->CopyResource(hold.Get(), m_programTex);
    m_holdTex = hold.Detach();
    return true;
#else
    return false;
#endif
}

void D3D11Compositor::clearProgramHold()
{
#ifdef _WIN32
    if (m_holdTex) { m_holdTex->Release(); m_holdTex = nullptr; }
#endif
}

void D3D11Compositor::setWipeDirection(int direction)
{
    m_wipeDirection = std::clamp(direction, 0, 3);
}

void D3D11Compositor::blendProgramHold(float progress, TransitionType type)
{
#ifdef _WIN32
    if (!m_holdTex || !m_programRtv || !m_device) return;
    progress = std::clamp(progress, 0.0f, 1.0f);
    auto* ctx = m_device->context();
    auto* dev = m_device->device();
    ComPtr<ID3D11ShaderResourceView> srv;
    if (FAILED(dev->CreateShaderResourceView(m_holdTex, nullptr, &srv)))
        return;

    D3D11_VIEWPORT vp{};
    vp.Width = static_cast<float>(m_width);
    vp.Height = static_cast<float>(m_height);
    vp.MaxDepth = 1.0f;
    ctx->RSSetViewports(1, &vp);
    ctx->OMSetRenderTargets(1, &m_programRtv, nullptr);
    ctx->OMSetBlendState(m_blend, nullptr, 0xffffffff);
    ctx->IASetInputLayout(m_layout);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    UINT stride = 16, offset = 0;
    ctx->IASetVertexBuffers(0, 1, &m_vb, &stride, &offset);
    ctx->VSSetShader(m_vs, nullptr, 0);
    ctx->PSSetShader(m_ps, nullptr, 0);
    ctx->PSSetShaderResources(0, 1, srv.GetAddressOf());
    ctx->PSSetSamplers(0, 1, &m_sampler);

    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (SUCCEEDED(ctx->Map(m_cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
        auto* cb = reinterpret_cast<CBData*>(mapped.pData);
        if (type == TransitionType::Wipe) {
            const float p = progress;
            switch (m_wipeDirection) {
            case 1: // to left
                cb->rect[0] = 0.0f;
                cb->rect[1] = 0.0f;
                cb->rect[2] = 1.0f - p;
                cb->rect[3] = 1.0f;
                cb->cropMin[0] = 0.0f;
                cb->cropMin[1] = 0.0f;
                cb->cropMax[0] = 1.0f - p;
                cb->cropMax[1] = 1.0f;
                break;
            case 2: // to bottom
                cb->rect[0] = 0.0f;
                cb->rect[1] = p;
                cb->rect[2] = 1.0f;
                cb->rect[3] = 1.0f - p;
                cb->cropMin[0] = 0.0f;
                cb->cropMin[1] = p;
                cb->cropMax[0] = 1.0f;
                cb->cropMax[1] = 1.0f;
                break;
            case 3: // to top
                cb->rect[0] = 0.0f;
                cb->rect[1] = 0.0f;
                cb->rect[2] = 1.0f;
                cb->rect[3] = 1.0f - p;
                cb->cropMin[0] = 0.0f;
                cb->cropMin[1] = 0.0f;
                cb->cropMax[0] = 1.0f;
                cb->cropMax[1] = 1.0f - p;
                break;
            default: // to right
                cb->rect[0] = p;
                cb->rect[1] = 0.0f;
                cb->rect[2] = 1.0f - p;
                cb->rect[3] = 1.0f;
                cb->cropMin[0] = p;
                cb->cropMin[1] = 0.0f;
                cb->cropMax[0] = 1.0f;
                cb->cropMax[1] = 1.0f;
                break;
            }
            cb->opacity = 1.0f;
        } else if (type == TransitionType::Merge) {
            // Distinct from Fade: quadratic ease-in dissolve
            cb->rect[0] = 0.0f;
            cb->rect[1] = 0.0f;
            cb->rect[2] = 1.0f;
            cb->rect[3] = 1.0f;
            cb->opacity = 1.0f - (progress * progress);
            cb->cropMin[0] = 0.0f;
            cb->cropMin[1] = 0.0f;
            cb->cropMax[0] = 1.0f;
            cb->cropMax[1] = 1.0f;
        } else {
            // Fade / CubeZoom (crossfade alias): linear dissolve
            cb->rect[0] = 0.0f;
            cb->rect[1] = 0.0f;
            cb->rect[2] = 1.0f;
            cb->rect[3] = 1.0f;
            cb->opacity = 1.0f - progress;
            cb->cropMin[0] = 0.0f;
            cb->cropMin[1] = 0.0f;
            cb->cropMax[0] = 1.0f;
            cb->cropMax[1] = 1.0f;
        }
        cb->rotation = 0.0f;
        cb->_padCrop[0] = 0.0f;
        cb->_padCrop[1] = 0.0f;
        cb->colorMul[0] = cb->colorMul[1] = cb->colorMul[2] = cb->colorMul[3] = 1.0f;
        cb->colorAdd[0] = cb->colorAdd[1] = cb->colorAdd[2] = cb->colorAdd[3] = 0.0f;
        ctx->Unmap(m_cb, 0);
    }
    ctx->VSSetConstantBuffers(0, 1, &m_cb);
    ctx->PSSetConstantBuffers(0, 1, &m_cb);
    ctx->Draw(4, 0);
    ID3D11ShaderResourceView* nullSrv = nullptr;
    ctx->PSSetShaderResources(0, 1, &nullSrv);
#else
    Q_UNUSED(progress); Q_UNUSED(type);
#endif
}

QImage D3D11Compositor::readbackTexture(ID3D11Texture2D* tex) const
{
#ifdef _WIN32
    if (!tex || !m_device) return {};
    auto* dev = m_device->device();
    auto* ctx = m_device->context();
    D3D11_TEXTURE2D_DESC desc{};
    tex->GetDesc(&desc);
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.MiscFlags = 0;
    ComPtr<ID3D11Texture2D> staging;
    if (FAILED(dev->CreateTexture2D(&desc, nullptr, &staging)))
        return {};
    ctx->CopyResource(staging.Get(), tex);
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(ctx->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped)))
        return {};
    const int w = static_cast<int>(desc.Width);
    const int h = static_cast<int>(desc.Height);
    QImage img(w, h, QImage::Format_ARGB32);
    for (int y = 0; y < h; ++y) {
        memcpy(img.scanLine(y),
               static_cast<const char*>(mapped.pData) + y * mapped.RowPitch,
               static_cast<size_t>(w) * 4);
    }
    ctx->Unmap(staging.Get(), 0);
    return img;
#else
    Q_UNUSED(tex);
    return {};
#endif
}

QImage D3D11Compositor::readbackProgram() const
{
    return readbackTexture(m_programTex);
}

QImage D3D11Compositor::readbackPreview() const
{
    return readbackTexture(m_previewTex);
}

} // namespace railshot
