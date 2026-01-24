#include "BaseBuffer.hlsl"

float4 PS_Main(VSOutput_Refraction input) : SV_Target
{
    float2 uv = input.pos.xy / screenSize;

    uv += float2(0.05f, 0.0f); // x방향으로 살짝 밀기

    float4 texRT = g_RTView.Sample(smpClamp, uv);
    return texRT;
}