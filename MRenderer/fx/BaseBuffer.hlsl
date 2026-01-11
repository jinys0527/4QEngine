cbuffer BaseBuffer : register(b0)
{
    matrix mWorld;
    matrix mView;
    matrix mProj;
    matrix mVP;
};

struct VSInput_P
{
    float3 pos : POSITION;
};

struct VSInput_PNUT
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
