#include "BaseBuffer.hlsl"
#include "Lights.hlsl"

float4 PS_Main(VSOutput_Wall i) : SV_TARGET
{
    float4 final = 0;
    final.a = 1;
    
    float4 tex = Masking(i.uvmask);
        
    
    final.a = tex.r;
    
    return final;
}