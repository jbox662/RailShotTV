#pragma once

// Embedded HLSL bytecode would be ideal; for MVP we compile at runtime via D3DCompile.

namespace railshot {
namespace shaders {

inline constexpr char kVsMain[] = R"(
struct VSIn { float2 pos : POSITION; float2 uv : TEXCOORD0; };
struct VSOut { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };
cbuffer CB : register(b0) {
    float4 rect; // x,y,w,h in NDC-ish 0..1 mapped in VS
    float opacity;
    float3 _pad;
};
VSOut main(VSIn i) {
    VSOut o;
    float2 p = float2(rect.x + i.pos.x * rect.z, rect.y + i.pos.y * rect.w);
    // Convert 0..1 top-left to clip space
    float2 clip = float2(p.x * 2.0 - 1.0, 1.0 - p.y * 2.0);
    o.pos = float4(clip, 0, 1);
    o.uv = i.uv;
    return o;
}
)";

inline constexpr char kPsMain[] = R"(
Texture2D tex : register(t0);
SamplerState samp : register(s0);
cbuffer CB : register(b0) {
    float4 rect;
    float opacity;
    float3 _pad;
};
float4 main(float4 pos : SV_POSITION, float2 uv : TEXCOORD0) : SV_TARGET {
    float4 c = tex.Sample(samp, uv);
    c.a *= opacity;
    return c;
}
)";

} // namespace shaders
} // namespace railshot
