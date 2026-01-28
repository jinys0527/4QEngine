#include "EnemyControllerComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"
#include "Scene.h"
#include "GameManager.h"
#include "ServiceRegistry.h"
#include "GridSystemComponent.h"
#include "EnemyMovementComponent.h"
#include "EnemyComponent.h"

REGISTER_COMPONENT(EnemyControllerComponent)

void EnemyControllerComponent::Start()
{
	GetSystem();
}

void EnemyControllerComponent::Update(float deltaTime)
{
	if (m_GameManager->GetTurn() != Turn::EnemyTurn)
	{
		return;
	}

	// 모든 Enemies 행동 종료 시.
	if (CheckActiveEnemies())
	{
		m_GameManager->SetTurn(Turn::PlayerTurn);
	}
}

void EnemyControllerComponent::OnEvent(EventType type, const void* data)
{



}

void EnemyControllerComponent::GetSystem()
{
	auto* object = GetOwner();
	if (!object) { return; }
	auto* grid = object->GetComponent<GridSystemComponent>();
	if (!grid) { return;  }

	auto* scene = object->GetScene();
	auto& service = scene->GetServices();

	m_GameManager = &service.Get<GameManager>();
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
