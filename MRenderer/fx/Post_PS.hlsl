#include "BaseBuffer.hlsl"

//tilt shift를 위한 화면 위 아래를 늘이는 함수
float2 WarpTopExpand(float2 uv, float amount, float power)
{
    float t = pow(saturate(uv.y), power);
    float scale = 1.0 + t * amount;
    uv.x = (uv.x - 0.5) / scale + 0.5;
    return uv;
}

// Emissive를 퍼지도록 픽셀을 미는 함수
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
    uvW = i.uv;
// ====== 워프된 UV로 샘플링 ======
    float4 RTView = g_RTView.Sample(smpClamp, uvW);


    float4 Blur1 = g_BlurHalf.Sample(smpClamp, uvW);
    float4 Blur2 = g_BlurHalf2.Sample(smpClamp, uvW);
    float4 Blur3 = g_BlurHalf3.Sample(smpClamp, uvW);


    float DepthMap = g_DepthMap.Sample(smpClamp, uvW).r;
    float viewZ = camParams.x * camParams.y / (camParams.y - DepthMap * (camParams.y - camParams.x));
    
    float coc = abs(viewZ - camParams.z) / camParams.w;
    coc = saturate(coc);
    
    //float diff = (viewZ - camParams.z) / camParams.w;
    //if (diff > 0)
    //    return float4(1, 0, 0, 1); // 타겟보다 뒤에 있으면 빨간색
    //else
    //    return float4(0, 1, 0, 1);
    
    float4 color2 = lerp(RTView, Blur3, coc);
    
    
     
    float2 d = i.uv - playerPos.xy;
    
// 타원 반경 (UV 기준)
    float2 radius = float2(1.15f, 0.35f);

    float v =
        (d.x * d.x) / (radius.x * radius.x) +
        (d.y * d.y) / (radius.y * radius.y);


// v == 1 → 타원 경계
    float w = smoothstep(0.8f, 1.2f, v);
    float color;
// w = 0 → 선명
// w = 1 → 완전 블러
    color = lerp(RTView, Blur3, w);
    
    float4 final = lerp(color, color2, 0.8f);
    
    
    
    return final;
    
    
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

    return RTView + emissive;
    
    //return tilt + emissive;
}