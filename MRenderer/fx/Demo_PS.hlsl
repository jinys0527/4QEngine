#include "BaseBuffer.hlsl"
#include "Lights.hlsl"

float4 PS_Main(VSOutput i) : SV_TARGET
{
    float4 final = 1;
    
    final = DirectLight(i.nrm);
    
    return final;
}