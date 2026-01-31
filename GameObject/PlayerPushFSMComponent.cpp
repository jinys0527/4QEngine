#include "PlayerPushFSMComponent.h"
#include "PlayerComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"

REGISTER_COMPONENT_DERIVED(PlayerPushFSMComponent, FSMComponent)

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

			const int amount = action.params.value("amount", 0);
			const bool consumed = player->ConsumeActResource(amount);
			if (!consumed)
			{
				DispatchEvent("Push_Revoke");
			}
		});

	BindActionHandler("Push_CheckPossible", [this](const FSMAction& action)
		{
			const bool canPush = action.params.value("canPush", true);
			DispatchEvent(canPush ? "Push_Possible" : "Push_Revoke");
		});

	BindActionHandler("Push_SearchTarget", [this](const FSMAction& action)
		{
			const bool hasTarget = action.params.value("hasTarget", false);
			DispatchEvent(hasTarget ? "Push_TargetFound" : "Push_TargetNone");
		});

	BindActionHandler("Push_SelectTarget", [this](const FSMAction&)
		{
			DispatchEvent("Push_TargetSelected");
		});

	BindActionHandler("Push_Resolve", [this](const FSMAction& action)
		{
			const bool success = action.params.value("success", false);
			DispatchEvent(success ? "Push_Success" : "Push_Fail");
		});
}


void PlayerPushFSMComponent::Start()
{
	FSMComponent::Start();
}