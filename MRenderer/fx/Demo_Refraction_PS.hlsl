#include "BaseBuffer.hlsl"

float4 PS_Main(VSOutput_Refraction input) : SV_Target
{
    float2 uv = input.pos.xy / screenSize;


    uv += float2(0.005f, 0.0f); // x방향으로 살짝 밀기


// dTime으로 노이즈 흐르게 만들기
    float2 noiseUV = uv * 4.0f + float2(dTime * 0.1f, dTime * 0.1f);


    float4 texNoise = g_WaterNoise.Sample(smpWrap, noiseUV);


    float2 noise = texNoise.rg * 2.0f - 1.0f; // 0~1 -> -1~1


    float noiseStrength = 0.02f;
    uv += noise * noiseStrength;


    float3 tint = float3(0.6f, 0.8f, 1.0f);
    float alpha = 0.4f;


    float4 texRT = g_RTView.Sample(smpClamp, uv);


    float3 tinted = texRT.rgb * tint;
    float3 outColor = lerp(texRT.rgb, tinted, alpha);


    return float4(outColor, texRT.a);
}