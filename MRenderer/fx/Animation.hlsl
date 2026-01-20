#ifndef ANIMATION_HLSL
#define ANIMATION_HLSL

#include "BaseBuffer.hlsl"

float4 ApplyBone(float4 pos, uint idx)
{
    if (idx >= count)
        return pos; // identity 적용한 것과 동일
    return mul(pos, bones[idx]);
}

float4 Skinning(float4 pos, float4 weight, uint4 index)
{
    float4 skinVtx;

    
    float4 v0 = mul(pos, bones[index.r]);
    float4 v1 = mul(pos, bones[index.g]);
    float4 v2 = mul(pos, bones[index.b]);
    float4 v3 = mul(pos, bones[index.a]);
    
    skinVtx = v0 * weight.r + v1 * weight.g + v2 * weight.b + v3 * weight.a;

    return skinVtx;
}

float3 GetBonePos(uint idx)
{
    if(idx >= count)
    {
        return float3(0.0f, 0.0f, 0.0f);
    }
    // row-major 기준 (너가 mul(pos, bones[i]) 쓰는 스타일이면 보통 이게 맞음)
    return float3(bones[idx]._41, bones[idx]._42, bones[idx]._43);
}

#endif