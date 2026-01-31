#include "PlayerCombatFSMComponent.h"
#include "CombatEvents.h"
#include "PlayerComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"

REGISTER_COMPONENT_DERIVED(PlayerCombatFSMComponent, FSMComponent)

PlayerCombatFSMComponent::PlayerCombatFSMComponent()
{
	BindActionHandler("Combat_ConsumeActResource", [this](const FSMAction& action)
		{
			auto* owner = GetOwner();
			auto* player = owner ? owner->GetComponent<PlayerComponent>() : nullptr;
			if (!player)
			{
				DispatchEvent("Combat_CostFail");
				return;
			}
			// 플레이어 무기 행동 포인트로 변경 예정
			const int amount = action.params.value("amount", 0);
			const bool consumed = player->ConsumeActResource(amount);
			DispatchEvent(consumed ? "Combat_CostOk" : "Combat_CostFail");
		});

	BindActionHandler("Combat_RangeCheck", [this](const FSMAction& action)
		{
			const bool inRange = action.params.value("inRange", true);
			DispatchEvent(inRange ? "Combat_RangeOk" : "Combat_RangeFail");
		});

	BindActionHandler("Combat_Confirm", [this](const FSMAction& action)
		{
			const bool confirmed = action.params.value("confirmed", true);
			DispatchEvent(confirmed ? "Combat_Confirm" : "Combat_Cancel");
		});

	BindActionHandler("Combat_Attack", [this](const FSMAction& action)
		{
			const bool resolved = action.params.value("resolved", true);
			if (resolved)
			{
				DispatchEvent("Combat_StartTurn");
			}
			// 공격 처리
			// 데미지 계산
		});

	BindActionHandler("Combat_Result", [this](const FSMAction& action)
		{
			// 드롭 / 사망 처리 후 결과
			const bool resolved = action.params.value("resolved", true);
			if (resolved)
			{
				DispatchEvent("Combat_TurnResolved");
			}
		});

	BindActionHandler("Combat_Enter", [this](const FSMAction& action)
		{
			const int initiatorId = action.params.value("initiatorId", 0);
			const int targetId = action.params.value("targetId", 0);
			const CombatEnterEvent eventData{ initiatorId, targetId };
			// 스탯 결정
			GetEventDispatcher().Dispatch(EventType::CombatEnter, &eventData);
		});

	BindActionHandler("Combat_Exit", [this](const FSMAction&)
		{
			const CombatExitEvent eventData;
			GetEventDispatcher().Dispatch(EventType::CombatExit, &eventData);
		});
}

void PlayerCombatFSMComponent::Start()
{
	FSMComponent::Start();
}