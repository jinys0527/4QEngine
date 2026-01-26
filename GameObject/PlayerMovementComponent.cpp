#include "PlayerMovementComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"
#include "Event.h"
#include "TransformComponent.h"
#include "ServiceRegistry.h"
#include "InputManager.h"
#include "Scene.h"
#include "RayHelper.h"
#include "CameraObject.h"
#include "BoxColliderComponent.h"
#include <cfloat>

REGISTER_COMPONENT(PlayerMovementComponent)
REGISTER_PROPERTY(PlayerMovementComponent, Speed)
REGISTER_PROPERTY(PlayerMovementComponent, DragSpeed)

static bool FindClosestSurfaceHit(
								  Scene* scene,
								  GameObject* ignoreObject,
								  const DirectX::XMFLOAT3& rayOrigin,
								  const DirectX::XMFLOAT3& rayDir,
								  DirectX::XMFLOAT3& outHitPos)
{
	if (!scene) return false;

	auto& gameObjects = scene->GetGameObjects();

	float closestT = FLT_MAX;
	GameObject* closestObj = nullptr;

	for (const auto& [name, object] : gameObjects)
	{
		if (!object) continue;

		GameObject* gameObect = object.get();
		if (gameObect == ignoreObject) continue; // 자기 자신 제외

		auto* col = gameObect->GetComponent<BoxColliderComponent>();
		if (!col || !col->HasBounds()) continue;

		float t = 0.0f;
		if (!col->IntersectsRay(rayOrigin, rayDir, t)) continue;

		if (t >= 0.0f && t < closestT)
		{
			closestT = t;
			closestObj = gameObect;
		}
	}

	if (!closestObj) return false;

	outHitPos = {
		rayOrigin.x + rayDir.x * closestT,
		rayOrigin.y + rayDir.y * closestT,
		rayOrigin.z + rayDir.z * closestT
	};
	return true;
}

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

	if (!m_IsDragging || !m_HasDragRay)
		return;

	auto* owner = GetOwner();
	if (!owner)
		return;

	auto* scene = owner->GetScene();
	if (!scene) return;

	auto* transComp = owner->GetComponent<TransformComponent>();
	if (!transComp)
		return;

	DirectX::XMFLOAT3 surfaceHit{};
	if (!FindClosestSurfaceHit(scene, dynamic_cast<GameObject*>(owner), m_DragRayOrigin, m_DragRayDir, surfaceHit))
		return;

	DirectX::XMFLOAT3 newPos{
		surfaceHit.x + m_DragOffset.x,
		surfaceHit.y + m_DragOffset.y,
		surfaceHit.z + m_DragOffset.z
	};

	transComp->SetPosition(newPos);
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

	if (type == EventType::MouseLeftClickUp)
	{
		m_IsDragging = false;
		m_HasDragRay = false;
		return;
	}

	if (!input.IsPointInViewport(mouseData->pos))
		return;

	if (type == EventType::MouseLeftClick)
	{
		Ray pickRay{};
		if (!input.BuildPickRay(camera->GetViewMatrix(), camera->GetProjMatrix(), *mouseData, pickRay))
			return;

		auto* collider = owner->GetComponent<BoxColliderComponent>();
		if (!collider || !collider->HasBounds())
			return;

		auto& gameObjects = scene->GetGameObjects();
		float closestT = FLT_MAX;
		GameObject* closestObject = nullptr;

		// 가장 가까운 collider hit 찾기 (선택 판정)
		for (const auto& [name, object] : gameObjects)
		{
			if (!object)
				continue;

			auto* otherCollider = object->GetComponent<BoxColliderComponent>();
			if (!otherCollider || !otherCollider->HasBounds())
				continue;

			float hitT = 0.0f;
			if (!otherCollider->IntersectsRay(pickRay.m_Pos, pickRay.m_Dir, hitT))
				continue;

			if (hitT >= 0.0f && hitT < closestT)
			{
				closestT = hitT;
				closestObject = object.get();
			}
		}

		if (closestObject != owner)
			return;

		// 2) 드래그 기준 표면 hit (바닥/발판). 자기 자신은 제외.
		DirectX::XMFLOAT3 surfaceHit{};
		GameObject* ownerGameObject = dynamic_cast<GameObject*>(owner);
		if (!FindClosestSurfaceHit(scene, ownerGameObject, pickRay.m_Pos, pickRay.m_Dir, surfaceHit))
		{
			// fallback: 아무 표면도 없으면 y=0 평면으로라도 이동시키고 싶을 때만 사용
			float t = 0.0f;
			if (!IntersectRayPlaneY(pickRay, 0.0f, t, surfaceHit)) return;
		}

		const auto pos = transComp->GetPosition();

		m_DragOffset = { pos.x - surfaceHit.x, pos.y - surfaceHit.y, pos.z - surfaceHit.z };

		m_DragRayOrigin = pickRay.m_Pos;
		m_DragRayDir    = pickRay.m_Dir;
		m_IsDragging    = true;
		m_HasDragRay    = true;
		return;
	}

	// 드래그 중: 레이만 갱신
	if (type != EventType::Dragged || !m_IsDragging)
		return;

	Ray dragRay{};
	if (!input.BuildPickRay(camera->GetViewMatrix(), camera->GetProjMatrix(), *mouseData, dragRay))
		return;

	m_DragRayOrigin = dragRay.m_Pos;
	m_DragRayDir    = dragRay.m_Dir;
	m_HasDragRay    = true;
}
