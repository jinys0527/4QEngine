#include "BaseBuffer.hlsl"

VSOutput_P VS_Main(VSInput_PNUT input)
{
    VSOutput_P o = (VSOutput_P) 0;
    
    float4 pos = float4(input.pos, 1.0f);      float4 originPos = pos;
    pos = mul(pos, mWorld);
    pos = mul(pos, mView);
    pos = mul(pos, mProj);
    
    o.pos = pos;
    o.originPos = originPos;
    return o;
}