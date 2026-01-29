#include "BaseBuffer.hlsl"

VSOutput_PU VS_Main(VSInput_PNUT input)
{
    VSOutput_PU o = (VSOutput_PU) 0;

    float4 pos = float4(input.pos, 1.0f);
    pos = mul(pos, mWorld);
    pos = mul(pos, mView);
    pos = mul(pos, mProj);
    
    o.pos = pos;
    o.uv = input.uv;
    
    return o;
}

