#include "BaseBuffer.hlsl"
#include "Lights.hlsl"

float4 PS_Main(VSOutput_PU i) : SV_TARGET
{
    float2 uv = i.uv;
    uv.y = 1.0f - uv.y;
    float4 tex = g_RTView.Sample(smpClamp, uv);
    
    float4 tint = float4(mTextureMask._11, mTextureMask._12, mTextureMask._13, mTextureMask._14);
    
    tex.rgb = LinearToSRGB(tex.rgb);
    
    return tex ;
}