#include "PlayerMovementComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"
#include "Event.h"
#include "TransformComponent.h"
REGISTER_COMPONENT(PlayerMovementComponent)
REGISTER_PROPERTY(PlayerMovementComponent, Speed)

PlayerMovementComponent::PlayerMovementComponent()
{
}

PlayerMovementComponent::~PlayerMovementComponent()
{
	GetEventDispatcher().RemoveListener(EventType::KeyDown, this);
	GetEventDispatcher().RemoveListener(EventType::KeyUp, this);
}

void PlayerMovementComponent::Start()
{
	GetEventDispatcher().AddListener(EventType::KeyDown, this); // 이벤트 추가
	GetEventDispatcher().AddListener(EventType::KeyUp, this);
}

void PlayerMovementComponent::Update(float deltaTime)
{
	// 키 입력 처리
 	if (!GetOwner()->GetScene())
		return;

	auto* transComp = GetOwner()->GetComponent<TransformComponent>();
	if (transComp == nullptr)
		return;

	float x = 0.f, z = 0.f;
	if (m_IsLeft)  x -= 1.f;
	if (m_IsRight) x += 1.f;
	if (m_IsUp)    z += 1.f;
	if (m_IsDown)  z -= 1.f;

	if (x == 0.f && z == 0.f) return;

	// 대각선 정규화(속도 동일)
	const float len = std::sqrt(x * x + z * z);
	x /= len; z /= len;

	transComp->Translate({ x * m_Speed * deltaTime, 0.f, z * m_Speed * deltaTime });
}

void PlayerMovementComponent::OnEvent(EventType type, const void* data)
{
	if (type == EventType::KeyDown)
	{
		auto keyData = static_cast<const Events::KeyEvent*>(data);
		if (!keyData) return;

		bool isDown = (type == EventType::KeyDown);

		switch (keyData->key)
		{
		case VK_UP    : m_IsUp	  = isDown; break;
		case VK_LEFT  : m_IsLeft  = isDown; break;
		case VK_DOWN  : m_IsDown  = isDown; break;
		case VK_RIGHT : m_IsRight = isDown; break;
		default: break;
		}
	}
	else if (type == EventType::KeyUp)
	{
		auto keyData = static_cast<const Events::KeyEvent*>(data);
		if (!keyData) return;

		bool isDown = (type == EventType::KeyDown);

		switch (keyData->key)
		{
		case VK_UP    : m_IsUp	  = isDown; break;
		case VK_LEFT  : m_IsLeft  = isDown; break;
		case VK_DOWN  : m_IsDown  = isDown; break;
		case VK_RIGHT : m_IsRight = isDown; break;
		default: break;
		}
	}
}
