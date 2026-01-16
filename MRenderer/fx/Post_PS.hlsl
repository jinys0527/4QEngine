#include "BaseBuffer.hlsl"

float4 PS_Main(VSOutput_PU i) : SV_TARGET
{
    float4 RTView = g_RTView.Sample(smpClamp, i.uv);
    float4 Blur = g_Blur.Sample(smpClamp, i.uv);
    float4 val = g_ShadowMap.Sample(smpClamp, i.uv);
    
    
    
    float4 tex = 1;
    
    tex = lerp(Blur, RTView, val);
    
    tex.a = 1;
    return tex;
}