#include "PlayerDoorFSMComponent.h"
#include "PlayerComponent.h"
#include "ReflectionMacro.h"
#include "PlayerFSMComponent.h"
#include "Object.h"

REGISTER_COMPONENT_DERIVED(PlayerDoorFSMComponent, FSMComponent)

#define DoorCost 1

PlayerDoorFSMComponent::PlayerDoorFSMComponent()
{
	BindActionHandler("Door_ConsumeActResource", [this](const FSMAction& action)
		{
			auto* owner = GetOwner();
			auto* player = owner ? owner->GetComponent<PlayerComponent>() : nullptr;
			if (!player)
			{
				DispatchEvent("Door_Revoke");
				return;
			}

			const bool consumed = player->ConsumeActResource(DoorCost);
			DispatchEvent(consumed ? "Door_CostPaid" : "Door_Revoke");
		});

	BindActionHandler("Door_Attempt", [this](const FSMAction& action)
		{
			// 난이도 표시 UI
			// 주사위
			const bool confirmed = true; // UI에서 확인인지 우클릭인지 이벤트 결과 받아오는 함수?
			DispatchEvent(confirmed ? "Door_Confirm" : "Door_Revoke");
		});
	BindActionHandler("Door_Select", [this](const FSMAction& action)
		{
			// 안내 UI
			const bool confirmed = true; // UI에서 확인인지 우클릭인지 이벤트 결과 받아오는 함수?
			DispatchEvent(confirmed ? "Door_Confirm" : "Door_Revoke");
		});
	BindActionHandler("Door_Verdict", [this](const FSMAction& action)
		{
			// 문 여는 거 판단
			const bool success = true; // 문에서 결과 받아오는 함수
			DispatchEvent(success ? "Door_Open" : "Door_Fail");
		});
	BindActionHandler("Door_Open", [this](const FSMAction& action)
		{
			// 이동 가능하게 바꾸기
			// 애니메이션
			if (auto* owner = GetOwner())
			{
				if (auto* playerFsm = owner->GetComponent<PlayerFSMComponent>())
				{
					playerFsm->DispatchEvent("Door_Complete");
				}
			}
		});
	BindActionHandler("Door_Fail", [this](const FSMAction& action)
		{
			if (auto* owner = GetOwner())
			{
				if (auto* playerFsm = owner->GetComponent<PlayerFSMComponent>())
				{
					playerFsm->DispatchEvent("Door_Complete");
				}
			}
		});
}


void PlayerDoorFSMComponent::Start()
{
	FSMComponent::Start();
}

void PlayerDoorFSMComponent::Update(float deltaTime)
{
	FSMComponent::Update(deltaTime);
}
