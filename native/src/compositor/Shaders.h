#pragma once

// Embedded HLSL bytecode would be ideal; for MVP we compile at runtime via D3DCompile.

namespace railshot {
namespace shaders {

inline constexpr char kVsMain[] = R"(
struct VSIn { float2 pos : POSITION; float2 uv : TEXCOORD0; };
struct VSOut { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };
cbuffer CB : register(b0) {
    float4 rect;      // x,y,w,h in 0..1 canvas space
    float opacity;
    float rotation;   // radians
    float2 cropMin;   // UV crop min
    float2 cropMax;   // UV crop max
    float2 _padCrop;
    float4 colorMul;  // rgb mul + pad
    float4 colorAdd;  // brightness bias in rgb
};
VSOut main(VSIn i) {
    VSOut o;
    float2 local = float2(i.pos.x - 0.5, i.pos.y - 0.5);
    float s = sin(rotation);
    float c = cos(rotation);
    float2 rotated = float2(local.x * c - local.y * s, local.x * s + local.y * c);
    float2 p = float2(rect.x + (rotated.x + 0.5) * rect.z, rect.y + (rotated.y + 0.5) * rect.w);
    float2 clip = float2(p.x * 2.0 - 1.0, 1.0 - p.y * 2.0);
    o.pos = float4(clip, 0, 1);
    o.uv = lerp(cropMin, cropMax, i.uv);
    return o;
}
)";

inline constexpr char kPsMain[] = R"(
Texture2D tex : register(t0);
SamplerState samp : register(s0);
cbuffer CB : register(b0) {
    float4 rect;
    float opacity;
    float rotation;
    float2 cropMin;
    float2 cropMax;
    float2 _padCrop;
    float4 colorMul;
    float4 colorAdd;
};
float4 main(float4 pos : SV_POSITION, float2 uv : TEXCOORD0) : SV_TARGET {
    float blur = colorAdd.a;
    float4 c = tex.Sample(samp, uv);
    if (blur > 0.0005) {
        float4 acc = c;
        acc += tex.Sample(samp, uv + float2( blur, 0));
        acc += tex.Sample(samp, uv + float2(-blur, 0));
        acc += tex.Sample(samp, uv + float2(0,  blur));
        acc += tex.Sample(samp, uv + float2(0, -blur));
        acc += tex.Sample(samp, uv + float2( blur,  blur) * 0.707);
        acc += tex.Sample(samp, uv + float2(-blur,  blur) * 0.707);
        acc += tex.Sample(samp, uv + float2( blur, -blur) * 0.707);
        acc += tex.Sample(samp, uv + float2(-blur, -blur) * 0.707);
        c = acc * (1.0 / 9.0);
    }
    c.rgb = c.rgb * colorMul.rgb + colorAdd.rgb;
    // chroma key when _padCrop.x > 0.5 (green screen default)
    if (_padCrop.x > 0.5) {
        float g = c.g;
        float rb = (c.r + c.b) * 0.5;
        float key = saturate((g - rb - (1.0 - _padCrop.y) * 0.35) / max(0.05, _padCrop.y));
        c.a *= (1.0 - key);
    }
    c.a *= opacity;
    return c;
}
)";

} // namespace shaders
} // namespace railshot
