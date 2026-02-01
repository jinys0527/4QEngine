#include "BaseBuffer.hlsl"
#include "Lights.hlsl"

float4 PS_Main(VSOutput_PU input) : SV_Target
{
    //float4 emissive = g_Emissive.Sample(smpClamp, input.uv);
    float alpha = g_Albedo.Sample(smpWrap, input.uv);
        
    clip(alpha - 0.05f);
    
    return float4(1, 1, 1, 1);
}