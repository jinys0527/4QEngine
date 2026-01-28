#include "EnemyMovementComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"
#include "GameObject.h"
#include "Scene.h"
#include "TransformComponent.h"
#include "NodeComponent.h"
#include "GridSystemComponent.h"
#include "EnemyComponent.h"

REGISTER_COMPONENT(EnemyMovementComponent)

void EnemyMovementComponent::Start()
{
	GetSystem();
}

void EnemyMovementComponent::Update(float deltaTime)
{
	auto* enemy = GetOwner()->GetComponent<EnemyComponent>();
	if (!enemy)
	{
		return;
	}

	const auto currentTurn = enemy->GetCurrentTurn();

	// Move
	if (currentTurn != Turn::EnemyTurn) {
		return;
	}

	if (!m_IsMoveComplete) {
		Move();
	}
	m_IsMoveComplete = true;
}

void EnemyMovementComponent::OnEvent(EventType type, const void* data)
{

}

// 움직임
void EnemyMovementComponent::Move()
{
	auto* enemy = GetOwner()->GetComponent<EnemyComponent>();
	const int moveRange = enemy->GetMoveDistance();

	if (moveRange <= 0) {
		return; 
	}

	const AxialKey start{ enemy->GetQ(), enemy->GetR() };
	NodeComponent* bestNode = nullptr;
	int bestDistance = 100; 


	for (auto* node : m_GridSystem->GetNodes())
	{
		if (!node)
		{
			continue;
		}
		if (!node->GetIsMoveable() || node->GetState() != NodeState::Empty)
		{
			continue;
		}

		const AxialKey target{ node->GetQ(), node->GetR() };
		const int distance = m_GridSystem->GetShortestPathLength(start, target);
		if (distance <= 0 || distance > moveRange)
		{
			continue;
		}

		if (distance < bestDistance)
		{
			bestDistance = distance;
			bestNode = node;
		}
	}
	if (!bestNode)
	{
		return;
	}


	auto* targetOwner = bestNode->GetOwner();
	auto* targetTransform = targetOwner ? targetOwner->GetComponent<TransformComponent>() : nullptr;
	auto* enemyTransform = GetOwner()->GetComponent<TransformComponent>();

	if (!targetTransform || !enemyTransform)
	{
		return;
	}

	enemyTransform->SetPosition(targetTransform->GetPosition());
}



void EnemyMovementComponent::GetSystem()
{
	auto* scene = GetOwner() -> GetScene();
	if (!scene){ return;}
	const auto& objects = scene->GetGameObjects();
	for (const auto& [name, object] : objects)
	{
		if (!object)
		{
			continue;
		}

		if (auto* grid = object->GetComponent<GridSystemComponent>())
		{
			m_GridSystem = grid;
		}
	}
}
