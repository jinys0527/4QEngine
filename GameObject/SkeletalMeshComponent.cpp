#include "SkeletalMeshComponent.h"
#include "ReflectionMacro.h"

REGISTER_COMPONENT(SkeletalMeshComponent);

void SkeletalMeshComponent::Update(float deltaTime)
{
}

void SkeletalMeshComponent::OnEvent(EventType type, const void* data)
{
}

void SkeletalMeshComponent::Serialize(nlohmann::json& j) const
{
	if (!m_MeshAssetPath.empty())
	{
		j["mesh"]["assetPath"] = m_MeshAssetPath;
		j["mesh"]["assetIndex"] = m_MeshAssetIndex;
	}

	if (!m_SkeletonAssetPath.empty())
	{
		j["skeleton"]["assetPath"] = m_SkeletonAssetPath;
		j["skeleton"]["assetIndex"] = m_SkeletonAssetIndex;
	}
}

void SkeletalMeshComponent::Deserialize(const nlohmann::json& j)
{
	if (j.contains("mesh"))
	{
		m_MeshAssetPath = j["mesh"].value("assetPath", std::string{});
		m_MeshAssetIndex = j["mesh"].value("assetIndex", 0u);
	}

	if (j.contains("skeleton"))
	{
		m_SkeletonAssetPath = j["skeleton"].value("assetPath", std::string{});
		m_SkeletonAssetIndex = j["skeleton"].value("assetIndex", 0u);
	}
}
