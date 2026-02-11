#include "BaseBuffer.hlsl"
#include "Lights.hlsl"

float4 PS_Main(VSOutput_PU i) : SV_TARGET
{
    float2 uv = i.uv;
    uv.y = 1.0f - uv.y;

    float4 tex = g_UI_01.Sample(smpClamp, uv);
    float4 mask = g_UI_02.Sample(smpClamp, uv);
    float alpha = tex.a;

    float4 tint = float4(mTextureMask._11, mTextureMask._12, mTextureMask._13, mTextureMask._14);
    
    tex.rgb *= tint.rgb;
    tex.rgb = LinearToSRGB(tex.rgb);
    tex.a = tint.a * alpha;
    clip(tex.a < 0.001f ? -1 : 1);
    
    return tex;
}