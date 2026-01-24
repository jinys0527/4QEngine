#include "BaseBuffer.hlsl"
#include "Lights.hlsl"

float4 PS_Main(VSOutput_Wall i) : SV_TARGET
{
    float4 final = float4(0.35f, 0.35f, 0.35f, 1.f);
    final.a = 1;
    
    float4 tex = Masking(i.uvmask);
        
    final.a = tex.a;
    
    return final;
    //return float4(1, 1, 0, 0.2f);
}