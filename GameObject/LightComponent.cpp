#include "LightComponent.h"
#include "ReflectionMacro.h"
#include <cassert>

REGISTER_COMPONENT(LightComponent);
REGISTER_PROPERTY(LightComponent, Color);
REGISTER_PROPERTY(LightComponent, Position);
REGISTER_PROPERTY(LightComponent, Intensity);
REGISTER_PROPERTY(LightComponent, CastShadow);

RenderData::LightData LightComponent::BuildLightData() const
{
	RenderData::LightData data{};
	data.type		   = m_Type;
	data.posiiton      = m_Position;
	data.range	       = m_Range;
	data.direction     = m_Direction;
	data.spotAngle     = m_SpotAngle;
	data.color	       = m_Color;
	data.intensity     = m_Intensity;
	data.lightViewProj = m_LightViewProj;
	data.castShadow    = m_CastShadow;
	return data;
}

void LightComponent::LightComponent::Update(float deltaTime)
{
}

void LightComponent::LightComponent::OnEvent(EventType type, const void* data)
{
}
