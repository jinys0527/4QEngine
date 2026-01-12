cbuffer BaseBuffer : register(b0)
{
    matrix mWorld;
    matrix mView;
    matrix mProj;
    matrix mVP;
};

struct Light
{
    float3 Pos;
    float Range;
    float3 Dir;
    float SpotAngle;
    float3 Color;
    float Intensity;
    matrix mVP;
    bool CastShadow;
    float3 padding;
};

cbuffer LightBuffer : register(b1)
{
    Light lights[16];
    int lightCount;
}

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
