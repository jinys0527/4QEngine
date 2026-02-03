
#include "CameraLogicComponent.h"
#include "CameraComponent.h"
#include "TransformComponent.h"
#include "ReflectionMacro.h"
#include "Scene.h"
// Game Main Camera Logic
REGISTER_COMPONENT(CameraLogicComponent)
REGISTER_PROPERTY(CameraLogicComponent, MaxZoom)
REGISTER_PROPERTY(CameraLogicComponent, MinZoom)
REGISTER_PROPERTY(CameraLogicComponent, MoveSpeed)


CameraLogicComponent::~CameraLogicComponent()
{
	//GetEventDispatcher().RemoveListener(EventType::MouseWheelUp, this);
	//GetEventDispatcher().RemoveListener(EventType::MouseWheelDown, this);
}

// Player 등록 필요. 
// Player의 GetWorldPos로 월드 기준 좌표를 Look으로 설정
void CameraLogicComponent::Start()
{
	//GetEventDispatcher().AddListener(EventType::MouseWheelUp, this);
	//GetEventDispatcher().AddListener(EventType::MouseWheelDown, this);

	auto* owner = GetOwner();
	auto*  trans = owner->GetComponent<TransformComponent>();
	auto* cam = owner->GetComponent<CameraComponent>();

	if (!trans || !cam) { return; }
	m_Transform = trans;
	m_Camera    = cam;
}

void CameraLogicComponent::Update(float deltaTime)
{

}

void CameraLogicComponent::OnEvent(EventType type, const void* data)
{
	// 맞게 변경
	/*if (io.MouseWheel != 0.0f)
	{
		const float zoomSpeed = 4.0f;
		const XMVECTOR dolly = XMVectorScale(forwardVec, io.MouseWheel * zoomSpeed);
		eyeVec = XMVectorAdd(eyeVec, dolly);
		lookVec = XMVectorAdd(lookVec, dolly);
		updated = true;
	}*/
}
