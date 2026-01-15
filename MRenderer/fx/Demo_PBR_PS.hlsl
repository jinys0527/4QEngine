#include "BaseBuffer.hlsl"
#include "Lights.hlsl"

float4 PS_Main(VSOutput_PBR input) : SV_Target
{
    float4 texAlbedo = g_Albedo.Sample(smpClamp, input.uv);
    float4 texNrm = g_Normal.Sample(smpClamp, input.uv);
    float4 texMetal = g_Metalic.Sample(smpClamp, input.uv);
    float4 texRough = g_Roughness.Sample(smpClamp, input.uv);
    float4 texAO = g_AO.Sample(smpClamp, input.uv);
    //float4 env
    
    texMetal.a = texRough.a = 1;
    //임시
    texMetal.rgb = texMetal.r;
    
    if (texAO.r < 0.001f && texAO.g < 0.001f && texAO.b < 0.001f)
    {
        texAO = 1.f;
    }
    
    float4 rough = 1;
    rough = saturate(1 - texRough);
    
    float3 T, B, N;
    float handedness = (input.T.w < 0.0f) ? -1.0f : 1.0f;
    
    BuildTBN(input.T.xyz, input.nrm.xyz, handedness, texNrm, T, B, N);

    float3 nW = normalize(mul(float4(N, 0), mWorldInvTranspose).xyz);
    float3 nV = normalize(mul(nW, (float3x3)mView));
    
    float3 eN = normalize(nW);
    float3 eL = normalize(input.wPos.xyz - cameraPos); //시선(월드)
    float3 eR = normalize(reflect(eL, eN)); //반사벡터 계산.
    
    float specParam = 0.5f;
    float4 dirLit = UE_DirectionalLighting(input.vPos, input.nrm, float4(lights[0].viewDir, 0), texAlbedo, texAO, texMetal, texRough, specParam);
    //dirLit.rgb *= lerp(0.05f, 1.0f, shadowFactor);
    
    //float4 ptLit = UE_PointLighting(viewPos, float4(nV, 0), viewLitPos, baseColor, ao, metallic, roughness, specParam);
    
    float4 lit = dirLit; //+ptLit;
    
    //env
    float4 F0 = ComputeF0(texAlbedo, texMetal, specParam);
    float ndotv = saturate(dot(eN, -eL));
    float4 F = UE_Fresnel_Schlick(F0, ndotv);
    
    
    float4 specOcc = lerp(1, texAO, texRough);
    float4 envStrength = pow(saturate(1 - texRough), 2);
    
    float4 env = 0.3f * F * specOcc * envStrength;
    
    float4 amb = /*g_GlobalAmbient * baseColor*/lights[0].Color * texAlbedo * (1 - texMetal) * texAO;
    amb.a = 1;
    
    float4 col = lit + amb;//    +env;
    col.a = texAlbedo.a;
        
    return col;
}