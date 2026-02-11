#include "BaseBuffer.hlsl"
#include "Lights.hlsl"

float4 PS_Main(VSOutput_Shadow i) : SV_TARGET
{
    float4 final = float4(0.23f, 0.23f, 0.35f, 1);
    //float4 final = 1;
    
    float4 texColor = g_Terrain.Sample(smpWrap, i.uv);
    
    return final;
    
    return texColor;
}