#include "AnimationComponent.h"

#include <algorithm>
#include <cmath>

#include "MathHelper.h"
#include "Object.h"
#include "SkeletalMeshComponent.h"
#include "ReflectionMacro.h"

REGISTER_COMPONENT(AnimationComponent);


void AnimationComponent::Update(float deltaTime)
{
	if (!m_Playback.playing)
		return;

	Object* owner = GetOwner();
	if (!owner)
		return;

	auto* skeletal = owner->GetComponent<SkeletalMeshComponent>();
	if (!skeletal)
		return;

	const RenderData::Skeleton* skeleton  = ResolveSkeleton(skeletal->GetSkeletonHandle());
	const RenderData::AnimationClip* clip = ResolveClip();
	if (!skeleton || !clip || skeleton->bones.empty())
		return;

	float nextTime = m_Playback.time + deltaTime * m_Playback.speed;
	if (clip->duration > 0.0f)
	{
		if (m_Playback.looping)						 
		{
			nextTime = fmod(nextTime, clip->duration);  // loop
			if (nextTime < 0.0f)
			{
				nextTime += clip->duration;
			}
		}
		else
		{
			if (nextTime >= clip->duration)				// end
			{
				nextTime = clip->duration;
				m_Playback.playing = false;
			}
			else if (nextTime < 0.0f)
			{
				nextTime = 0.0f;
			}
		}
	}

	m_Playback.time = nextTime;

	BuildPose(*skeleton, *clip, m_Playback.time);
	skeletal->SetSkinningPalette(m_SkinningPalette);
}

void AnimationComponent::OnEvent(EventType type, const void* data)
{
}

void AnimationComponent::Serialize(nlohmann::json& j) const
{
	if (!m_AnimationAssetPath.empty())
	{
		j["animation"]["assetPath"] = m_AnimationAssetPath;
		j["animation"]["clipIndex"] = m_AnimationClipIndex;
	}

	j["animation"]["time"]    = m_Playback.time;
	j["animation"]["speed"]   = m_Playback.speed;
	j["animation"]["looping"] = m_Playback.looping;
	j["animation"]["playing"] = m_Playback.playing;
}

void AnimationComponent::Deserialize(const nlohmann::json& j)
{
	if (j.contains("animation"))
	{
		const auto& anim     = j.at("animation");
		m_AnimationAssetPath = anim.value("assetPath", std::string{});
		m_AnimationClipIndex = anim.value("clipIndex", 0u);
		m_Playback.time      = anim.value("time", 0.0f);
		m_Playback.speed     = anim.value("speed", 1.0f);
		m_Playback.looping   = anim.value("looping", true);
		m_Playback.playing   = anim.value("playing", true);
	}
}

AnimationComponent::LocalPose AnimationComponent::ToLocalPose(const RenderData::AnimationKeyFrame& key)
{
	AnimationComponent::LocalPose pose{};
	pose.translation = key.translation;
	pose.rotation    = key.rotation;
	pose.scale       = key.scale;
	return pose;
}

const RenderData::AnimationClip* AnimationComponent::ResolveClip() const
{
	if (!m_ClipHandle.IsValid() || !m_Animations)
	{
		return nullptr;
	}

	return m_Animations->Get(m_ClipHandle);
}

const RenderData::Skeleton* AnimationComponent::ResolveSkeleton(SkeletonHandle handle) const
{
	if (!handle.IsValid() || !m_Skeletons)
	{
		return nullptr;
	}

	return m_Skeletons->Get(handle);
}

AnimationComponent::LocalPose AnimationComponent::SampleTrack(
																const RenderData::AnimationTrack& track, 
																float timeSec
															 )
{
	const auto& keys = track.keyFrames;
	if (keys.empty())
		return{};

	if (keys.size() == 1 || timeSec <= keys.front().time)
	{
		return ToLocalPose(keys.front());
	}

	if (timeSec >= keys.back().time)
	{
		return ToLocalPose(keys.back());
	}

	
	auto it = std::upper_bound(keys.begin(), keys.end(), timeSec,
		[](float value, const RenderData::AnimationKeyFrame& key) {
			return value < key.time;
		});			// key.time > timeSec 를 처음 만족하는 요소
					// 다음 키프레임

	if (it == keys.begin())
	{
		return ToLocalPose(*it);
	}

	const size_t curIndex = static_cast<size_t>(std::distance(keys.begin(), it));
	const size_t prevIndex    = curIndex - 1;
	const auto&  prevKey	  = keys[prevIndex];
	const auto&  curKey       = keys[curIndex];

	const float  term = curKey.time - prevKey.time;
	const float  t = term > 0.0f ? (timeSec - prevKey.time) / term : 0.0f;	// 보간 계수
									// 현재 진행 정도

	LocalPose pose{};
	pose.translation = MathUtils::Lerp3(prevKey.translation, curKey.translation, t);
	pose.scale	     = MathUtils::Lerp3(prevKey.scale, curKey.scale, t);
	pose.rotation    = MathUtils::Slerp4(prevKey.rotation, curKey.rotation, t);
	const auto q = DirectX::XMQuaternionNormalize(XMLoadFloat4(&pose.rotation));
	DirectX::XMStoreFloat4(&pose.rotation, q);

	return pose;
}



void AnimationComponent::BuildPose(
									const RenderData::Skeleton& skeleton, 
									const RenderData::AnimationClip& clip, 
									float timeSec
								  )
{
	const size_t boneCount = skeleton.bones.size();
	m_LocalPose.resize(boneCount);
	m_GlobalPose.resize(boneCount);
	m_SkinningPalette.resize(boneCount);

	std::vector<LocalPose> localPoses(boneCount);

	for (const auto& track : clip.tracks)
	{
		if (track.boneIndex < 0 || static_cast<size_t>(track.boneIndex) >= boneCount)
			continue;
		localPoses[track.boneIndex] = SampleTrack(track, timeSec);
	}

	for (size_t i = 0; i < boneCount; ++i)
	{
		const auto& pose = localPoses[i];
		const auto local = MathUtils::CreateTRS(pose.translation, pose.rotation, pose.scale);
		DirectX::XMStoreFloat4x4(&m_LocalPose[i], local);

		const int parentIndex = skeleton.bones[i].parentIndex;
		if (parentIndex >= 0)
		{
			const auto parent = DirectX::XMLoadFloat4x4(&m_GlobalPose[static_cast<size_t>(parentIndex)]);
			const auto global = DirectX::XMMatrixMultiply(local, parent);
			DirectX::XMStoreFloat4x4(&m_GlobalPose[i], global);
		}
		else
		{
			DirectX::XMStoreFloat4x4(&m_GlobalPose[i], local);
		}

		const auto global  = DirectX::XMLoadFloat4x4(&m_GlobalPose[i]);
		const auto invBind = DirectX::XMLoadFloat4x4(&skeleton.bones[i].inverseBindPose);
		const auto skin    = DirectX::XMMatrixMultiply(global, invBind);
		DirectX::XMStoreFloat4x4(&m_SkinningPalette[i], skin);
	}
}
