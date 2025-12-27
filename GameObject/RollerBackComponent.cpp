#include "RollerBackComponent.h"
#include "GameObject.h"
#include "UIImageComponent.h"

void RollerBackComponent::Start()
{
}

void RollerBackComponent::Update(float deltaTime)
{
	if (m_Opacity > 0)
		m_Opacity -= deltaTime * 1.f;
	else
	{
		m_Opacity = 0;
	}

	m_Owner->GetComponent<UIImageComponent>()->SetOpacity(m_Opacity);
}

void RollerBackComponent::OnEvent(EventType type, const void* data)
{
	if (type == EventType::OnLoadedScene)
	{
		m_Opacity = 1.5f;
	}
}

void RollerBackComponent::Serialize(nlohmann::json& j) const
{
}

void RollerBackComponent::Deserialize(const nlohmann::json& j)
{
}
