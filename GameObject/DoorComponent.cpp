#include "DoorComponent.h"
#include "ReflectionMacro.h"

REGISTER_COMPONENT(DoorComponent)

DoorComponent::DoorComponent()
{
}

DoorComponent::~DoorComponent()
{
	GetEventDispatcher().RemoveListener(EventType::MouseLeftClick, this);
}

void DoorComponent::Start()
{
	GetEventDispatcher().AddListener(EventType::MouseLeftClick, this);
}

void DoorComponent::Update(float deltaTime)
{
}

void DoorComponent::OnEvent(EventType type, const void* data)
{
	
}
