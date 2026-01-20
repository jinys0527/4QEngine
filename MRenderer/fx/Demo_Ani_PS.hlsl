#include "BaseBuffer.hlsl"
#include "Lights.hlsl"


//float4 PS_Main(VSOutputLine i) : SV_TARGET
//{
//    return float4(1, 0, 0, 1); // 빨강
//}

float4 PS_Main(VSOutput i) : SV_TARGET
{
    float4 final = 1;
    
    float4 albedo = g_Albedo.Sample(smpClamp, i.uv);
        
    float4 diffuse = DirectLight(i.nrm);
           
    float4 spec = SpecularLight_Direction(i.wPos, i.nrm);
    
    final = diffuse + spec;
    
    return final;
}