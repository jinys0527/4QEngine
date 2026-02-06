#include "BaseBuffer.hlsl"
#include "Lights.hlsl"

// Arc-style progress mask shader.
// Uses mTextureMask._21 as progress (0..1).
float4 PS_Main(VSOutput_PU i) : SV_TARGET
{
    float2 uv = i.uv;
    uv.y = 1.0f - uv.y;

    float4 tex = g_UI_01.Sample(smpClamp, uv);
    tex.rgb = LinearToSRGB(tex.rgb);
    float alpha = tex.a;

    const float progress = saturate(mTextureMask._21);
    const float reverseFill = step(0.5f, mTextureMask._22);

    float2 centered = uv - float2(0.5f, 0.5f);
    float radius = length(centered);

    // Adjust these for ring thickness/size.
    const float outerRadius = 0.475f;
    const float innerRadius = 0.37f;

    float ringMask = step(innerRadius, radius) * step(radius, outerRadius);

    float angle = atan2(centered.y, centered.x);
    angle = (angle + 3.14159265f) / (2.0f * 3.14159265f);

    float start = 0.765f;
    float end = 0.07f;

    float arcLength = frac(end - start + 1.0f);
    float targetLength = arcLength * progress;

    float forwardOffset = frac(end - angle + 1.0f);
    float reverseOffset = frac(angle - start + 1.0f);
    float angleOffset = lerp(forwardOffset, reverseOffset, reverseFill);

    float angleMask = step(angleOffset, targetLength) * step(angleOffset, arcLength);
    float mask = ringMask * angleMask;

    float4 tint = float4(mTextureMask._11, mTextureMask._12, mTextureMask._13, mTextureMask._14);
    tex.rgb *= tint.rgb;
    tex.a = tint.a * mask * alpha;
    return tex;
}
