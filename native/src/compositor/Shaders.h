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
    float4 colorAdd;  // brightness bias in rgb + blur in a
    float4 fxParams;  // scrollU, scrollV, sharpen, unused
    float4 keyColor;  // rgb key + mode (0 off, 1 chroma, 2 color, 3 luma)
    float4 keyParams; // similarity, smoothness, lumaMin, lumaMax
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
    float4 fxParams;
    float4 keyColor;
    float4 keyParams;
};
float2 rgbToCbCr(float3 rgb) {
    float cb = dot(rgb, float3(-0.100644, -0.338572, 0.439216)) + 0.501961;
    float cr = dot(rgb, float3( 0.439216, -0.398942,-0.040274)) + 0.501961;
    return float2(cb, cr);
}
float4 main(float4 pos : SV_POSITION, float2 uv : TEXCOORD0) : SV_TARGET {
    float2 suv = frac(uv + fxParams.xy);
    float blur = colorAdd.a;
    float4 c = tex.Sample(samp, suv);
    if (blur > 0.0005) {
        float4 acc = c;
        acc += tex.Sample(samp, suv + float2( blur, 0));
        acc += tex.Sample(samp, suv + float2(-blur, 0));
        acc += tex.Sample(samp, suv + float2(0,  blur));
        acc += tex.Sample(samp, suv + float2(0, -blur));
        acc += tex.Sample(samp, suv + float2( blur,  blur) * 0.707);
        acc += tex.Sample(samp, suv + float2(-blur,  blur) * 0.707);
        acc += tex.Sample(samp, suv + float2( blur, -blur) * 0.707);
        acc += tex.Sample(samp, suv + float2(-blur, -blur) * 0.707);
        c = acc * (1.0 / 9.0);
    }
    float sharpen = fxParams.z;
    if (sharpen > 0.001) {
        float r = 0.0025;
        float4 neigh = tex.Sample(samp, suv + float2( r, 0));
        neigh += tex.Sample(samp, suv + float2(-r, 0));
        neigh += tex.Sample(samp, suv + float2(0,  r));
        neigh += tex.Sample(samp, suv + float2(0, -r));
        neigh *= 0.25;
        float4 sharpBase = tex.Sample(samp, suv);
        c = lerp(c, sharpBase + (sharpBase - neigh) * (sharpen * 2.5), saturate(sharpen * 2.0));
    }
    c.rgb = c.rgb * colorMul.rgb + colorAdd.rgb;
    float mode = keyColor.w;
    if (mode > 0.5) {
        float sim = keyParams.x;
        float sm = max(0.001, keyParams.y);
        if (mode < 1.5) {
            // Chroma key (Cb/Cr distance + spill suppress)
            float dist = distance(rgbToCbCr(c.rgb), rgbToCbCr(keyColor.rgb));
            float mask = pow(saturate((dist - sim) / sm), 1.5);
            float spill = pow(saturate((dist - sim) / max(sm, 0.05)), 1.5);
            float desat = dot(c.rgb, float3(0.2126, 0.7152, 0.0722));
            c.rgb = lerp(float3(desat, desat, desat), c.rgb, spill);
            c.a *= mask;
        } else if (mode < 2.5) {
            // Color key (RGB distance)
            float dist = distance(c.rgb, keyColor.rgb);
            c.a *= saturate(max(dist - sim, 0.0) / sm);
        } else {
            // Luma key — keep mid luminance band
            float luma = dot(c.rgb, float3(0.2126, 0.7152, 0.0722));
            float lo = keyParams.z;
            float hi = keyParams.w;
            float clo = smoothstep(lo, lo + sm, luma);
            float chi = 1.0 - smoothstep(hi - sm, hi, luma);
            c.a *= clo * chi;
        }
    }
    c.a *= opacity;
    return c;
}
)";

/// Fullscreen A/B transition: t0 = old (hold), t1 = new. mode selects Wirecast-style effect.
inline constexpr char kPsTransition[] = R"(
Texture2D texOld : register(t0);
Texture2D texNew : register(t1);
SamplerState samp : register(s0);
cbuffer TransCB : register(b0) {
    float progress;
    float mode;
    float aspect;
    float wipeDir;
};

float4 blur5(Texture2D tex, float2 uv, float radius) {
    float4 acc = tex.Sample(samp, uv);
    acc += tex.Sample(samp, uv + float2( radius, 0));
    acc += tex.Sample(samp, uv + float2(-radius, 0));
    acc += tex.Sample(samp, uv + float2(0,  radius));
    acc += tex.Sample(samp, uv + float2(0, -radius));
    return acc * 0.2;
}

