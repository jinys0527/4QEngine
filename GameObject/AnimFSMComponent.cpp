#include "AnimFSMComponent.h"
#include "AnimBlendCurveFunction.h"
#include "AnimationComponent.h"
#include "FSMActionRegistry.h"
#include "FSMEventRegistry.h"
#include "Object.h"
#include "ReflectionMacro.h"

AnimationComponent::BlendCurveFn ResolveBlendCurve(const std::string& name)
{
	if (name == "EaseInSine") return BlendCurveFunc::EaseInSine;
	if (name == "EaseOutSine") return BlendCurveFunc::EaseOutSine;
	if (name == "EaseInOutSine") return BlendCurveFunc::EaseInOutSine;
	if (name == "EaseInQuad") return BlendCurveFunc::EaseInQuad;
	if (name == "EaseOutQuad") return BlendCurveFunc::EaseOutQuad;
	if (name == "EaseInOutQuad") return BlendCurveFunc::EaseInOutQuad;
	if (name == "EaseInCubic") return BlendCurveFunc::EaseInCubic;
	if (name == "EaseOutCubic") return BlendCurveFunc::EaseOutCubic;
	if (name == "EaseInOutCubic") return BlendCurveFunc::EaseInOutCubic;
	if (name == "EaseInQuart") return BlendCurveFunc::EaseInQuart;
	if (name == "EaseOutQuart") return BlendCurveFunc::EaseOutQuart;
	if (name == "EaseInOutQuart") return BlendCurveFunc::EaseInOutQuart;
	if (name == "EaseInQuint") return BlendCurveFunc::EaseInQuint;
	if (name == "EaseOutQuint") return BlendCurveFunc::EaseOutQuint;
	if (name == "EaseInOutQuint") return BlendCurveFunc::EaseInOutQuint;
	if (name == "EaseInExpo") return BlendCurveFunc::EaseInExpo;
	if (name == "EaseOutExpo") return BlendCurveFunc::EaseOutExpo;
	if (name == "EaseInOutExpo") return BlendCurveFunc::EaseInOutExpo;
	if (name == "EaseInCirc") return BlendCurveFunc::EaseInCirc;
	if (name == "EaseOutCirc") return BlendCurveFunc::EaseOutCirc;
	if (name == "EaseInOutCirc") return BlendCurveFunc::EaseInOutCirc;
	if (name == "EaseInBack") return BlendCurveFunc::EaseInBack;
	if (name == "EaseOutBack") return BlendCurveFunc::EaseOutBack;
	if (name == "EaseInOutBack") return BlendCurveFunc::EaseInOutBack;
	if (name == "EaseInElastic") return BlendCurveFunc::EaseInElastic;
	if (name == "EaseOutElastic") return BlendCurveFunc::EaseOutElastic;
	if (name == "EaseInElastic") return BlendCurveFunc::EaseInOutElastic;
	if (name == "EaseInBounce") return BlendCurveFunc::EaseInBounce;
	if (name == "EaseOutBounce") return BlendCurveFunc::EaseOutBounce;
	if (name == "EaseInOutBounce") return BlendCurveFunc::EaseInOutBounce;
	
	
	return nullptr;
}

void RegisterAnimFSMDefinitions()
{
	auto& actionRegistry = FSMActionRegistry::Instance();
	actionRegistry.RegisterAction({
		"Anim_Play",
		"Animation",
		{}
		});
	actionRegistry.RegisterAction({
		"Anim_Stop",
		"Animation",
		{}
		});
	actionRegistry.RegisterAction({
		"Anim_SetSpeed",
		"Animation",
		{
			{ "value", "float", 1.0f, false }
		}
		});

	actionRegistry.RegisterAction({
		"Anim_BlendTo",
		"Animation",
		{
			{ "assetPath", "string", "", true },
			{ "assetIndex", "int", 0, false },
			{ "blendTime", "float", 0.2f, false },
			{ "curve", "string", "Linear", false }
		}
		});

	auto& eventRegistry = FSMEventRegistry::Instance();
	eventRegistry.RegisterEvent({ "AnimNotify_Hit", "Animation" });
	eventRegistry.RegisterEvent({ "AnimNotify_Footstep", "Animation" });
}

