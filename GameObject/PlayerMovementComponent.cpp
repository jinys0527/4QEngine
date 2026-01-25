#include "PlayerMovementComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"
#include "Event.h"
#include "TransformComponent.h"
#include "ServiceRegistry.h"
#include "InputManager.h"
#include "Scene.h"
#include "CameraObject.h"

REGISTER_COMPONENT(PlayerMovementComponent)
REGISTER_PROPERTY(PlayerMovementComponent, Speed)
REGISTER_PROPERTY(PlayerMovementComponent, DragSpeed)

PlayerMovementComponent::PlayerMovementComponent()
{
}

PlayerMovementComponent::~PlayerMovementComponent()
{
	GetEventDispatcher().RemoveListener(EventType::KeyDown, this);
	GetEventDispatcher().RemoveListener(EventType::KeyUp, this);
	GetEventDispatcher().RemoveListener(EventType::MouseLeftClick, this);
	GetEventDispatcher().RemoveListener(EventType::MouseLeftDoubleClick, this);
	GetEventDispatcher().RemoveListener(EventType::Dragged, this);
	GetEventDispatcher().RemoveListener(EventType::MouseLeftClickUp, this);
}

void PlayerMovementComponent::Start()
{
	GetEventDispatcher().AddListener(EventType::KeyDown, this); // 이벤트 추가
	GetEventDispatcher().AddListener(EventType::KeyUp, this);
	GetEventDispatcher().AddListener(EventType::MouseLeftClick, this);
	GetEventDispatcher().AddListener(EventType::MouseLeftDoubleClick, this);
	GetEventDispatcher().AddListener(EventType::Dragged, this);
	GetEventDispatcher().AddListener(EventType::MouseLeftClickUp, this);
}

void PlayerMovementComponent::Update(float deltaTime)
{
	// 키 입력 처리
//  	if (!GetOwner()->GetScene())
// 		return;
// 
// 	auto* transComp = GetOwner()->GetComponent<TransformComponent>();
// 	if (transComp == nullptr)
// 		return;
// 
// 	float x = 0.f, z = 0.f;
// 	if (m_IsLeft)  x -= 1.f;
// 	if (m_IsRight) x += 1.f;
// 	if (m_IsUp)    z += 1.f;
// 	if (m_IsDown)  z -= 1.f;
// 
// 	if (x == 0.f && z == 0.f) return;

	// 대각선 정규화(속도 동일)
// 	const float len = std::sqrt(x * x + z * z);
// 	x /= len; z /= len;
// 
// 	transComp->Translate({ x * m_Speed * deltaTime, 0.f, z * m_Speed * deltaTime });
}

void PlayerMovementComponent::OnEvent(EventType type, const void* data)
{
// 	if (type == EventType::KeyDown)
// 	{
// 		auto keyData = static_cast<const Events::KeyEvent*>(data);
// 		if (!keyData) return;
// 
// 		bool isDown = (type == EventType::KeyDown);
// 
// 		switch (keyData->key)
// 		{
// 		case VK_UP    : m_IsUp	  = isDown; break;
// 		case VK_LEFT  : m_IsLeft  = isDown; break;
// 		case VK_DOWN  : m_IsDown  = isDown; break;
// 		case VK_RIGHT : m_IsRight = isDown; break;
// 		default: break;
// 		}
// 	}
// 	else if (type == EventType::KeyUp)
// 	{
// 		auto keyData = static_cast<const Events::KeyEvent*>(data);
// 		if (!keyData) return;
// 
// 		bool isDown = (type == EventType::KeyDown);
// 
// 		switch (keyData->key)
// 		{
// 		case VK_UP    : m_IsUp	  = isDown; break;
// 		case VK_LEFT  : m_IsLeft  = isDown; break;
// 		case VK_DOWN  : m_IsDown  = isDown; break;
// 		case VK_RIGHT : m_IsRight = isDown; break;
// 		default: break;
// 		}
// 	}

	const auto* mouseData = static_cast<const Events::MouseState*>(data);
	if (!mouseData)
		return;

	auto* owner = GetOwner();
	if (!owner)
		return;

	auto* scene = owner->GetScene();
	if (!scene)
		return;

	if (!scene->GetServices().Has<InputManager>())
		return;

	auto& input = scene->GetServices().Get<InputManager>();
	auto camera = scene->GetGameCamera();
	if (!camera)
		return;

	auto* transComp = owner->GetComponent<TransformComponent>();
	if (!transComp)
		return;

	DirectX::XMFLOAT3 rayOrigin{};
	DirectX::XMFLOAT3 rayDir{};

	auto tryHitPlane = [&](float planeY, DirectX::XMFLOAT3& outHit) -> bool
		{
			const float denom = rayDir.y;
			if (std::abs(denom) < 1e-5f)
				return false;

			const float t = (planeY - rayOrigin.y) / denom;
			if (t < 0.0f)
				return false;

			outHit = {
				rayOrigin.x + rayDir.x * t,
				planeY,
				rayOrigin.z + rayDir.z * t
			};
			return true;
		};

	if (type == EventType::MouseLeftClick)
	{
		if (!input.BuildPickRay(camera->GetViewMatrix(), camera->GetProjMatrix(), *mouseData, rayOrigin, rayDir))
			return;

		m_DragPlaneY = transComp->GetPosition().y;
		DirectX::XMFLOAT3 hit{};
		if (!tryHitPlane(m_DragPlaneY, hit))
			return;

		const auto pos = transComp->GetPosition();
		m_DragOffset = { pos.x - hit.x, pos.y - hit.y, pos.z - hit.z };
		m_IsDragging = true;
		return;
	}

	if (type == EventType::MouseLeftClickUp)
	{
		m_IsDragging = false;
		return;
	}

	if (type != EventType::Dragged || !m_IsDragging)
		return;

	if (!input.BuildPickRay(camera->GetViewMatrix(), camera->GetProjMatrix(), *mouseData, rayOrigin, rayDir))
		return;

	DirectX::XMFLOAT3 hit{};
	if (!tryHitPlane(m_DragPlaneY, hit))
		return;

	const DirectX::XMFLOAT3 newPos{
		hit.x + m_DragOffset.x,
		hit.y + m_DragOffset.y,
		hit.z + m_DragOffset.z
	};

	transComp->SetPosition(newPos);
}
