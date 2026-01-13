#include "BaseBuffer.hlsl"


float4 DirectLight(float4 nrm)
{
    float4 N = nrm;
    N.w = 0;
    float4 L = float4(lights[0].Dir, 0);

    //뷰공간으로 정보를 변환.
    N = mul(N, mWV);
    L = mul(L, mView);
    
    //각 벡터 노멀라이즈.
    N = normalize(N);
    L = normalize(L);


    //조명 계산 
    float4 diff = max(dot(N, L), 0) * lights[0].Color;
    float4 amb = lights[0].Color * 0.1f;        //임의로 0.1배
    //float4 amb = 0;


	//포화도 처리 : 광량의 합을 1로 제한합니다.
	return saturate(diff + amb);
}