#include "compositor/D3D11Compositor.h"
#include "compositor/D3D11Device.h"
#include "compositor/LutCubeLoader.h"
#include "compositor/Shaders.h"
#include "core/Project.h"
#include "core/Logger.h"
#include <QElapsedTimer>
#include <QJsonArray>
#include <QColor>
#include <QImage>
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
    float _padCrop[2];   // 40 — unused (legacy)
    float colorMul[4];   // 48
    float colorAdd[4];   // 64 — rgb + blur in .a
    float fxParams[4];   // 80 — scrollU, scrollV, sharpen, maskOpacity
    float keyColor[4];   // 96 — rgb + mode (0 off, 1 chroma, 2 color, 3 luma)
    float keyParams[4];  // 112 — sim, smooth, lumaMin, lumaMax
};

struct alignas(16) TransCBData {
    float progress;
    float mode;
    float aspect;
    float wipeDir;
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
    if (m_transScratch) { m_transScratch->Release(); m_transScratch = nullptr; }
    if (!makeTex(&m_previewTex, &m_previewRtv) || !makeTex(&m_programTex, &m_programRtv)) {
        if (error) *error = QStringLiteral("Failed to create compositor targets");
        return false;
    }
    // Scratch holds a copy of the new program during A/B blends (SRV only).
    {
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = static_cast<UINT>(m_width);
        desc.Height = static_cast<UINT>(m_height);
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        if (FAILED(dev->CreateTexture2D(&desc, nullptr, &m_transScratch))) {
            if (error) *error = QStringLiteral("Failed to create transition scratch");
            return false;
        }
    }
    if (!ensureNestTargets()) {
        if (error) *error = QStringLiteral("Failed to create nest targets");
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
    ComPtr<ID3DBlob> transPsBlob;
    errBlob.Reset();
    hr = D3DCompile(shaders::kPsTransition, strlen(shaders::kPsTransition), "PSTrans", nullptr, nullptr,
                    "main", "ps_5_0", 0, 0, &transPsBlob, &errBlob);
    if (FAILED(hr)) {
        QString detail = QStringLiteral("Transition PS compile failed");
        if (errBlob) {
            detail += QStringLiteral(": ");
            detail += QString::fromUtf8(static_cast<const char*>(errBlob->GetBufferPointer()),
                                        int(errBlob->GetBufferSize()));
        }
        if (error) *error = detail;
        Logger::error(detail);
        return false;
    }
    dev->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vs);
    dev->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_ps);
    dev->CreatePixelShader(transPsBlob->GetBufferPointer(), transPsBlob->GetBufferSize(), nullptr, &m_transPs);

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

    D3D11_BUFFER_DESC tcb{};
    tcb.ByteWidth = sizeof(TransCBData);
    tcb.Usage = D3D11_USAGE_DYNAMIC;
    tcb.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    tcb.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    dev->CreateBuffer(&tcb, nullptr, &m_transCb);

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

    D3D11_BLEND_DESC obd{};
    obd.RenderTarget[0].BlendEnable = FALSE;
    obd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    dev->CreateBlendState(&obd, &m_blendOpaque);
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
    m_fxClock.start();
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
    clearMaskCache();
    auto rel = [](auto*& p) { if (p) { p->Release(); p = nullptr; } };
    rel(m_blendOpaque); rel(m_blend); rel(m_sampler); rel(m_transCb); rel(m_cb); rel(m_vb);
    rel(m_layout); rel(m_transPs); rel(m_ps); rel(m_vs);
    rel(m_previewRtv); rel(m_programRtv); rel(m_nestRtv);
    rel(m_previewTex); rel(m_programTex); rel(m_holdTex); rel(m_transScratch); rel(m_nestTex);
#endif
}

void D3D11Compositor::clearMaskCache()
{
#ifdef _WIN32
    for (auto it = m_maskCache.begin(); it != m_maskCache.end(); ++it) {
        if (it->srv) { it->srv->Release(); it->srv = nullptr; }
        if (it->tex) { it->tex->Release(); it->tex = nullptr; }
    }
    m_maskCache.clear();
#endif
}

