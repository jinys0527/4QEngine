#include "BaseBuffer.hlsl"
#include "Lights.hlsl"

float4 PS_Main(VSOutput_PBR input) : SV_Target
{   
    float4 texAlbedo = g_Albedo.Sample(smpWrap, input.uv);
    float4 texNrm = g_Normal.Sample(smpWrap, input.uv);
    float texMetalr = g_Metalic.Sample(smpWrap, input.uv).r;
    float texRoughr = g_Roughness.Sample(smpWrap, input.uv).r;
    float texAOr = g_AO.Sample(smpWrap, input.uv).r;
    
    
    
    //감마
    float alpha = texAlbedo.a * matColor.a;
        
    float3 baseColor = texAlbedo.rgb * matColor.rgb;
    float3 baseColorDiffuse = saturate(baseColor * lightness);
    baseColorDiffuse = AdjustSaturation(baseColorDiffuse, saturation);

    float4 texAlbedo_Diff = float4(baseColorDiffuse, alpha);
    float4 texMetal = texMetalr.xxxx;
    float4 texRough = texRoughr.xxxx;
    float4 texAO = texAOr.xxxx;
    
    texRough = 0.f;
    
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
    
    float specParam = 0.5f;         //UE 기본값
    float4 dirLit = UE_DirectionalLighting(input.vPos, float4(nV, 0), float4(lV, 0), texAlbedo_Diff, texAO, texMetal, texRough, specParam);
    //dirLit.rgb *= lerp(0.05f, 1.0f, shadowFactor);

    float4 ptLit = 0;
    float4 spotLit = 0;

    for (uint i = 1; i < lightcount; i++)
    {
        if(lights[i].type == 2)
        {
            ptLit += UE_PointLighting_FromLight(
                input.vPos,
                float4(nV, 0),
                lights[i],
                texAlbedo_Diff, texAO, texMetal, texRough,
                specParam
            );

        }
        if (lights[i].type == 3)
        {
            spotLit += UE_SpotLighting_FromLight(
                input.vPos,
                float4(nV, 0),
                lights[i],
                texAlbedo_Diff, texAO, texMetal, texRough,
                specParam
            );
        }
    }
    
    float shadow = CastShadow(input.uvshadow);

    dirLit.rgb *= shadow;

    float4 lit = dirLit + ptLit + spotLit;
    
    //env
    float4 F0 = ComputeF0(float4(baseColor, 1.0f), texMetal, specParam);
    float ndotv = saturate(dot(eN, eL));
    float4 F = UE_Fresnel_Schlick(F0, ndotv);
    
    
    float4 specOcc = lerp(1.0f, texAO, texRough);
    float4 envStrength = pow(saturate(1.0 - texRough), 2.0f);
    float mipLevel = texRough.r * 6.0f;
    float3 envColor = g_SkyBox.SampleLevel(smpClamp, eR, mipLevel).rgb;
    
    //envColor = float3(0.125f, 0.125f, 0.125f);
    //envColor = float3(1, 1, 1);
    
    float3 envSpec = envColor * F.rgb;
    envSpec *= texMetal.r;
    envSpec *= specOcc.rgb;
    envSpec *= envStrength.rgb;
        
    float4 env = float4(envSpec, 1.0f) * 0.7f;
    env *= lerp(0.3f, 1.2f, texMetal.r);
    env.a = 1;
    
    float4 amb = /*g_GlobalAmbient * baseColor*/lights[0].Color * 0.1f * texAlbedo_Diff * (1.0f - texMetal) * texAO;
    amb.a = 1;
    
    float4 col = lit + amb + env;
    
// Fake Specular Light (강화 버전)


    float3 fakeL_ws = normalize(float3(0.3f, 0.6f, 0.7f));
    float3 fakeL = normalize(mul(fakeL_ws, (float3x3) mView));

    float ndotl_fake = saturate(dot(eN, fakeL));
    float fakeSpec = pow(ndotl_fake, 24.0f);

// 각도 보정
    fakeSpec *= saturate(1.0f - ndotv * 0.7f);

    float3 fakeSpecColor =
    fakeSpec * baseColor * texMetal.r * 1.2f;
    fakeSpecColor *= 20.f;
    //return float4(fakeSpecColor, 1);
    col.rgb += fakeSpecColor;
    
    //그림자
    //float shadow = CastShadow(input.uvshadow);
    //col.rgb *= shadow;
    
    col.rgb = LinearToSRGB(col.rgb);

    col.a = alpha;
    
    //return texAlbedo;
    //return texNrm   ;
    //return float4(texMetal.xyz, 1);
    //return texRough ;
    //return texAO;    
    
    
    return col;
}