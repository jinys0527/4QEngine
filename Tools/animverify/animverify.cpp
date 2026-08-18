// 4QEngine 애니메이션 검증 도구 (헤드리스)
//   A) 상하체 본 마스크가 스켈레톤 계층과 정합한가
//   B) BlendLocalPoses 의 마스크 가중 블렌딩이 의도대로 동작하는가
//   C) SetRetargetFromBindPose / SetRetargetFromSkeletonHandles 의 리타겟 오프셋이 올바른가
//
// 렌더링 없이 .skelbin 을 직접 읽어 AnimationComponent 의 계산을 그대로 재현한다.

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <DirectXMath.h>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cmath>
#include <fstream>
#include <string>
#include <unordered_set>
#include <vector>

using namespace DirectX;

// ---- .skelbin 포맷 (ResourceManager/AssetLoader.cpp:95-118 의 리더 기준) ----
#pragma pack(push, 1)
struct SkelBinHeader {
    uint32_t magic; uint16_t version; uint16_t boneCount;
    uint32_t stringTableBytes; uint32_t upperCount; uint32_t lowerCount;
};
struct SkelBinEquipmentData { int32_t equipmentBoneIndex; float equipmentBindPose[16]; };
struct BoneBin {
    uint32_t nameOffset; int32_t parentIndex;
    float inverseBindPose[16]; float localBind[16];
};
#pragma pack(pop)

struct Bone { std::string name; int parentIndex = -1; XMFLOAT4X4 bindPose{}; XMFLOAT4X4 inverseBindPose{}; };
struct Skeleton {
    std::vector<Bone> bones;
    std::vector<int> upperBodyBones, lowerBodyBones;
    int equipmentBoneIndex = -1;
    uint16_t version = 0;
};

static std::string ReadStringAtOffset(const std::string& table, uint32_t offset) {
    if (offset >= table.size()) return {};
    return std::string(table.c_str() + offset);
}

static bool LoadSkeleton(const char* path, Skeleton& out) {
    std::ifstream s(path, std::ios::binary);
    if (!s) { printf("  [!] 열 수 없음: %s\n", path); return false; }

    SkelBinHeader h{};
    s.read(reinterpret_cast<char*>(&h), sizeof(h));
    if (h.magic != 0x534B454C) { printf("  [!] magic 불일치: %s\n", path); return false; }
    out.version = h.version;

    if (h.version >= 4) {
        SkelBinEquipmentData eq{};
        s.read(reinterpret_cast<char*>(&eq), sizeof(eq));
        out.equipmentBoneIndex = eq.equipmentBoneIndex;
    }
    if (h.version >= 3) { float gi[16]{}; s.read(reinterpret_cast<char*>(gi), sizeof(gi)); }

    std::vector<BoneBin> bins(h.boneCount);
    s.read(reinterpret_cast<char*>(bins.data()), sizeof(BoneBin) * bins.size());

    std::string table;
    if (h.stringTableBytes > 0) { table.resize(h.stringTableBytes); s.read(table.data(), h.stringTableBytes); }

    out.bones.reserve(h.boneCount);
    for (const auto& b : bins) {
        Bone bone{};
        bone.name = ReadStringAtOffset(table, b.nameOffset);
        bone.parentIndex = (b.parentIndex >= 0 && b.parentIndex < h.boneCount) ? b.parentIndex : -1;
        std::memcpy(&bone.bindPose, b.localBind, sizeof(float) * 16);
        std::memcpy(&bone.inverseBindPose, b.inverseBindPose, sizeof(float) * 16);
        out.bones.push_back(std::move(bone));
    }
    if (h.upperCount > 0) {
        std::vector<int32_t> v(h.upperCount);
        s.read(reinterpret_cast<char*>(v.data()), sizeof(int32_t) * v.size());
        out.upperBodyBones.assign(v.begin(), v.end());
    }
    if (h.lowerCount > 0) {
        std::vector<int32_t> v(h.lowerCount);
        s.read(reinterpret_cast<char*>(v.data()), sizeof(int32_t) * v.size());
        out.lowerBodyBones.assign(v.begin(), v.end());
    }
    return true;
}

