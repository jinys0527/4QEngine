#include "AssetLoader.h"

#include <array>
#include <filesystem>
#include <fstream>
#include <cstdint>
#include <unordered_map>
#include <iostream>

#include "json.hpp"
using nlohmann::json;
namespace fs = std::filesystem;

namespace
{
	enum class ETextureType : uint8_t { ALBEDO, NORMAL, METALLIC, ROUGHNESS, AO, EMISSIVE, MAX };
	static constexpr size_t TEX_MAX = static_cast<size_t>(ETextureType::MAX);

	enum MeshFlags : uint16_t
	{
		MESH_HAS_SKINNING = 1 << 0,
	};

#pragma pack(push, 1)
	struct AABBf
	{
		float min[3];
		float max[3];
	};

	struct MeshBinHeader
	{
		uint32_t magic = 0x4D455348; // "MESH"
		uint16_t version = 0;
		uint16_t flags = 0;          // bit0: hasSkinning
		uint32_t vertexCount = 0;
		uint32_t indexCount = 0;
		uint32_t subMeshCount = 0;
		uint32_t stringTableBytes = 0;
		AABBf    bounds{};
	};

	struct SubMeshBin
	{
		uint32_t indexStart;
		uint32_t indexCount;
		uint32_t materialNameOffset;
		AABBf    bounds{};
	};

	struct Vertex
	{
		float px, py, pz;
		float nx, ny, nz;
		float u, v;
		float tx, ty, tz, handedness;
	};

	struct VertexSkinned : Vertex
	{
		uint16_t boneIndex [4] = { 0, 0, 0, 0 };
		uint16_t boneWeight[4] = { 0, 0, 0, 0 };
	};

	struct MatBinHeader
	{
		uint32_t magic = 0x4D41544C; // "MATL"
		uint16_t version = 1;
		uint16_t materialCount = 0;
		uint32_t stringTableBytes = 0;
	};

	struct MatData
	{
		uint32_t materialNameOffset = 0;   // string table offset
		float baseColor[4];
		float metallic = 0.0f;
		float roughness = 1.0f;
		uint8_t alphaMode = 0;             // 0 : Opaque 1 : Mask 2 : Blend
		float alphaCutOff = 0.5f;
		float opacity = 1.0f;
		uint8_t doubleSided = 0;
		uint8_t pad[3] = { 0, 0, 0 };
		uint32_t texPathOffset[TEX_MAX] = {}; // 0이면 없음
	};

	struct SkelBinHeader
	{
		uint32_t magic            = 0x534B454C; // "SKEL"
		uint16_t version          = 2;
		uint16_t boneCount        = 0;
		uint32_t stringTableBytes = 0;

		uint32_t upperCount		  = 0;
		uint32_t lowerCount		  = 0;
	};

	struct BoneBin
	{
		uint32_t nameOffset = 0;
		int32_t  parentIndex = -1;
		float	 inverseBindPose[16]; // Row-Major
		float	 localBind[16];
	};
#pragma pack(pop)

	constexpr uint32_t kMeshMagic = 0x4D455348; // "MESH"
	constexpr uint32_t kMatMagic  = 0x4D41544C; // "MATL"
	constexpr uint32_t kSkelMagic = 0x534B454C; // "SKEL"

	std::string ReadStringAtOffset(const std::string& table, uint32_t offset)
	{
		if (offset == 0 || offset >= table.size())
		{
			return {};
		}

		const char* ptr = table.data() + offset;
		return std::string(ptr);
	}

	fs::path ResolvePath(const fs::path& baseDir, const std::string& relative)
	{
		fs::path path(relative);
		if (path.is_absolute())
		{
			return path.lexically_normal();
		}

		return (baseDir / path).lexically_normal();
	}

