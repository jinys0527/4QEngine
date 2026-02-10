#include "VendingComponent.h"
#include "Object.h"
#include "Scene.h"
#include "GameObject.h"
#include "ReflectionMacro.h"
#include "NodeComponent.h"
#include "PlayerComponent.h"
#include "TransformComponent.h"

REGISTER_COMPONENT(VendingComponent)


VendingComponent::VendingComponent()
{
}

VendingComponent::~VendingComponent()
{
	GetEventDispatcher().RemoveListener(EventType::MouseLeftClick, this);
}

void VendingComponent::Start()
{
	GetEventDispatcher().AddListener(EventType::MouseLeftClick, this);
}

void VendingComponent::Update(float deltaTime)
{
}


void VendingComponent::OnEvent(EventType type, const void* data)
{

}
