#include "BaseBuffer.hlsl"
#include "Lights.hlsl"

void PS_Main(VSOutput_PU input)
{
    //float4 emissive = g_Emissive.Sample(smpClamp, input.uv);
    float alpha = g_Albedo.Sample(smpWrap, input.uv).a;
        
    clip(alpha - 0.5f);
    
}