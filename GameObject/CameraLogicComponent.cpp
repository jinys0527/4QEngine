
#include "CameraLogicComponent.h"
#include "CameraComponent.h"
#include "PlayerComponent.h"
#include "TransformComponent.h"
#include "ReflectionMacro.h"
#include "Scene.h"
#include <algorithm>
#include <limits>
// Game Main Camera Logic
REGISTER_COMPONENT(CameraLogicComponent)
REGISTER_PROPERTY(CameraLogicComponent, MaxZoom)
REGISTER_PROPERTY(CameraLogicComponent, MinZoom)
REGISTER_PROPERTY(CameraLogicComponent, MoveSpeed)


CameraLogicComponent::~CameraLogicComponent()
{
	GetEventDispatcher().RemoveListener(EventType::MouseWheelUp, this);
	GetEventDispatcher().RemoveListener(EventType::MouseWheelDown, this);
}

// Player 등록 필요. 
// Player의 GetWorldPos로 월드 기준 좌표를 Look으로 설정
void CameraLogicComponent::Start()
{
	auto* owner = GetOwner();
	if (!owner)
		return;

	GetEventDispatcher().AddListener(EventType::MouseWheelUp, this);
	GetEventDispatcher().AddListener(EventType::MouseWheelDown, this);

	auto*  trans = owner->GetComponent<TransformComponent>();
	auto* cam = owner->GetComponent<CameraComponent>();
	if (!trans || !cam) { return; }
	m_Transform = trans;
	m_Camera = cam;

	auto* scene = owner->GetScene();
	for (const auto& [name, gameObject] : scene->GetGameObjects())
	{
		(void)name;
		if (!gameObject)
			continue;

		if (gameObject->GetComponent<PlayerComponent>())
		{
			if (auto* playerTransform = gameObject->GetComponent<TransformComponent>())
			{
				m_PlayerTransform = playerTransform;
				break;
			}
		}
	}
}

void CameraLogicComponent::Update(float deltaTime)
{
	if (!m_Camera)
		return;
	if (!m_PlayerTransform)
		return;
	CamZoom();


}

void CameraLogicComponent::OnEvent(EventType type, const void* data)
{
	if (type == EventType::MouseWheelUp)
	{
		m_PendingZoomInput += 1.0f;
	}
	else if (type == EventType::MouseWheelDown) {

		m_PendingZoomInput -= 1.0f;
	}
}

void CameraLogicComponent::CamZoom()
{
	const XMFLOAT3 currentEye = m_Camera->GetEye();
	XMFLOAT3 targetLook = m_Camera->GetLook();
	if (m_PlayerTransform)
	{
		targetLook = m_PlayerTransform->GetWorldPos();
	}

	XMVECTOR eyeVec = XMLoadFloat3(&currentEye);
	XMVECTOR lookVec = XMLoadFloat3(&targetLook);
	XMVECTOR toEye = XMVectorSubtract(eyeVec, lookVec);
	const float currentDistance = XMVectorGetX(XMVector3Length(toEye));
	XMVECTOR dir = XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f);
	if (currentDistance > 0.0001f)
	{
		dir = XMVectorScale(toEye, 1.0f / currentDistance);
	}

	float desiredDistance = currentDistance;
	if (m_PendingZoomInput != 0.0f)
	{
		desiredDistance = currentDistance - (m_PendingZoomInput * m_ZoomSpeed);
		const float minZoom = m_MinZoom;
		const float maxZoom = m_MaxZoom;
		if (maxZoom > 0.0f || minZoom > 0.0f)
		{
			const float clampedMin = (std::min)(minZoom, maxZoom > 0.0f ? maxZoom : minZoom);
			const float clampedMax = (std::max)(maxZoom, clampedMin);
			desiredDistance = std::clamp(desiredDistance, clampedMin, clampedMax);
		}
		m_PendingZoomInput = 0.0f;
	}

	XMVECTOR newEyeVec = XMVectorAdd(lookVec, XMVectorScale(dir, desiredDistance));
	XMFLOAT3 newEye{};
	XMStoreFloat3(&newEye, newEyeVec);
	m_Camera->SetEyeLookUp(newEye, targetLook, m_Camera->GetUp());
}
