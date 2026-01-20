cbuffer BaseBuffer : register(b0)
{
    matrix mWorld;
    matrix mView;
    matrix mProj;
    matrix mVP;
}
    
struct VSInput
{
    float3 pos : POSITION;
    float3 nrm : NORMAL;
    float2 uv : TEXCOORD0;
    float4 T : TANGENT;
};

struct VSOutput
{
    float4 pos : SV_POSITION;
};

VSOutput VS_Main(VSInput input)
{
    float4 pos = float4(input.pos, 1);
    pos = mul(pos, mWorld);
    pos = mul(pos, mView);
    pos = mul(pos, mProj);
    
    VSOutput o;
    o.pos = pos;
    return o;
}

float4 PS_Main(
                VSOutput i
               ) : SV_TARGET //[출력] 색상.(필수), "렌더타겟" 으로 출력합니다.
{
    
    
    return 1;
}
