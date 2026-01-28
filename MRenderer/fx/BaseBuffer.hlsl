#ifndef BASEBUFFER_HLSL
#define BASEBUFFER_HLSL

// https://www.notion.so/GPU-2ea21c4cb4638021a66bf959e6db1ee4
cbuffer BaseBuffer : register(b0)
{
    matrix mWorld;
    matrix mWorldInvTranspose;
    matrix mTextureMask;
    float2 screenSize;
    float2 playerPos;           //x,y값은 uv좌표
};

cbuffer CameraBuffer : register(b1)
{
    matrix mView;
    matrix mProj;
    matrix mVP;
    matrix mSkyBox;
    matrix mShadow;
    float3 cameraPos;
    //float campadding;
    float dTime;
    float4 camParams; //x: near, y: far, z: focusZ(초점거리), w: focalRange(초점오차?범위)
};

struct Light
{
    matrix mLightVP; 

    float4 Color; 

    float3 Pos; 
    float Range; 

    float3 worldDir; 
    float  padding0; 

    float3 viewDir; 
    float Intensity; 

    float SpotInnerAngle;
    float SpotOutterAngle;
    float AttenuationRadius;
    float padding1;

    uint CastShadow;
    uint type;
    float2 padding; 
};

cbuffer LightBuffer : register(b2)
{
    Light lights[16];
    uint lightcount;
    float3 lightpadding;
};

cbuffer SkinningBuffer : register(b3)
{
    matrix bones[256];
    uint count;
    float3 skinningpadding;
};

cbuffer MaterialBuffer : register(b5)
{
    float saturation;
    float lightness;
    float2 matpadding;
};




//인풋(고정)
struct VSInput_PNUT
{
    float3 pos : POSITION;
    float3 nrm : NORMAL;
    float2 uv : TEXCOORD0;
    float4 T : TANGENT;
    uint4 boneIndices : BONEINDICES;
    float4 boneWeights : BONEWEIGHTS;
};

//(디버그용)
struct VSInput_P
{
    float3 pos : POSITION;
};

//아웃풋
//기본
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

//큐브맵
struct VSOutput_PUVW
{
    float4 pos : SV_POSITION;
    float3 uvw : TEXCOORD0;
};

//그림자 매핑 테스트
struct VSOutput_Shadow
{
    float4 pos : SV_POSITION;
    float4 uvshadow : TEXCOORD0;
};

//벽 테스트
struct VSOutput_Wall
{
    float4 pos : SV_POSITION;
    float4 uvmask : TEXCOORD0;
    float2 uv : TEXCOORD1;

};

//그리드
struct VSOutput_P
{
    float4 pos : SV_POSITION;
    float4 originPos : TEXCOORD0;
};

//PBR
struct VSOutput_PBR
{
    float4 pos : SV_POSITION;
    float4 nrm : NORMAL;
    float2 uv : TEXCOORD0;
    float4 wPos : TEXCOORD1;
    float4 vPos : TEXCOORD2;
    float3 envUVW : TEXCOORD3;
    float4 T : TEXCOORD4;
    float4 uvshadow : TEXCOORD5;
};

//굴절 레진
struct VSOutput_Refraction
{
    float4 pos : SV_POSITION;
    float4 nrm : NORMAL;
};

//임시 반드시 지울것
struct VSOutputLine
{
    float4 pos : SV_POSITION;
};

//ShaderResourceView
Texture2D g_RTView              : register(t0);
//Texture2D g_Blur                : register(t1);
Texture2D g_ShadowMap           : register(t2);
TextureCube g_SkyBox            : register(t3);
Texture2D g_DepthMap            : register(t4);
Texture2D g_Mask_Wall           : register(t5);
Texture2D g_WaterNoise          : register(t6);
Texture2D g_RTEmissive          : register(t7);
Texture2D g_RTEmissiveHalf      : register(t8);
Texture2D g_RTEmissiveHalf2     : register(t9);
Texture2D g_RTEmissiveHalf3     : register(t10);


Texture2D g_Albedo              : register(t11);
Texture2D g_Normal              : register(t12);
Texture2D g_Metalic             : register(t13);
Texture2D g_Roughness           : register(t14);
Texture2D g_AO                  : register(t15);
Texture2D g_Env                 : register(t16);
Texture2D g_Emissive            : register(t17);

Texture2D g_UI_01               : register(t21);
Texture2D g_UI_02               : register(t22);
Texture2D g_UI_03               : register(t23);
Texture2D g_UI_04               : register(t24);
Texture2D g_UI_05               : register(t25);

Texture2D g_Blur                : register(t31);
Texture2D g_BlurHalf            : register(t32);
Texture2D g_BlurHalf2           : register(t33);
Texture2D g_BlurHalf3           : register(t34);




//Sampler State
SamplerState smpWrap            : register(s0);
SamplerState smpMirror          : register(s1);
SamplerState smpClamp           : register(s2);
SamplerState smpBorder          : register(s3);
SamplerState smpBorderShadow    : register(s4);


#endif