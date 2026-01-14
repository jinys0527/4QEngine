#include "BaseBuffer.hlsl"
#include "Lights.hlsl"

float4 PS_Main(VSOutput i) : SV_TARGET
{
    float4 final = 1;
    
    float4 albedo = g_Albedo.Sample(smpClamp, i.uv);
    
    final = DirectLight(i.nrm);
    
    final = albedo * final;
    
    return final;
}