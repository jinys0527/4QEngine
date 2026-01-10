#pragma once

#include "MeshComponent.h"
#include "RenderData.h"
#include "ResourceHandle.h"

class SkeletalMeshComponent : public MeshComponent
{
	friend class Editor;

public:
	static constexpr const char* StaticTypeName = "SkeletalMeshComponent";
	const char* GetTypeName() const override;

	SkeletalMeshComponent() = default;
	virtual ~SkeletalMeshComponent() = default;

	void           SetSkeletonHandle(SkeletonHandle handle) { m_SkeletonHandle = handle; }
	SkeletonHandle GetSkeletonHandle() const                { return m_SkeletonHandle;   }

	void SetSkeletonAssetReference(const std::string& assetPath, UINT32 skeletonIndex)
	{
		m_SkeletonAssetPath = assetPath;
		m_SkeletonAssetIndex = skeletonIndex;
	}

	const std::string& GetSkeletonAssetPath () const { return m_SkeletonAssetPath;  }
	UINT32             GetSkeletonAssetIndex() const { return m_SkeletonAssetIndex; }

	void SetSkinningPalette(const std::vector<DirectX::XMFLOAT4X4>& palette)
	{
		m_SkinningPalette = palette;
	}

	const std::vector<DirectX::XMFLOAT4X4>& GetSkinningPalette() const { return m_SkinningPalette; }
	bool                                    HasSkinningPalette() const { return !m_SkinningPalette.empty(); }

	void Update (float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void Serialize  (nlohmann::json& j) const override;
	void Deserialize(const nlohmann::json& j) override;
protected:
	SkeletonHandle m_SkeletonHandle = SkeletonHandle::Invalid();

	std::string m_SkeletonAssetPath;
	UINT32      m_SkeletonAssetIndex = 0;

	std::vector<DirectX::XMFLOAT4X4> m_SkinningPalette;
};

