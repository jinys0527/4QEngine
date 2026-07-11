#include "WaterRiseComponent.h"

#include "ReflectionMacro.h"
#include "Object.h"
#include "GameObject.h"
#include "Scene.h"
#include "GameManager.h"
#include "FloodSystemComponent.h"
#include "TransformComponent.h"

#include <algorithm>

REGISTER_COMPONENT(WaterRiseComponent)
REGISTER_PROPERTY(WaterRiseComponent, MinY)
REGISTER_PROPERTY(WaterRiseComponent, MaxY)
REGISTER_PROPERTY(WaterRiseComponent, RiseDurationSeconds)
REGISTER_PROPERTY(WaterRiseComponent, ApplyMinYOnStart)
REGISTER_PROPERTY(WaterRiseComponent, Loop)
REGISTER_PROPERTY(WaterRiseComponent, Enabled)
REGISTER_PROPERTY(WaterRiseComponent, FloodLevelMin)
REGISTER_PROPERTY(WaterRiseComponent, FloodLevelMax)
REGISTER_PROPERTY_READONLY(WaterRiseComponent, ElapsedSeconds)
REGISTER_PROPERTY_READONLY(WaterRiseComponent, Finished)

void WaterRiseComponent::Start()
{
	m_ElapsedSeconds = 0.0f;
	m_Finished = false;
	m_FloodSystem = nullptr;

	if (m_ApplyMinYOnStart)
	{
		ApplyCurrentY();
	}
}

void WaterRiseComponent::Update(float deltaTime)
{
	(void)deltaTime;

	if (!m_Enabled)
	{
		return;
	}

	if (!m_FloodSystem)
	{
		auto* owner = GetOwner();
		auto* scene = owner ? owner->GetScene() : nullptr;
		if (scene)
		{
			for (const auto& [name, object] : scene->GetGameObjects())
			{
				(void)name;
				if (!object)
				{
					continue;
				}

				m_FloodSystem = object->GetComponent<FloodSystemComponent>();
				if (m_FloodSystem)
				{
					break;
				}
			}
		}
	}

	if (!m_FloodSystem)
	{
		return;
	}

	const float levelMin = min(m_FloodLevelMin, m_FloodLevelMax);
	const float levelMax = max(m_FloodLevelMin, m_FloodLevelMax);
	const float levelRange = max(0.001f, levelMax - levelMin);
	const float normalized = std::clamp((m_FloodSystem->GetWaterLevel() - levelMin) / levelRange, 0.0f, 1.0f);
	const float safeDuration = max(0.001f, m_RiseDurationSeconds);
	m_ElapsedSeconds = safeDuration * normalized;
	m_Finished = (normalized >= 1.0f);

	ApplyCurrentY();
}

void WaterRiseComponent::OnEvent(EventType type, const void* data)
{
	(void)type;
	(void)data;
}

void WaterRiseComponent::ResetRise()
{
	m_ElapsedSeconds = 0.0f;
	m_Finished = false;
	ApplyCurrentY();
}

void WaterRiseComponent::ApplyCurrentY()
{
	auto* owner = GetOwner();
	if (!owner)
	{
		return;
	}

	auto* transform = owner->GetComponent<TransformComponent>();
	if (!transform)
	{
		return;
	}

	const float minY = min(m_MinY, m_MaxY);
	const float maxY = max(m_MinY, m_MaxY);
	const float safeDuration = max(0.001f, m_RiseDurationSeconds);
	const float t = std::clamp(m_ElapsedSeconds / safeDuration, 0.0f, 1.0f);
	const float currentY = minY + (maxY - minY) * t;

	XMFLOAT3 pos = transform->GetPosition();
	pos.y = currentY;
	transform->SetPosition(pos);
}