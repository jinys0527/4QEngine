#include "BaseBuffer.hlsl"

float4 PS_Main(VSOutput i) : SV_TARGET
{
    float4 shadow = float4(1, 1, 1, 1);
    if(lights[0].CastShadow)
    {
        //shadow = SampleShadowMap();           이런식으로 그림자 실행하면 됨. if문을 써도 블록단위로 실행되기때문에 리소스 많이 안먹음

    }
    
    return float4(1, 1, 1, 1);
}