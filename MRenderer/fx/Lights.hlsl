#ifndef LIGHTS_HLSL
#define LIGHTS_HLSL
#define PI 3.141592f

#include "BaseBuffer.hlsl"

float4 DirectLight(float4 nrm)
{
    float4 diff = 0;   diff.a = 1;
    float4 amb = 0;    amb.a = 1;
    
    float4 N = nrm;
    N.w = 0;
    float4 L = float4(lights[0].viewDir, 0);
        
    //뷰공간으로 정보를 변환.
    N = mul(N, mul(mWorld, mView));
    
    //각 벡터 노멀라이즈.
    N = normalize(N);
    L = normalize(L);

    
    //조명 계산 
    float4 color = saturate(lights[0].Color);
    diff = max(dot(N, L), 0) * color;
    amb = saturate(lights[0].Color) * 0.1f;
    //float4 amb = 0;


	//포화도 처리 : 광량의 합을 1로 제한합니다.
	return saturate(diff + amb);
}

float PCF(float4 smUV)
{
    float2 uv = smUV.xy ;
    float curDepth = smUV.z;

    float offset = 1.0f / 4096.0f;
    float bias = pow(0.5f, 23);

    float sum = 0.0f;

    // 4x4 PCF (0~3)
    [unroll]
    for (int y = 0; y < 4; y++)
    {
        [unroll]
        for (int x = 0; x < 4; x++)
        {
            float2 suv = uv + float2((x - 1) * offset, (y - 1) * offset);
            float smDepth = g_ShadowMap.Sample(smpBorderShadow, suv).r;
            
            sum += (smDepth >= curDepth - bias) ? 1.0f : 0.0f;
        }
    }
    float shadow = smoothstep(0.0f, 16.0f, sum);
    return max(shadow, 0.3f);
}

float CastShadow(float4 uv)
{
    uv.xy /= uv.w;

    float shadowDepth = g_ShadowMap.Sample(smpBorderShadow, uv.xy).r;

    if (shadowDepth == 0.0f)
        return 1.0f; // 그림자 없음(또는 밖)

    return PCF(uv);
}


float4 SpecularLight_Point(float4 pos, float4 nrm, Light light)
{
    float4 spec = 0;    spec.a = 1;
    
    float3 N = nrm.xyz;
    N = normalize(N);
    
    float3 L = light.Pos - pos.xyz;

    float3 V = normalize(cameraPos - pos.xyz);
    
    float3 H = normalize(L + V);

    float scalar = saturate(dot(H, N));
    
    scalar = pow(scalar, 80.f);
    
    spec.xyz = light.Color.xyz * scalar;
    
    return saturate(spec);
}

float4 SpecularLight_Direction(float4 pos, float4 nrm)
{
    float4 spec = 0;
    spec.a = 1;
    
    float3 N = nrm.xyz;
    N = normalize(mul(N, (float3x3)mWorldInvTranspose));
    
    float3 L = normalize(lights[0].viewDir);

    float3 V = normalize(cameraPos - pos.xyz);
    
    float3 H = normalize(L + V);

    float scalar = saturate(dot(H, N));
    
    scalar = pow(scalar, 80.f);
    
    spec.xyz = lights[0].Color.xyz * scalar;
    
    return saturate(spec);
}

void BuildTBN(
    float3 inT,
    float3 inN,
    float handedness,
    float4 texN,
    out float3 T,
    out float3 B,
    out float3 N)
{
    inN = normalize(inN);
    inT = normalize(inT - dot(inT, inN) * inN);
    B = cross(inN, inT) * handedness;

    T = normalize(inT);
    B = normalize(B);
    N = normalize(inN);

    float3 n = texN.xyz * 2 - 1;
    n.y = -n.y; // GL → DX
    
    float3x3 mTBN = float3x3(T, B, N);
    n = normalize(mul(n, mTBN));

    N = n;
}


// UE style helpers -----------------------------------------------------------

float Pow5(float x)
{
    float x2 = x * x;
    return x2 * x2 * x;
}

float4 UE_Fresnel_Schlick(float4 F0, float vdoth)
{
    return F0 + (1.0 - F0) * Pow5(1.0 - vdoth);
}

// UE4/5: GGX / Trowbridge-Reitz NDF
float4 UE_D_GGX(float ndoth, float4 roughness)
{
    float4 a = roughness * roughness; // alpha
    float4 a2 = a * a;
    float4 d = (ndoth * ndoth) * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d);
}

// UE4/5: Smith Schlick-GGX geometry term
float4 UE_G_SchlickGGX(float ndotx, float4 k)
{
    return ndotx / (ndotx * (1.0 - k) + k);
}

float4 UE_G_Smith(float ndotv, float ndotl, float4 roughness)
{
   
    float4 r = roughness + 1.0;
    // UE: k = (r+1)^2 / 8
    float4 k = (r * r) * 0.125;
    float4 gv = UE_G_SchlickGGX(ndotv, k);
    float4 gl = UE_G_SchlickGGX(ndotl, k);
    return gv * gl;
}


