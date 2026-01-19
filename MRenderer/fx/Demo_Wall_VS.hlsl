#include "BaseBuffer.hlsl"

VSOutput_Wall VS_Main(VSInput_PNUT input)
{
    VSOutput_Wall o = (VSOutput_Wall) 0;
    
    float4 pos = float4(input.pos, 1.0f);
    pos = mul(pos, mWorld);
    float4 uvmask = mul(pos, mTextureMask);
    
    pos = mul(pos, mView);
    pos = mul(pos, mProj);
    
    o.pos = pos;
    o.uvmask = uvmask;
    o.uv = input.uv;
    return o;
}