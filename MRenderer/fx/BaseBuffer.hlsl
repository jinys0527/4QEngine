#ifndef BASEBUFFER_HLSL
#define BASEBUFFER_HLSL

cbuffer BaseBuffer : register(b0)
{
    matrix mWorld;
    matrix mWorldInvTranspose;
};

cbuffer CameraBuffer : register(b1)
{
    matrix mView;
    matrix mProj;
    matrix mVP;
    float3 cameraPos;
    float padding;
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

cbuffer LightBuffer : register(b2)
{
    Light lights[16];
    uint lightcount;
    float3 lightpadding;
};

cbuffer SkinningBuffer : register(b3)
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

//아웃풋

struct VSOutput
{
    float4 pos : SV_POSITION;
    float4 nrm : NORMAL;
    float2 uv : TEXCOORD0;
    float4 wPos : TEXCOORD1;
};

struct VSOutput_PU
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

struct VSOutput_P
{
    float4 pos : SV_POSITION;
    float4 originPos : TEXCOORD0;
};


//ShaderResourceView
Texture2D g_RTView : register(t0);

Texture2D g_Albedo : register(t11);
Texture2D g_Normal : register(t12);
Texture2D g_Metalic : register(t13);
Texture2D g_Roughness : register(t14);
Texture2D g_AO : register(t15);
Texture2D g_Env : register(t16);

//Sampler State
SamplerState smpClamp : register(s0);


#endif