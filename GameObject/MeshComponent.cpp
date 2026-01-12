#include "MeshComponent.h"
#include "ReflectionMacro.h"
REGISTER_COMPONENT(MeshComponent);
REGISTER_PROPERTY_HANDLE(MeshComponent, MeshHandle)
REGISTER_PROPERTY_READONLY_LOADABLE(MeshComponent, Mesh)

void MeshComponent::Update(float deltaTime)
{
}

void MeshComponent::OnEvent(EventType type, const void* data)
{
}