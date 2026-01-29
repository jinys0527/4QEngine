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

REGISTER_COMPONENT(EnemyComponent)
REGISTER_PROPERTY_READONLY(EnemyComponent, Q)
REGISTER_PROPERTY_READONLY(EnemyComponent, R)
REGISTER_PROPERTY(EnemyComponent, MoveDistance)

EnemyComponent::EnemyComponent() {

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

		if (object->GetComponent<PlayerComponent>())
		{
			m_TargetTransform = object->GetComponent<TransformComponent>();
			break;
		}
	}
}

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

	if (auto* stat = owner->GetComponent<EnemyStatComponent>())
	{
		bb.Set(BlackboardKeys::SightDistance, stat->GetSightDistance());
		bb.Set(BlackboardKeys::SightAngle,    stat->GetSightAngle());
		bb.Set(BlackboardKeys::ThrowRange,    static_cast<float>(stat->GetMaxDiceValue()));
		bb.Set(BlackboardKeys::MeleeRange,    1.0f);
		bb.Set(BlackboardKeys::HP,			  stat->GetCurrentHP());
	}
	else
	{
		bb.Set(BlackboardKeys::SightDistance, 100.0f);
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
