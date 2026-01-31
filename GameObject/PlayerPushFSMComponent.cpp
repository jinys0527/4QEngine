#include "PlayerPushFSMComponent.h"
#include "PlayerComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"

REGISTER_COMPONENT_DERIVED(PlayerPushFSMComponent, FSMComponent)

#define PushCost 1

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
			const bool canPush = true; // player 기준 사정거리면서 밀 수 있는지(벽이나 난간) 판단하는 함수 연결
			DispatchEvent(canPush ? "Push_Possible" : "Push_Revoke");
		});

	BindActionHandler("Push_SearchTarget", [this](const FSMAction& action)
		{
			const bool hasTarget = true; // 밀 수 있는 장소에 적이 있는 지 판단하는 함수
			DispatchEvent(hasTarget ? "Push_TargetFound" : "Push_TargetNone");
		});

	BindActionHandler("Push_SelectTarget", [this](const FSMAction&)
		{
			DispatchEvent("Push_TargetSelected");
		});

	BindActionHandler("Push_Resolve", [this](const FSMAction& action)
		{
			const bool success = true; // 성공 여부 주사위 판정 함수
			DispatchEvent(success ? "Push_Success" : "Push_Fail");
		});
}


void PlayerPushFSMComponent::Start()
{
	FSMComponent::Start();
}