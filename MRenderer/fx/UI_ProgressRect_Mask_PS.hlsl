#include "BaseBuffer.hlsl"
#include "Lights.hlsl"

float4 PS_Main(VSOutput_PU i) : SV_TARGET
{
    float2 uv = i.uv;
    uv.y = 1.0f - uv.y;

    float4 tex = g_UI_01.Sample(smpClamp, uv);
    float4 mask = g_UI_02.Sample(smpClamp, uv);
    clip(mask.a - 0.999f);
    float alpha = tex.a;

    float progress = saturate(mTextureMask._21);
    int direction = (int) (mTextureMask._22 + 0.5f);

    float coord = (direction >= 2) ? uv.y : uv.x;
    float reverseFill = (direction == 1 || direction == 3) ? 1.0f : 0.0f;
    float threshold = reverseFill > 0.5f ? (1.0f - progress) : progress;
    float fillMask = reverseFill > 0.5f ? step(threshold, coord) : step(coord, threshold);
    clip(fillMask - 0.5f);
    
    float4 tint = float4(mTextureMask._11, mTextureMask._12, mTextureMask._13, mTextureMask._14);
    
    tex.rgb = LinearToSRGB(tex.rgb);
    tex.a = tint.a * alpha;
    
    return tex;
}