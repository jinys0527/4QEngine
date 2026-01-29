#include "StatComponent.h"
#include "ReflectionMacro.h"

REGISTER_COMPONENT(StatComponent)
REGISTER_PROPERTY(StatComponent, CurrentHP)
REGISTER_PROPERTY(StatComponent, Range)
REGISTER_PROPERTY(StatComponent, MoveDistance)

void StatComponent::Start()
{
}

void StatComponent::Update(float deltaTime)
{
}

void StatComponent::OnEvent(EventType type, const void* data)
{
}
