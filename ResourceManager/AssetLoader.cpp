#include "AssetLoader.h"

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
		uint16_t boneIndex[4] = { 0,0,0,0 };
		uint16_t boneWeight[4] = { 0,0,0,0 }; // normalized to 0..65535
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
		uint32_t magic = 0x534B454C; // "SKEL"
		uint16_t version = 1;
		uint16_t boneCount = 0;
		uint32_t stringTableBytes = 0;
	};

	struct BoneBin
	{
		uint32_t nameOffset = 0;
		int32_t  parentIndex = -1;
		float inverseBindPose[16]; // Row-Major
		float localBind[16];
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

				std::cout << "[MaterialBin] path=" << materialBinPath
					<< " material=" << materialName
					<< " slot=" << t
					<< " texture=" << texPathRaw
					<< std::endl;
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

	RenderData::Vertex ToRenderVertex(const Vertex& in)
	{
		RenderData::Vertex out{};
		out.position = { in.px, in.py, in.pz };
		out.normal = { in.nx, in.ny, in.nz };
		out.uv = { in.u, in.v };
		out.tangent = { in.tx, in.ty, in.tz, in.handedness };
		return out;
	}

	RenderData::Vertex ToRenderVertex(const VertexSkinned& in)
	{
		RenderData::Vertex out = ToRenderVertex(static_cast<const Vertex&>(in));
		for (size_t i = 0; i < 4; ++i)
		{
			out.boneIndex[i] = in.boneIndex[i];
			out.boneWeight[i] = in.boneWeight[i];
		}
		return out;
	}

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

			std::cout << path.string() << std::endl;

			LoadAsset(path.string());
		}
	}
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

				LogMaterialBinTextures(materialPath.generic_string(), mats, stringTable);

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
						PushUnique(result.textures, textureHandle);
					}

					const std::string materialKey = materialPath.generic_string() + ":" + materialName;
					MaterialHandle handle = m_Materials.Load(materialKey, [material]()
						{
							return std::make_unique<RenderData::MaterialData>(material);
						});

					materialHandles.push_back(handle);
					materialByName.emplace(materialName, handle);
					result.materials.push_back(handle);
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
					std::memcpy(&out.inverseBindPose, bone.inverseBindPose, sizeof(float) * 16);
					skeleton.bones.push_back(std::move(out));
				}

				result.skeleton = m_Skeletons.Load(skelPath.generic_string(), [skeleton]()
					{
						return std::make_unique<RenderData::Skeleton>(skeleton);
					});
			}
		}
	}

	if (meta.contains("meshes") && meta["meshes"].is_array())
	{
		for (const auto& meshJson : meta["meshes"])
		{
			const std::string meshFile = meshJson.value("file", "");
			if (meshFile.empty())
			{
				continue;
			}

			const fs::path meshPath = ResolvePath(baseDir, meshFile);
			std::ifstream meshStream(meshPath, std::ios::binary);
			if (!meshStream)
			{
				continue;
			}

			MeshBinHeader header{};
			meshStream.read(reinterpret_cast<char*>(&header), sizeof(header));
			if (header.magic != kMeshMagic)
			{
				continue;
			}

			std::vector<SubMeshBin> subMeshes(header.subMeshCount);
			meshStream.read(reinterpret_cast<char*>(subMeshes.data()), sizeof(SubMeshBin) * subMeshes.size());

			RenderData::MeshData meshData{};
			meshData.hasSkinning = (header.flags & MESH_HAS_SKINNING) != 0;

			std::string stringTable;
			if (header.stringTableBytes > 0)
			{
				stringTable.resize(header.stringTableBytes);
				meshStream.read(stringTable.data(), header.stringTableBytes);
			}

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

			MeshHandle handle = m_Meshes.Load(meshPath.generic_string(), [meshData]()
				{
					return std::make_unique<RenderData::MeshData>(meshData);
				});
			result.meshes.push_back(handle);
		}
	}

	if (meta.contains("animations") && meta["animations"].is_array())
	{
		for (const auto& animJson : meta["animations"])
		{
			const std::string animFile = animJson.value("file", "");
			if (animFile.empty())
			{
				continue;
			}

			const fs::path animPath = ResolvePath(baseDir, animFile);
			std::ifstream animStream(animPath);
			if (!animStream)
			{
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
		}
	}

	return result;
}