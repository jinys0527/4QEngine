#include "BaseBuffer.hlsl"

float4 PS_Main(VSOutput_P i) : SV_TARGET
{
    float4 final = 1;
    
    if(i.originPos.z == 0.0f)
        final = float4(1, 0, 0, 1);
    
    if(i.originPos.x == 0.0f)
        final = float4(0, 0, 1, 1);
    
    return final;
}