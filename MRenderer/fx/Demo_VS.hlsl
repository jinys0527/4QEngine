#include "BaseBuffer.hlsl"

VSOutput VS_Main(VSInput_PNUT input)
{
    VSOutput o = (VSOutput) 0;
    
    float4 pos;
    pos = float4(input.pos, 1.0f);
    pos = mul(pos, mWorld);    float4 wPos = pos;
    pos = mul(pos, mView);
    pos = mul(pos, mProj);
    
    float4 nrm;
    nrm = float4(input.nrm, 0.0f);

    o.pos = pos;
    o.nrm = nrm;
    o.uv = input.uv;
    o.wPos = wPos;
    return o;
}