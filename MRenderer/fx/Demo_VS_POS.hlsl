#include "BaseBuffer.hlsl"

VSOutput VS_Main(VSInput_P input)
{
    VSOutput o;

    float4 pos = float4(input.pos, 1.0f);
    pos = mul(pos, mWorld);
    pos = mul(pos, mView);
    pos = mul(pos, mProj);

    o.pos = pos;
    return o;
}