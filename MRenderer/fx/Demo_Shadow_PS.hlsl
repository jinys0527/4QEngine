#include "BaseBuffer.hlsl"
#include "Lights.hlsl"

float4 PS_Main(VSOutput_Shadow i) : SV_TARGET
{
    float4 final = 0.3f;
    final.a = 1;
    
    float shadow = CastShadow(i.uvshadow);
    
    final.rgb *= shadow;
    
    return final;
}