	void LogMaterialBinTextures(
		const std::string& materialBinPath,
		const std::vector<MatData>& mats,
		const std::string& stringTable)
	{
		for (size_t i = 0; i < mats.size(); ++i)
		{
			const MatData& mat = mats[i];
			std::string materialName = ReadStringAtOffset(stringTable, mat.materialNameOffset);
			if (materialName.empty())
			{
				materialName = "Material_" + std::to_string(i);
			}

			for (size_t t = 0; t < TEX_MAX; ++t)
			{
				const std::string texPathRaw = ReadStringAtOffset(stringTable, mat.texPathOffset[t]);
				if (texPathRaw.empty())
				{
					continue;
				}

#ifdef _DEBUG
// 				std::cout << "[MaterialBin] path=" << materialBinPath
// 					<< " material=" << materialName
// 					<< " slot=" << t
// 					<< " texture=" << texPathRaw
// 					<< std::endl;
#endif
			}
		}
	}

	bool PushUnique(std::vector<TextureHandle>& out, TextureHandle handle)
	{
		for (const auto& existing : out)
		{
			if (existing == handle)
			{
				return false;
			}
		}

		out.push_back(handle);
		return true;
	}

	RenderData::MaterialTextureSlot ToMaterialSlot(ETextureType type, bool& outValid)
	{
		outValid = true;
		switch (type)
		{
		case ETextureType::ALBEDO:
			return RenderData::MaterialTextureSlot::Albedo;
		case ETextureType::NORMAL:
			return RenderData::MaterialTextureSlot::Normal;
		case ETextureType::METALLIC:
			return RenderData::MaterialTextureSlot::Metallic;
		case ETextureType::ROUGHNESS:
			return RenderData::MaterialTextureSlot::Roughness;
		case ETextureType::AO:
			return RenderData::MaterialTextureSlot::AO;
		case ETextureType::EMISSIVE:
			outValid = false;
			return RenderData::MaterialTextureSlot::Albedo;
		default:
			outValid = false;
			return RenderData::MaterialTextureSlot::Albedo;
		}
	}

	template <typename HandleType>
	uint64_t MakeHandleKey(const HandleType& handle)
	{
		return (static_cast<uint64_t>(handle.generation) << 32) | handle.id;
	}

	template <typename HandleType>
	void StoreReferenceIfMissing(
		std::unordered_map<uint64_t, AssetRef>& out,
		const HandleType& handle,
		const std::string& assetMetaPath,
		UINT32 index
	)
	{
		const uint64_t key = MakeHandleKey(handle);
		if (out.find(key) != out.end())
		{
			return;
		}
		
		out.emplace(key, AssetRef{ assetMetaPath, index });
	}

	RenderData::Vertex ToRenderVertex(const Vertex& in)
	{
		RenderData::Vertex out{};
		out.position = { in.px, in.py, in.pz };
		out.normal   = { in.nx, in.ny, in.nz };
		out.uv       = { in.u, in.v };
		out.tangent  = { in.tx, in.ty, in.tz, in.handedness };
		return out;
	}

