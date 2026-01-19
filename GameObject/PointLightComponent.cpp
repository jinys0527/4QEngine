#include "PointLightComponent.h"
#include "ReflectionMacro.h"

REGISTER_COMPONENT_DERIVED(PointLightComponent, LightComponent)
REGISTER_PROPERTY(PointLightComponent, AttenuationRadius)

PointLightComponent::PointLightComponent()
{
	m_Type = LightType::Point;
}

void PointLightComponent::Update(float deltaTime)
{
}

void PointLightComponent::OnEvent(EventType type, const void* data)
{
}
