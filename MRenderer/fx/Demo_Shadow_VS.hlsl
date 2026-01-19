#include "BaseBuffer.hlsl"

VSOutput_Shadow VS_Main(VSInput_PNUT input)
{
    VSOutput_Shadow o = (VSOutput_Shadow) 0;
    
    float4 pos = float4(input.pos, 1.0f);      
    pos = mul(pos, mWorld);
    float4 uvshadow = mul(pos, mShadow);
    
    
    pos = mul(pos, mView);
    pos = mul(pos, mProj);
    
    o.pos = pos;
    o.uvshadow = uvshadow;
    return o;
}