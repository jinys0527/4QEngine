#include "BaseBuffer.hlsl"

VSOutput_PU VS_Main(VSInput_PU input)
{
    VSOutput_PU o;

    float4 pos = float4(input.pos, 1);

    o.pos = pos;
    o.uv = input.uv;
    return o;
}