// ---- AnimationComponent 의 LocalPose / 블렌딩 재현 ----
struct LocalPose { XMFLOAT3 translation{ 0,0,0 }; XMFLOAT4 rotation{ 0,0,0,1 }; XMFLOAT3 scale{ 1,1,1 }; };

static XMFLOAT3 Lerp3(const XMFLOAT3& a, const XMFLOAT3& b, float t) {
    return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t };
}
static XMFLOAT4 Slerp4(const XMFLOAT4& a, const XMFLOAT4& b, float t) {
    XMFLOAT4 r; XMStoreFloat4(&r, XMQuaternionSlerp(XMLoadFloat4(&a), XMLoadFloat4(&b), t)); return r;
}

// AnimationComponent::BlendLocalPoses (AnimationComponent.cpp:854) 재현
static void BlendLocalPoses(const std::vector<LocalPose>& from, const std::vector<LocalPose>& to,
                            float alpha, const std::vector<float>& mask, std::vector<LocalPose>& blended) {
    const size_t n = (std::max)(from.size(), to.size());
    blended.resize(n);
    const float ca = std::clamp(alpha, 0.0f, 1.0f);
    for (size_t i = 0; i < n; ++i) {
        const LocalPose f = (i < from.size()) ? from[i] : LocalPose{};
        const LocalPose t = (i < to.size()) ? to[i] : LocalPose{};
        const float m = (i < mask.size()) ? mask[i] : 1.0f;
        const float wa = std::clamp(ca * m, 0.0f, 1.0f);
        blended[i] = LocalPose{};
        blended[i].translation = Lerp3(f.translation, t.translation, wa);
        blended[i].scale = Lerp3(f.scale, t.scale, wa);
        blended[i].rotation = Slerp4(f.rotation, t.rotation, wa);
        XMStoreFloat4(&blended[i].rotation, XMQuaternionNormalize(XMLoadFloat4(&blended[i].rotation)));
    }
}

// AnimationComponent::SetBoneMaskFromIndices (AnimationComponent.cpp:323) 재현
static std::vector<float> BuildMask(size_t boneCount, const std::vector<int>& indices,
                                    float weight, float defaultWeight) {
    std::vector<float> m(boneCount, defaultWeight);
    for (int idx : indices) if (idx >= 0 && (size_t)idx < m.size()) m[idx] = weight;
    return m;
}

struct RetargetOffset { XMFLOAT3 translation{ 0,0,0 }; XMFLOAT4 rotation{ 0,0,0,1 }; XMFLOAT3 scale{ 1,1,1 }; };

// AnimationComponent::SetRetargetFromBindPose (AnimationComponent.cpp:451) 재현
static std::vector<RetargetOffset> BuildRetargetOffsets(
    const std::vector<XMFLOAT4X4>& sourceBind, const std::vector<XMFLOAT4X4>& targetBind, int& outDecomposeFail) {
    outDecomposeFail = 0;
    const size_t n = (std::min)(sourceBind.size(), targetBind.size());
    std::vector<RetargetOffset> offsets; offsets.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        const auto src = XMLoadFloat4x4(&sourceBind[i]);
        const auto tgt = XMLoadFloat4x4(&targetBind[i]);
        const auto srcInv = XMMatrixInverse(nullptr, src);
        const auto off = XMMatrixMultiply(tgt, srcInv);   // MathUtils::Mul(target, sourceInv)
        XMVECTOR s, r, t;
        RetargetOffset o{};
        if (XMMatrixDecompose(&s, &r, &t, off)) {
            XMStoreFloat3(&o.translation, t); XMStoreFloat4(&o.rotation, r); XMStoreFloat3(&o.scale, s);
        } else { ++outDecomposeFail; }
        offsets.push_back(o);
    }
    return offsets;
}

