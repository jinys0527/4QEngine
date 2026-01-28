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

float2 Rotate2D(float2 v, float a)
{
    float s = sin(a);
    float c = cos(a);
    return float2(c * v.x - s * v.y, s * v.x + c * v.y);
}

float Hash12(float2 p)
{
float3 p3 = frac(float3(p.xyx) * 0.1031);
p3 += dot(p3, p3.yzx + 33.33);
return frac((p3.x + p3.y) * p3.z);
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

       
// 2D 방향 노이즈

    float DepthMap = g_DepthMap.Sample(smpClamp, uvW).r;
    float viewZ = camParams.x * camParams.y / (camParams.y - DepthMap * (camParams.y - camParams.x));
    
    float coc = abs(viewZ - camParams.z) / camParams.w;
    coc = saturate(coc);
    
    // 화면 해상도 기준 픽셀 스케일
    float2 pixelSize = 1.0 / screenSize.xy; // 또는 View.ViewSizeAndInvSize.zw


// CoC → 픽셀 밀림 크기
    float radialStrength = coc * 6.0; // ★ 핵심 조절값 (3~10 추천)


    float2 radialOffset = pixelSize * radialStrength;
    
    
    float angle = Hash12(uvW * screenSize.xy) * 6.28318; // 0~2π
    
    
    float2 baseOffset = radialOffset;


// 회전된 오프셋
    float2 rRot = Rotate2D(baseOffset, angle);

    float jitter = lerp(0.7, 1.3, Hash12(uvW * screenSize.xy + 17.0));
    float2 rJittered = rRot * jitter;

    float4 radialBlur = SampleEmissiveRadial(g_RTView, uvW, rJittered);
    
    
    float4 Blur1 = g_BlurHalf.Sample(smpClamp, uvW);
    float4 Blur2 = g_BlurHalf2.Sample(smpClamp, uvW);
    float4 Blur3 = g_BlurHalf3.Sample(smpClamp, uvW);
    float4 Blur4 = g_BlurHalf4.Sample(smpClamp, uvW);
    float4 Blur5 = g_BlurHalf5.Sample(smpClamp, uvW);
    float4 Blur6 = g_BlurHalf6.Sample(smpClamp, uvW);
    float4 Blur7 = g_BlurHalf7.Sample(smpClamp, uvW);
    float4 Blur8 = g_BlurHalf8.Sample(smpClamp, uvW);

    
    //float diff = (viewZ - camParams.z) / camParams.w;
    //if (diff > 0)
    //    return float4(1, 0, 0, 1); // 타겟보다 뒤에 있으면 빨간색
    //else
    //    return float4(0, 1, 0, 1);
    
    float4 blurCombined =
            Blur1 * 0.2f
            + Blur2 * 0.3f
            + Blur3 * 0.3f
            + Blur4 * 0.2f;
    

    float4 dofColor = lerp(RTView, blurCombined, coc);


// CoC가 클수록 radial 영향 증가
    float radialWeight = saturate(coc * 1.2);


    float4 finalColor = lerp(dofColor, radialBlur, radialWeight);
    return finalColor;
     
    float2 d = i.uv - playerPos.xy;
    
// 타원 반경 (UV 기준)
    float2 radius = float2(1.15f, 0.35f);

    float v =
        (d.x * d.x) / (radius.x * radius.x) +
        (d.y * d.y) / (radius.y * radius.y);


// v == 1 → 타원 경계
    float w = smoothstep(0.8f, 1.2f, v);
    float4 color;
// w = 0 → 선명
// w = 1 → 완전 블러
    color = lerp(RTView, Blur3, w);
    
    // 타원 안에서만 거리 블러 추가
    float depthBlurInEllipse = (1.0f - w) * coc;

    // 최종 블러 강도
    float finalBlur = saturate(w + depthBlurInEllipse);

    // 최종 색
    //float4 finalColor = lerp(RTView, Blur3, finalBlur);
    //return finalColor;

    
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