REGISTER_COMPONENT_DERIVED(AnimFSMComponent, FSMComponent)

AnimFSMComponent::AnimFSMComponent()
{
	BindActionHandler("Anim_Play", [this](const FSMAction&)
		{
			auto* owner = GetOwner();
			auto anims = owner ? owner->GetComponentsDerived<AnimationComponent>() : std::vector<AnimationComponent*>{};
			auto* anim = anims.empty() ? nullptr : anims.front();
			if (anim)
			{
				anim->Play();
			}
		});

	BindActionHandler("Anim_Stop", [this](const FSMAction&)
		{
			auto* owner = GetOwner();
			auto anims = owner ? owner->GetComponentsDerived<AnimationComponent>() : std::vector<AnimationComponent*>{};
			auto* anim = anims.empty() ? nullptr : anims.front();
			if (anim)
			{
				anim->Stop();
			}
		});

	BindActionHandler("Anim_SetSpeed", [this](const FSMAction& action)
		{
			auto* owner = GetOwner();
			auto anims = owner ? owner->GetComponentsDerived<AnimationComponent>() : std::vector<AnimationComponent*>{};
			auto* anim = anims.empty() ? nullptr : anims.front();
			if (!anim)
				return;

			auto playback = anim->GetPlayback();
			playback.speed = action.params.value("value", playback.speed);
			anim->SetPlayback(playback);
		});

	BindActionHandler("Anim_BlendTo", [this](const FSMAction& action)
		{
			auto* owner = GetOwner();
			auto anims = owner ? owner->GetComponentsDerived<AnimationComponent>() : std::vector<AnimationComponent*>{};
			auto* anim = anims.empty() ? nullptr : anims.front();
			if (!anim)
				return;

			const int handleId = action.params.value("handleId", 0);
			const int handleGen = action.params.value("handleGen", 0);
			const std::string assetPath = action.params.value("assetPath", std::string{});
			const int assetIndex = action.params.value("assetIndex", 0);
			const float blendTime = action.params.value("blendTime", 0.2f);
			const std::string curveName = action.params.value("curve", std::string("Linear"));
			const bool useBlendConfig = action.params.value("useBlendConfig", false);

			AnimationHandle handle = AnimationHandle::Invalid();
			float resolvedBlendTime = blendTime;
			std::string resolvedCurve = curveName;
			AnimationComponent::BlendType resolvedType = AnimationComponent::BlendType::Linear;

			if (useBlendConfig)
			{
				const auto& config = anim->GetBlendConfig();
				handle = config.toClip;
				resolvedBlendTime = config.blendTime;
				resolvedCurve = config.curveName;
				resolvedType = config.blendType;
			}
			else if (handleId > 0)
			{
				handle.id = static_cast<UINT32>(handleId);
				handle.generation = static_cast<UINT32>(handleGen);
			}
			else if (!assetPath.empty())
			{
				auto* loader = AssetLoader::GetActive();
				if (!loader)
					return;

				handle = loader->ResolveAnimation(assetPath, static_cast<UINT32>(assetIndex));
			}

			if (!handle.IsValid())
				return;

			if (resolvedCurve == "Linear" && resolvedType != AnimationComponent::BlendType::Curve)
			{
				anim->StartBlend(handle, resolvedBlendTime);
				return;
			}

			if (auto curveFn = ResolveBlendCurve(resolvedCurve))
			{
				anim->StartBlend(handle, resolvedBlendTime, AnimationComponent::BlendType::Curve, curveFn);
				return;
			}

			anim->StartBlend(handle, resolvedBlendTime);
		});
}

void AnimFSMComponent::Notify(const std::string& notifyName)
{
	DispatchEvent("animNotify_" + notifyName);
}
