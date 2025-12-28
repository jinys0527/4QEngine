#include "LightComponent.h"
#include "MathHelper.h"
using namespace MathUtils;

LightComponent::LightComponent::LightComponent()
{
	m_LightViewProj = Identity();
}

void LightComponent::LightComponent::Update(float deltaTime)
{
}

void LightComponent::LightComponent::OnEvent(EventType type, const void* data)
{
}

void LightComponent::LightComponent::Serialize(nlohmann::json& j) const
{
	j["type"] = static_cast<int>(m_Type);
	j["color"]["x"]     = m_Color.x;
	j["color"]["y"]     = m_Color.y;
	j["color"]["z"]     = m_Color.z;
	j["intensity"]      = m_Intensity;
	j["range"]          = m_Range;
	j["spotAngle"]      = m_SpotAngle;
	j["direction"]["x"] = m_Direction.x;
	j["direction"]["y"] = m_Direction.y;
	j["direction"]["z"] = m_Direction.z;
	j["position"]["x"]  = m_Position.x;
	j["position"]["y"]  = m_Position.y;
	j["position"]["z"]  = m_Position.z;
	j["castShadow"]     = m_CastShadow;
}

void LightComponent::LightComponent::Deserialize(const nlohmann::json& j)
{
	m_Type = static_cast<RenderData::LightType>(j.value("type", 0));
	if (j.contains("color"))
	{
		m_Color.x = j["color"].value("x", 1.0f);
		m_Color.y = j["color"].value("y", 1.0f);
		m_Color.z = j["color"].value("z", 1.0f);
	}
	m_Intensity = j.value("intensity", 1.0f);
	m_Range     = j.value("range", 0.0f);
	if (j.contains("direction"))
	{
		m_Direction.x = j["direction"].value("x",  0.0f);
		m_Direction.y = j["direction"].value("y", -1.0f);
		m_Direction.z = j["direction"].value("z",  0.0f);
	}
	if (j.contains("position"))
	{
		m_Position.x = j["position"].value("x", 0.0f);
		m_Position.y = j["position"].value("y", 0.0f);
		m_Position.z = j["position"].value("z", 0.0f);
	}
	m_CastShadow = j.value("castShadow", true);
}
