#include "BaseBuffer.hlsl"
#include "Lights.hlsl"

float4 PS_Main(VSOutput_PBR input) : SV_Target
{
    float4 texAlbedo = g_Albedo.Sample(smpClamp, input.uv);
    float4 texNrm = g_Normal.Sample(smpClamp, input.uv);
    float  texMetalr = g_Metalic.Sample(smpClamp, input.uv).r;
    float  texRoughr = g_Roughness.Sample(smpClamp, input.uv).r;
    float  texAOr = g_AO.Sample(smpClamp, input.uv).r;
    //float4 texEnv;
            
    //감마
    float alpha = texAlbedo.a;
        
    float3 baseColor = texAlbedo.rgb;
    float3 baseColorDiffuse = saturate(baseColor * lightness);
    baseColorDiffuse = AdjustSaturation(baseColorDiffuse, saturation);

    float4 texAlbedo_Diff = float4(baseColorDiffuse, alpha);
    float4 texMetal = texMetalr.xxxx;
    float4 texRough = texRoughr.xxxx;
    float4 texAO = texAOr.xxxx;
    
    texRough = max(texRough, 0.04f); //GGX 안정을 위한 최소값 
    
    float3 T, B, N;
    float handedness = (input.T.w < 0.0f) ? -1.0f : 1.0f;
    
    BuildTBN(input.T.xyz, input.nrm.xyz, handedness, texNrm, T, B, N);

    
    float3 nW = normalize(N);
    //return float4(nW, 1);
    float3 nV = normalize(mul(nW, (float3x3) mView));
    float3 eN = normalize(nW);
    float3 eL = normalize(cameraPos - input.wPos.xyz);
    
    float3 eR = normalize(reflect(-eL, eN));
    
    float3 lV = normalize(mul(lights[0].viewDir, (float3x3) mView));
    
    float specParam = 0.2f;
    float4 dirLit = UE_DirectionalLighting(input.vPos, float4(nV, 0), float4(lV, 0), texAlbedo_Diff, texAO, texMetal, texRough, specParam);
    //dirLit.rgb *= lerp(0.05f, 1.0f, shadowFactor);

    float4 ptLit = 0;
    for (uint i = 1; i < lightcount; i++)
    {
        ptLit += UE_PointLighting_FromLight(
        input.vPos,
        float4(nV, 0),
        lights[i],
        texAlbedo_Diff, texAO, texMetal, texRough,
        specParam
        );
    }
    float4 lit = dirLit + ptLit;
    
    //env
    float4 F0 = ComputeF0(float4(baseColor, 1.0f), texMetal, specParam);
    float ndotv = saturate(dot(eN, eL));
    float4 F = UE_Fresnel_Schlick(F0, ndotv);
    
    
    float4 specOcc = lerp(1.0f, texAO, texRough);
    float4 envStrength = pow(saturate(1.0 - texRough), 2.0f);
    float mipLevel = texRough.r * 6.0f;
    float3 envColor = g_SkyBox.SampleLevel(smpClamp, eR, mipLevel).rgb;
    float3 envSpec = envColor * F.rgb;
    envSpec *= specOcc.rgb;
    envSpec *= envStrength.rgb;
    
    float4 env = float4(envSpec, 1.0f) * 0.7f;
    env.a = 1;
    
    float4 amb = /*g_GlobalAmbient * baseColor*/lights[0].Color * 0.1f * texAlbedo_Diff * (1.0f - texMetal) * texAO;
    amb.a = 1;
    
    float4 col = lit + amb + env;
    
    
    
    //그림자
    float shadow = CastShadow(input.uvshadow);
    col.rgb *= shadow;
    
    col.rgb = LinearToSRGB(col.rgb);

    col.a = alpha;
    
    //return texAlbedo;
    //return texNrm   ;
    //return float4(texMetal.xyz, 1);
    //return texRough ;
    //return texAO;    
    
    
    return col;
}