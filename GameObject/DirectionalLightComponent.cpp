#include "DirectionalLightComponent.h"
#include "MathHelper.h"
#include "ReflectionMacro.h"
using namespace MathUtils;

REGISTER_COMPONENT_DERIVED(DirectionalLightComponent, LightComponent)
REGISTER_PROPERTY(DirectionalLightComponent, Direction)

DirectionalLightComponent::DirectionalLightComponent()
{
	m_LightViewProj = Identity();
	m_Type = LightType::Directional;
}

void DirectionalLightComponent::Update(float deltaTime)
{
}

void DirectionalLightComponent::OnEvent(EventType type, const void* data)
{
}
