#include "BaseBuffer.hlsl"

VSOutput_Refraction VS_Main(VSInput_PNUT input)
{
    VSOutput_Refraction o = (VSOutput_Refraction) 0;
    
    float4 pos;
    pos = float4(input.pos, 1.0f);
    pos = mul(pos, mWorld);
    pos = mul(pos, mView);
    pos = mul(pos, mProj);

    float4 nrm;
    nrm = float4(input.nrm, 0.0f);
    nrm = normalize(mul(nrm, mWorldInvTranspose));

    o.pos = pos;
    o.nrm = nrm;
    
    return o;
}