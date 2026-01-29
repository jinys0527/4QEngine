#include "FloodUIComponent.h"
#include "FloodSystemComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"
#include "Scene.h"
#include "GameObject.h"

REGISTER_COMPONENT_DERIVED(FloodUIComponent, UIComponent)
REGISTER_PROPERTY_READONLY(FloodUIComponent, DisplayedWaterLevel)
REGISTER_PROPERTY_READONLY(FloodUIComponent, DisplayedTimeRemaining)
REGISTER_PROPERTY_READONLY(FloodUIComponent, DisplayedGameOver)

void FloodUIComponent::Start()
{
	auto* owner = GetOwner();
	auto* scene = owner ? owner->GetScene() : nullptr;

	if (!scene)
		return;

	for (const auto& [name, object] : scene->GetGameObjects())
	{
		if (!object)
			continue;

		m_FloodSystem = object->GetComponent<FloodSystemComponent>();
		if (m_FloodSystem)
			break;
	}
}

void FloodUIComponent::Update(float deltaTime)
{
	(void)deltaTime;
	if (!m_FloodSystem)
		return;

	m_DisplayedWaterLevel	 = m_FloodSystem->GetWaterLevel();
	m_DisplayedTimeRemaining = m_FloodSystem->GetTurnRemaining();
	m_DisplayedGameOver		 = m_FloodSystem->GetGameOver();
}

void FloodUIComponent::OnEvent(EventType type, const void* data)
{
	(void)type;
	(void)data;
}
