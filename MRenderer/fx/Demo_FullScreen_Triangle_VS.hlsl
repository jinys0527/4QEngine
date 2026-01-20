#include "BaseBuffer.hlsl"

VSOutput_PU VS_Main(uint vID : SV_VertexID)
{
    VSOutput_PU o = (VSOutput_PU) 0;
    
    o.uv = float2((vID << 1) & 2, vID & 2);
    
    o.pos = float4(o.uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);

    return o;
}