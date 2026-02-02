
#include "CameraLogicComponent.h"
#include "CameraComponent.h"
#include "TransformComponent.h"
#include "ReflectionMacro.h"
#include "Scene.h"

REGISTER_COMPONENT(CameraLogicComponent)
REGISTER_PROPERTY(CameraLogicComponent, MaxZoom)
REGISTER_PROPERTY(CameraLogicComponent, MinZoom)
REGISTER_PROPERTY(CameraLogicComponent, MoveSpeed)

void CameraLogicComponent::Start()
{

}

void CameraLogicComponent::Update(float deltaTime)
{

}

void CameraLogicComponent::OnEvent(EventType type, const void* data)
{

}
