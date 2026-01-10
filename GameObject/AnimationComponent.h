#pragma once

#include "Component.h"
#include "RenderData.h"
#include "ResourceHandle.h"
#include "ResourceStore.h"

class SkeletalMeshComponent;

class AnimationComponent : public Component
{
	friend class Editor;

public:
	static constexpr const char* StaticTypeName = "AnimationComponent";
	const char* GetTypeName() const override;

	struct PlaybackState
	{
		float time    = 0.0f;
		float speed   = 1.0f;
		bool  looping = true;
		bool  playing = true;
	};

	AnimationComponent() = default;
	virtual ~AnimationComponent() = default;

	void SetClipHandle(AnimationHandle handle) { m_ClipHandle = handle; }
	AnimationHandle GetClipHandle()			   { return m_ClipHandle;   }

	void SetAnimationAssetReference(const std::string& assetPath, UINT32 clipIndex)
	{
		m_AnimationAssetPath = assetPath;
		m_AnimationClipIndex = clipIndex;
	}

	const std::string& GetAnimationAssetPath() const { return m_AnimationAssetPath; }
	UINT32 GetAnimationClipIndex            () const { return m_AnimationClipIndex; }

	void SetResourceStores(ResourceStore<RenderData::Skeleton, SkeletonHandle>* skeletons,
						   ResourceStore<RenderData::AnimationClip, AnimationHandle>* animations)
	{
		m_Skeletons = skeletons;
		m_Animations = animations;
	}

	PlaybackState& GetPlaybackState      ()       { return m_Playback; }
	const PlaybackState& GetPlaybackState() const { return m_Playback; }

	const std::vector<DirectX::XMFLOAT4X4>& GetSkinningPalette() const { return m_SkinningPalette; }


	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void Serialize(nlohmann::json& j) const override;
	void Deserialize(const nlohmann::json& j) override;
private:
	struct LocalPose
	{
		DirectX::XMFLOAT3 translation{ 0.0f, 0.0f, 0.0f };
		DirectX::XMFLOAT4 rotation   { 0.0f, 0.0f, 0.0f, 1.0f };
		DirectX::XMFLOAT3 scale		 { 1.0f, 1.0f, 1.0f };
	};

	static LocalPose ToLocalPose(const RenderData::AnimationKeyFrame& key);
	const RenderData::AnimationClip* ResolveClip    () const;
	const RenderData::Skeleton*      ResolveSkeleton(SkeletonHandle handle) const;
	static LocalPose SampleTrack(const RenderData::AnimationTrack& track, float timeSec);
	void             BuildPose
							 (
								 const RenderData::Skeleton& skeleton,
								 const RenderData::AnimationClip& clip,
								 float timeSec
							 );

	ResourceStore<RenderData::Skeleton, SkeletonHandle>*       m_Skeletons  = nullptr;
	ResourceStore<RenderData::AnimationClip, AnimationHandle>* m_Animations = nullptr;

	AnimationHandle m_ClipHandle = AnimationHandle::Invalid();
	std::string		m_AnimationAssetPath;
	UINT32			m_AnimationClipIndex = 0;

	PlaybackState m_Playback{};

	std::vector<DirectX::XMFLOAT4X4> m_LocalPose;
	std::vector<DirectX::XMFLOAT4X4> m_GlobalPose;
	std::vector<DirectX::XMFLOAT4X4> m_SkinningPalette;
};

