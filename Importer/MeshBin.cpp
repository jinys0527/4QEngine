#include "pch.h"
#include "MeshBin.h"


struct Influence
{
	uint32_t bone = 0;
	float weight = 0.0f;
};

static std::string JoinPath(const std::string& dir, const std::string& file)
{
	if (dir.empty()) return file;
	const char last = dir.back();
	if (last == '/' || last == '\\') return dir + file;
	return dir + "/" + file;
}

static void AABBInit(AABBf& bounds)
{
	bounds.min[0] = bounds.min[1] = bounds.min[2] = +FLT_MAX;
	bounds.max[0] = bounds.max[1] = bounds.max[2] = -FLT_MAX;
}

static void AABBExpand(AABBf& bounds, float x, float y, float z)
{
	bounds.min[0] = std::min(bounds.min[0], x); bounds.max[0] = std::max(bounds.max[0], x);
	bounds.min[1] = std::min(bounds.min[1], y); bounds.max[1] = std::max(bounds.max[1], y);
	bounds.min[2] = std::min(bounds.min[2], z); bounds.max[2] = std::max(bounds.max[2], z);
}

static void NormalizeTop4(std::vector<Influence>& v)	//weight 상위 4개로 Normalize
{
	std::sort(v.begin(), v.end(), [](const Influence& a, const Influence& b) { return a.weight > b.weight; });
	if (v.size() > 4) v.resize(4);

	float sum = 0.0f;
	for (auto& x : v) sum += x.weight;
	if (sum <= 0.0f) { v.clear(); return; }

	for (auto& x : v) x.weight /= sum;
}

static uint16_t ToU16Norm(float weight)
{
	if (weight < 0.0f) weight = 0.0f;
	if (weight > 1.0f) weight = 1.0f;
	return static_cast<uint16_t>(weight * 65535.0f + 0.5f);
}

static void SaveVertex(const aiMesh* mesh, uint32_t i, Vertex& v)
{
	v.px = mesh->mVertices[i].x;
	v.py = mesh->mVertices[i].y;
	v.pz = mesh->mVertices[i].z;


	if (mesh->HasNormals())
	{
		v.nx = mesh->mNormals[i].x;
		v.ny = mesh->mNormals[i].y;
		v.nz = mesh->mNormals[i].z;
	}

	if (mesh->HasTextureCoords(0))
	{
		v.u = mesh->mTextureCoords[0][i].x;
		v.v = mesh->mTextureCoords[0][i].y;
	}

	// tangent + handedness
	if (mesh->HasTangentsAndBitangents() && mesh->HasNormals())
	{
		const aiVector3D& T = mesh->mTangents[i];
		const aiVector3D& B = mesh->mBitangents[i];
		const aiVector3D& N = mesh->mNormals[i];

		v.tx = T.x; v.ty = T.y; v.tz = T.z;

		// handedness sign: +1 or -1
		const aiVector3D c = Cross3(N, T);
		const float det = Dot3(c, B);
		v.handedness = (det < 0.0f) ? -1.0f : 1.0f;
	}
	else
	{
		// tangent space가 없으면 기본값
		v.tx = 0.0f; v.ty = 0.0f; v.tz = 0.0f;
		v.handedness = 1.0f;
	}
}

