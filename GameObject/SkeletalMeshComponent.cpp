#include "SkeletalMeshComponent.h"
#include "ReflectionMacro.h"

REGISTER_COMPONENT_DERIVED(SkeletalMeshComponent, MeshComponent)
REGISTER_PROPERTY_HANDLE(SkeletalMeshComponent, SkeletonHandle)
REGISTER_PROPERTY_READONLY_LOADABLE(SkeletalMeshComponent, Skeleton)
REGISTER_PROPERTY_READONLY_LOADABLE(SkeletalMeshComponent, SkinningPalette)

void SkeletalMeshComponent::Update(float deltaTime)
{
}

void SkeletalMeshComponent::OnEvent(EventType type, const void* data)
{
}