	RenderData::Vertex ToRenderVertex(const VertexSkinned& in)
	{
		RenderData::Vertex out = ToRenderVertex(static_cast<const Vertex&>(in));
		out.boneIndices = { in.boneIndex[0], in.boneIndex[1], in.boneIndex[2], in.boneIndex[3] };
		const float invWeight = 1.0f / 65535.0f;
		out.boneWeights = {
			static_cast<float>(in.boneWeight[0])* invWeight,
			static_cast<float>(in.boneWeight[1])* invWeight,
			static_cast<float>(in.boneWeight[2])* invWeight,
			static_cast<float>(in.boneWeight[3])* invWeight
		};
		return out;
	}

#ifdef _DEBUG
	void WriteMeshBinLoadDebugJson(
		const fs::path& meshPath,
		const MeshBinHeader& header,
		const std::vector<SubMeshBin>& subMeshes,
		const RenderData::MeshData& meshData)
	{
		json root;
		root["path"] = meshPath.generic_string();
		root["header"] = {
			{"magic", header.magic},
			{"version", header.version},
			{"flags", header.flags},
			{"vertexCount", header.vertexCount},
			{"indexCount", header.indexCount},
			{"subMeshCount", header.subMeshCount},
			{"stringTableBytes", header.stringTableBytes},
			{"bounds", {
				{"min", { header.bounds.min[0], header.bounds.min[1], header.bounds.min[2] }},
				{"max", { header.bounds.max[0], header.bounds.max[1], header.bounds.max[2] }}
			}}
		};

		root["subMeshes"] = json::array();
		for (const auto& subMesh : subMeshes)
		{
			root["subMeshes"].push_back({
				{"indexStart", subMesh.indexStart},
				{"indexCount", subMesh.indexCount},
				{"materialNameOffset", subMesh.materialNameOffset},
				{"bounds", {
					{"min", { subMesh.bounds.min[0], subMesh.bounds.min[1], subMesh.bounds.min[2] }},
					{"max", { subMesh.bounds.max[0], subMesh.bounds.max[1], subMesh.bounds.max[2] }}
				}}
				});
		}

		root["isSkinned"] = meshData.hasSkinning ? true : false;
		root["vertices"] = json::array();
		for (const auto& v : meshData.vertices)
		{
			root["vertices"].push_back({
				{"pos", { v.position.x, v.position.y, v.position.z }},
				{"normal", { v.normal.x, v.normal.y, v.normal.z }},
				{"uv", { v.uv.x, v.uv.y }},
				{"tangent", { v.tangent.x, v.tangent.y, v.tangent.z, v.tangent.w }},
				{"boneIndex", { v.boneIndices[0], v.boneIndices[1], v.boneIndices[2], v.boneIndices[3] }},
				{"boneWeight", { v.boneWeights[0], v.boneWeights[1], v.boneWeights[2], v.boneWeights[3] }}
				});
		}

		root["indices"] = meshData.indices;

		fs::path debugPath = meshPath;
		debugPath += ".load.debug.json";
		std::ofstream ofs(debugPath);
		if (ofs)
		{
			ofs << root.dump(2);
		}
	}
#endif

	RenderData::AnimationClip ParseAnimationJson(const json& j)
	{
		RenderData::AnimationClip clip{};
		clip.name = j.value("name", "");
		clip.duration = j.value("duration", 0.0f);
		clip.ticksPerSecond = j.value("ticksPerSecond", 0.0f);

		if (!j.contains("tracks") || !j["tracks"].is_array())
		{
			return clip;
		}

		for (const auto& trackJson : j["tracks"])
		{
			RenderData::AnimationTrack track{};
			track.boneIndex = trackJson.value("boneIndex", -1);

			if (trackJson.contains("keys") && trackJson["keys"].is_array())
			{
				for (const auto& keyJson : trackJson["keys"])
				{
					RenderData::AnimationKeyFrame key{};
					key.time = keyJson.value("time", 0.0f);

					const auto& pos = keyJson.value("position", std::vector<float>{});
					if (pos.size() >= 3)
					{
						key.translation = { pos[0], pos[1], pos[2] };
					}

					const auto& rot = keyJson.value("rotation", std::vector<float>{});
					if (rot.size() >= 4)
					{
						key.rotation = { rot[0], rot[1], rot[2], rot[3] };
					}

					const auto& scale = keyJson.value("scale", std::vector<float>{});
					if (scale.size() >= 3)
					{
						key.scale = { scale[0], scale[1], scale[2] };
					}

					track.keyFrames.push_back(key);
				}
			}

			clip.tracks.push_back(std::move(track));
		}

		return clip;
	}
}

