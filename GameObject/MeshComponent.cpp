#include "MeshComponent.h"

void MeshComponent::Update(float deltaTime)
{
}

void MeshComponent::OnEvent(EventType type, const void* data)
{
}

void MeshComponent::Serialize(nlohmann::json& j) const
{
	if (!m_MeshAssetPath.empty())
	{
		j["mesh"]["assetPath"]  = m_MeshAssetPath;
		j["mesh"]["assetIndex"] = m_MeshAssetIndex;
	}
}

void MeshComponent::Deserialize(const nlohmann::json& j)
{
	if (j.contains("mesh"))
	{
		m_MeshAssetPath  = j["mesh"].value("assetPath", std::string{});
		m_MeshAssetIndex = j["mesh"].value("assetIndex", 0u);
	}
}
