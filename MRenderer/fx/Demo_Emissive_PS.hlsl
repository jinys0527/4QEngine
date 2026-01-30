#include "BaseBuffer.hlsl"

float4 PS_Main(VSOutput_PU input) : SV_Target
{
    float4 emissive = g_Emissive.Sample(smpClamp, input.uv);
    //float4 emissive = g_Metalic.Sample(smpClamp, input.uv);
    
    //emissive = float4(1, 1, 0, 1);
    
    
    return emissive;
}