void AssetLoader::LoadAll()
{
	const fs::path assetRoot = "../ResourceOutput";
	if (!fs::exists(assetRoot) || !fs::is_directory(assetRoot))
		return;

	for (const auto& dirEntry : fs::directory_iterator(assetRoot))
	{
		if (!dirEntry.is_directory())
			continue;

		fs::path metaDir = dirEntry.path() / "Meta";
		if (!fs::exists(metaDir) || !fs::is_directory(metaDir))
			continue;

		for (const auto& fileEntry : fs::directory_iterator(metaDir))
		{
			if (!fileEntry.is_regular_file())
				continue;

			const fs::path& path = fileEntry.path();
			if (path.extension() != ".json")
				continue;

			if (path.filename().string().find(".asset.json") == std::string::npos)
				continue;

#ifdef _DEBUG
		//	std::cout << path.string() << std::endl;
#endif
			LoadAsset(path.string());
		}
	}
}

const AssetLoader::AssetLoadResult* AssetLoader::GetAsset(const std::string& assetMetaPath) const
{
	auto it = m_AssetsByPath.find(assetMetaPath);
	if (it == m_AssetsByPath.end())
	{
		return nullptr;
	}

	return &it->second;
}

MaterialHandle AssetLoader::ResolveMaterial(const std::string& assetMetaPath, UINT32 index) const
{
	const auto* asset = GetAsset(assetMetaPath);
	if (!asset)
	{
		return MaterialHandle::Invalid();
	}

	if (index >= asset->materials.size())
	{
		return MaterialHandle::Invalid();
	}

	return asset->materials[index];
}

bool AssetLoader::GetMaterialAssetReference(MaterialHandle handle, std::string& outPath, UINT32& outIndex) const
{
	auto it = m_MaterialRefs.find(MakeHandleKey(handle));
	if (it == m_MaterialRefs.end())
	{
		return false;
	}

	outPath = it->second.assetPath;
	outIndex = it->second.assetIndex;
	return true;
}

bool AssetLoader::GetMeshAssetReference(MeshHandle handle, std::string& outPath, UINT32& outIndex) const
{
	auto it = m_MeshRefs.find(MakeHandleKey(handle));
	if (it == m_MeshRefs.end())
	{
		return false;
	}

	outPath = it->second.assetPath;
	outIndex = it->second.assetIndex;
	return true;
}

bool AssetLoader::GetTextureAssetReference(TextureHandle handle, std::string& outPath, UINT32& outIndex) const
{
	auto it = m_TextureRefs.find(MakeHandleKey(handle));
	if (it == m_TextureRefs.end())
	{
		return false;
	}

	outPath = it->second.assetPath;
	outIndex = it->second.assetIndex;
	return true;
}

bool AssetLoader::GetSkeletonAssetReference(SkeletonHandle handle, std::string& outPath, UINT32& outIndex) const
{
	auto it = m_SkeletonRefs.find(MakeHandleKey(handle));
	if (it == m_SkeletonRefs.end())
	{
		return false;
	}

	outPath = it->second.assetPath;
	outIndex = it->second.assetIndex;
	return true;
}

bool AssetLoader::GetAnimationAssetReference(AnimationHandle handle, std::string& outPath, UINT32& outIndex) const
{
	auto it = m_AnimationRefs.find(MakeHandleKey(handle));
	if (it == m_AnimationRefs.end())
	{
		return false;
	}

	outPath = it->second.assetPath;
	outIndex = it->second.assetIndex;
	return true;
}


