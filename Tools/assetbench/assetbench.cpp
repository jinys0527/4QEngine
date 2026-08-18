// 4QEngine 에셋 로드 벤치마크
//  A) .meshbin 런타임 로드 경로 (AssetLoader.cpp:1262-1315 재현)
//  B) 원본 FBX를 Assimp로 로드 (Importer.cpp:593-601 과 동일 플래그)
// Assimp는 헤더/lib 없이 C API를 동적 로드해서 호출한다.

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <DirectXMath.h>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>

using namespace DirectX;
namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;

// ---- 엔진 포맷 구조체 (Importer/MeshBin.h, SkeletonBin.h 와 동일) ----
#pragma pack(push, 1)
struct AABBf { float min[3]; float max[3]; };
struct MeshBinHeader {
    uint32_t magic; uint16_t version; uint16_t flags;
    uint32_t vertexCount, indexCount, subMeshCount, instanceCount, stringTableBytes;
    AABBf bounds;
};
struct SubMeshBin {
    uint32_t indexStart, indexCount, materialNameOffset, nameOffset;
    AABBf bounds; uint32_t instanceStart, instanceCount;
};
struct InstanceTransformBin { XMFLOAT4X4 localToWorld; };
struct Vertex { float px, py, pz, nx, ny, nz, u, v, tx, ty, tz, handedness; };
struct VertexSkinned : Vertex { uint16_t boneIndex[4]; uint16_t boneWeight[4]; };
struct SkelBinHeader {
    uint32_t magic; uint16_t version; uint16_t boneCount;
    uint32_t stringTableBytes, upperCount, lowerCount;
    int32_t equipmentBoneIndex; float equipmentBindPose[16];
};
#pragma pack(pop)

enum MeshFlags : uint16_t { MESH_HAS_SKINNING = 1 << 0 };

// ---- 런타임 정점 (ResourceManager/RenderData.h) ----
struct RVertex {
    XMFLOAT3 position{}; XMFLOAT3 normal{}; XMFLOAT2 uv{}; XMFLOAT4 tangent{ 0,0,0,1 };
    std::array<uint16_t, 4> boneIndices{}; std::array<float, 4> boneWeights{};
};

static RVertex ToRenderVertex(const Vertex& in) {
    RVertex out{};
    out.position = { in.px, in.py, in.pz };
    out.normal = { in.nx, in.ny, in.nz };
    out.uv = { in.u, in.v };
    out.tangent = { in.tx, in.ty, in.tz, in.handedness };
    return out;
}
static RVertex ToRenderVertex(const VertexSkinned& in) {
    RVertex out = ToRenderVertex(static_cast<const Vertex&>(in));
    out.boneIndices = { in.boneIndex[0], in.boneIndex[1], in.boneIndex[2], in.boneIndex[3] };
    const float inv = 1.0f / 65535.0f;
    out.boneWeights = { in.boneWeight[0] * inv, in.boneWeight[1] * inv,
                        in.boneWeight[2] * inv, in.boneWeight[3] * inv };
    float sum = out.boneWeights[0] + out.boneWeights[1] + out.boneWeights[2] + out.boneWeights[3];
    if (sum > 0.0f) { float is = 1.0f / sum; for (auto& w : out.boneWeights) w *= is; }
    else { out.boneIndices = { 0,0,0,0 }; out.boneWeights = { 1.f,0.f,0.f,0.f }; }
    return out;
}

struct MeshStats {
    bool ok = false; uint16_t version = 0; bool skinned = false;
    uint32_t verts = 0, indices = 0, subMeshes = 0, instances = 0;
    size_t cpuBytes = 0;
};

// AssetLoader 의 meshbin 읽기 경로를 그대로 재현
static MeshStats LoadMeshBin(const fs::path& p) {
    MeshStats st;
    std::ifstream in(p, std::ios::binary);
    if (!in) return st;

    MeshBinHeader h{};
    in.read(reinterpret_cast<char*>(&h), sizeof(h));
    if (h.magic != 0x4D455348) return st;

    std::vector<SubMeshBin> subs(h.subMeshCount);
    in.read(reinterpret_cast<char*>(subs.data()), sizeof(SubMeshBin) * subs.size());

    std::vector<InstanceTransformBin> insts;
    if (h.version >= 3 && h.instanceCount > 0) {
        insts.resize(h.instanceCount);
        in.read(reinterpret_cast<char*>(insts.data()), sizeof(InstanceTransformBin) * insts.size());
    }

    const bool skinned = (h.flags & MESH_HAS_SKINNING) != 0;
    std::vector<RVertex> outVerts;
    outVerts.reserve(h.vertexCount);
    if (skinned) {
        std::vector<VertexSkinned> vs(h.vertexCount);
        in.read(reinterpret_cast<char*>(vs.data()), sizeof(VertexSkinned) * vs.size());
        for (const auto& v : vs) outVerts.push_back(ToRenderVertex(v));
    }
    else {
        std::vector<Vertex> vs(h.vertexCount);
        in.read(reinterpret_cast<char*>(vs.data()), sizeof(Vertex) * vs.size());
        for (const auto& v : vs) outVerts.push_back(ToRenderVertex(v));
    }

    std::vector<uint32_t> idx(h.indexCount);
    in.read(reinterpret_cast<char*>(idx.data()), sizeof(uint32_t) * idx.size());

    std::string strTable;
    if (h.stringTableBytes > 0) { strTable.resize(h.stringTableBytes); in.read(strTable.data(), h.stringTableBytes); }

    st.ok = true; st.version = h.version; st.skinned = skinned;
    st.verts = h.vertexCount; st.indices = h.indexCount;
    st.subMeshes = h.subMeshCount; st.instances = h.instanceCount;
    st.cpuBytes = outVerts.size() * sizeof(RVertex) + idx.size() * sizeof(uint32_t);

    volatile float sink = outVerts.empty() ? 0.f : outVerts[0].position.x; // 최적화 방지
    (void)sink;
    return st;
}

