#include "BaseBuffer.hlsl"

float4 PS_Main(VSOutput_Refraction input) : SV_Target
{
    float2 uv = input.pos.xy / screenSize;
    
    float4 texRT = g_RTView.Sample(smpClamp, uv);
    
    return texRT;
}