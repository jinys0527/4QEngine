#include "EnemyComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"
#include "AIController.h"
#include "BTExecutor.h"
#include "Blackboard.h"
#include "BlackboardKeys.h"
#include "CombatBehaviorTreeFactory.h"
#include "TransformComponent.h"
#include "EnemyStatComponent.h"
#include "GameObject.h"
#include "PlayerComponent.h"
#include "Scene.h"
#include "EnemyMovementComponent.h"
#include "GridSystemComponent.h"
#include "NodeComponent.h"
#include <array>
#include <cmath>

REGISTER_COMPONENT(EnemyComponent)
REGISTER_PROPERTY_READONLY(EnemyComponent, Q)
REGISTER_PROPERTY_READONLY(EnemyComponent, R)
REGISTER_PROPERTY(EnemyComponent, MoveDistance)

EnemyComponent::EnemyComponent() {
	m_Facing = ERotationOffset::clock_9;
}

EnemyComponent::~EnemyComponent() {
	// Event Listener 쓰는 경우만
	GetEventDispatcher().RemoveListener(EventType::TurnChanged, this);
}

void EnemyComponent::Start()
{
	m_BTExecutor = std::make_unique<BTExecutor>();
	m_BTExecutor->SetRoot(CombatBehaviorTreeFactory::BuildDefaultTree(&GetEventDispatcher()));
	m_AIController = std::make_unique<AIController>(*m_BTExecutor);
	GetEventDispatcher().AddListener(EventType::TurnChanged, this);

	auto* owner = GetOwner();
	auto* scene = owner ? owner->GetScene() : nullptr;
	if (!scene)
	{
		return;
	}

	for (const auto& [name, object] : scene->GetGameObjects())
	{
		(void)name;
		if (!object)
		{
			continue;
		}

		if (!m_GridSystem)
		{
			if (auto* grid = object->GetComponent<GridSystemComponent>())
			{
				m_GridSystem = grid;
			}
		}

		if (auto* player = object->GetComponent<PlayerComponent>())
		{
			m_TargetTransform = object->GetComponent<TransformComponent>();
			m_TargetPlayer = player;
			if (m_GridSystem)
			{
				break;
			}
		}
	}
}


///  시야 판별
//-----------------------------------
struct AxialDirection
{
	int q = 0;
	int r = 0;
};

constexpr std::array<AxialDirection, 6> kFacingDirections{ {
	{ 0, 1 },   // clock_1
	{ 1, 0 },   // clock_3
	{ 1, -1 },  // clock_5
	{ 0, -1 },  // clock_7
	{ -1, 0 },  // clock_9
	{ -1, 1 }   // clock_11
} };

bool IsTargetVisibleOnHexLine(
	GridSystemComponent* grid,
	int selfQ,
	int selfR,
	int facingIndex,
	int sightRange)
{
	if (!grid || sightRange <= 0)
	{
		return false;
	}

	const int dirCount = static_cast<int>(kFacingDirections.size());
	if (facingIndex < 0 || facingIndex >= dirCount)
	{
		return false;
	}

	const int leftIndex = (facingIndex + dirCount - 1) % dirCount;
	const int rightIndex = (facingIndex + 1) % dirCount;
	const AxialDirection candidates[] = {
		kFacingDirections[facingIndex],
		kFacingDirections[leftIndex],
		kFacingDirections[rightIndex]
	};

	for (const auto& dir : candidates)
	{
		for (int step = 1; step <= sightRange; ++step)
		{
			const int q = selfQ + dir.q * step;
			const int r = selfR + dir.r * step;
			NodeComponent* node = grid->GetNodeByKey({ q, r });
			if (!node)
			{
				break;
			}
			if (!node->GetIsSight())
			{
				break;
			}
			if (node->GetState() == NodeState::HasPlayer)
			{
				return true;
			}
		}
	}

	return false;
}
//-----------------------------------


