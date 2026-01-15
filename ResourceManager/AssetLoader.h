#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "RenderData.h"
#include "ResourceStore.h"
#include "ResourceRefs.h"

class AssetLoader
{
public:
	AssetLoader();
	~AssetLoader();

	static void SetActive(AssetLoader* loader);
	static AssetLoader* GetActive();

	struct AssetLoadResult
	{
		std::vector<MeshHandle>		 meshes;
		std::vector<MaterialHandle>  materials;
		std::vector<TextureHandle>	 textures;
		SkeletonHandle				 skeleton = SkeletonHandle::Invalid();
		std::vector<AnimationHandle> animations;
	};

	void LoadAll();
	

	const AssetLoadResult* GetAsset(const std::string& assetMetaPath) const;
	MaterialHandle  ResolveMaterial (const std::string& assetMetaPath, UINT32 index) const;
	MeshHandle		ResolveMesh(const std::string& assetMetaPath, UINT32 index) const;
	TextureHandle   ResolveTexture(const std::string& assetMetaPath, UINT32 index) const;
	SkeletonHandle  ResolveSkeleton(const std::string& assetMetaPath, UINT32 index) const;
	AnimationHandle ResolveAnimation(const std::string& assetMetaPath, UINT32 index) const;
	bool GetMaterialAssetReference (MaterialHandle handle, std::string& outPath, UINT32& outIndex) const;
	bool GetMeshAssetReference     (MeshHandle handle, std::string& outPath, UINT32& outIndex) const;
	bool GetTextureAssetReference  (TextureHandle handle, std::string& outPath, UINT32& outIndex) const;
	bool GetSkeletonAssetReference (SkeletonHandle handle, std::string& outPath, UINT32& outIndex) const;
	bool GetAnimationAssetReference(AnimationHandle handle, std::string& outPath, UINT32& outIndex) const;

	const ResourceStore<RenderData::MeshData, MeshHandle>& GetMeshes() const { return m_Meshes; }
	ResourceStore<RenderData::MeshData, MeshHandle>& GetMeshes() { return m_Meshes; }
	ResourceStore<RenderData::MaterialData, MaterialHandle>& GetMaterials() { return m_Materials; }
	ResourceStore<RenderData::TextureData, TextureHandle>& GetTextures() { return m_Textures; }
	ResourceStore<RenderData::Skeleton, SkeletonHandle>& GetSkeletons() { return m_Skeletons; }
	ResourceStore<RenderData::AnimationClip, AnimationHandle>& GetAnimations() { return m_Animations; }
	AssetLoadResult LoadAsset(const std::string& assetMetaPath);
private:
	

	ResourceStore<RenderData::MeshData, MeshHandle>           m_Meshes;
	ResourceStore<RenderData::MaterialData, MaterialHandle>   m_Materials;
	ResourceStore<RenderData::TextureData, TextureHandle>     m_Textures;
	ResourceStore<RenderData::Skeleton, SkeletonHandle>       m_Skeletons;
	ResourceStore<RenderData::AnimationClip, AnimationHandle> m_Animations;

	std::unordered_map<std::string, AssetLoadResult> m_AssetsByPath;
	std::unordered_map<uint64_t, MeshRef>			 m_MeshRefs;
	std::unordered_map<uint64_t, MaterialRef>		 m_MaterialRefs;
	std::unordered_map<uint64_t, TextureRef>		 m_TextureRefs;
	std::unordered_map<uint64_t, SkeletonRef>		 m_SkeletonRefs;
	std::unordered_map<uint64_t, AnimationRef>		 m_AnimationRefs;

	inline static AssetLoader* s_ActiveLoader = nullptr;
};

