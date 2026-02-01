#include "PlayerPushFSMComponent.h"
#include "PlayerComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"

REGISTER_COMPONENT_DERIVED(PlayerPushFSMComponent, FSMComponent)

namespace
{
	constexpr int PushCost = 1;
}

PlayerPushFSMComponent::PlayerPushFSMComponent()
{
	BindActionHandler("Push_ConsumeActResource", [this](const FSMAction& action)
		{
			auto* owner = GetOwner();
			auto* player = owner ? owner->GetComponent<PlayerComponent>() : nullptr;
			if (!player)
			{
				return;
			}

			const bool consumed = player->ConsumeActResource(PushCost);
			if (!consumed)
			{
				DispatchEvent("Push_Revoke");
			}
		});

	BindActionHandler("Push_CheckPossible", [this](const FSMAction& action)
		{
			auto* owner = GetOwner();
			auto* player = owner ? owner->GetComponent<PlayerComponent>() : nullptr;
			const bool canPush = player ? player->ConsumePushPossible() : false;
			DispatchEvent(canPush ? "Push_Possible" : "Push_Revoke");
		});

	BindActionHandler("Push_SearchTarget", [this](const FSMAction& action)
		{
			auto* owner = GetOwner();
			auto* player = owner ? owner->GetComponent<PlayerComponent>() : nullptr;
			const bool hasTarget = player ? player->ConsumePushTargetFound() : false;
			DispatchEvent(hasTarget ? "Push_TargetFound" : "Push_TargetNone");
		});

	BindActionHandler("Push_SelectTarget", [this](const FSMAction&)
		{
			DispatchEvent("Push_TargetSelected");
		});

	BindActionHandler("Push_Resolve", [this](const FSMAction& action)
		{
			auto* owner = GetOwner();
			auto* player = owner ? owner->GetComponent<PlayerComponent>() : nullptr;
			const bool success = player ? player->ConsumePushSuccess() : false;
			DispatchEvent(success ? "Push_Success" : "Push_Fail");
		});
}


void PlayerPushFSMComponent::Start()
{
	FSMComponent::Start();
}