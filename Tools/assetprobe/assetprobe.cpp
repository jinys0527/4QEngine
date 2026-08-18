// AssetLoader::LoadAll() 을 헤드리스로 돌려 무엇이 등록되는지 확인한다.
// 에디터 ResourceBrowser 가 비어 보일 때 원인이 경로인지 파싱인지 가르는 용도.
//
// 주의 : AssetLoader 는 `../ResourceOutput` 을 상대 경로로 스캔한다.
//        반드시 저장소 루트의 '한 단계 아래' 디렉터리에서 실행해야 한다. (예: x64\)

#include <cstdio>
#include <filesystem>
#include <string>

#include "AssetLoader.h"

namespace fs = std::filesystem;

int main() {
    printf("cwd            : %s\n", fs::current_path().string().c_str());
    const fs::path assetRoot = "../ResourceOutput";
    printf("assetRoot      : %s\n", assetRoot.string().c_str());
    printf("  -> 절대경로  : %s\n", fs::weakly_canonical(assetRoot).string().c_str());
    printf("  -> 존재      : %s\n\n", fs::exists(assetRoot) ? "YES" : "NO  <-- 이러면 아무것도 안 뜬다");

    if (!fs::exists(assetRoot))
        return 1;

    AssetLoader loader;
    AssetLoader::SetActive(&loader);
    loader.LoadAll();

    printf("등록된 리소스\n");
    printf("  meshes     : %zu\n", loader.GetMeshes().GetKeyToHandle().size());
    printf("  materials  : %zu\n", loader.GetMaterials().GetKeyToHandle().size());
    printf("  textures   : %zu\n", loader.GetTextures().GetKeyToHandle().size());
    printf("  skeletons  : %zu\n", loader.GetSkeletons().GetKeyToHandle().size());
    printf("  animations : %zu\n", loader.GetAnimations().GetKeyToHandle().size());
    printf("  vshaders   : %zu\n", loader.GetVertexShaders().GetKeyToHandle().size());
    printf("  pshaders   : %zu\n", loader.GetPixelShaders().GetKeyToHandle().size());

    printf("\n에셋별 상세\n");
    for (const auto& dirEntry : fs::directory_iterator(assetRoot)) {
        if (!dirEntry.is_directory()) continue;
        const fs::path metaDir = dirEntry.path() / "Meta";
        if (!fs::exists(metaDir)) continue;
        for (const auto& f : fs::directory_iterator(metaDir)) {
            const std::string fn = f.path().filename().string();
            if (fn.find(".asset.json") == std::string::npos) continue;
            const auto* res = loader.GetAsset(f.path().string());
            if (!res) { printf("  %-24s : (로드 실패)\n", dirEntry.path().filename().string().c_str()); continue; }
            printf("  %-24s : mesh %zu, mat %zu, tex %zu, skel %s, anim %zu\n",
                dirEntry.path().filename().string().c_str(),
                res->meshes.size(), res->materials.size(), res->textures.size(),
                res->skeleton.IsValid() ? "O" : "X", res->animations.size());
        }
    }
    return 0;
}
