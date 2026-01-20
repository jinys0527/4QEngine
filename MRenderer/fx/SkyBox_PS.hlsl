#include "BaseBuffer.hlsl"

float4 PS_Main(VSOutput_PUVW i) : SV_TARGET
{
    float4 SkyBox = g_SkyBox.Sample(smpClamp, i.uvw);
    
    
    
    
    return SkyBox;
}