AssetLoader::AssetLoadResult AssetLoader::LoadAsset(const std::string& assetMetaPath)
{
	AssetLoadResult result{};
	std::ifstream ifs(assetMetaPath);
	if (!ifs)
	{
		return result;
	}

	json meta;
	ifs >> meta;

	const fs::path metaPath(assetMetaPath);
	const fs::path metaDir = metaPath.parent_path();
	const json pathJson = meta.value("path", json::object());
	const fs::path baseDir = (metaDir / pathJson.value("baseDir", "")).lexically_normal();
	const fs::path textureDir = (baseDir / pathJson.value("textureDir", "")).lexically_normal();

	const json filesJson = meta.value("files", json::object());
	const std::string materialsFile = filesJson.value("materials", "");
	const std::string skeletonFile = filesJson.value("skeleton", "");

	std::vector<MaterialHandle> materialHandles;
	std::unordered_map<std::string, MaterialHandle> materialByName;
	if (!materialsFile.empty())
	{
		const fs::path materialPath = ResolvePath(baseDir, materialsFile);
		std::ifstream matStream(materialPath, std::ios::binary);
		if (matStream)
		{
			MatBinHeader header{};
			matStream.read(reinterpret_cast<char*>(&header), sizeof(header));
			if (header.magic == kMatMagic && header.materialCount > 0)
			{
				std::vector<MatData> mats(header.materialCount);
				matStream.read(reinterpret_cast<char*>(mats.data()), sizeof(MatData) * mats.size());

				std::string stringTable;
				if (header.stringTableBytes > 0)
				{
					stringTable.resize(header.stringTableBytes);
					matStream.read(stringTable.data(), header.stringTableBytes);
				}

#ifdef _DEBUG
		//		LogMaterialBinTextures(materialPath.generic_string(), mats, stringTable);
#endif // _DEBUG

				materialHandles.reserve(mats.size());
				materialByName.reserve(mats.size());
				for (size_t i = 0; i < mats.size(); ++i)
				{
					const MatData& mat = mats[i];
					RenderData::MaterialData material{};
					material.baseColor = { mat.baseColor[0], mat.baseColor[1], mat.baseColor[2], mat.baseColor[3] };
					material.metallic = mat.metallic;
					material.roughness = mat.roughness;

					std::string materialName = ReadStringAtOffset(stringTable, mat.materialNameOffset);
					if (materialName.empty())
					{
						materialName = "Material_" + std::to_string(i);
					}

					for (size_t t = 0; t < TEX_MAX; ++t)
					{
						const std::string texPathRaw = ReadStringAtOffset(stringTable, mat.texPathOffset[t]);
						if (texPathRaw.empty())
						{
							continue;
						}

						bool slotValid = false;
						const auto slot = ToMaterialSlot(static_cast<ETextureType>(t), slotValid);
						if (!slotValid)
						{
							continue;
						}

						const fs::path texPath = ResolvePath(textureDir, texPathRaw);
						const bool isSRGB = (slot == RenderData::MaterialTextureSlot::Albedo);

						TextureHandle textureHandle = m_Textures.Load(
							texPath.generic_string(),
							[texPath, isSRGB]()
							{
								auto tex = std::make_unique<RenderData::TextureData>();
								tex->path = texPath.generic_string();
								tex->sRGB = isSRGB;
								return tex;
							});
						material.textures[static_cast<size_t>(slot)] = textureHandle;
						if (PushUnique(result.textures, textureHandle))
						{
							const UINT32 textureIndex = static_cast<UINT32>(result.textures.size() - 1);
							StoreReferenceIfMissing(m_TextureRefs, textureHandle, assetMetaPath, textureIndex);
						}
					}

					const std::string materialKey = materialPath.generic_string() + ":" + materialName;
					MaterialHandle handle = m_Materials.Load(materialKey, [material]()
						{
							return std::make_unique<RenderData::MaterialData>(material);
						});

					materialHandles.push_back(handle);
					materialByName.emplace(materialName, handle);
					result.materials.push_back(handle);
					StoreReferenceIfMissing(m_MaterialRefs, handle, assetMetaPath, static_cast<UINT32>(i));
				}
			}
		}
	}

	if (!skeletonFile.empty())
	{
		const fs::path skelPath = ResolvePath(baseDir, skeletonFile);
		std::ifstream skelStream(skelPath, std::ios::binary);
		if (skelStream)
		{
			SkelBinHeader header{};
			skelStream.read(reinterpret_cast<char*>(&header), sizeof(header));

			if (header.magic == kSkelMagic && header.boneCount > 0)
			{
				std::vector<BoneBin> bones(header.boneCount);
				skelStream.read(reinterpret_cast<char*>(bones.data()), sizeof(BoneBin) * bones.size());

				std::string stringTable;
				if (header.stringTableBytes > 0)
				{
					stringTable.resize(header.stringTableBytes);
					skelStream.read(stringTable.data(), header.stringTableBytes);
				}

				RenderData::Skeleton skeleton{};
				skeleton.bones.reserve(bones.size());
				for (const auto& bone : bones)
				{
					RenderData::Bone out{};
					out.name = ReadStringAtOffset(stringTable, bone.nameOffset);
					out.parentIndex = bone.parentIndex;
					std::memcpy(&out.bindPose, bone.localBind, sizeof(float) * 16);
					std::memcpy(&out.inverseBindPose, bone.inverseBindPose, sizeof(float) * 16);
					skeleton.bones.push_back(std::move(out));
				}

				if (header.upperCount > 0)
				{
					std::vector<int32_t> indices(header.upperCount);
					skelStream.read(reinterpret_cast<char*>(indices.data()), sizeof(int32_t) * indices.size());
					skeleton.upperBodyBones.assign(indices.begin(), indices.end());
				}
				if (header.lowerCount > 0)
				{
					std::vector<int32_t> indices(header.lowerCount);
					skelStream.read(reinterpret_cast<char*>(indices.data()), sizeof(int32_t) * indices.size());
					skeleton.lowerBodyBones.assign(indices.begin(), indices.end());
				}

				result.skeleton = m_Skeletons.Load(skelPath.generic_string(), [skeleton]()
					{
						return std::make_unique<RenderData::Skeleton>(skeleton);
					});

				if (result.skeleton.IsValid())
				{
					StoreReferenceIfMissing(m_SkeletonRefs, result.skeleton, assetMetaPath, 0u);
				}
			}
		}
	}

	if (meta.contains("meshes") && meta["meshes"].is_array())
	{
		for (const auto& meshJson : meta["meshes"])
		{
			UINT32 meshIndex = 0;
			const std::string meshFile = meshJson.value("file", "");
			if (meshFile.empty())
			{
				++meshIndex;
				continue;
			}

			const fs::path meshPath = ResolvePath(baseDir, meshFile);
			std::ifstream meshStream(meshPath, std::ios::binary);
			if (!meshStream)
			{
				++meshIndex;
				continue;
			}

			MeshBinHeader header{};
			meshStream.read(reinterpret_cast<char*>(&header), sizeof(header));
			if (header.magic != kMeshMagic)
			{
				++meshIndex;
				continue;
			}

			std::vector<SubMeshBin> subMeshes(header.subMeshCount);
			meshStream.read(reinterpret_cast<char*>(subMeshes.data()), sizeof(SubMeshBin) * subMeshes.size());

			RenderData::MeshData meshData{};
			meshData.hasSkinning = (header.flags & MESH_HAS_SKINNING) != 0;

			meshData.vertices.reserve(header.vertexCount);
			if (meshData.hasSkinning)
			{
				std::vector<VertexSkinned> vertices(header.vertexCount);
				meshStream.read(reinterpret_cast<char*>(vertices.data()), sizeof(VertexSkinned) * vertices.size());
				for (const auto& v : vertices)
				{
					meshData.vertices.push_back(ToRenderVertex(v));
				}
			}
			else
			{
				std::vector<Vertex> vertices(header.vertexCount);
				meshStream.read(reinterpret_cast<char*>(vertices.data()), sizeof(Vertex) * vertices.size());
				for (const auto& v : vertices)
				{
					meshData.vertices.push_back(ToRenderVertex(v));
				}
			}

			meshData.indices.resize(header.indexCount);
			meshStream.read(reinterpret_cast<char*>(meshData.indices.data()), sizeof(uint32_t) * meshData.indices.size());

#ifdef _DEBUG
// 			std::cout << "[MeshBin] load mesh=" << meshPath.filename().string()
// 				<< " verts=" << meshData.vertices.size()
// 				<< " indices=" << meshData.indices.size()
// 				<< " skinned=" << meshData.hasSkinning
// 				<< std::endl;
// 			for (auto i = 0; i < meshData.vertices.size(); ++i)
// 			{
// 				const auto& v = meshData.vertices[i];
// 				std::cout << "[MeshBin] v" << i
// 					<< " pos=(" << v.position.x << "," << v.position.y << "," << v.position.z << ")"
// 					<< " n=(" << v.normal.x << "," << v.normal.y << "," << v.normal.z << ")"
// 					<< " uv=(" << v.uv.x << "," << v.uv.y << ")"
// 					<< " t=(" << v.tangent.x << "," << v.tangent.y << "," << v.tangent.z << "," << v.tangent.w << ")"
// 					<< std::endl;
// 			}
// 			for (size_t i = 0; i + 2 < meshData.indices.size(); i += 3)
// 			{
// 				std::cout << "[MeshBin] tri" << (i / 3)
// 					<< " idx=(" << meshData.indices[i] << "," << meshData.indices[i + 1] << "," << meshData.indices[i + 2] << ")"
// 					<< std::endl;
// 			}
#endif

			std::string stringTable;
			if (header.stringTableBytes > 0)
			{
				stringTable.resize(header.stringTableBytes);
				meshStream.read(stringTable.data(), header.stringTableBytes);
			}

#ifdef _DEBUG
			WriteMeshBinLoadDebugJson(meshPath, header, subMeshes, meshData);
#endif

			meshData.subMeshes.reserve(subMeshes.size());
			for (const auto& subMesh : subMeshes)
			{
				RenderData::MeshData::SubMesh out{};
				out.indexStart = subMesh.indexStart;
				out.indexCount = subMesh.indexCount;

				const std::string materialName = ReadStringAtOffset(stringTable, subMesh.materialNameOffset);
				if (!materialName.empty())
				{
					auto it = materialByName.find(materialName);
					if (it != materialByName.end())
					{
						out.material = it->second;
					}
				}
				else if (materialHandles.size() == 1)
				{
					out.material = materialHandles.front();
				}

				meshData.subMeshes.push_back(out);
			}

			MeshHandle handle = m_Meshes.Load(meshPath.generic_string(), [meshData]()
				{
					return std::make_unique<RenderData::MeshData>(meshData);
				});
			result.meshes.push_back(handle);

			if (handle.IsValid())
			{
				StoreReferenceIfMissing(m_MeshRefs, handle, assetMetaPath, meshIndex);
			}
			++meshIndex;
		}
	}

	if (meta.contains("animations") && meta["animations"].is_array())
	{
		UINT32 animationIndex = 0;
		for (const auto& animJson : meta["animations"])
		{
			const std::string animFile = animJson.value("file", "");
			if (animFile.empty())
			{
				++animationIndex;
				continue;
			}

			const fs::path animPath = ResolvePath(baseDir, animFile);
			std::ifstream animStream(animPath);
			if (!animStream)
			{
				++animationIndex;
				continue;
			}

			json animData;
			animStream >> animData;
			RenderData::AnimationClip clip = ParseAnimationJson(animData);

			AnimationHandle handle = m_Animations.Load(animPath.generic_string(), [clip]()
				{
					return std::make_unique<RenderData::AnimationClip>(clip);
				});

			result.animations.push_back(handle);
			if (handle.IsValid())
			{
				StoreReferenceIfMissing(m_AnimationRefs, handle, assetMetaPath, animationIndex);
			}
			++animationIndex;
		}
	}

	m_AssetsByPath[assetMetaPath] = result;
	return result;
}