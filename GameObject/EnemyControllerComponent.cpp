#include "EnemyControllerComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"
#include "GridSystemComponent.h"
#include "EnemyMovementComponent.h"
#include "EnemyComponent.h"
#include "Scene.h"
#include "GameManager.h"

REGISTER_COMPONENT(EnemyControllerComponent)

EnemyControllerComponent::~EnemyControllerComponent()
{
	// 이벤트 리스너 제거 (Start에서 등록하면 반드시 제거해야 함)
	auto& disp = GetEventDispatcher();
	disp.RemoveListener(EventType::AIMoveRequested, this);
	disp.RemoveListener(EventType::AIRunOffMoveRequested, this);
	disp.RemoveListener(EventType::AIMaintainRangeRequested, this);
}

void EnemyControllerComponent::Start()
{
	GetSystem();

	// BT에서 올라오는 요청 이벤트를 여기서 받음
	auto& disp = GetEventDispatcher();
	disp.AddListener(EventType::AIMoveRequested, this);
	disp.AddListener(EventType::AIRunOffMoveRequested, this);
	disp.AddListener(EventType::AIMaintainRangeRequested, this);
}

void EnemyControllerComponent::Update(float deltaTime)
{
	auto* owner = GetOwner();
	auto* scene = owner ? owner->GetScene() : nullptr;
	auto* gameManager = scene ? scene->GetGameManager() : nullptr;

	if (!gameManager || !m_GridSystem)
		return;

	const Phase phase = gameManager->GetPhase();


	// 1) 탐색 루프의 EnemyStep (다수 적 이동 끝나면 ExploreEnemyStepEnded 쏨)
	if (phase == Phase::ExplorationLoop)
	{
		if (gameManager->GetExplorationTurnState() != ExplorationTurnState::EnemyStep)
		{
			m_TurnEndRequested = false;
			return;
		}


		// EnemyTurn인지 확인
		Turn currentTurn = Turn::PlayerTurn;
		bool hasEnemies = false;

		const auto& enemies = m_GridSystem->GetEnemies();
		for (const auto* enemy : enemies)
		{
			if (!enemy) continue;
			hasEnemies = true;
			currentTurn = enemy->GetCurrentTurn();
			break;
		}

		if (currentTurn != Turn::EnemyTurn)
		{
			m_TurnEndRequested = false;
			return;
		}

		if (!hasEnemies)
		{
			if (!m_TurnEndRequested)
			{
				GetEventDispatcher().Dispatch(EventType::ExploreEnemyStepEnded, nullptr);
				m_TurnEndRequested = true;
			}
			return;
		}

		// 모든 적 이동 완료 시 EnemyStep 종료
		if (!m_TurnEndRequested && CheckActiveEnemies())
		{
			GetEventDispatcher().Dispatch(EventType::ExploreEnemyStepEnded, nullptr);
			m_TurnEndRequested = true;
		}

		return;
	}

	// 2) 전투(턴 기반): 여기서는 “AI 행동 요청 후 이동 완료되면 AITurnEndRequested”만 책임지게 하는 게 안전함
	if (phase == Phase::TurnBasedCombat && gameManager->GetTurn() == Turn::EnemyTurn)
	{
		// AI가 이동/도주/거리유지 같은 “이동계열 요청”을 내렸고,
        // 그 이동이 끝났으면 적 턴 종료를 GameManager에 알려줌.
		if (m_CombatMoveInProgress)
		{
			if (IsCurrentEnemyMoveComplete())
			{
				m_CombatMoveInProgress = false;
				GetEventDispatcher().Dispatch(EventType::AITurnEndRequested, nullptr);
			}
		}
	}
}

void EnemyControllerComponent::OnEvent(EventType type, const void* data)
{
	(void)data;

	auto* owner = GetOwner();
	auto* scene = owner ? owner->GetScene() : nullptr;
	auto* gameManager = scene ? scene->GetGameManager() : nullptr;
	if (!gameManager || !m_GridSystem)
		return;

	// 전투 중 EnemyTurn일 때만 AI 이동계열 요청 처리
	if (gameManager->GetPhase() != Phase::TurnBasedCombat || gameManager->GetTurn() != Turn::EnemyTurn)
		return;

	EnemyMovementComponent* move = GetCurrentEnemyMovement();
	if (!move)
		return;

	switch (type)
	{
	case EventType::AIMoveRequested:
		move->RequestMoveToTarget();
		m_CombatMoveInProgress = true;
		break;

	case EventType::AIRunOffMoveRequested:
		move->RequestRunOff();   
		m_CombatMoveInProgress = true;
		break;

	case EventType::AIMaintainRangeRequested:
		move->RequestMaintainRange();
		m_CombatMoveInProgress = true;
		break;

	default:
		break;
	}
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
		if (!enemy) continue;

		auto* owner = enemy->GetOwner();
		if (!owner) continue;

		auto* movement = owner->GetComponent<EnemyMovementComponent>();
		if (!movement) continue;

		if (!movement->IsMoveComplete())
			return false;
	}
	return true;
}

// 전투에서 "현재 행동 중인 적"을 찾는 최소 구현
EnemyMovementComponent* EnemyControllerComponent::GetCurrentEnemyMovement()
{
	const auto& enemies = m_GridSystem->GetEnemies();

	for (const auto* enemy : enemies)
	{
		if (!enemy) continue;

		// 전투 턴에서 현재 적(actor) 판정 기준이 따로 있으면 그걸로 바꿔야 함.
		// 지금은 최소로: EnemyTurn인 개체를 하나 집음.
		if (enemy->GetCurrentTurn() != Turn::EnemyTurn)
			continue;

		auto* owner = enemy->GetOwner();
		if (!owner) continue;

		return owner->GetComponent<EnemyMovementComponent>();
	}
	return nullptr;
}

bool EnemyControllerComponent::IsCurrentEnemyMoveComplete()
{
	EnemyMovementComponent* move = GetCurrentEnemyMovement();
	if (!move) return true; // 없으면 더 할 게 없다고 보고 턴 종료
	return move->IsMoveComplete();
}