// F0 from metallic workflow (UE style)
float4 ComputeF0(float4 baseColor, float4 metallic, float4 specular)
{
    // UE default dielectric F0 ~= 0.04.
    // Specular input in UE is 0..1 and scales dielectric F0 around ~0.08 max.
    // Common approximation: dielectricF0 = 0.08 * specular
    float4 dielectricF0 = 0.08 * specular;
    return lerp(dielectricF0, baseColor, metallic);
}

struct BRDFResult
{
    float4 diffuse;
    float4 specular;
};

BRDFResult BRDF_UE_Direct(float4 viewNrm, float4 viewPos, float4 viewLitDir,
                          float4 baseColor, float4 metallic, float4 roughness,
                          float4 ao, float specularParam) // specularParam: UE "Specular" (default 0.5)
{
    BRDFResult o;
    o.diffuse = 0;
    o.specular = 0;

    float4 P = viewPos;
    P.w = 1;
    float4 N = normalize(viewNrm);
    float4 V = normalize(-P);
    float4 L = normalize(viewLitDir);

    float4 H = normalize(V + L);

    float ndotl = saturate(dot(N, L));
    float ndotv = saturate(dot(N, V));
    float ndoth = saturate(dot(N, H));
    float vdoth = saturate(dot(V, H));

    // avoid divide by 0
    ndotl = max(ndotl, 1e-4);
    ndotv = max(ndotv, 1e-4);

    // F0
    float4 F0 = ComputeF0(baseColor, metallic, specularParam);

    // Specular microfacet
    float4 D = UE_D_GGX(ndoth, roughness);
    float4 G = UE_G_Smith(ndotv, ndotl, roughness);
    float4 F = UE_Fresnel_Schlick(F0, vdoth);
         
    float4 spec = (D * G) * F / (4.0 * ndotv * ndotl);

    // Diffuse (Lambert) with energy conservation (UE style)
    float4 kd = (1.0 - F) * (1.0 - metallic); // metals have no diffuse
    float4 diff = kd * baseColor / PI;

    diff *= ao;

    o.diffuse = diff;
    o.specular = spec;
    return o;
}

float4 UE_DirectionalLighting(float4 viewPos, float4 viewNrm, float4 viewLitDir,
                              float4 base, float4 ao, float4 metallic, float4 roughness,
                              float specularParam)
{
    float4 P = viewPos;
    float4 N = viewNrm;
    float4 L = normalize(viewLitDir);
    L.w = 0;
    float4 V = normalize(-P);

    BRDFResult brdf = BRDF_UE_Direct(N, V, L, base, metallic, roughness, ao, specularParam);
        
    float4 radianceDiff = lights[0].Color * lights[0].Intensity;
    float4 radianceSpec = lights[0].Color * lights[0].Intensity;

    float ndotl = saturate(dot(normalize(N), L));
    
    float4 albedo = (base * (1.0 - metallic)) * ao;

    float4 color = (brdf.diffuse * radianceDiff + brdf.specular * radianceSpec) * ndotl;
    color.a = 1;

    float4 amb = lights[0].Color * 0.1f /*주변광 임시로 0.1배*/ * lights[0].Intensity * albedo;    amb.a = 1;
    color += amb;
    color.a = 1;
    //return float4(lights[0].Intensity, lights[0].Intensity, lights[0].Intensity, 1);
    
    return saturate(color);
}

// Point Light ----------------------------------------------------------------
//float4 UE_PointLighting(float4 viewPos, float4 viewNrm, float4 viewLitPos,
//                        float4 base, float4 ao, float4 metallic, float4 roughness,
//                        float specularParam)
//{
//    float4 P = viewPos;
//    float4 LP = viewLitPos;

//    float4 N = normalize(viewNrm);

//    float4 toL = LP - P;
//    float dist2 = dot(toL, toL);
//    float dist = sqrt(max(dist2, 1e-8));
//    float4 L = toL / dist;

//    float4 V = normalize(-P);


//    float att = rcp(max(dist2, 1e-4));
//    float s = saturate(1.0 - dist / max(g_PointLit.Range, 1e-4));
//    att *= (s * s);

//    BRDFResult brdf = BRDF_UE_Direct(N, V, L, base, metallic, roughness, ao, specularParam);

//    float4 radiance = g_PointLit.Diffuse * g_PointLit.Strength;

//    float ndotl = saturate(dot(N, L));
    
//    float4 albedo = (base * (1.0 - metallic)) * ao;

//    float4 color = (brdf.diffuse + brdf.specular) * radiance * ndotl * att;


//    color += g_PointLit.Ambient * albedo * att;
//    color.a = 1;

//    return saturate(color);
//}
#endif