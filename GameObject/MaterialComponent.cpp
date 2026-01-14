#include "MaterialComponent.h"
#include "ReflectionMacro.h"

REGISTER_COMPONENT(MaterialComponent);
REGISTER_PROPERTY_HANDLE(MaterialComponent, MaterialHandle)
REGISTER_PROPERTY(MaterialComponent, Overrides)
REGISTER_PROPERTY_READONLY_LOADABLE(MaterialComponent, Material)

void MaterialComponent::SetOverrides(const RenderData::MaterialData& overrides)
{
	m_Overrides = overrides;
	m_UseOverrides = true;	
}

void MaterialComponent::ClearOverrides()
{
	m_Overrides = RenderData::MaterialData{};
	m_UseOverrides = false;
}

void MaterialComponent::Update(float deltaTime)
{
}

void MaterialComponent::OnEvent(EventType type, const void* data)
{
}
