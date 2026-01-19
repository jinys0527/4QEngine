#include "SpotLightComponent.h"
#include "ReflectionMacro.h"

REGISTER_COMPONENT_DERIVED(SpotLightComponent, LightComponent)
REGISTER_PROPERTY(SpotLightComponent, Direction)
REGISTER_PROPERTY(SpotLightComponent, Range)
REGISTER_PROPERTY(SpotLightComponent, SpotInnerAngle)
REGISTER_PROPERTY(SpotLightComponent, SpotOutterAngle)
REGISTER_PROPERTY(SpotLightComponent, AttenuationRadius)

SpotLightComponent::SpotLightComponent()
{
	m_Type = LightType::Spot;
}

void SpotLightComponent::Update(float deltaTime)
{
}

void SpotLightComponent::OnEvent(EventType type, const void* data)
{
}
