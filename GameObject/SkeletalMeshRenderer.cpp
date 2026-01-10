#include "SkeletalMeshRenderer.h"
#include "TransformComponent.h"
#include "SkeletalMeshComponent.h"
#include "MaterialComponent.h"
#include "Object.h"
#include "ReflectionMacro.h"
REGISTER_COMPONENT(SkeletalMeshRenderer)

bool SkeletalMeshRenderer::BuildRenderItem(RenderData::RenderItem& out) const
{
	if (!m_Visible)
		return false;

	MeshHandle     mesh     = MeshHandle::Invalid();
	MaterialHandle material = MaterialHandle::Invalid();
	SkeletonHandle skeleton = SkeletonHandle::Invalid();
	ResolveHandles(mesh, material, skeleton);

	if (!mesh.IsValid() || !material.IsValid() || !skeleton.IsValid())
		return false;

	TransformComponent* transform = GetTransform();
	if (!transform)
		return false;

	out.mesh     = mesh;
	out.material = material;
	out.skeleton = skeleton;
	out.world    = transform->GetWorldMatrix();
	out.sortKey  = BuildSortKey(mesh, material, m_RenderLayer);
	return true;
}

void SkeletalMeshRenderer::Update(float deltaTime)
{
}

void SkeletalMeshRenderer::OnEvent(EventType type, const void* data)
{
}

void SkeletalMeshRenderer::ResolveHandles(MeshHandle& mesh, MaterialHandle& material, SkeletonHandle& skeleton) const
{
	mesh = m_MeshHandle;
	material = m_MaterialHandle;
	skeleton = m_SkeletonHandle;

	const Object* owner = GetOwner();
	if (!owner)
		return;

	if (!mesh.IsValid())
	{
		if (const auto* skeletalMeshComp = owner->GetComponent<SkeletalMeshComponent>())
		{
			mesh = skeletalMeshComp->GetMeshHandle();
		}
	}

	if (!material.IsValid())
	{
		if (const auto* materialComp = owner->GetComponent<MaterialComponent>())
		{
			material = materialComp->GetMaterialHandle();
		}
	}

	if (!skeleton.IsValid())
	{
		if (const auto* skeletalMeshComp = owner->GetComponent<SkeletalMeshComponent>())
		{
			skeleton = skeletalMeshComp->GetSkeletonHandle();
		}
	}
}