ID3D11ShaderResourceView* D3D11Compositor::ensureMaskSrv(const QString& path)
{
#ifdef _WIN32
    if (path.isEmpty() || !m_device || !m_device->device())
        return nullptr;
    auto it = m_maskCache.find(path);
    if (it != m_maskCache.end() && it->srv)
        return it->srv;

    QImage img;
    if (path.endsWith(QLatin1String(".cube"), Qt::CaseInsensitive)) {
        QString err;
        img = LutCubeLoader::loadAsObsPng(path, &err);
        if (img.isNull()) {
            Logger::warn(err.isEmpty() ? QStringLiteral("Failed to load .cube LUT: %1").arg(path) : err);
            return nullptr;
        }
    } else {
        img = QImage(path);
        if (img.isNull())
            return nullptr;
        img = img.convertToFormat(QImage::Format_ARGB32);
    }
    auto* dev = m_device->device();

    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = static_cast<UINT>(img.width());
    desc.Height = static_cast<UINT>(img.height());
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA init{};
    init.pSysMem = img.bits();
    init.SysMemPitch = static_cast<UINT>(img.bytesPerLine());

    ComPtr<ID3D11Texture2D> tex;
    if (FAILED(dev->CreateTexture2D(&desc, &init, &tex)))
        return nullptr;
    ComPtr<ID3D11ShaderResourceView> srv;
    if (FAILED(dev->CreateShaderResourceView(tex.Get(), nullptr, &srv)))
        return nullptr;

    MaskEntry entry;
    entry.tex = tex.Detach();
    entry.srv = srv.Detach();
    m_maskCache.insert(path, entry);
    return entry.srv;
#else
    Q_UNUSED(path);
    return nullptr;
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
    if (src.type == SourceType::Scene || src.type == SourceType::Group) {
        drawSceneOrGroup(src, bus, rtv);
        return;
    }
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
    ctx->PSSetSamplers(0, 1, &m_sampler);

    const QString maskPath = src.settings.value(QStringLiteral("maskPath")).toString();
    const float maskOpacity = static_cast<float>(
        std::clamp(src.settings.value(QStringLiteral("maskOpacity")).toDouble(0.0), 0.0, 100.0) / 100.0);
    const bool maskInvert = src.settings.value(QStringLiteral("maskInvert")).toBool(false);
    ID3D11ShaderResourceView* maskSrv = nullptr;
    if (maskOpacity > 0.001f && !maskPath.isEmpty())
        maskSrv = ensureMaskSrv(maskPath);

    const QString lutPath = src.settings.value(QStringLiteral("lutPath")).toString();
    const float lutAmount = static_cast<float>(
        std::clamp(src.settings.value(QStringLiteral("lutAmount")).toDouble(0.0), 0.0, 100.0) / 100.0);
    ID3D11ShaderResourceView* lutSrv = nullptr;
    if (lutAmount > 0.001f && !lutPath.isEmpty())
        lutSrv = ensureMaskSrv(lutPath); // same QImage→Texture2D cache

    ID3D11ShaderResourceView* srvs[3] = { srv.Get(), maskSrv, lutSrv };
    ctx->PSSetShaderResources(0, 3, srvs);

    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (SUCCEEDED(ctx->Map(m_cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
        auto* cb = reinterpret_cast<CBData*>(mapped.pData);
        float rx = static_cast<float>(src.transform.x);
        float ry = static_cast<float>(src.transform.y);
        float rw = static_cast<float>(src.transform.w);
        float rh = static_cast<float>(src.transform.h);

        const float fL = static_cast<float>(std::clamp(src.settings.value(QStringLiteral("filterCropL")).toDouble(0.0), 0.0, 50.0) / 100.0);
        const float fR = static_cast<float>(std::clamp(src.settings.value(QStringLiteral("filterCropR")).toDouble(0.0), 0.0, 50.0) / 100.0);
        const float fT = static_cast<float>(std::clamp(src.settings.value(QStringLiteral("filterCropT")).toDouble(0.0), 0.0, 50.0) / 100.0);
        const float fB = static_cast<float>(std::clamp(src.settings.value(QStringLiteral("filterCropB")).toDouble(0.0), 0.0, 50.0) / 100.0);
        const bool pad = src.settings.value(QStringLiteral("filterPad")).toBool(false);

        float cl = static_cast<float>(std::clamp(src.transform.cropLeft, 0.0, 1.0));
        float cr = static_cast<float>(std::clamp(src.transform.cropRight, 0.0, 1.0));
        float ct = static_cast<float>(std::clamp(src.transform.cropTop, 0.0, 1.0));
        float cbot = static_cast<float>(std::clamp(src.transform.cropBottom, 0.0, 1.0));

        if (pad) {
            rx += rw * fL;
            ry += rh * fT;
            rw *= std::max(0.01f, 1.0f - fL - fR);
            rh *= std::max(0.01f, 1.0f - fT - fB);
        } else {
            const float u0 = cl, u1 = 1.0f - cr, v0 = ct, v1 = 1.0f - cbot;
            const float uw = std::max(0.01f, u1 - u0);
            const float vh = std::max(0.01f, v1 - v0);
            cl = u0 + uw * fL;
            cr = 1.0f - (u0 + uw * (1.0f - fR));
            ct = v0 + vh * fT;
            cbot = 1.0f - (v0 + vh * (1.0f - fB));
        }

        cb->rect[0] = rx;
        cb->rect[1] = ry;
        cb->rect[2] = rw;
        cb->rect[3] = rh;
        cb->opacity = static_cast<float>(src.transform.opacity);
        cb->rotation = static_cast<float>(src.transform.rotation * 3.14159265358979323846 / 180.0);
        cb->cropMin[0] = cl;
        cb->cropMin[1] = ct;
        cb->cropMax[0] = 1.0f - cr;
        cb->cropMax[1] = 1.0f - cbot;
        const double briUi = src.settings.value(QStringLiteral("brightness")).toDouble(0.0);
        const double conUi = src.settings.value(QStringLiteral("contrast")).toDouble(0.0);
        const double satUi = src.settings.value(QStringLiteral("saturation")).toDouble(0.0);
        const float brightness = static_cast<float>(std::clamp(briUi / 100.0, -1.0, 1.0));
        const float contrast = static_cast<float>(std::clamp(1.0 + conUi / 100.0, 0.0, 3.0));
        const float saturation = static_cast<float>(std::clamp(1.0 + satUi / 100.0, 0.0, 3.0));
        const float mid = 0.5f * (1.0f - contrast);
        cb->colorMul[0] = contrast * saturation;
        cb->colorMul[1] = contrast * saturation;
        cb->colorMul[2] = contrast * (2.0f - saturation);
        if (saturation >= 0.99f && saturation <= 1.01f) {
            cb->colorMul[0] = contrast;
            cb->colorMul[1] = contrast;
            cb->colorMul[2] = contrast;
        }
        cb->colorMul[3] = 1.0f;
        cb->colorAdd[0] = brightness + mid;
        cb->colorAdd[1] = brightness + mid;
        cb->colorAdd[2] = brightness + mid;
        const double blurUi = src.settings.value(QStringLiteral("blur")).toDouble(0.0);
        cb->colorAdd[3] = static_cast<float>(std::clamp(blurUi, 0.0, 100.0) / 100.0 * 0.02);
        cb->_padCrop[0] = (maskSrv && maskInvert) ? 1.0f : 0.0f;
        cb->_padCrop[1] = lutSrv ? lutAmount : 0.0f;

        // Key filters: mode 0=off 1=chroma 2=color 3=luma
        const int keyMode = src.settings.value(QStringLiteral("keyMode")).toInt(0);
        QColor keyRgb(src.settings.value(QStringLiteral("keyColor")).toString(QStringLiteral("#00FF00")));
        if (!keyRgb.isValid())
            keyRgb = QColor(0, 255, 0);
        cb->keyColor[0] = keyRgb.redF();
        cb->keyColor[1] = keyRgb.greenF();
        cb->keyColor[2] = keyRgb.blueF();
        cb->keyColor[3] = float(keyMode);
        // similarity UI 0..100 → 0..1 (OBS uses /1000 with ~400 default ≈ 0.4)
        cb->keyParams[0] = static_cast<float>(
            std::clamp(src.settings.value(QStringLiteral("chromaSimilarity")).toDouble(40.0), 0.0, 100.0) / 100.0);
        // smoothness UI 1..100 → map as (v*10)/1000 = v/100 (OBS default 80 → store 8)
        cb->keyParams[1] = static_cast<float>(
            std::clamp(src.settings.value(QStringLiteral("keySmoothness")).toDouble(8.0), 0.1, 100.0) / 100.0);
        cb->keyParams[2] = static_cast<float>(
            std::clamp(src.settings.value(QStringLiteral("lumaMin")).toDouble(0.0), 0.0, 100.0) / 100.0);
        cb->keyParams[3] = static_cast<float>(
            std::clamp(src.settings.value(QStringLiteral("lumaMax")).toDouble(100.0), 0.0, 100.0) / 100.0);

        const double tSec = m_fxClock.isValid() ? m_fxClock.elapsed() / 1000.0 : 0.0;
        const double sx = src.settings.value(QStringLiteral("scrollSpeedX")).toDouble(0.0);
        const double sy = src.settings.value(QStringLiteral("scrollSpeedY")).toDouble(0.0);
        // speed −100..100 → cycles/sec scale
        cb->fxParams[0] = static_cast<float>(sx / 100.0 * tSec);
        cb->fxParams[1] = static_cast<float>(sy / 100.0 * tSec);
        cb->fxParams[2] = static_cast<float>(std::clamp(src.settings.value(QStringLiteral("sharpen")).toDouble(0.0), 0.0, 100.0) / 100.0);
        cb->fxParams[3] = maskSrv ? maskOpacity : 0.0f;
        ctx->Unmap(m_cb, 0);
    }
    ctx->VSSetConstantBuffers(0, 1, &m_cb);
    ctx->PSSetConstantBuffers(0, 1, &m_cb);
    ctx->Draw(4, 0);

    ID3D11ShaderResourceView* nullSrvs[3] = { nullptr, nullptr, nullptr };
    ctx->PSSetShaderResources(0, 3, nullSrvs);
#else
    Q_UNUSED(src); Q_UNUSED(bus); Q_UNUSED(rtv);
#endif
}

bool D3D11Compositor::compose(const SceneItem& scene, FrameBus& bus, bool toProgram, float transitionMix,
                              float clearR, float clearG, float clearB, const Project* project)
{
#ifdef _WIN32
    auto* rtv = toProgram ? m_programRtv : m_previewRtv;
    if (!rtv) return false;
    m_project = project;
    const int savedDepth = m_nestDepth;
    clearTarget(rtv, clearR, clearG, clearB, 1.0f);
    const QSet<QString> grouped = groupedChildIds(scene);
    for (const auto& src : scene.sources) {
        if (!src.visible) continue;
        if (grouped.contains(src.id)) continue; // drawn via parent Group
        SourceItem drawn = src;
        drawn.transform.opacity *= transitionMix;
        drawSource(drawn, bus, rtv);
    }
    m_nestDepth = savedDepth;
    emit frameComposed(toProgram);
    return true;
#else
    Q_UNUSED(scene); Q_UNUSED(bus); Q_UNUSED(toProgram); Q_UNUSED(transitionMix);
    Q_UNUSED(clearR); Q_UNUSED(clearG); Q_UNUSED(clearB); Q_UNUSED(project);
    return false;
#endif
}

bool D3D11Compositor::ensureNestTargets()
{
#ifdef _WIN32
    if (!m_device || !m_device->device()) return false;
    if (m_nestTex && m_nestRtv) return true;
    auto* dev = m_device->device();
    if (m_nestRtv) { m_nestRtv->Release(); m_nestRtv = nullptr; }
    if (m_nestTex) { m_nestTex->Release(); m_nestTex = nullptr; }
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = static_cast<UINT>(m_width);
    desc.Height = static_cast<UINT>(m_height);
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    if (FAILED(dev->CreateTexture2D(&desc, nullptr, &m_nestTex)))
        return false;
    return SUCCEEDED(dev->CreateRenderTargetView(m_nestTex, nullptr, &m_nestRtv));
#else
    return false;
#endif
}

QSet<QString> D3D11Compositor::groupedChildIds(const SceneItem& scene) const
{
    QSet<QString> ids;
    for (const auto& src : scene.sources) {
        if (src.type != SourceType::Group) continue;
        const auto arr = src.settings.value(QStringLiteral("childIds")).toArray();
        for (const auto& v : arr)
            ids.insert(v.toString());
    }
    return ids;
}

Transform D3D11Compositor::combineTransform(const Transform& parent, const Transform& child) const
{
    Transform out = child;
    out.x = parent.x + child.x * parent.w;
    out.y = parent.y + child.y * parent.h;
    out.w = child.w * parent.w;
    out.h = child.h * parent.h;
    out.opacity = child.opacity * parent.opacity;
    out.rotation = child.rotation + parent.rotation;
    return out;
}

void D3D11Compositor::drawTexture(ID3D11Texture2D* tex, const Transform& xf, float opacityMul,
                                  ID3D11RenderTargetView* rtv)
{
#ifdef _WIN32
    if (!tex || !rtv || !m_device) return;
    auto* ctx = m_device->context();
    auto* dev = m_device->device();
    ComPtr<ID3D11ShaderResourceView> srv;
    if (FAILED(dev->CreateShaderResourceView(tex, nullptr, &srv)))
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
        cb->rect[0] = float(xf.x);
        cb->rect[1] = float(xf.y);
        cb->rect[2] = float(xf.w);
        cb->rect[3] = float(xf.h);
        cb->opacity = float(xf.opacity * opacityMul);
        cb->rotation = float(xf.rotation * 3.14159265358979323846 / 180.0);
        cb->cropMin[0] = 0; cb->cropMin[1] = 0;
        cb->cropMax[0] = 1; cb->cropMax[1] = 1;
        cb->_padCrop[0] = 0; cb->_padCrop[1] = 0;
        cb->colorMul[0] = cb->colorMul[1] = cb->colorMul[2] = cb->colorMul[3] = 1.0f;
        cb->colorAdd[0] = cb->colorAdd[1] = cb->colorAdd[2] = cb->colorAdd[3] = 0.0f;
        cb->fxParams[0] = cb->fxParams[1] = cb->fxParams[2] = cb->fxParams[3] = 0.0f;
        cb->keyColor[0] = cb->keyColor[1] = cb->keyColor[2] = cb->keyColor[3] = 0.0f;
        cb->keyParams[0] = cb->keyParams[1] = cb->keyParams[2] = 0.0f;
        cb->keyParams[3] = 1.0f;
        ctx->Unmap(m_cb, 0);
    }
    ctx->VSSetConstantBuffers(0, 1, &m_cb);
    ctx->PSSetConstantBuffers(0, 1, &m_cb);
    ctx->Draw(4, 0);
    ID3D11ShaderResourceView* nullSrv = nullptr;
    ctx->PSSetShaderResources(0, 1, &nullSrv);
#else
    Q_UNUSED(tex); Q_UNUSED(xf); Q_UNUSED(opacityMul); Q_UNUSED(rtv);
#endif
}

void D3D11Compositor::drawSceneOrGroup(const SourceItem& src, FrameBus& bus, ID3D11RenderTargetView* rtv)
{
#ifdef _WIN32
    if (!m_project || m_nestDepth >= 4) return;

    if (src.type == SourceType::Group) {
        // Find owning scene that contains this group (search all scenes).
        const SceneItem* host = nullptr;
        for (const auto& sc : m_project->scenes) {
            for (const auto& s : sc.sources) {
                if (s.id == src.id) { host = &sc; break; }
            }
            if (host) break;
        }
        if (!host) return;
        const auto arr = src.settings.value(QStringLiteral("childIds")).toArray();
        for (const auto& v : arr) {
            const QString cid = v.toString();
            const SourceItem* child = nullptr;
            for (const auto& s : host->sources) {
                if (s.id == cid) { child = &s; break; }
            }
            if (!child || !child->visible) continue;
            SourceItem drawn = *child;
            drawn.transform = combineTransform(src.transform, child->transform);
            drawSource(drawn, bus, rtv);
        }
        return;
    }

    // Scene-as-source
    const QString sceneId = src.settings.value(QStringLiteral("sceneId")).toString();
    const SceneItem* nested = m_project->findScene(sceneId);
    if (!nested) return;
    // Prevent trivial self-embed
    for (const auto& sc : m_project->scenes) {
        for (const auto& s : sc.sources) {
            if (s.id == src.id && sc.id == sceneId)
                return;
        }
    }
    if (!ensureNestTargets() || !m_nestTex || !m_nestRtv) return;
    ++m_nestDepth;
    clearTarget(m_nestRtv, 0.f, 0.f, 0.f, 0.f);
    const QSet<QString> grouped = groupedChildIds(*nested);
    for (const auto& child : nested->sources) {
        if (!child.visible) continue;
        if (grouped.contains(child.id)) continue;
        drawSource(child, bus, m_nestRtv);
    }
    --m_nestDepth;
    drawTexture(m_nestTex, src.transform, 1.0f, rtv);
#else
    Q_UNUSED(src); Q_UNUSED(bus); Q_UNUSED(rtv);
#endif
}

void D3D11Compositor::drawFullscreenOverlay(ID3D11Texture2D* tex, float opacity, bool toProgram)
{
#ifdef _WIN32
    auto* rtv = toProgram ? m_programRtv : m_previewRtv;
    Transform xf;
    xf.x = 0; xf.y = 0; xf.w = 1; xf.h = 1;
    xf.opacity = 1.0;
    drawTexture(tex, xf, opacity, rtv);
#else
    Q_UNUSED(tex); Q_UNUSED(opacity); Q_UNUSED(toProgram);
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
    if (!m_holdTex || !m_programRtv || !m_device || !m_transScratch || !m_transPs || !m_transCb)
        return;
    const int mode = transitionShaderMode(type);
    if (mode <= 0)
        return;
    progress = std::clamp(progress, 0.0f, 1.0f);
    auto* ctx = m_device->context();
    auto* dev = m_device->device();

    // Snapshot the newly composed program so we can sample both A and B.
    ctx->CopyResource(m_transScratch, m_programTex);

    ComPtr<ID3D11ShaderResourceView> srvOld;
    ComPtr<ID3D11ShaderResourceView> srvNew;
    if (FAILED(dev->CreateShaderResourceView(m_holdTex, nullptr, &srvOld)))
        return;
    if (FAILED(dev->CreateShaderResourceView(m_transScratch, nullptr, &srvNew)))
        return;

    D3D11_VIEWPORT vp{};
    vp.Width = static_cast<float>(m_width);
    vp.Height = static_cast<float>(m_height);
    vp.MaxDepth = 1.0f;
    ctx->RSSetViewports(1, &vp);
    ctx->OMSetRenderTargets(1, &m_programRtv, nullptr);
    ctx->OMSetBlendState(m_blendOpaque, nullptr, 0xffffffff);
    ctx->IASetInputLayout(m_layout);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    UINT stride = 16, offset = 0;
    ctx->IASetVertexBuffers(0, 1, &m_vb, &stride, &offset);
    ctx->VSSetShader(m_vs, nullptr, 0);
    ctx->PSSetShader(m_transPs, nullptr, 0);

    ID3D11ShaderResourceView* srvs[2] = { srvOld.Get(), srvNew.Get() };
    ctx->PSSetShaderResources(0, 2, srvs);
    ctx->PSSetSamplers(0, 1, &m_sampler);

    // Fullscreen rect via the shared VS constant buffer.
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (SUCCEEDED(ctx->Map(m_cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
        auto* cb = reinterpret_cast<CBData*>(mapped.pData);
        cb->rect[0] = 0.0f;
        cb->rect[1] = 0.0f;
        cb->rect[2] = 1.0f;
        cb->rect[3] = 1.0f;
        cb->opacity = 1.0f;
        cb->rotation = 0.0f;
        cb->cropMin[0] = 0.0f;
        cb->cropMin[1] = 0.0f;
        cb->cropMax[0] = 1.0f;
        cb->cropMax[1] = 1.0f;
        cb->_padCrop[0] = 0.0f;
        cb->_padCrop[1] = 0.0f;
        cb->colorMul[0] = cb->colorMul[1] = cb->colorMul[2] = cb->colorMul[3] = 1.0f;
        cb->colorAdd[0] = cb->colorAdd[1] = cb->colorAdd[2] = cb->colorAdd[3] = 0.0f;
        cb->fxParams[0] = cb->fxParams[1] = cb->fxParams[2] = cb->fxParams[3] = 0.0f;
        cb->keyColor[0] = cb->keyColor[1] = cb->keyColor[2] = cb->keyColor[3] = 0.0f;
        cb->keyParams[0] = cb->keyParams[1] = cb->keyParams[2] = 0.0f;
        cb->keyParams[3] = 1.0f;
        ctx->Unmap(m_cb, 0);
    }
    if (SUCCEEDED(ctx->Map(m_transCb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
        auto* tcb = reinterpret_cast<TransCBData*>(mapped.pData);
        tcb->progress = progress;
        tcb->mode = static_cast<float>(mode);
        tcb->aspect = m_height > 0 ? float(m_width) / float(m_height) : 1.0f;
        tcb->wipeDir = static_cast<float>(m_wipeDirection);
        ctx->Unmap(m_transCb, 0);
    }
    ctx->VSSetConstantBuffers(0, 1, &m_cb);
    ctx->PSSetConstantBuffers(0, 1, &m_transCb);
    ctx->Draw(4, 0);

    ID3D11ShaderResourceView* nullSrvs[2] = { nullptr, nullptr };
    ctx->PSSetShaderResources(0, 2, nullSrvs);
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
