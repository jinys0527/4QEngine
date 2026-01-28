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

	m_AIController->Tick(deltaTime);
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
}
