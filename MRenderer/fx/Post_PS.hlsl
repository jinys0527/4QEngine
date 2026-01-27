#include "BaseBuffer.hlsl"


float2 WarpTopExpand(float2 uv, float amount, float power)
{
    float t = pow(saturate(uv.y), power);
    float scale = 1.0 + t * amount;
    uv.x = (uv.x - 0.5) / scale + 0.5;
    return uv;
}

float4 SampleEmissiveRadial(Texture2D tex, float2 uv, float2 r)
{
    float2 o = float2(r.x, r.y);
    float4 c = 0;
    c += tex.Sample(smpClamp, uv);
    c += tex.Sample(smpClamp, uv + float2(r.x, 0));
    c += tex.Sample(smpClamp, uv + float2(-r.x, 0));
    c += tex.Sample(smpClamp, uv + float2(0, r.y));
    c += tex.Sample(smpClamp, uv + float2(0, -r.y));
    c += tex.Sample(smpClamp, uv + o);
    c += tex.Sample(smpClamp, uv - o);
    c += tex.Sample(smpClamp, uv + float2(r.x, -r.y));
    c += tex.Sample(smpClamp, uv + float2(-r.x, r.y));
    return c / 9.0;
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


    float4 Blur1 = g_BlurHalf.Sample(smpClamp, uvW);
    float4 Blur2 = g_BlurHalf2.Sample(smpClamp, uvW);
    float4 Blur3 = g_BlurHalf3.Sample(smpClamp, uvW);


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

    //float4 emissive = emissive1 * 0.6
    //                + emissive2 * 0.3
    //                + emissive3 * 0.1;

    float2 texelFull = 1.0 / screenSize;
    float2 texelHalf = texelFull * 6.0;
    float2 texelQuarter = texelFull * 12.0;
    float2 texel8 = texelFull * 20.0;

    float4 e1 = SampleEmissiveRadial(g_RTEmissiveHalf, uvW, texelHalf);
    float4 e2 = SampleEmissiveRadial(g_RTEmissiveHalf2, uvW, texelQuarter);
    float4 e3 = SampleEmissiveRadial(g_RTEmissiveHalf3, uvW, texel8);

    float4 emissive =
      e1 * 0.6
    + e2 * 0.3
    + e3 * 0.1;

    
    
    //return RTView + emissive;

    return dofColor + emissive;
}