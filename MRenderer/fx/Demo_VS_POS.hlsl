#include "BaseBuffer.hlsl"

VSOutput VS_Main(VSInput_P input)
{
    VSOutput o;

    float4 pos = float4(input.pos, 1.0f);
    pos = mul(pos, mWorld);
    pos = mul(pos, mView);
    pos = mul(pos, mProj);
    
    float4 nrm = 1;
    nrm.w = 0;

    o.pos = pos;
    o.nrm = nrm;
    return o;
}