static bool ReadSkelHeader(const fs::path& p, SkelBinHeader& out) {
    std::ifstream f(p, std::ios::binary);
    if (!f) return false;
    f.read(reinterpret_cast<char*>(&out), sizeof(out));
    return out.magic == 0x534B454C;
}

// ---- Assimp C API 동적 로드 ----
typedef const void* (__cdecl* PFN_aiImportFile)(const char*, unsigned int);
typedef void (__cdecl* PFN_aiReleaseImport)(const void*);
typedef const char* (__cdecl* PFN_aiGetErrorString)(void);

// Importer.cpp:593-601 과 동일한 플래그
static const unsigned int kFlags =
      0x8            // aiProcess_Triangulate
    | 0x20           // aiProcess_GenNormals
    | 0x1            // aiProcess_CalcTangentSpace
    | 0x8000000      // aiProcess_GlobalScale
    | 0x2            // aiProcess_JoinIdenticalVertices
    | (0x4 | 0x800000 | 0x1000000); // aiProcess_ConvertToLeftHanded

static double Median(std::vector<double>& v) {
    if (v.empty()) return -1.0;
    std::sort(v.begin(), v.end());
    return v[v.size() / 2];
}

int main(int argc, char** argv) {
    const int iters = (argc > 1) ? atoi(argv[1]) : 7;

    // Release 빌드 Assimp 우선. 의존 DLL(zlib1, pugixml...)이 같은 폴더에서 풀리도록
    // 절대경로 + LOAD_WITH_ALTERED_SEARCH_PATH 로 로드한다.
    const char* dllCandidates[] = {
        "C:\\ys\\vcpkg\\installed\\x64-windows\\bin\\assimp-vc143-mt.dll",   // Release
        "C:\\ys\\4QEngine\\x64\\assimp-vc143-mtd.dll",                       // Debug (fallback)
    };
    HMODULE dll = nullptr;
    const char* usedDll = "(none)";
    for (const char* cand : dllCandidates) {
        dll = LoadLibraryExA(cand, nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
        if (dll) { usedDll = cand; break; }
        printf("  [load fail] %s (GetLastError=%lu)\n", cand, GetLastError());
    }

    PFN_aiImportFile aiImportFile = nullptr;
    PFN_aiReleaseImport aiReleaseImport = nullptr;
    PFN_aiGetErrorString aiGetErrorString = nullptr;
    if (dll) {
        aiImportFile = (PFN_aiImportFile)GetProcAddress(dll, "aiImportFile");
        aiReleaseImport = (PFN_aiReleaseImport)GetProcAddress(dll, "aiReleaseImport");
        aiGetErrorString = (PFN_aiGetErrorString)GetProcAddress(dll, "aiGetErrorString");
    }
    printf("assimp dll : %s\n", (dll && aiImportFile) ? usedDll : "NOT LOADED");
    printf("flags      : 0x%08X   iters = %d\n\n", kFlags, iters);

    // 원본 FBX와 .meshbin 이 모두 존재하는 에셋을 전부 수집한다.
    // FBX 는 Resources/FBX 를 먼저 보고, 없으면 MRenderer/fx 를 본다.
    std::vector<std::string> names;
    {
        const fs::path outRoot("ResourceOutput");
        if (fs::exists(outRoot))
        {
            for (const auto& dir : fs::directory_iterator(outRoot))
            {
                if (!dir.is_directory()) continue;
                const std::string name = dir.path().filename().string();
                const fs::path bin = dir.path() / "Meshes" / (name + ".meshbin");
                if (!fs::exists(bin)) continue;
                if (!fs::exists(fs::path("Resources/FBX") / (name + ".fbx"))
                 && !fs::exists(fs::path("MRenderer/fx") / (name + ".fbx"))) continue;
                names.push_back(name);
            }
        }
        std::sort(names.begin(), names.end());
        printf("대상 에셋 : %zu개 (FBX 원본과 .meshbin 이 모두 있는 것)\n\n", names.size());
    }

    printf("%-22s %9s %9s %9s %8s %5s %6s | %10s %9s %8s | %6s %8s\n",
        "asset", "fbx(KB)", "bin(KB)", "cpu(KB)", "verts", "subs", "skin",
        "fbx(ms)", "bin(ms)", "ratio", "bones", "up/low");
    printf("%s\n", std::string(140, '-').c_str());

    double totFbx = 0, totBin = 0;
    unsigned long long totVerts = 0, totSubs = 0, totFbxKB = 0, totBinKB = 0, totCpuKB = 0;
    int totSkinned = 0;

    for (const std::string& nameStr : names) {
        const char* name = nameStr.c_str();
        fs::path fbx = fs::path("Resources/FBX") / (nameStr + ".fbx");
        if (!fs::exists(fbx)) fbx = fs::path("MRenderer/fx") / (nameStr + ".fbx");
        const fs::path bin = fs::path("ResourceOutput") / nameStr / "Meshes" / (nameStr + ".meshbin");
        const fs::path skel = fs::path("ResourceOutput") / nameStr / "Skels" / (nameStr + ".skelbin");

        if (!fs::exists(bin)) { printf("%-22s  (meshbin 없음)\n", name); continue; }

        MeshStats st = LoadMeshBin(bin); // 워밍업
        std::vector<double> binMs;
        for (int i = 0; i < iters; ++i) {
            auto t0 = Clock::now();
            LoadMeshBin(bin);
            binMs.push_back(std::chrono::duration<double, std::milli>(Clock::now() - t0).count());
        }

        std::vector<double> fbxMs;
        if (aiImportFile && fs::exists(fbx)) {
            const std::string fbxStr = fbx.string();
            if (const void* warm = aiImportFile(fbxStr.c_str(), kFlags)) aiReleaseImport(warm);
            else printf("  [assimp error] %s : %s\n", name, aiGetErrorString ? aiGetErrorString() : "?");
            for (int i = 0; i < iters; ++i) {
                auto t0 = Clock::now();
                const void* sc = aiImportFile(fbxStr.c_str(), kFlags);
                double ms = std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
                if (sc) { aiReleaseImport(sc); fbxMs.push_back(ms); }
            }
        }

        SkelBinHeader sh{};
        const bool hasSkel = ReadSkelHeader(skel, sh);

        const double fbxMed = Median(fbxMs);
        const double binMed = Median(binMs);
        if (fbxMed > 0) totFbx += fbxMed;
        if (binMed > 0) totBin += binMed;

        const auto fbxKB = fs::exists(fbx) ? fs::file_size(fbx) / 1024 : 0;
        const auto binKB = fs::file_size(bin) / 1024;

        totVerts += st.verts;
        totSubs += st.subMeshes;
        totFbxKB += fbxKB;
        totBinKB += binKB;
        totCpuKB += st.cpuBytes / 1024;
        if (st.skinned) ++totSkinned;

        char ratio[16], bonesS[16], uplow[16], fbxS[16];
        if (fbxMed > 0 && binMed > 0) snprintf(ratio, sizeof(ratio), "%.1fx", fbxMed / binMed);
        else snprintf(ratio, sizeof(ratio), "-");
        if (fbxMed > 0) snprintf(fbxS, sizeof(fbxS), "%.2f", fbxMed);
        else snprintf(fbxS, sizeof(fbxS), "-");
        if (hasSkel) {
            snprintf(bonesS, sizeof(bonesS), "%u", sh.boneCount);
            snprintf(uplow, sizeof(uplow), "%u/%u", sh.upperCount, sh.lowerCount);
        }
        else { snprintf(bonesS, sizeof(bonesS), "-"); snprintf(uplow, sizeof(uplow), "-"); }

        printf("%-22s %9llu %9llu %9llu %8u %5u %6s | %10s %9.3f %8s | %6s %8s\n",
            name, (unsigned long long)fbxKB, (unsigned long long)binKB,
            (unsigned long long)(st.cpuBytes / 1024),
            st.verts, st.subMeshes, st.skinned ? "Y" : "N",
            fbxS, binMed, ratio, bonesS, uplow);
    }

    printf("%s\n", std::string(140, '-').c_str());
    printf("합계 %zu종\n", names.size());
    printf("  로드 시간 : FBX %.1f ms  ->  meshbin %.2f ms   (%.1f배)\n",
        totFbx, totBin, (totBin > 0) ? totFbx / totBin : 0.0);
    printf("  정점      : %llu   서브메시 : %llu   스키닝 에셋 : %d\n",
        (unsigned long long)totVerts, (unsigned long long)totSubs, totSkinned);
    printf("  파일 크기 : FBX %.1f MB  ->  meshbin %.1f MB   (%.2f배)\n",
        totFbxKB / 1024.0, totBinKB / 1024.0,
        (totFbxKB > 0) ? (double)totBinKB / (double)totFbxKB : 0.0);
    printf("  런타임 정점+인덱스 메모리 : %.1f MB\n", totCpuKB / 1024.0);

    if (dll) FreeLibrary(dll);
    return 0;
}
