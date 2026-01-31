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
		
			const int cost = true; // player->GetCurrentWeaponCost(); Player의 현재 무기 cost 받아오는 함수 연결
			const bool consumed = player->ConsumeActResource(cost);
			DispatchEvent(consumed ? "Combat_CostOk" : "Combat_CostFail");
		});

	BindActionHandler("Combat_RangeCheck", [this](const FSMAction& action)
		{
			auto* owner = GetOwner();
			auto* player = owner ? owner->GetComponent<PlayerComponent>() : nullptr;
			if (!player)
			{
				DispatchEvent("Combat_RangeFail");
				return;
			}
			const bool inRange = true; // Player가 때릴 수 있는 곳에 적이 있는지 받아오는 함수 연결
			DispatchEvent(inRange ? "Combat_RangeOk" : "Combat_RangeFail");
		});

	BindActionHandler("Combat_Confirm", [this](const FSMAction& action)
		{
			auto* owner = GetOwner();
			auto* player = owner ? owner->GetComponent<PlayerComponent>() : nullptr;
			if (!player)
			{
				DispatchEvent("Combat_Cancel");
				return;
			}
			const bool confirmed = true; // Player가 2번 클릭했는지 여부 확인 함수
			DispatchEvent(confirmed ? "Combat_Confirm" : "Combat_Cancel");
		});

	BindActionHandler("Combat_Attack", [this](const FSMAction& action)
		{
			DispatchEvent("Combat_StartTurn");
			// CombatManager랑 연동처리
			// 공격 처리
			// 데미지 계산
		});

	BindActionHandler("Combat_Enter", [this](const FSMAction& action)
		{
			// CombatManager랑 연동처리
			// 스탯 결정
			//GetEventDispatcher().Dispatch(EventType::CombatEnter, );
		});

	BindActionHandler("Combat_Result", [this](const FSMAction& action)
		{
			// 드롭 / 사망 처리 후 결과
			// Combat Manager에서 결과 받아오기
			DispatchEvent("Combat_TurnResolved");
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