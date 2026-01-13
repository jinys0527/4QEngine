#pragma once
#include "Component.h"
#include "ResourceHandle.h"
#include "ResourceRefs.h"

class MeshComponent : public Component
{
	friend class Editor;

public:
	static constexpr const char* StaticTypeName = "MeshComponent";
	const char* GetTypeName() const override;

	MeshComponent() = default;
	virtual ~MeshComponent() = default;

	void			  SetMeshHandle(const MeshHandle& handle) { m_MeshHandle = handle; }
	const MeshHandle& GetMeshHandle() const					  { return m_MeshHandle;   }

	void LoadSetMesh(const MeshRef& meshRef)
	{
		m_Mesh = meshRef;
	}

	const MeshRef& GetMesh() const { return m_Mesh; }

	const std::vector<MaterialRef>& GetSubMeshMaterialOverrides() const { return m_SubMeshMaterialOverrides; }
	void SetSubMeshMaterialOverrides(const std::vector<MaterialRef>& overrides);
	void SetSubMeshMaterialOverride(size_t index, const MaterialRef& overrideRef);
	void ClearSubMeshMaterialOverride(size_t index);

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

protected:
	MeshHandle  m_MeshHandle;
	MeshRef		m_Mesh;
	std::vector<MaterialRef> m_SubMeshMaterialOverrides;
};



