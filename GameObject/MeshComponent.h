#pragma once
#include "Component.h"
#include "ResourceHandle.h"

class MeshComponent : public Component
{
	friend class Editor;

public:
	static constexpr const char* StaticTypeName = "MeshComponent";
	const char* GetTypeName() const override;

	MeshComponent() = default;
	virtual ~MeshComponent() = default;

	void SetMeshHandle(MeshHandle handle) { m_MeshHandle = handle; }
	MeshHandle GetMeshHandle() const      { return m_MeshHandle;   }

	void SetMeshAssetReference(const std::string& assetPath, UINT32 meshIndex)
	{
		m_MeshAssetPath = assetPath;
		m_MeshAssetIndex = meshIndex;
	}

	const std::string& GetMeshAssetPath() const { return m_MeshAssetPath; }
	UINT32 GetMeshAssetIndex() const { return m_MeshAssetIndex; }

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void Serialize(nlohmann::json& j) const override;
	void Deserialize(const nlohmann::json& j) override;
protected:
	MeshHandle m_MeshHandle;
	std::string m_MeshAssetPath;
	UINT32 m_MeshAssetIndex = 0;
};