bool WriteAiMeshToMeshBin(const aiScene* scene, uint32_t m, const std::string& outPath,
						  const std::unordered_map<std::string, uint32_t>* boneNameToIndex //skinned
)
{
	const aiMesh* mesh = scene->mMeshes[m];
	if (!mesh) return false;

	const bool isSkinned = mesh->HasBones() && mesh->mNumBones > 0;

	// skinned면 boneNameToIndex가 있어야 정상
	if (isSkinned && (!boneNameToIndex || boneNameToIndex->empty()))
		return false;

	std::vector<Vertex> vertices;
	std::vector<VertexSkinned> verticesSkinned;
	std::vector<uint32_t> indices;

	//AABB Init
	AABBf bounds{};
	AABBInit(bounds);

	// influences (skinned only)
	std::vector<std::vector<Influence>> infl;

	// vertex buffer
	if (!isSkinned)
	{
		vertices.reserve(mesh->mNumVertices);

		for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
		{
			Vertex v{};
			SaveVertex(mesh, i, v);
			AABBExpand(bounds, v.px, v.py, v.pz);
			vertices.push_back(v);
		}
	}
	else
	{
		infl.resize(mesh->mNumVertices);

		//aiBone -> vertex influences 누적
		for (uint32_t b = 0; b < mesh->mNumBones; ++b)
		{
			const aiBone* bone = mesh->mBones[b];
			if (!bone) continue;

			const std::string boneName = bone->mName.C_Str();
			auto it = boneNameToIndex->find(boneName);
			if (it == boneNameToIndex->end())
			{
				return false;
			}

			const uint32_t boneIndex = it->second;

			for (uint32_t w = 0; w < bone->mNumWeights; ++w)
			{
				const aiVertexWeight& vw = bone->mWeights[w];
				if (vw.mVertexId >= mesh->mNumVertices) continue;
				infl[vw.mVertexId].push_back({boneIndex, vw.mWeight});
			}
		}

		for (auto& v : infl) NormalizeTop4(v);


		verticesSkinned.reserve(mesh->mNumVertices);
		// TODO: influences 누적 -> VertexSkinned.boneIndex/weight 채우기


		for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
		{
			VertexSkinned v{};
			SaveVertex(mesh, i, static_cast<Vertex&>(v));
			AABBExpand(bounds, v.px, v.py, v.pz);

			const auto& inf = infl[i];
			for (size_t k = 0; k < inf.size(); ++k)
			{
				v.boneIndex[k]  = (uint16_t)inf[k].bone;
				v.boneWeight[k] = ToU16Norm(inf[k].weight);
			}

			verticesSkinned.push_back(v);
		}
	}


	for (uint32_t f = 0; f < mesh->mNumFaces; ++f)
	{
		const aiFace& face = mesh->mFaces[f];
		if (face.mNumIndices != 3) continue;

		indices.push_back(static_cast<uint32_t>(face.mIndices[0]));
		indices.push_back(static_cast<uint32_t>(face.mIndices[1]));
		indices.push_back(static_cast<uint32_t>(face.mIndices[2]));
	}

	// subMesh 기록
	SubMeshBin sm{};
	sm.indexStart = 0;
	sm.indexCount = (uint32_t)indices.size();
	sm.materialIndex = mesh->mMaterialIndex;
	sm.bounds = bounds;

	//헤더 구성
	MeshBinHeader header{};
	header.version = 1;
	header.flags = isSkinned ? MESH_HAS_SKINNING : 0;
	header.vertexCount = isSkinned ? static_cast<uint32_t>(verticesSkinned.size()) : static_cast<uint32_t>(vertices.size());
	header.indexCount = static_cast<uint32_t>(indices.size());
	header.subMeshCount = 1;
	header.bounds = bounds;

	//저장
	std::ofstream ofs(outPath, std::ios::binary);
	if (!ofs) return false;

	ofs.write(reinterpret_cast<const char*>(&header), sizeof(header));
	ofs.write(reinterpret_cast<const char*>(&sm), sizeof(sm));
	isSkinned ? ofs.write(reinterpret_cast<const char*>(verticesSkinned.data()), sizeof(VertexSkinned) * verticesSkinned.size()) 
			  : ofs.write(reinterpret_cast<const char*>(vertices.data()), sizeof(Vertex) * vertices.size());
	ofs.write(reinterpret_cast<const char*>(indices.data()), sizeof(uint32_t) * indices.size());

	return true;
}

bool ImportFBXToMeshBin(
	const std::string& fbxPath, 
	const std::string& outDir, 
	const std::string& baseName,
	const std::unordered_map<std::string, uint32_t>& boneNameToIndex, //skinned
	std::vector<std::string>& outMeshFiles
)
{
	Assimp::Importer importer;

	// DX11 / LH 기준
	const aiScene* scene = importer.ReadFile(
		fbxPath,
		aiProcess_Triangulate           |			// 모든 면을 삼각형으로 반환
		aiProcess_GenNormals            |			// 노멀 생성
		aiProcess_CalcTangentSpace      |			// 탄젠트 공간 계산
		aiProcess_JoinIdenticalVertices |			// 중복 정점 제거
		aiProcess_ConvertToLeftHanded				// LH 변환
	);

	if (!scene || !scene->HasMeshes()) return false;


	outMeshFiles.clear();
	outMeshFiles.reserve(scene->mNumMeshes);
	
	for (uint32_t m = 0; m < scene->mNumMeshes; ++m)
	{
		const aiMesh* mesh = scene->mMeshes[m];
		if (!mesh) continue;

		const bool skinned = mesh->HasBones() && mesh->mNumBones > 0;

		std::string file =
			baseName + "_mesh_" + std::to_string(m) + (skinned ? "_skinned.meshbin" : "_static.meshbin");
		std::string outPath = JoinPath(outDir, file);

		const std::unordered_map<std::string, uint32_t>* mapPtr = skinned ? &boneNameToIndex : nullptr;

		if (!WriteAiMeshToMeshBin(scene, m, outPath, mapPtr)) return false;

		outMeshFiles.push_back(file);
	}
	
	return true;
}