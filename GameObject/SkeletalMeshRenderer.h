#pragma once

#include "MeshRenderer.h"
#include "RenderData.h"

class TransformComponent;

class SkeletalMeshRenderer : public MeshRenderer
{
	friend class Editor;

public:
	static constexpr const char* StaticTypeName = "SkeletalMeshRenderer";
	const char* GetTypeName() const override;
	
	SkeletalMeshRenderer() = default;
	virtual ~SkeletalMeshRenderer() = default;

	void           SetSkeletonHandle(SkeletonHandle handle) { m_SkeletonHandle = handle; }
	SkeletonHandle GetSkeletonHandle() const                { return m_SkeletonHandle;   }

	bool BuildRenderItem(RenderData::RenderItem& out) const override;

	void Update (float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

private:
	void ResolveHandles		  (MeshHandle& mesh, MaterialHandle& material, SkeletonHandle& skeleton) const;

protected:
	SkeletonHandle m_SkeletonHandle  = SkeletonHandle::Invalid();
};

