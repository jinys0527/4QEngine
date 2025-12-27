#include "ColliderComponent.h"
#include "EventDispatcher.h"


ColliderComponent::ColliderComponent()
{
	State noneState{
		[]() {},
		[](float dt) {},
		[]() {}
	};
	State enterState{
		[]() {},
		[](float dt) {},
		[]() {}
	};
	State stayState{
		[]() {},
		[](float dt) {},
		[]() {}
	};
	State exitState{
		[]() {},
		[](float dt) {},
		[]() {}
	};
	m_Fsm.AddState("None", noneState);
	m_Fsm.AddState("Enter", enterState);
	m_Fsm.AddState("Stay", stayState);
	m_Fsm.AddState("Exit", exitState);

	m_Fsm.AddTransition("None", "Enter", "CollisionEnter");
	m_Fsm.AddTransition("Enter", "Stay", "CollisionStay");
	m_Fsm.AddTransition("Enter", "Exit", "CollisionExit");
	m_Fsm.AddTransition("Stay", "Exit", "CollisionExit");
	m_Fsm.AddTransition("Exit", "None", "None");

	m_Fsm.SetInitialState("None");
}

void ColliderComponent::Start()
{
	GetEventDispatcher().AddListener(EventType::CollisionEnter, this);
	GetEventDispatcher().AddListener(EventType::CollisionStay, this);
	GetEventDispatcher().AddListener(EventType::CollisionExit, this);
}

void ColliderComponent::Update(float deltaTime)
{
	m_Fsm.Update(deltaTime);
}

void ColliderComponent::OnEvent(EventType type, const void* data)
{
	const CollisionInfo* info = static_cast<const CollisionInfo*>(data);
	switch (type)
	{
	case EventType::CollisionEnter:
		OnCollisionEnter(info);              // 유저 콜백 실행
		m_Fsm.Trigger("CollisionEnter");     // FSM 상태 전이
		break;
	case EventType::CollisionStay:
		OnCollisionStay(info);
		m_Fsm.Trigger("CollisionStay");
		break;
	case EventType::CollisionExit:
		OnCollisionExit(info);
		m_Fsm.Trigger("CollisionExit");
		break;
	}
}

void ColliderComponent::OnDestroy()
{
	GetEventDispatcher().RemoveListener(EventType::CollisionEnter, this);
	GetEventDispatcher().RemoveListener(EventType::CollisionStay, this);
	GetEventDispatcher().RemoveListener(EventType::CollisionExit, this);
}

void ColliderComponent::Serialize(nlohmann::json& j) const
{
}

void ColliderComponent::Deserialize(const nlohmann::json& j)
{
}