static int g_pass = 0, g_fail = 0;
static void Check(bool ok, const char* fmt, ...) {
    va_list ap; va_start(ap, fmt);
    printf(ok ? "  [PASS] " : "  [FAIL] " ); vprintf(fmt, ap); printf("\n");
    va_end(ap);
    ok ? ++g_pass : ++g_fail;
}

// ---------- A. 마스크 <-> 계층 정합성 ----------
static void VerifyMask(const Skeleton& sk, const char* label,
                       const std::vector<int>& idx, const char* setName) {
    if (idx.empty()) { printf("  (%s: 마스크 없음)\n", setName); return; }

    std::unordered_set<int> inSet(idx.begin(), idx.end());

    bool inRange = true;
    for (int i : idx) if (i < 0 || (size_t)i >= sk.bones.size()) inRange = false;
    Check(inRange, "%s: 모든 인덱스가 본 범위(0..%zu) 안", setName, sk.bones.size() - 1);

    Check(inSet.size() == idx.size(), "%s: 중복 인덱스 없음 (%zu개)", setName, idx.size());

    // 마스크 집합의 '루트' = 부모가 집합 밖인 본. 잘 만들어진 마스크는 하나의 서브트리여야 한다.
    std::vector<int> roots;
    for (int i : idx) {
        if (i < 0 || (size_t)i >= sk.bones.size()) continue;
        const int p = sk.bones[i].parentIndex;
        if (p < 0 || !inSet.count(p)) roots.push_back(i);
    }
    Check(roots.size() == 1, "%s: 연결된 단일 서브트리 (루트 %zu개)", setName, roots.size());
    for (int r : roots) printf("         루트 본 : [%d] %s\n", r, sk.bones[r].name.c_str());

    // 서브트리 폐쇄성: 집합 안 본의 자식이 집합 밖으로 새는가
    int leaks = 0;
    for (size_t c = 0; c < sk.bones.size(); ++c) {
        const int p = sk.bones[c].parentIndex;
        if (p >= 0 && inSet.count(p) && !inSet.count((int)c)) ++leaks;
    }
    printf("         하위로 새는 본 : %d개 %s\n", leaks, leaks ? "(마스크 경계 바깥 자식)" : "");
}

