#include "EnemyMovementComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"
#include "GameObject.h"
#include "Scene.h"
#include "TransformComponent.h"
#include "NodeComponent.h"
#include "GridSystemComponent.h"
#include "EnemyComponent.h"
#include "GameManager.h"

REGISTER_COMPONENT(EnemyMovementComponent)

EnemyMovementComponent::~EnemyMovementComponent()
{
	GetEventDispatcher().RemoveListener(EventType::TurnChanged, this);
}

void EnemyMovementComponent::Start()
{
	GetSystem();
	GetEventDispatcher().AddListener(EventType::TurnChanged, this);
}

void EnemyMovementComponent::Update(float deltaTime)
{
	(void)deltaTime;

	auto* owner = GetOwner();
	auto* scene = owner ? owner->GetScene() : nullptr;
	auto* gameManager = scene ? scene->GetGameManager() : nullptr;
	if (!gameManager)
		return;

	// 탐색 EnemyStep 또는 전투 EnemyTurn에서만 움직임 허용
	const bool explorationEnemyStep =
		(gameManager->GetPhase() == Phase::ExplorationLoop &&
			gameManager->GetExplorationTurnState() == ExplorationTurnState::EnemyStep);

	const bool combatEnemyTurn =
		(gameManager->GetPhase() == Phase::TurnBasedCombat &&
			gameManager->GetTurn() == Turn::EnemyTurn);

	if (!explorationEnemyStep && !combatEnemyTurn)
		return;


	auto* enemy = GetOwner()->GetComponent<EnemyComponent>();
	if (!enemy)
		return;

	if (enemy->GetCurrentTurn() != Turn::EnemyTurn)
		return;

	bool hasRequest = false;

	// 1) 기존 탐색 이동 요청도 계속 지원
	if (enemy->ConsumeMoveRequest())
	{
		m_PendingOrder = EMoveOrder::Patrol;
		hasRequest = true;
	}

	// 2) 전투/BT 쪽에서 직접 요청한 이동도 지원
	if (m_PendingOrder != EMoveOrder::None)
	{
		hasRequest = true;
	}

	if (!hasRequest)
		return;

	// 요청 실행 (실패하더라도 턴은 진행되게 "완료 처리"는 한다)
	switch (m_PendingOrder)
	{
	case EMoveOrder::RunOff:
		MovePatrol(); // TODO: 실제 도주 알고리즘으로 교체
		break;
	case EMoveOrder::MaintainRange:
		MovePatrol(); // TODO: 실제 거리유지 알고리즘으로 교체
		break;
	case EMoveOrder::Approach:
		MovePatrol(); // TODO: 실제 접근 알고리즘으로 교체
		break;
	case EMoveOrder::Patrol:
	default:
		MovePatrol();
		break;
	}

	m_PendingOrder = EMoveOrder::None;
	m_IsMoveComplete = true;

}

void EnemyMovementComponent::OnEvent(EventType type, const void* data)
{
	if (type != EventType::TurnChanged || !data)
	{
		return;
	}

	const auto* payload = static_cast<const Events::TurnChanged*>(data);
	if (!payload)
	{
		return;
	}

	const auto turn = static_cast<Turn>(payload->turn);

	auto* scene = GetOwner() ? GetOwner()->GetScene() : nullptr;
	auto* gameManager = scene ? scene->GetGameManager() : nullptr;

	// EnemyTurn 시작이면 매번 이동 완료 플래그 리셋
	if (turn == Turn::EnemyTurn && gameManager)
	{
		// 탐색/전투 둘 다 EnemyTurn이면 리셋
		m_IsMoveComplete = false;
	}
}

void EnemyMovementComponent::MoveRunOff()
{
}

void EnemyMovementComponent::MoveApproach()
{
}

void EnemyMovementComponent::MoveMaintainRange()
{
}

void EnemyMovementComponent::RequestMoveToTarget()
{
	m_PendingOrder = EMoveOrder::Approach;
	m_IsMoveComplete = false;
}

void EnemyMovementComponent::RequestRunOff()
{
	m_PendingOrder = EMoveOrder::RunOff;
	m_IsMoveComplete = false;
}

void EnemyMovementComponent::RequestMaintainRange()
{
	m_PendingOrder = EMoveOrder::MaintainRange;
	m_IsMoveComplete = false;
}

void EnemyMovementComponent::MovePatrol()
{
	auto* owner = GetOwner();
	auto* enemy = owner ? owner->GetComponent<EnemyComponent>() : nullptr;
	if (!enemy || !m_GridSystem)
		return;

	const int moveRange = enemy->GetMoveDistance();
	if (moveRange <= 0)
		return;

	const AxialKey start{ enemy->GetQ(), enemy->GetR() };

	NodeComponent* bestNode = nullptr;
	int bestDistance = 100;

	for (auto* node : m_GridSystem->GetNodes())
	{
		if (!node) continue;
		if (!node->GetIsMoveable() || node->GetState() != NodeState::Empty)
			continue;

		const AxialKey target{ node->GetQ(), node->GetR() };
		const int distance = m_GridSystem->GetShortestPathLength(start, target);
		if (distance <= 0 || distance > moveRange)
			continue;

		if (distance < bestDistance)
		{
			bestDistance = distance;
			bestNode = node;
		}
	}

	if (!bestNode)
		return;

	auto* targetOwner = bestNode->GetOwner();
	auto* targetTransform = targetOwner ? targetOwner->GetComponent<TransformComponent>() : nullptr;
	auto* enemyTransform = owner->GetComponent<TransformComponent>();
	if (!targetTransform || !enemyTransform)
		return;

	enemyTransform->SetPosition(targetTransform->GetPosition());
}


void EnemyMovementComponent::SetEnemyRotation(TransformComponent* transComp, ERotationOffset dir)
{

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
