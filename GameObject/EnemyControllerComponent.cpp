#include "EnemyControllerComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"
#include "GridSystemComponent.h"
#include "EnemyMovementComponent.h"
#include "EnemyComponent.h"

REGISTER_COMPONENT(EnemyControllerComponent)

EnemyControllerComponent::~EnemyControllerComponent()
{
}

void EnemyControllerComponent::Start()
{
	GetSystem();
}

void EnemyControllerComponent::Update(float deltaTime)
{
	Turn currentTurn = Turn::PlayerTurn;
	if (m_GridSystem)
	{
		const auto& enemies = m_GridSystem->GetEnemies();
		for (const auto* enemy : enemies)
		{
			if (enemy)
			{
				currentTurn = enemy->GetCurrentTurn();
				break;
			}
		}
	}

	if (currentTurn != Turn::EnemyTurn)
	{
		m_TurnEndRequested = false;
		return;
	}

	// 모든 Enemies 행동 종료 시.
	if (!m_TurnEndRequested && CheckActiveEnemies())
	{
		GetEventDispatcher().Dispatch(EventType::EnemyTurnEndRequested, nullptr);
		m_TurnEndRequested = true;
	}
}

void EnemyControllerComponent::OnEvent(EventType type, const void* data)
{
	(void)type;
	(void)data;
}

void EnemyControllerComponent::GetSystem()
{
	auto* object = GetOwner();
	if (!object) { return; }
	auto* grid = object->GetComponent<GridSystemComponent>();
	if (!grid) { return;  }

	m_GridSystem = grid;
}

bool EnemyControllerComponent::CheckActiveEnemies()
{
	const auto& enemies = m_GridSystem->GetEnemies();

	for (const auto* enemy : enemies) {
		if (!enemy) {
			continue;
		}
		auto* owner = enemy->GetOwner();
		auto* movement = owner->GetComponent<EnemyMovementComponent>();
		if (!movement->IsMoveComplete())
		{
			return false;
		}
	}
	return true;
}
