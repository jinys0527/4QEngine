#include "AnimationComponent.h"

#include <algorithm>
#include <cmath>

#include "MathHelper.h"
#include "Object.h"
#include "SkeletalMeshComponent.h"
#include "ReflectionMacro.h"

REGISTER_COMPONENT(AnimationComponent);

float ClampTimeToClip(float timeSec, const RenderData::AnimationClip* clip)
{
	if (!clip || clip->duration <= 0.0f)
	{
		return timeSec;
	}

	if (timeSec < 0.0f)
	{
		return 0.0f;
	}

	if (timeSec > clip->duration)
	{
		return clip->duration;
	}

	return timeSec;
}

float UpdatePlaybackTime(
							float timeSec, 
							float deltaTime,
							const RenderData::AnimationClip* clip, 
							bool  looping, 
							bool* stopped
						)
{
	float nextTime = timeSec + deltaTime;
	if (!clip || clip->duration <= 0.0f)
	{
		return nextTime;
	}

	if (looping)
	{
		nextTime = fmod(nextTime, clip->duration);
		if (nextTime < 0.0f)
		{
			nextTime += clip->duration;
		}
	}
	else
	{
		if (nextTime >= clip->duration)
		{
			nextTime = clip->duration;
			if (stopped)
			{
				*stopped = true;
			}
		}
		else if (nextTime < 0.0f)
		{
			nextTime = 0.0f;
		}
	}

	return nextTime;
}

void AnimationComponent::Play()
{
	m_Playback.playing = true;
}

void AnimationComponent::Stop()
{
	m_Playback.time    = 0.0f;
	m_Playback.playing = false;
}

void AnimationComponent::Pause()
{
	m_Playback.playing = false;
}

void AnimationComponent::Resume()
{
	m_Playback.playing = true;
}

void AnimationComponent::SeekTime(float timeSec)
{
	const RenderData::AnimationClip* clip = ResolveClip();
	m_Playback.time = ClampTimeToClip(timeSec, clip);
}

void AnimationComponent::SeekNormalized(float normalizedTime)
{
	const RenderData::AnimationClip* clip = ResolveClip();
	if (!clip || clip->duration <= 0.0f)
	{
		m_Playback.time = 0.0f;
		return;
	}
	const float clamped    = ClampTimeToClip(normalizedTime, clip);
	const float targetTime = clamped * clip->duration;
	m_Playback.time        = ClampTimeToClip(targetTime, clip);
}

float AnimationComponent::GetNormalizedTime() const
{
	const RenderData::AnimationClip* clip = ResolveClip();
	if (!clip || clip->duration <= 0.0f)
	{
		return 0.0f;
	}
	return m_Playback.time / clip->duration;
}

void AnimationComponent::StartBlend(AnimationHandle toClip, float blendTime)
{
	const RenderData::AnimationClip* fromClip = ResolveClip(m_ClipHandle);
	const RenderData::AnimationClip* nextClip = ResolveClip(toClip);
	if (!nextClip)
	{
		return;
	}

	if (!fromClip || blendTime <= 0.0f)
	{
		m_Blend.active     = false;
		m_ClipHandle       = toClip;
		m_Playback.time    = 0.0f;
		m_Playback.playing = true;
		return;
	}

	m_Blend.active     = true;
	m_Blend.fromClip   = m_ClipHandle;
	m_Blend.toClip     = toClip;
	m_Blend.duration   = max(blendTime, 0.0001f);
	m_Blend.elapsed    = 0.0f;
	m_Blend.fromTime   = m_Playback.time;
	m_Blend.toTime     = 0.0f;

	//전이 시작 시 타겟 클립 설정
	m_ClipHandle       = toClip;
	m_Playback.playing = true;
}

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
	if (!skeleton || skeleton->bones.empty())
		return;

	const RenderData::AnimationClip* clip = ResolveClip();
	
	if (m_Blend.active)
	{
		const RenderData::AnimationClip* fromClip = ResolveClip(m_Blend.fromClip);
		const RenderData::AnimationClip* toClip   = ResolveClip(m_Blend.toClip);
		if (!fromClip || !toClip)
		{
			m_Blend.active = false;
		}
		else
		{
			const float scaledDelta = deltaTime * m_Playback.speed;
			m_Blend.fromTime = UpdatePlaybackTime(m_Blend.fromTime, scaledDelta, fromClip, m_Playback.looping, nullptr);
			m_Blend.toTime   = UpdatePlaybackTime(m_Blend.toTime, scaledDelta, toClip, m_Playback.looping, nullptr);
			m_Blend.elapsed += deltaTime;

			const float alpha = min(m_Blend.elapsed / m_Blend.duration, 1.0f);

			std::vector<LocalPose> fromPoses;
			std::vector<LocalPose> toPoses;
			std::vector<LocalPose> blendedPoses;
			SampleLocalPoses  (*skeleton, *fromClip, m_Blend.fromTime, fromPoses);
			SampleLocalPoses  (*skeleton, *toClip, m_Blend.toTime, toPoses);
			BlendLocalPoses   (fromPoses, toPoses, alpha, blendedPoses);
			BuildPoseFromLocal(*skeleton, blendedPoses);

			m_Playback.time = m_Blend.toTime;

			if (alpha >= 1.0f)
			{
				m_Blend.active = false;
			}
		}
	}
	else
	{
		const RenderData::AnimationClip* clip = ResolveClip();
		if (!clip)
			return;

		bool stopped = false;
		const float scaledDelta = deltaTime * m_Playback.speed;
		const float nextTime    = UpdatePlaybackTime(m_Playback.time, scaledDelta, clip, m_Playback.looping, &stopped);

		m_Playback.time = nextTime;
		if (stopped)
		{
			m_Playback.playing = false;
		}

		BuildPose(*skeleton, *clip, m_Playback.time);
	}

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

