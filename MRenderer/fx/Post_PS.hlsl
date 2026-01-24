#include "BaseBuffer.hlsl"

float4 PS_Main(VSOutput_PU i) : SV_TARGET
{
    float4 RTView = g_RTView.Sample(smpClamp, i.uv);
    
    float4 Blur1 = g_Blur.SampleLevel(smpClamp, i.uv, 1);
    float4 Blur2 = g_Blur.SampleLevel(smpClamp, i.uv, 2);
    float4 Blur3 = g_Blur.SampleLevel(smpClamp, i.uv, 3);
        
    float DepthMap = g_DepthMap.Sample(smpClamp, i.uv).r;
    
    //Circle of Confusion
    float coc;
    float focusDist = 0.9f;     //초점 거리
    float focusRange = 0.1f;    //허용 초점 범위 (오차범위)
    coc = abs(DepthMap - focusDist) / focusRange;
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
    
    
    float4 tex = lerp(RTView, dofColor, 1);
    tex.a = 1;
    //return RTView;
    return dofColor;

}