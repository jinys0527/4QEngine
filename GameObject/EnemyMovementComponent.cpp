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

static bool TryGetRotationFromStep(const AxialKey& previous, const AxialKey& current, ERotationOffset& outDir)
{
	const AxialKey delta{ current.q - previous.q, current.r - previous.r };
	constexpr std::array<std::pair<AxialKey, ERotationOffset>, 6> kDirections{ {
		{ { 1, 0 }, ERotationOffset::clock_3 },
		{ { 1, -1 }, ERotationOffset::clock_5 },
		{ { 0, -1 }, ERotationOffset::clock_7 },
		{ { -1, 0 }, ERotationOffset::clock_9 },
		{ { -1, 1 }, ERotationOffset::clock_11 },
		{ { 0, 1 }, ERotationOffset::clock_1 }
	} };

	for (const auto& [dir, rotation] : kDirections)
	{
		if (dir.q == delta.q && dir.r == delta.r)
		{
			outDir = rotation;
			return true;
		}
	}

	return false;
}

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

	if (!m_IsMoveComplete && enemy->ConsumeMoveRequest()) {
		Move();
		m_IsMoveComplete = true;
	}

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
	if (turn == Turn::EnemyTurn)
	{
		m_IsMoveComplete = false;
	}
}

// 움직임
void EnemyMovementComponent::Move()
{
	auto* enemy = GetOwner()->GetComponent<EnemyComponent>();
	const int moveRange = enemy->GetMoveDistance();

	if (moveRange <= 0 || !m_GridSystem) {
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

	const AxialKey target{ bestNode->GetQ(), bestNode->GetR() };
	const auto path = m_GridSystem->GetShortestPath(start, target);
	if (path.size() >= 2)
	{
		const AxialKey& previousKey = path[path.size() - 2];
		const AxialKey& currentKey = path.back();
		ERotationOffset rotation{};
		if (TryGetRotationFromStep(previousKey, currentKey, rotation))
		{
			SetEnemyRotation(enemyTransform, rotation);
			enemy->SetFacing(rotation); // 시야 범위
		}
	}

	enemyTransform->SetPosition(targetTransform->GetPosition());
}



void EnemyMovementComponent::SetEnemyRotation(TransformComponent* transComp, ERotationOffset dir)
{
	if (!transComp)
	{
		return;
	}

	switch (dir)
	{
	case ERotationOffset::clock_1:
		transComp->SetRotationEuler({ 0.0f,-150.0f ,0.0f });
		break;
	case ERotationOffset::clock_3:
		transComp->SetRotationEuler({ 0.0f,-90.0f ,0.0f });
		break;
	case ERotationOffset::clock_5:
		transComp->SetRotationEuler({ 0.0f,-30.0f ,0.0f });
		break;
	case ERotationOffset::clock_7:
		transComp->SetRotationEuler({ 0.0f,30.0f ,0.0f });
		break;
	case ERotationOffset::clock_9:
		transComp->SetRotationEuler({ 0.0f,90.0f ,0.0f });
		break;
	case ERotationOffset::clock_11:
		transComp->SetRotationEuler({ 0.0f,150.0f ,0.0f });
		break;
	default:
		break;
	}
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