const RenderData::AnimationClip* AnimationComponent::ResolveClip(AnimationHandle handle) const
{
	if (!handle.IsValid() || !m_Animations)
	{
		return nullptr;
	}

	return m_Animations->Get(handle);
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

void AnimationComponent::SampleLocalPoses(const RenderData::Skeleton& skeleton, const RenderData::AnimationClip& clip, float timeSec, std::vector<LocalPose>& localPoses) const
{
	const size_t boneCount = skeleton.bones.size();
	localPoses.assign(boneCount, LocalPose{});

	for (const auto& track : clip.tracks)
	{
		if (track.boneIndex < 0 || static_cast<size_t>(track.boneIndex) > boneCount)
			continue;

		localPoses[track.boneIndex] = SampleTrack(track, timeSec);
	}

}



void AnimationComponent::BuildPose(
									const RenderData::Skeleton& skeleton, 
									const RenderData::AnimationClip& clip, 
									float timeSec
								  )
{
	std::vector<LocalPose> localPoses;
	SampleLocalPoses(skeleton, clip, timeSec, localPoses);
	BuildPoseFromLocal(skeleton, localPoses);
}

void AnimationComponent::BuildPoseFromLocal(
											   const RenderData::Skeleton & skeleton,
											   const std::vector<LocalPose>&localPoses
										   )
{
	const size_t boneCount = skeleton.bones.size();
	m_LocalPose.resize(boneCount);
	m_GlobalPose.resize(boneCount);
	m_SkinningPalette.resize(boneCount);

	const size_t poseCount = localPoses.size();

	for (size_t i = 0; i < boneCount; ++i)
	{
		const LocalPose& pose = i < poseCount ? localPoses[i] : LocalPose{};
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

void AnimationComponent::BlendLocalPoses(
											const std::vector<LocalPose>& fromPoses,
											const std::vector<LocalPose>& toPoses,
											float alpha,
											std::vector<LocalPose>& blended
										) const
{
	const size_t boneCount = max(fromPoses.size(), toPoses.size());
	blended.resize(boneCount);
	const float clampedAlpha = clamp(alpha, 0.0f, 1.0f);

	for (size_t i = 0; i < boneCount; ++i)
	{
		const LocalPose& fromPose = i < fromPoses.size() ? fromPoses[i] : LocalPose{};
		const LocalPose& toPose   = i < toPoses.size()   ? toPoses[i]   : LocalPose{};

		blended[i].translation = MathUtils::Lerp3(fromPose.translation, toPose.translation, clampedAlpha);
		blended[i].scale	   = MathUtils::Lerp3(fromPose.scale, toPose.scale, clampedAlpha);
		blended[i].rotation    = MathUtils::Slerp4(fromPose.rotation, toPose.rotation, clampedAlpha);
		const auto q = XMQuaternionNormalize(XMLoadFloat4(&blended[i].rotation));
		XMStoreFloat4(&blended[i].rotation, q);
	}
}