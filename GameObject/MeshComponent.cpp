#include "MeshComponent.h"
#include "ReflectionMacro.h"
REGISTER_COMPONENT(MeshComponent);
REGISTER_PROPERTY_HANDLE(MeshComponent, MeshHandle)
REGISTER_PROPERTY_READONLY_LOADABLE(MeshComponent, Mesh)
REGISTER_PROPERTY(MeshComponent, SubMeshMaterialOverrides)

void MeshComponent::SetSubMeshMaterialOverrides(const std::vector<MaterialRef>& overrides)
{
	m_SubMeshMaterialOverrides = overrides;
}

void MeshComponent::SetSubMeshMaterialOverride(size_t index, const MaterialRef& overrideRef)
{
	if (m_SubMeshMaterialOverrides.size() <= index)
	{
		m_SubMeshMaterialOverrides.resize(index + 1);
	}

	m_SubMeshMaterialOverrides[index] = overrideRef;
}

void MeshComponent::ClearSubMeshMaterialOverride(size_t index)
{
	if (index >= m_SubMeshMaterialOverrides.size())
	{
		return;
	}

	m_SubMeshMaterialOverrides[index] = MaterialRef{};
}

void MeshComponent::Update(float deltaTime)
{
}

void MeshComponent::OnEvent(EventType type, const void* data)
{
}