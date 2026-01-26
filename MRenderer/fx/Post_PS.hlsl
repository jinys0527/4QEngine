#include "BaseBuffer.hlsl"


float2 WarpTopExpand(float2 uv, float amount, float power)
{
    float t = pow(saturate(uv.y), power);
    float scale = 1.0 + t * amount;
    uv.x = (uv.x - 0.5) / scale + 0.5;
    return uv;
}


float4 PS_Main(VSOutput_PU i) : SV_TARGET
{
// ====== 여기서 워프 ======
    float warpAmount = 0.15f; // 0.05~0.15 추천
    float warpPower = 3.0f; // 2~4 추천


    float2 uvW = WarpTopExpand(i.uv, warpAmount, warpPower);
    uvW = saturate(uvW); // 범위 밖 방지


// ====== 워프된 UV로 샘플링 ======
    float4 RTView = g_RTView.Sample(smpClamp, uvW);


    float4 Blur1 = g_Blur.SampleLevel(smpClamp, uvW, 1);
    float4 Blur2 = g_Blur.SampleLevel(smpClamp, uvW, 2);
    float4 Blur3 = g_Blur.SampleLevel(smpClamp, uvW, 3);


    float DepthMap = g_DepthMap.Sample(smpClamp, uvW).r;


//Circle of Confusion
    float focusDist = 0.996f;
    float focusRange = 0.003f;


    float coc = abs(DepthMap - focusDist) / focusRange;
    coc = saturate(coc);


    float4 dofColor;


    if (coc < 0.33f)
    {
        float t = coc / 0.33f;
        dofColor = lerp(RTView, Blur1, t);
    }
    else if (coc < 0.66f)
    {
        float t = (coc - 0.33f) / 0.33f;
        dofColor = lerp(Blur1, Blur2, t);
    }
    else
    {
        float t = (coc - 0.66f) / 0.34f;
        dofColor = lerp(Blur2, Blur3, t);
    }


    dofColor.a = 1;
    return RTView;
}