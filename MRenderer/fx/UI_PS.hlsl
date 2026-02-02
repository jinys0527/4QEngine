#include "BaseBuffer.hlsl"

float4 PS_Main(VSOutput_PU i) : SV_TARGET
{
    float4 tex = g_RTView.Sample(smpClamp, i.uv);
    float4 tint = float4(mTextureMask._11, mTextureMask._12, mTextureMask._13, mTextureMask._14);
    return tex * tint;
}