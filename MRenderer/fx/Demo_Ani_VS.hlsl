#include "BaseBuffer.hlsl"
#include "Animation.hlsl"


//VSOutputLine VS_Main(uint vid : SV_VertexID)
//{
//    VSOutputLine o = (VSOutputLine) 0;

//    // LineList: 2개 정점이 1개 라인
//    uint boneIdx = vid / 2; // 어떤 본 라인인지
//    uint endIdx = vid % 2; // 0: 시작점, 1: 끝점

//    // 범위 체크
//    if (boneIdx >= count)
//    {
//        o.pos = float4(0, 0, 0, 0);
//        return o;
//    }

//    // 지금은 "본의 중심 점"만 찍는 형태로
//    // (부모 인덱스가 없으니 뼈대 연결은 못하고 길이 0 라인만 생김)
//    float3 p = GetBonePos(boneIdx);
//    float3 p2 = p + float3(0.02f, 0, 0); // X로
//    float3 wp = (endIdx == 0) ? p : p2;
    
//    // endIdx로 살짝 offset 주면 점처럼 보이게도 가능하지만
//    // 일단 동일 점 2번 넣어서 "degenerate line"로 둠
//    float4 worldPos = mul(float4(wp, 1), mWorld);
    
//    float4 viewPos = mul(worldPos, mView);
//    float4 clipPos = mul(viewPos, mProj);

//    o.pos = clipPos;
//    return o;
//}


VSOutput VS_Main(VSInput_PNUT input)
{
    VSOutput o = (VSOutput) 0;
    
    float4 pos;
    pos = float4(input.pos, 1.0f);
    pos = Skinning(pos, input.boneWeights, input.boneIndices);

    pos = mul(pos, mWorld);
    float4 wPos = pos;

    pos = mul(pos, mView);
    pos = mul(pos, mProj);
    
    float4 nrm;
    nrm = float4(input.nrm, 0.0f);
    
    o.pos = pos;
    o.nrm = nrm;
    o.uv = input.uv;
    o.wPos = wPos;
    return o;
}