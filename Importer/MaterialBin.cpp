#include "pch.h"
#include "MaterialBin.h"
#include "BinHelper.h"

static std::string GetTexPath(const aiMaterial* mat, aiTextureType type)
{
	if (!mat) return {};
	if (mat->GetTextureCount(type) <= 0) return {};

	aiString path;
	if (mat->GetTexture(type, 0, &path) != AI_SUCCESS) return {};

	// Assimp는 embedded 텍스처면 "*0" 같은 이름이 올 수 있음.
	// 프로젝트에서 embedded 지원 안 하면 이 경우를 걸러야 함.
	return std::string(path.C_Str());
}


bool ImportFBXToMaterialBin(const std::string& fbxPath, const std::string& outMaterialBin)
{
	Assimp::Importer importer;

	// DX11 / LH 기준
	const aiScene* scene = importer.ReadFile(
		fbxPath,
		aiProcess_Triangulate			|			// 모든 면을 삼각형으로 반환
		aiProcess_GenNormals			|			// 노멀 생성
		aiProcess_CalcTangentSpace		|			// 탄젠트 공간 계산
		aiProcess_JoinIdenticalVertices |			// 중복 정점 제거
		aiProcess_ConvertToLeftHanded				// LH 변환
	);

	if (!scene || !scene->HasMaterials()) return false;

	std::vector<MatData> materials;
	materials.reserve(scene->mNumMaterials);

	std::string stringTable;
	stringTable.reserve(4096);
	stringTable.push_back('\0');

	for (uint32_t m = 0; m < scene->mNumMaterials; ++m)
	{
		const aiMaterial* mat = scene->mMaterials[m];
		if (!mat) continue;

		MatData out{};

		aiString name;
		if (mat->Get(AI_MATKEY_NAME, name) == AI_SUCCESS)
			out.materialNameOffset = AddString(stringTable, name.C_Str());

		// baseColor (PBR 우선, 없으면 diffuse fallback)
		aiColor4D baseColor(1, 1, 1, 1);
		if (mat->Get(AI_MATKEY_BASE_COLOR, baseColor) != AI_SUCCESS)
		{
			mat->Get(AI_MATKEY_COLOR_DIFFUSE, baseColor);
		}

		out.baseColor[0] = baseColor.r;
		out.baseColor[1] = baseColor.g;
		out.baseColor[2] = baseColor.b;
		out.baseColor[3] = baseColor.a;

		// metallic
		float metallic = 0.0f;
		if (mat->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == AI_SUCCESS)
		{
			out.metallic = metallic;
		}

		// roughness
		float roughness = 0.0f;
		if (mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == AI_SUCCESS)
		{
			out.roughness = roughness;
		}
		else
		{
			// glossiness만 있으면 roughness로 변환(있을 때만)
			float gloss = 0.0f;
			if (mat->Get(AI_MATKEY_GLOSSINESS_FACTOR, gloss) == AI_SUCCESS)
				out.roughness = 1.0f - gloss;
		}

		float opacity = 1.0f;
		if (mat->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS)
			out.opacity = opacity;
		
		int twoSided = 0;
		if (mat->Get(AI_MATKEY_TWOSIDED, twoSided) == AI_SUCCESS)
			out.doubleSided = (twoSided != 0) ? 1 : 0;

		// alphaMode / alphaCutoff
		// FBX는 표준 alphaMode 키가 안정적이지 않아서 "프로젝트 규칙"으로 결정하는 게 보통임.
		// 여기서는 "opacity<1이면 Blend" 정도만 예시로 둠.
		{
			out.alphaMode = 0; // Opaque
			out.alphaCutOff = 0.5f;

			if (out.opacity < 1.0f)
				out.alphaMode = 2; // Blend

			// glTF 경로로 들어온 경우에만 들어올 수 있는 키(있으면)
#ifdef AI_MATKEY_GLTF_ALPHACUTOFF
			float cutoff = 0.5f;
			if (mat->Get(AI_MATKEY_GLTF_ALPHACUTOFF, cutoff) == AI_SUCCESS)
				out.alphaCutOff = cutoff;
#endif
		}


		std::string albedo = GetTexPath(mat, aiTextureType_BASE_COLOR);
		if (albedo.empty()) albedo = GetTexPath(mat, aiTextureType_DIFFUSE);
		out.texPathOffset[(size_t)ETextureType::ALBEDO] = AddString(stringTable, albedo);

		std::string normal = GetTexPath(mat, aiTextureType_NORMALS);
		if (normal.empty()) normal = GetTexPath(mat, aiTextureType_HEIGHT);
		out.texPathOffset[(size_t)ETextureType::NORMAL] = AddString(stringTable, normal);

		// Metallic / Roughness / AO / Emissive
		{
			out.texPathOffset[(size_t)ETextureType::METALLIC]  = AddString(stringTable, GetTexPath(mat, aiTextureType_METALNESS));
			out.texPathOffset[(size_t)ETextureType::ROUGHNESS] = AddString(stringTable, GetTexPath(mat, aiTextureType_DIFFUSE_ROUGHNESS));
			out.texPathOffset[(size_t)ETextureType::AO]        = AddString(stringTable, GetTexPath(mat, aiTextureType_AMBIENT_OCCLUSION));
			out.texPathOffset[(size_t)ETextureType::EMISSIVE]  = AddString(stringTable, GetTexPath(mat, aiTextureType_EMISSIVE));
		}

		materials.push_back(out);
	}

	MatBinHeader header{};
	header.materialCount    = static_cast<uint16_t>(materials.size());
	header.stringTableBytes = static_cast<uint32_t>(stringTable.size());

	std::ofstream ofs(outMaterialBin, std::ios::binary);
	if (!ofs) return false;

	ofs.write(reinterpret_cast<const char*>(&header), sizeof(header));

	if (!materials.empty())
	{
		ofs.write(reinterpret_cast<const char*>(materials.data()), sizeof(MatData) * materials.size());
	}
	
	if (!stringTable.empty())
		ofs.write((const char*)stringTable.data(), stringTable.size());

	return true;
}