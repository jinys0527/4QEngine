#include "FloodSystemComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"

REGISTER_COMPONENT(FloodSystemComponent)
REGISTER_PROPERTY(FloodSystemComponent, TurnTimeLimitSeconds)
REGISTER_PROPERTY(FloodSystemComponent, WaterRisePerSecond)
REGISTER_PROPERTY(FloodSystemComponent, GameOverLevel)
REGISTER_PROPERTY_READONLY(FloodSystemComponent, WaterLevel)
REGISTER_PROPERTY_READONLY(FloodSystemComponent, TurnElapsed)
REGISTER_PROPERTY_READONLY(FloodSystemComponent, GameOver)

void FloodSystemComponent::Start()
{
	m_TurnElapsed = 0.0f;
	m_IsGameOver  = false;
}

void FloodSystemComponent::Update(float deltaTime)
{
	if (m_IsGameOver)
		return;

	m_TurnElapsed += deltaTime;
	if (m_TurnElapsed >= m_TurnTimeLimitSeconds)
	{
		m_WaterLevel += m_WaterRisePerSecond * m_TurnElapsed;
		m_TurnElapsed = 0.0f;
	}

	if (m_WaterLevel >= m_GameOverLevel)
	{
		m_IsGameOver = true;
	}
}

void FloodSystemComponent::OnEvent(EventType type, const void* data)
{
	(void)type;
	(void)data;
}

const float& FloodSystemComponent::GetTurnRemaining() const
{
	const float remaining = m_TurnTimeLimitSeconds - m_TurnElapsed;
	return remaining > 0.0f ? remaining : 0.0f;
}
