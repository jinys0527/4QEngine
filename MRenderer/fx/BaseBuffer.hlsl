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
    uint CastShadow;
    float3 padding;
};

cbuffer LightBuffer : register(b1)
{
    Light lights[16];
    uint lightcount;
    float3 lightpadding;
};

cbuffer SkinningBuffer : register(b2)
{
    matrix bones[128];
    uint count;
    float3 skinningpadding;
};









struct VSInput_P
{
    float3 pos : POSITION;
};

struct VSInput_PU
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD0;
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

struct VSOutput_PU
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};



//ShaderResourceView
Texture2D g_RTView : register(t0);

//Sampler State
SamplerState smpClamp : register(s0);