void EnemyComponent::Update(float deltaTime) {
	auto* owner = GetOwner();
	if (!owner || !m_AIController)
	{
		return;
	}

	auto& bb = m_AIController->GetBlackboard();
	auto* transform = owner->GetComponent<TransformComponent>();
	if (transform)
	{
		const auto pos = transform->GetPosition();
		const auto forward = transform->GetForward();
		bb.Set(BlackboardKeys::SelfPosX,         pos.x);
		bb.Set(BlackboardKeys::SelfPosY,         pos.y);
		bb.Set(BlackboardKeys::SelfPosZ,         pos.z);
		bb.Set(BlackboardKeys::SelfForwardX, forward.x);
		bb.Set(BlackboardKeys::SelfForwardY, forward.y);
		bb.Set(BlackboardKeys::SelfForwardZ, forward.z);
	}
	bb.Set(BlackboardKeys::SelfQ, m_Q);
	bb.Set(BlackboardKeys::SelfR, m_R);
	bb.Set(BlackboardKeys::FacingDirection, static_cast<int>(m_Facing));
	float sightDistance = 0.0f;

	if (auto* stat = owner->GetComponent<EnemyStatComponent>())
	{
		sightDistance = stat->GetSightDistance();
		bb.Set(BlackboardKeys::SightDistance, sightDistance);
		bb.Set(BlackboardKeys::SightAngle,    stat->GetSightAngle());
		bb.Set(BlackboardKeys::ThrowRange,    static_cast<float>(stat->GetMaxDiceValue()));
		bb.Set(BlackboardKeys::MeleeRange,    1.0f);
		bb.Set(BlackboardKeys::HP,			  stat->GetCurrentHP());
	}
	else
	{
		sightDistance = 100.0f;
		bb.Set(BlackboardKeys::SightDistance, sightDistance);
		bb.Set(BlackboardKeys::SightAngle, 180.0f);
		bb.Set(BlackboardKeys::ThrowRange, 3.0f);
		bb.Set(BlackboardKeys::MeleeRange, 1.0f);
		bb.Set(BlackboardKeys::HP, 30);
	}

	if (m_TargetTransform)
	{
		const auto targetPos = m_TargetTransform->GetPosition();
		bb.Set(BlackboardKeys::TargetPosX, targetPos.x);
		bb.Set(BlackboardKeys::TargetPosY, targetPos.y);
		bb.Set(BlackboardKeys::TargetPosZ, targetPos.z);
	}

	const bool hasHexData = m_GridSystem && m_TargetPlayer;
	bb.Set(BlackboardKeys::HasHexSightData, hasHexData);
	if (hasHexData)
	{
		const int sightRange = static_cast<int>(std::floor(sightDistance));
		const bool targetVisible = IsTargetVisibleOnHexLine(
			m_GridSystem,
			m_Q,
			m_R,
			static_cast<int>(m_Facing),
			sightRange);
		bb.Set(BlackboardKeys::HasTargetHexLine, targetVisible);
	}

	bb.Set(BlackboardKeys::PreferRanged, false);
	bb.Set(BlackboardKeys::MaintainRange, false);

	m_AIController->Tick(deltaTime);

	bool moveRequested = false;
	bool runOffRequested = false;
	bool maintainRangeRequested = false;
	if (bb.TryGet(BlackboardKeys::MoveRequested, moveRequested) && moveRequested)
	{
		m_MoveRequested = true;
		bb.Set(BlackboardKeys::MoveRequested, false);
	}
	if (bb.TryGet(BlackboardKeys::RequestRunOffMove, runOffRequested) && runOffRequested)
	{
		m_MoveRequested = true;
		bb.Set(BlackboardKeys::RequestRunOffMove, false);
	}
	if (bb.TryGet(BlackboardKeys::RequestMaintainRange, maintainRangeRequested) && maintainRangeRequested)
	{
		m_MoveRequested = true;
		bb.Set(BlackboardKeys::RequestMaintainRange, false);
	}
}

void EnemyComponent::OnEvent(EventType type, const void* data)
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

	m_CurrentTurn = static_cast<Turn>(payload->turn);
	if (m_CurrentTurn == Turn::EnemyTurn)
	{
		m_MoveRequested = true;
	}
	else
	{
		m_MoveRequested = false;
	}
}

bool EnemyComponent::ConsumeMoveRequest()
{
	if (!m_MoveRequested)
	{
		return false;
	}

	m_MoveRequested = false;
	return true;
}