int main() {
    const char* kQuinn = "ResourceOutput/SKM_Quinn_Simple/Skels/SKM_Quinn_Simple.skelbin";
    const char* kWalk = "ResourceOutput/Unarmed Walk Forward/Skels/Unarmed Walk Forward.skelbin";
    const char* kDying = "ResourceOutput/Dying/Skels/Dying.skelbin";

    Skeleton quinn, walk, dying;
    if (!LoadSkeleton(kQuinn, quinn)) return 1;
    LoadSkeleton(kWalk, walk);
    LoadSkeleton(kDying, dying);

    printf("=====================================================================\n");
    printf(" A. 상하체 본 마스크 검증\n");
    printf("=====================================================================\n");
    printf("SKM_Quinn_Simple : v%u, 본 %zu개, 상체 %zu, 하체 %zu, equipmentBone=%d\n\n",
        quinn.version, quinn.bones.size(), quinn.upperBodyBones.size(),
        quinn.lowerBodyBones.size(), quinn.equipmentBoneIndex);

    VerifyMask(quinn, "Quinn", quinn.upperBodyBones, "상체");
    printf("\n");
    VerifyMask(quinn, "Quinn", quinn.lowerBodyBones, "하체");
    printf("\n");

    // 상/하체 교집합
    {
        std::unordered_set<int> up(quinn.upperBodyBones.begin(), quinn.upperBodyBones.end());
        std::vector<int> both;
        for (int i : quinn.lowerBodyBones) if (up.count(i)) both.push_back(i);
        Check(both.empty(), "상체 ∩ 하체 = 공집합 (겹침 %zu개)", both.size());
        for (int i : both) printf("         겹침 : [%d] %s\n", i, quinn.bones[i].name.c_str());

        const size_t covered = quinn.upperBodyBones.size() + quinn.lowerBodyBones.size() - both.size();
        printf("         커버리지 : %zu / %zu 본 (미분류 %zu)\n\n",
            covered, quinn.bones.size(), quinn.bones.size() - covered);

        printf("         미분류 본 (기본 가중치 적용) :\n");
        std::unordered_set<int> classified(up.begin(), up.end());
        for (int i : quinn.lowerBodyBones) classified.insert(i);
        int shown = 0;
        for (size_t i = 0; i < quinn.bones.size(); ++i) {
            if (classified.count((int)i)) continue;
            if (shown++ < 30) printf("           [%2zu] %s\n", i, quinn.bones[i].name.c_str());
        }
        if (shown > 30) printf("           ... 외 %d개\n", shown - 30);
    }

    printf("\n=====================================================================\n");
    printf(" B. 마스크 가중 블렌딩 동작 검증 (BlendLocalPoses)\n");
    printf("=====================================================================\n");
    {
        const size_t n = quinn.bones.size();
        // from = 하체 애니메이션(이동), to = 상체 애니메이션(공격) 이라고 가정한 서로 다른 포즈
        std::vector<LocalPose> from(n), to(n);
        for (size_t i = 0; i < n; ++i) {
            from[i].translation = { 1.0f, 0.0f, 0.0f };
            XMStoreFloat4(&from[i].rotation, XMQuaternionRotationRollPitchYaw(0, 0, 0));
            to[i].translation = { 0.0f, 5.0f, 0.0f };
            XMStoreFloat4(&to[i].rotation, XMQuaternionRotationRollPitchYaw(0, XM_PIDIV2, 0));
        }

        // UseSkeletonUpperBodyMask(weight=1, defaultWeight=0)
        const auto mask = BuildMask(n, quinn.upperBodyBones, 1.0f, 0.0f);
        std::vector<LocalPose> blended;
        BlendLocalPoses(from, to, 1.0f, mask, blended);

        std::unordered_set<int> up(quinn.upperBodyBones.begin(), quinn.upperBodyBones.end());
        int upperOk = 0, upperBad = 0, otherOk = 0, otherBad = 0;
        for (size_t i = 0; i < n; ++i) {
            const bool isUpper = up.count((int)i) != 0;
            const auto& b = blended[i];
            const bool tookTo = std::fabs(b.translation.y - 5.0f) < 1e-4f && std::fabs(b.translation.x) < 1e-4f;
            const bool tookFrom = std::fabs(b.translation.x - 1.0f) < 1e-4f && std::fabs(b.translation.y) < 1e-4f;
            if (isUpper) { tookTo ? ++upperOk : ++upperBad; }
            else { tookFrom ? ++otherOk : ++otherBad; }
        }
        Check(upperBad == 0, "alpha=1, 상체마스크: 상체 본 %d개가 상체 포즈를 100%% 취함 (불일치 %d)", upperOk, upperBad);
        Check(otherBad == 0, "alpha=1, 상체마스크: 비상체 본 %d개가 하체 포즈를 유지 (불일치 %d)", otherOk, otherBad);

        // alpha 절반 지점 확인
        BlendLocalPoses(from, to, 0.5f, mask, blended);
        bool halfOk = true;
        for (int i : quinn.upperBodyBones)
            if (std::fabs(blended[i].translation.y - 2.5f) > 1e-4f) halfOk = false;
        Check(halfOk, "alpha=0.5: 상체 본이 정확히 중간 지점 (y=2.5)");

        // 마스크 미적용 시 전체 본이 동일하게 블렌딩되는가
        BlendLocalPoses(from, to, 1.0f, {}, blended);
        bool allOk = true;
        for (size_t i = 0; i < n; ++i) if (std::fabs(blended[i].translation.y - 5.0f) > 1e-4f) allOk = false;
        Check(allOk, "마스크 없음: 전체 %zu본이 동일하게 블렌딩", n);
    }

    printf("\n=====================================================================\n");
    printf(" C. 리타겟팅 검증 (SetRetargetFromBindPose)\n");
    printf("=====================================================================\n");
    {
        // C-1. source == target 이면 오프셋은 반드시 항등이어야 한다
        std::vector<XMFLOAT4X4> bind;
        for (const auto& b : quinn.bones) bind.push_back(b.bindPose);
        int fail = 0;
        auto offsets = BuildRetargetOffsets(bind, bind, fail);

        int nonIdentity = 0; float worstT = 0, worstR = 0, worstS = 0;
        for (const auto& o : offsets) {
            const float dt = (std::max)({ std::fabs(o.translation.x), std::fabs(o.translation.y), std::fabs(o.translation.z) });
            const float ds = (std::max)({ std::fabs(o.scale.x - 1), std::fabs(o.scale.y - 1), std::fabs(o.scale.z - 1) });
            const float dr = std::fabs(std::fabs(o.rotation.w) - 1.0f);
            worstT = (std::max)(worstT, dt); worstS = (std::max)(worstS, ds); worstR = (std::max)(worstR, dr);
            if (dt > 1e-3f || ds > 1e-3f || dr > 1e-3f) ++nonIdentity;
        }
        Check(fail == 0, "동일 스켈레톤: XMMatrixDecompose 실패 %d건", fail);
        Check(nonIdentity == 0,
            "동일 스켈레톤 -> 항등 오프셋 (이탈 %d본, 최대오차 T=%.2e R=%.2e S=%.2e)",
            nonIdentity, worstT, worstR, worstS);

        // C-2. 서로 다른 리그: 인덱스 기준으로 짝지어지므로 본 이름이 대응해야 한다
        if (!walk.bones.empty()) {
            printf("\n  --- 교차 리타겟 : SKM_Quinn_Simple(%zu본) <- Unarmed Walk Forward(%zu본) ---\n",
                quinn.bones.size(), walk.bones.size());

            const size_t n = (std::min)(quinn.bones.size(), walk.bones.size());
            int nameMatch = 0; std::vector<size_t> mismatches;
            for (size_t i = 0; i < n; ++i) {
                if (quinn.bones[i].name == walk.bones[i].name) ++nameMatch;
                else if (mismatches.size() < 12) mismatches.push_back(i);
            }
            printf("  인덱스 기준 짝짓기 대상 : %zu본 (Quinn %zu본 중 %zu본은 잘려나감)\n",
                n, quinn.bones.size(), quinn.bones.size() - n);
            Check(nameMatch == (int)n,
                "인덱스 i의 본 이름이 양쪽에서 일치 : %d/%zu", nameMatch, n);
            for (size_t i : mismatches)
                printf("         [%2zu] Quinn=\"%s\"  vs  Walk=\"%s\"\n",
                    i, quinn.bones[i].name.c_str(), walk.bones[i].name.c_str());

            std::vector<XMFLOAT4X4> srcBind, tgtBind;
            for (size_t i = 0; i < n; ++i) { srcBind.push_back(walk.bones[i].bindPose); tgtBind.push_back(quinn.bones[i].bindPose); }
            int xfail = 0;
            auto xoff = BuildRetargetOffsets(srcBind, tgtBind, xfail);
            Check(xfail == 0, "교차 리타겟: XMMatrixDecompose 실패 %d건", xfail);

            float maxT = 0; int huge = 0;
            for (const auto& o : xoff) {
                const float dt = (std::max)({ std::fabs(o.translation.x), std::fabs(o.translation.y), std::fabs(o.translation.z) });
                maxT = (std::max)(maxT, dt);
                if (dt > 100.0f) ++huge;
            }
            printf("  오프셋 최대 translation : %.3f (100 초과 본 %d개)\n", maxT, huge);
        }
    }

    printf("\n=====================================================================\n");
    printf(" 결과 : PASS %d / FAIL %d\n", g_pass, g_fail);
    printf("=====================================================================\n");
    return g_fail == 0 ? 0 : 1;
}