float4 sampleSafe(Texture2D tex, float2 uv) {
    if (uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1)
        return float4(0, 0, 0, 1);
    return tex.Sample(samp, uv);
}

float4 main(float4 pos : SV_POSITION, float2 uv : TEXCOORD0) : SV_TARGET {
    float p = saturate(progress);
    int m = (int)(mode + 0.5);
    float4 o = texOld.Sample(samp, uv);
    float4 n = texNew.Sample(samp, uv);
    const float soft = 0.04;

    // 1 Cross dissolve / Fade
    if (m == 1) {
        return lerp(o, n, p);
    }

    // 2 Directional wipe
    if (m == 2) {
        float edge = 0;
        if (wipeDir < 0.5) edge = uv.x;           // to right: new from left
        else if (wipeDir < 1.5) edge = 1.0 - uv.x; // to left
        else if (wipeDir < 2.5) edge = uv.y;       // to bottom
        else edge = 1.0 - uv.y;                    // to top
        float w = smoothstep(p - soft, p + soft, edge);
        return lerp(n, o, w);
    }

    // 3 Merge (ease-in dissolve)
    if (m == 3) {
        float e = p * p;
        return lerp(o, n, e);
    }

    // 4 CubeZoom — zoom out old / zoom in new
    if (m == 4) {
        float zo = 1.0 + p * 0.85;
        float zn = 2.0 - p * 1.0;
        float2 uo = (uv - 0.5) / zo + 0.5;
        float2 un = (uv - 0.5) / zn + 0.5;
        float4 a = sampleSafe(texOld, uo);
        float4 b = sampleSafe(texNew, un);
        return lerp(a, b, smoothstep(0.35, 0.65, p));
    }

    // 5 3D Plane — perspective tilt + dissolve
    if (m == 5) {
        float tilt = (p - 0.5) * 1.2;
        float persp = 1.0 + (uv.y - 0.5) * tilt;
        float2 uo = float2((uv.x - 0.5) / max(0.35, persp) + 0.5 + p * 0.15, uv.y);
        float2 un = float2((uv.x - 0.5) / max(0.35, 1.0 - tilt * 0.5) + 0.5 - (1.0 - p) * 0.15, uv.y);
        return lerp(sampleSafe(texOld, uo), sampleSafe(texNew, un), p);
    }

    // 6 Bands — horizontal band wipe
    if (m == 6) {
        float bands = 10.0;
        float y = uv.y * bands;
        float idx = floor(y);
        float local = frac(y);
        float thresh = saturate(p * (bands + 1.0) - idx);
        float w = smoothstep(thresh - 0.08, thresh + 0.08, local);
        return lerp(n, o, w);
    }

    // 7 Clock wipe
    if (m == 7) {
        float2 d = uv - 0.5;
        float ang = atan2(d.x, -d.y); // 0 at top, clockwise-ish
        float a = (ang + 3.14159265) / (6.2831853);
        float w = smoothstep(p - soft, p + soft, a);
        return lerp(n, o, w);
    }

    // 8 Cross blur
    if (m == 8) {
        float rad = (p < 0.5 ? p : 1.0 - p) * 0.045;
        float4 bo = blur5(texOld, uv, rad);
        float4 bn = blur5(texNew, uv, rad);
        return lerp(bo, bn, p);
    }

    // 9 Crosshair — grow from center axes
    if (m == 9) {
        float hx = abs(uv.x - 0.5);
        float hy = abs(uv.y - 0.5);
        float arm = p * 0.55;
        float on = (hx < arm * 0.08 + soft * 0.5) || (hy < arm * 0.08 + soft * 0.5) ? 1.0 : 0.0;
        float grow = saturate((arm - max(hx, hy)) / soft);
        float w = max(on * grow, saturate((p - 0.55) / 0.45));
        // Expand cross then fill
        float fill = smoothstep(0.0, 1.0, (p - max(hx, hy) * 1.2));
        return lerp(o, n, saturate(max(w, fill)));
    }

    // 10 Radial wipe — iris open
    if (m == 10) {
        float2 d = (uv - 0.5) * float2(aspect, 1.0);
        float r = length(d);
        float w = smoothstep(p * 1.2 - soft, p * 1.2 + soft, r);
        return lerp(n, o, w);
    }

    // 11 Swap — halves slide past each other
    if (m == 11) {
        float left = uv.x < 0.5;
        float2 uo = uv;
        float2 un = uv;
        if (left) {
            uo.x = uv.x - p;
            un.x = uv.x + 1.0 - p;
        } else {
            uo.x = uv.x + p;
            un.x = uv.x - 1.0 + p;
        }
        float4 a = sampleSafe(texOld, uo);
        float4 b = sampleSafe(texNew, un);
        return (p < 0.5) ? a : b;
    }

    // 12 Flip Over — vertical card flip
    if (m == 12) {
        float t = p * 3.14159265;
        float c = cos(t);
        float scaleX = max(0.05, abs(c));
        float2 u = float2((uv.x - 0.5) / scaleX + 0.5, uv.y);
        if (c >= 0.0)
            return sampleSafe(texOld, u);
        return sampleSafe(texNew, float2(1.0 - u.x, u.y));
    }

    // 13 Grid wipe
    if (m == 13) {
        float gx = 8.0, gy = 5.0;
        float2 cell = float2(floor(uv.x * gx), floor(uv.y * gy));
        float order = (cell.x + cell.y * gx) / (gx * gy);
        float w = smoothstep(order, order + 0.12, p);
        return lerp(o, n, w);
    }

    // 14 Curtain drop — top-down drape
    if (m == 14) {
        float wave = sin(uv.x * 12.0 + p * 6.0) * 0.03 * (1.0 - p);
        float edge = uv.y + wave;
        float w = smoothstep(p - soft, p + soft, edge);
        return lerp(n, o, w);
    }

    // 15 Circle wipe
    if (m == 15) {
        float2 d = (uv - 0.5) * float2(aspect, 1.0);
        float r = length(d);
        float w = smoothstep(p * 0.85 - soft, p * 0.85 + soft, r);
        return lerp(n, o, w);
    }

    // 16 Vacuum — suck old into center, reveal new
    if (m == 16) {
        float2 dir = uv - 0.5;
        float pull = p * 1.4;
        float2 uo = 0.5 + dir * (1.0 + pull);
        float fade = saturate(1.0 - length(dir) * pull * 2.0);
        float4 a = sampleSafe(texOld, uo) * fade;
        return lerp(a, n, smoothstep(0.25, 0.85, p));
    }

    // 17 Wave wipe
    if (m == 17) {
        float wave = sin(uv.y * 18.0 + p * 8.0) * 0.06;
        float edge = uv.x + wave;
        float w = smoothstep(p - soft, p + soft, edge);
        return lerp(n, o, w);
    }

    // 18 Push — slide
    if (m == 18) {
        float2 uOld = uv;
        float2 uNew = uv;
        bool useNew = false;
        if (wipeDir < 0.5) {
            uOld.x = uv.x + p;
            uNew.x = uv.x + p - 1.0;
            useNew = uv.x < p;
        } else if (wipeDir < 1.5) {
            uOld.x = uv.x - p;
            uNew.x = uv.x - p + 1.0;
            useNew = uv.x > 1.0 - p;
        } else if (wipeDir < 2.5) {
            uOld.y = uv.y + p;
            uNew.y = uv.y + p - 1.0;
            useNew = uv.y < p;
        } else {
            uOld.y = uv.y - p;
            uNew.y = uv.y - p + 1.0;
            useNew = uv.y > 1.0 - p;
        }
        return useNew ? sampleSafe(texNew, uNew) : sampleSafe(texOld, uOld);
    }

    // 19 Windshield wipe — angled pivoting blade
    if (m == 19) {
        float2 d = uv - float2(0.5, 1.0);
        float ang = atan2(d.x, -d.y);
        float a = saturate((ang + 1.2) / 2.4);
        float w = smoothstep(p - soft, p + soft, a);
        return lerp(n, o, w);
    }

    // 20 Fly Over — new slides down from above
    if (m == 20) {
        float2 mapNew = float2(uv.x, uv.y - (p - 1.0));
        bool over = mapNew.y >= 0.0 && mapNew.y <= 1.0;
        return over ? sampleSafe(texNew, mapNew) : o;
    }

    // 21 RGB Channels — prism split then recombine to new
    if (m == 21) {
        float split = (p < 0.5 ? p : 1.0 - p) * 0.08;
        float4 mid = float4(
            lerp(texOld.Sample(samp, uv + float2(split, 0)).r,
                 texNew.Sample(samp, uv + float2(split, 0)).r, p),
            lerp(texOld.Sample(samp, uv).g,
                 texNew.Sample(samp, uv).g, p),
            lerp(texOld.Sample(samp, uv - float2(split, 0)).b,
                 texNew.Sample(samp, uv - float2(split, 0)).b, p),
            1.0);
        return lerp(mid, n, smoothstep(0.55, 1.0, p));
    }

    return lerp(o, n, p);
}
)";

} // namespace shaders
} // namespace railshot
