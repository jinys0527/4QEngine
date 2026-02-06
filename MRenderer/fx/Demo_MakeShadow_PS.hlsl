#include "BaseBuffer.hlsl"

void PS_Main(VSOutput_PU input)
{
    //float4 emissive = g_Emissive.Sample(smpClamp, input.uv);
    float alpha = g_Albedo.Sample(smpWrap, input.uv);
        
}