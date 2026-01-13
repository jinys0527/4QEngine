#include "BaseBuffer.hlsl"

float4 PS_Main(VSOutput_PU i) : SV_TARGET
{
    float4 tex = g_RTView.Sample(smpClamp, i.uv);
    
    tex.a = 1;
    return tex;
}