#include "BaseBuffer.hlsl"
#include "Lights.hlsl"

float4 PS_Main(VSOutput i) : SV_TARGET
{
    float4 final = 1;
    
    float4 albedo = g_Albedo.Sample(smpClamp, i.uv);
    
    float4 diffuse = DirectLight(i.nrm);
       
    float4 spec = SpecularLight_Direction(i.wPos, i.nrm);
    
    final = albedo * diffuse + spec;
    
    return final;
    return float4(i.nrm.xyz, 1);
}