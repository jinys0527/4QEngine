// FBX 일괄 재임포트 드라이버
//
// Importer::ImportAll() 은 `../Resources/FBX` 를 스캔하지만 이 저장소의 원본 FBX는
// `MRenderer/fx` 에 있어 아무것도 찾지 못한다. 이 도구는 원본 디렉터리를 인자로 받아
// ImportFBX() 를 직접 호출한다.
//
//   reimport.exe [fbxDir] [outDir]
//   기본값 : MRenderer/fx  ->  ResourceOutput
//
// 주의 : <asset>/SkelMeta/<asset>.skelmeta.json 에 upperBodyBones/lowerBodyBones 가
// 들어 있으면 임포터가 그 값을 그대로 재사용하고 자동 분류를 건너뛴다.
// 자동 분류를 다시 돌리려면 해당 파일을 `{}` 로 비우고 실행한다.

#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>
#include <algorithm>

namespace fs = std::filesystem;

void ImportFBX(const std::string& FBXPath, const std::string& outDir);

int main(int argc, char** argv) {
    const fs::path fbxDir = (argc > 1) ? fs::u8path(argv[1]) : fs::u8path("MRenderer/fx");
    const fs::path outDir = (argc > 2) ? fs::u8path(argv[2]) : fs::u8path("ResourceOutput");

    if (!fs::exists(fbxDir) || !fs::is_directory(fbxDir)) {
        printf("[!] FBX 디렉터리를 찾을 수 없음: %s\n", fbxDir.string().c_str());
        return 1;
    }

    std::vector<fs::path> targets;
    for (const auto& entry : fs::directory_iterator(fbxDir)) {
        if (!entry.is_regular_file()) continue;
        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return (char)::tolower(c); });
        if (ext == ".fbx") targets.push_back(entry.path());
    }
    std::sort(targets.begin(), targets.end());

    printf("원본 : %s\n출력 : %s\n대상 : %zu개\n\n",
        fbxDir.string().c_str(), outDir.string().c_str(), targets.size());

    int ok = 0;
    for (const auto& p : targets) {
        printf("[%d/%zu] %s ... ", ok + 1, targets.size(), p.filename().string().c_str());
        fflush(stdout);
        ImportFBX(p.string(), outDir.string());
        printf("완료\n");
        ++ok;
    }

    printf("\n%d개 임포트 완료\n", ok);
    return 0;
}
