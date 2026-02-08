#pragma once

#include "Component.h"
#include "ResourceHandle.h"
#include "ResourceRefs.h"

class PlayerVisualPresetComponent : public Component
{
	friend class Editor;

public:
	static constexpr const char* StaticTypeName = "PlayerVisualPresetComponent";
	const char* GetTypeName() const override;

	PlayerVisualPresetComponent() = default;
	virtual ~PlayerVisualPresetComponent() = default;

	void Start() override;
	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	const bool& GetApplyOnStart() const { return m_ApplyOnStart; }
	void SetApplyOnStart(const bool& value) { m_ApplyOnStart = value; }

	const bool& GetApplyAnimationWithBlend() const { return m_ApplyAnimationWithBlend; }
	void SetApplyAnimationWithBlend(const bool& value) { m_ApplyAnimationWithBlend = value; }

	const std::string& GetCurrentStateTag() const { return m_CurrentStateTag; }
	void SetCurrentStateTag(const std::string& stateTag) { m_CurrentStateTag = stateTag; }

	const std::string& GetStateTag0() const { return m_StateTag0; }
	void SetStateTag0(const std::string& value) { m_StateTag0 = value; }
	const MeshHandle& GetMeshHandle0() const { return m_MeshHandle0; }
	void SetMeshHandle0(const MeshHandle& value);
	const MeshRef& GetMesh0() const { return m_Mesh0; }
	void SetMesh0(const MeshRef& value) { m_Mesh0 = value; }
	const SkeletonHandle& GetSkeletonHandle0() const { return m_SkeletonHandle0; }
	void SetSkeletonHandle0(const SkeletonHandle& value);
	const SkeletonRef& GetSkeleton0() const { return m_Skeleton0; }
	void SetSkeleton0(const SkeletonRef& value) { m_Skeleton0 = value; }
	const AnimationHandle& GetAnimationHandle0() const { return m_AnimationHandle0; }
	void SetAnimationHandle0(const AnimationHandle& value);
	const AnimationRef& GetAnimation0() const { return m_Animation0; }
	void SetAnimation0(const AnimationRef& value) { m_Animation0 = value; }
	const bool& GetUseAnimation0() const { return m_UseAnimation0; }
	void SetUseAnimation0(const bool& value) { m_UseAnimation0 = value; }
	const float& GetBlendTime0() const { return m_BlendTime0; }
	void SetBlendTime0(const float& value) { m_BlendTime0 = value; }

	const std::string& GetStateTag1() const { return m_StateTag1; }
	void SetStateTag1(const std::string& value) { m_StateTag1 = value; }
	const MeshHandle& GetMeshHandle1() const { return m_MeshHandle1; }
	void SetMeshHandle1(const MeshHandle& value);
	const MeshRef& GetMesh1() const { return m_Mesh1; }
	void SetMesh1(const MeshRef& value) { m_Mesh1 = value; }
	const SkeletonHandle& GetSkeletonHandle1() const { return m_SkeletonHandle1; }
	void SetSkeletonHandle1(const SkeletonHandle& value);
	const SkeletonRef& GetSkeleton1() const { return m_Skeleton1; }
	void SetSkeleton1(const SkeletonRef& value) { m_Skeleton1 = value; }
	const AnimationHandle& GetAnimationHandle1() const { return m_AnimationHandle1; }
	void SetAnimationHandle1(const AnimationHandle& value);
	const AnimationRef& GetAnimation1() const { return m_Animation1; }
	void SetAnimation1(const AnimationRef& value) { m_Animation1 = value; }
	const bool& GetUseAnimation1() const { return m_UseAnimation1; }
	void SetUseAnimation1(const bool& value) { m_UseAnimation1 = value; }
	const float& GetBlendTime1() const { return m_BlendTime1; }
	void SetBlendTime1(const float& value) { m_BlendTime1 = value; }

	const std::string& GetStateTag2() const { return m_StateTag2; }
	void SetStateTag2(const std::string& value) { m_StateTag2 = value; }
	const MeshHandle& GetMeshHandle2() const { return m_MeshHandle2; }
	void SetMeshHandle2(const MeshHandle& value);
	const MeshRef& GetMesh2() const { return m_Mesh2; }
	void SetMesh2(const MeshRef& value) { m_Mesh2 = value; }
	const SkeletonHandle& GetSkeletonHandle2() const { return m_SkeletonHandle2; }
	void SetSkeletonHandle2(const SkeletonHandle& value);
	const SkeletonRef& GetSkeleton2() const { return m_Skeleton2; }
	void SetSkeleton2(const SkeletonRef& value) { m_Skeleton2 = value; }
	const AnimationHandle& GetAnimationHandle2() const { return m_AnimationHandle2; }
	void SetAnimationHandle2(const AnimationHandle& value);
	const AnimationRef& GetAnimation2() const { return m_Animation2; }
	void SetAnimation2(const AnimationRef& value) { m_Animation2 = value; }
	const bool& GetUseAnimation2() const { return m_UseAnimation2; }
	void SetUseAnimation2(const bool& value) { m_UseAnimation2 = value; }
	const float& GetBlendTime2() const { return m_BlendTime2; }
	void SetBlendTime2(const float& value) { m_BlendTime2 = value; }

	bool ApplyByStateTag(const std::string& stateTag);
	bool ApplyCurrentState();

private:
	bool ApplySlot(int slotIndex);
	bool TryApplySlotByTag(const std::string& stateTag);

private:
	bool m_ApplyOnStart = true;
	bool m_ApplyAnimationWithBlend = true;
	std::string m_CurrentStateTag = "Default";

	std::string m_StateTag0 = "Default";
	MeshHandle m_MeshHandle0 = MeshHandle::Invalid();
	MeshRef m_Mesh0;
	SkeletonHandle m_SkeletonHandle0 = SkeletonHandle::Invalid();
	SkeletonRef m_Skeleton0;
	AnimationHandle m_AnimationHandle0 = AnimationHandle::Invalid();
	AnimationRef m_Animation0;
	bool m_UseAnimation0 = false;
	float m_BlendTime0 = 0.15f;

	std::string m_StateTag1;
	MeshHandle m_MeshHandle1 = MeshHandle::Invalid();
	MeshRef m_Mesh1;
	SkeletonHandle m_SkeletonHandle1 = SkeletonHandle::Invalid();
	SkeletonRef m_Skeleton1;
	AnimationHandle m_AnimationHandle1 = AnimationHandle::Invalid();
	AnimationRef m_Animation1;
	bool m_UseAnimation1 = false;
	float m_BlendTime1 = 0.15f;

	std::string m_StateTag2;
	MeshHandle m_MeshHandle2 = MeshHandle::Invalid();
	MeshRef m_Mesh2;
	SkeletonHandle m_SkeletonHandle2 = SkeletonHandle::Invalid();
	SkeletonRef m_Skeleton2;
	AnimationHandle m_AnimationHandle2 = AnimationHandle::Invalid();
	AnimationRef m_Animation2;
	bool m_UseAnimation2 = false;
	float m_BlendTime2 = 0.15f;
};
