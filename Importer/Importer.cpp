#include "pch.h"
#include "Importer.h"

#include <filesystem>
namespace fs = std::filesystem;

static std::string ToGenericString(const fs::path& p)
{
	// JSON/툴링에서 슬래시 통일용
	return p.generic_string();
}

static void EnsureDir(const fs::path& p)
{
	std::error_code ec;
	fs::create_directories(p, ec);
	// ec 무시할지 assert할지 정책에 맞게 처리
}

static std::string RelToBase(const fs::path& baseDir, const fs::path& fullPath)
{
	std::error_code ec;
	fs::path rel = fs::relative(fullPath, baseDir, ec);
	if (ec) return ToGenericString(fullPath); // fallback
	return ToGenericString(rel);
}

// file: "Hero_Idle.anim.json" or ".../Hero_Idle.anim.json"
// baseName: "Hero"  -> returns "Idle"
static std::string ExtractClipName(const std::string& filePath, const std::string& baseName)
{
	fs::path p = fs::u8path(filePath);
	std::string filename = p.filename().u8string();

	const std::string suffix = ".anim.json";
	if (filename.size() >= suffix.size() &&
		filename.compare(filename.size() - suffix.size(), suffix.size(), suffix) == 0)
	{
		filename.erase(filename.size() - suffix.size()); // remove ".anim.json"
	}
	else
	{
		// fallback: 그냥 stem
		filename = p.stem().u8string();
	}

	const std::string prefix = baseName + "_";
	if (filename.rfind(prefix, 0) == 0) // starts_with
	{
		filename.erase(0, prefix.size()); // remove "Hero_"
	}

	// SanitizeName는 파일명용이니까, "이름"에는 굳이 안 해도 되지만
	// 프로젝트 정책상 통일 원하면 여기서 적용
	return filename;
}

void ImportFBX(const std::string& FBXPath, const std::string& outDir)
{
	// ----- 1) 이름/경로 규칙: 단일 진실원천 -----
	const fs::path fbxPath = fs::u8path(FBXPath);
	const std::string assetName = SanitizeName(fbxPath.stem().u8string());

	const fs::path baseDir = fs::u8path(outDir) / assetName; // asset 폴더(진짜 루트)
	const fs::path meshesDir = baseDir / "Meshes";
	const fs::path animsDir = baseDir / "Anims";
	const fs::path matsDir = baseDir / "Mats";
	const fs::path texDir = baseDir / "Textures";
	const fs::path metaDir = baseDir / "Meta";

	EnsureDir(meshesDir);
	EnsureDir(animsDir);
	EnsureDir(matsDir);
	EnsureDir(texDir);
	EnsureDir(metaDir);

	const fs::path skelBinPath = metaDir / (assetName + ".skelbin");
	const fs::path matBinPath = matsDir / (assetName + ".matbin");
	const fs::path metaPath = metaDir / (assetName + ".asset.json");


	// ----- 2) Assimp load -----
	Assimp::Importer importer;

	// DX11 / LH 기준
	const aiScene* scene = importer.ReadFile(
		FBXPath,
		aiProcess_Triangulate			|			// 모든 면을 삼각형으로 반환
		aiProcess_GenNormals			|			// 노멀 생성
		aiProcess_CalcTangentSpace		|			// 탄젠트 공간 계산
		aiProcess_JoinIdenticalVertices |			// 중복 정점 제거
		aiProcess_ConvertToLeftHanded				// LH 변환
	);

	if (!scene) return ;

	// ----- 3) Import pipeline -----
	std::unordered_map<std::string, uint32_t> boneMap;
	// Skeleton (없으면 boneMap 비거나 skel 파일 안 만들어도 됨)
	if(!ImportFBXToSkelBin(scene, ToGenericString(skelBinPath), boneMap)) return;

	std::vector<std::string> meshFiles;
	if(!ImportFBXToMeshBin(scene, ToGenericString(meshesDir), assetName, boneMap, meshFiles)) return;

	std::vector<MetaMeshItem> metaMeshes;

	for (auto& file : meshFiles)
	{
		const bool isSkinned = (file.find("_skinned") != std::string::npos);

		// ImportFBXToMeshBin이 fileName만 반환한다고 가정 -> "Meshes/<fileName>" 형태로 meta에 기록
		fs::path fn = fs::u8path(file).filename();
		std::string rel = ToGenericString(fs::path("Meshes") / fn);

		metaMeshes.push_back({ rel, isSkinned });
	}

	std::vector<std::string> animFiles;
	if(!ImportFBXToAnimJson(scene, ToGenericString(animsDir), assetName, boneMap, animFiles)) return;

	std::vector<MetaAnimItem> metaAnims;

	for (auto& file : animFiles)
	{
		// file이 파일명만 와도 되고, 전체 경로가 와도 됨 -> filename만 뽑아서 규칙으로 처리
		fs::path fn = fs::u8path(file).filename();
		std::string rel = ToGenericString(fs::path("Anims") / fn);

		std::string clipName = ExtractClipName(fn.u8string(), assetName);

		metaAnims.push_back({ SanitizeName(clipName), rel });
	}

	if(!ImportFBXToMaterialBin(scene, ToGenericString(matBinPath))) return;

	// ----- 4) Write meta -----
	// meta는 Meta 폴더에 있으므로, 런타임 기준(metaDir)에서 asset 루트(baseDir)로 올라가게 baseDir=".."
	// textureDir은 asset 루트 기준 "Textures"로 통일
	const std::string metaBaseDir = "..";
	const std::string metaTextureDir = "Textures";

	if(!WriteAssetMetaJson(
		ToGenericString(metaPath),			//outPath
		assetName,							//assetName
		metaBaseDir,						// baseDir (meta 파일 기준)
		metaTextureDir,                     // textureDir (asset 루트 기준)
		RelToBase(baseDir, matBinPath),		//matBin	상대경로
		RelToBase(baseDir, skelBinPath),	//skelBin	상대경로
		metaMeshes, 
		metaAnims)) return;
}
