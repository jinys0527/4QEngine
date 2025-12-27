#include "LoadingRollerComponent.h"
#include "GameObject.h"
#include "UIImageComponent.h"
//#include "Scene.h"



void LoadingRollerComponent::Start()
{
	m_Opacity = 1;

	m_IsLoaded = false;

	if (m_Index == 0)
		m_RectTransComp->SetPosition({ -715.0f, -2000 });
	else if (m_Index == 1)
		m_RectTransComp->SetPosition({ 0.0f, -4000 }); 
	else if (m_Index == 2)
		m_RectTransComp->SetPosition({ 715.0f, -6000 });
	else
		m_RectTransComp->SetPosition({ 0, -2550 }); //-2550 -550

	m_IsStart = true;
}

void LoadingRollerComponent::Update(float deltaTime)
{
	if(m_RectTransComp == nullptr)
		m_RectTransComp = m_Owner->GetComponent<RectTransformComponent>();

	m_Owner->GetComponent<UIImageComponent>()->SetOpacity(m_Opacity);

	if (m_IsLoaded)
	{
		m_Opacity = 0;
	}

	Math::Vector2F pos = m_RectTransComp->GetPosition();

	pos.x = pos.x - 960.0f + (m_RectTransComp->GetSize().x / 2.0f);
	pos.y = pos.y - 540.0f + (m_RectTransComp->GetSize().y / 2.0f);

	if (pos.y < 450)
	{
		pos.y += deltaTime * 2000;
	}

	if(pos.y >= 450)
	{
		pos.y = 450;
		if (m_SceneLoadFunc && m_IsStart)
		{
			m_SceneLoadFunc();
			m_IsStart = false;
		}
	}


	m_RectTransComp->SetPosition(pos);
}

void LoadingRollerComponent::OnEvent(EventType type, const void* data)
{
	m_Opacity = 0;

	if (scenePtr != data)
		return;

	if (type == EventType::OnLoadedScene)
	{
		m_IsLoaded = true;
	}
}

void LoadingRollerComponent::Serialize(nlohmann::json& j) const
{
}

void LoadingRollerComponent::Deserialize(const nlohmann::json& j)
{
}
