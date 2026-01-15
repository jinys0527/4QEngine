#ifndef LIGHTS_HLSL
#define LIGHTS_HLSL

#include "BaseBuffer.hlsl"

float4 DirectLight(float4 nrm)
{
    float4 diff = 0;   diff.a = 1;
    float4 amb = 0;    amb.a = 1;
    
    float4 N = nrm;
    N.w = 0;
    float4 L = float4(lights[0].Dir, 0);
        
    //뷰공간으로 정보를 변환.
    N = mul(N, mul(mWorld, mView));
    L = mul(L, mView);
    
    //각 벡터 노멀라이즈.
    N = normalize(N);
    L = normalize(L);

    
    //조명 계산 
    float4 color = float4(saturate(lights[0].Color), 1);
    diff = max(dot(N, L), 0) * color;
    amb = float4(saturate(lights[0].Color) * 0.1f, 1.0f);
    //float4 amb = 0;


	//포화도 처리 : 광량의 합을 1로 제한합니다.
	return saturate(diff + amb);
}

float4 SpecularLight_Point(float4 pos, float4 nrm, Light light)
{
    float4 spec = 0;    spec.a = 1;
    
    float3 N = nrm.xyz;
    N = normalize(N);
    
    float3 L = light.Pos - pos.xyz;

    float3 V = normalize(cameraPos - pos.xyz);
    
    float3 H = normalize(L + V);

    float scalar = saturate(dot(H, N));
    
    scalar = pow(scalar, 80.f);
    
    spec.xyz = light.Color * scalar;
    
    return saturate(spec);
}

float4 SpecularLight_Direction(float4 pos, float4 nrm)
{
    float4 spec = 0;
    spec.a = 1;
    
    float3 N = nrm.xyz;
    N = normalize(mul(N, (float3x3)mWorldInvTranspose));
    
    float3 L = normalize(lights[0].Dir);

    float3 V = normalize(cameraPos - pos.xyz);
    
    float3 H = normalize(L + V);

    float scalar = saturate(dot(H, N));
    
    scalar = pow(scalar, 80.f);
    
    spec.xyz = lights[0].Color * scalar;
    
    return saturate(spec);
}
#endif