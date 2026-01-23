#include "BaseBuffer.hlsl"

VSOutput_PBR VS_Main(VSInput_PNUT input)
{
    VSOutput_PBR o = (VSOutput_PBR) 0;
    
    float4 pos;
    pos = float4(input.pos, 1.0f);
    pos = mul(pos, mWorld);
    float4 wPos = pos;
    float4 uvshadow = mul(pos, mShadow);
    
    pos = mul(pos, mView);     float4 vPos = pos;
    pos = mul(pos, mProj);
    
    float4 nrm;
    nrm = float4(input.nrm, 0.0f);
    nrm = normalize(mul(nrm, mWorldInvTranspose));
    
    float3 eR = float3(1, 1, 1);
    
    float4 T = float4(input.T.xyz, 0.0f);
    T = mul(T, mWorld);
    
    
    o.pos = pos;
    o.nrm = nrm;
    o.uv = input.uv;
    o.wPos = wPos;
    o.vPos = vPos;
    o.envUVW = eR;
    o.T = float4(normalize(T.xyz), input.T.w);
    o.uvshadow = uvshadow;
    return o;
}