#include "MeshComponent.h"

void MeshComponent::Update(float deltaTime)
{
}

void MeshComponent::OnEvent(EventType type, const void* data)
{
}

void MeshComponent::Serialize(nlohmann::json& j) const
{
	j["mesh"]["id"]         = m_MeshHandle.id;
	j["mesh"]["generation"] = m_MeshHandle.generation;
}

void MeshComponent::Deserialize(const nlohmann::json& j)
{
	if (j.contains("mesh"))
	{
		m_MeshHandle.id         = j["mesh"].value("id", 0u);
		m_MeshHandle.generation = j["mesh"].value("generation", 0u);
	}
}
