#pragma once

#include <string>
#include <vector>

#include "RenderData.h"
#include "ResourceStore.h"

class AssetLoader
{
public:
	struct AssetLoadResult
	{
		std::vector<MeshHandle>		 meshes;
		std::vector<MaterialHandle>  materials;
		std::vector<TextureHandle>	 textures;
		SkeletonHandle				 skeleton = SkeletonHandle::Invalid();
		std::vector<AnimationHandle> animations;
	};

	void LoadAll();
	
	ResourceStore<RenderData::MeshData, MeshHandle>& GetMeshes() { return m_Meshes; }
	ResourceStore<RenderData::MaterialData, MaterialHandle>& GetMaterials() { return m_Materials; }
	ResourceStore<RenderData::TextureData, TextureHandle>& GetTextures() { return m_Textures; }
	ResourceStore<RenderData::Skeleton, SkeletonHandle>& GetSkeletons() { return m_Skeletons; }
	ResourceStore<RenderData::AnimationClip, AnimationHandle>& GetAnimations() { return m_Animations; }

private:
	AssetLoadResult LoadAsset(const std::string& assetMetaPath);

	ResourceStore<RenderData::MeshData, MeshHandle> m_Meshes;
	ResourceStore<RenderData::MaterialData, MaterialHandle> m_Materials;
	ResourceStore<RenderData::TextureData, TextureHandle> m_Textures;
	ResourceStore<RenderData::Skeleton, SkeletonHandle> m_Skeletons;
	ResourceStore<RenderData::AnimationClip, AnimationHandle> m_Animations;
};

