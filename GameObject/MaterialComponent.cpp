#include "MaterialComponent.h"

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

void MaterialComponent::Serialize(nlohmann::json& j) const
{
	j["material"]["id"]              = m_MaterialHandle.id;
	j["material"]["generation"]      = m_MaterialHandle.generation;
	j["overrides"]["enabled"]        = m_UseOverrides;
	j["overrides"]["baseColor"]["x"] = m_Overrides.baseColor.x;
	j["overrides"]["baseColor"]["y"] = m_Overrides.baseColor.y;
	j["overrides"]["baseColor"]["z"] = m_Overrides.baseColor.z;
	j["overrides"]["baseColor"]["w"] = m_Overrides.baseColor.w;
	j["overrides"]["metallic"]       = m_Overrides.metallic;
	j["overrides"]["roughness"]      = m_Overrides.roughness;
}

void MaterialComponent::Deserialize(const nlohmann::json& j)
{
	if (j.contains("material"))
	{
		m_MaterialHandle.id = j["material"].value("id", 0u);
		m_MaterialHandle.generation = j["material"].value("generation", 0u);
	}

	if (j.contains("overrides"))
	{
		m_UseOverrides          = j["overrides"].value("enabled", false);
		m_Overrides.baseColor.x = j["overrides"]["basecolor"].value("x", 1.0f);
		m_Overrides.baseColor.y = j["overrides"]["basecolor"].value("y", 1.0f);
		m_Overrides.baseColor.z = j["overrides"]["basecolor"].value("z", 1.0f);
		m_Overrides.baseColor.w = j["overrides"]["basecolor"].value("w", 1.0f);
		m_Overrides.metallic    = j["overrides"].value("metallic", 0.0f);
		m_Overrides.roughness   = j["overrides"].value("roughness", 1.0f);
	}
}
