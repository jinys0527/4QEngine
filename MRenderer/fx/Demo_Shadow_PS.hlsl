#include "BaseBuffer.hlsl"
#include "Lights.hlsl"

float4 PS_Main(VSOutput_Shadow i) : SV_TARGET
{
    float4 final = float4(0.41f, 0.41f, 0.41f, 1);
    //float4 final = 1;
    
    float shadow = CastShadow(i.uvshadow);
    
    final.rgb *= shadow;
    
    return final;
}