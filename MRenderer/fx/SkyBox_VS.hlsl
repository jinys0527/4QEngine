#include "BaseBuffer.hlsl"

VSOutput_PUVW VS_Main(VSInput_PNUT input)
{
    VSOutput_PUVW o = (VSOutput_PUVW) 0;
    
    float4 pos = float4(input.pos, 1.0f);
    
    float4 uvw = mul(pos, mSkyBox);
    
    
    o.pos = pos;
    o.uvw = uvw.xyz;
    return o;
}