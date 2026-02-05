#include "PlayerDoorFSMComponent.h"
#include "PlayerComponent.h"
#include "DoorComponent.h"
#include "ReflectionMacro.h"
#include "PlayerFSMComponent.h"
#include "Object.h"
#include "Scene.h"
#include "ServiceRegistry.h"
#include "DiceSystem.h"

REGISTER_COMPONENT_DERIVED(PlayerDoorFSMComponent, FSMComponent)

namespace
{
	constexpr int DoorCost = 1;
	constexpr int DoorRollThreshold = 12; // 문 성공 값(이상)
}

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
			std::cout << "Door Attempt" << std::endl;
			auto* owner = GetOwner();
			auto* player = owner ? owner->GetComponent<PlayerComponent>() : nullptr;
			const bool confirmed = player ? player->ConsumeDoorConfirmed() : false;
			DispatchEvent(confirmed ? "Door_Confirm" : "Door_Revoke");
			if (confirmed && player)
			{
				auto* scene = owner ? owner->GetScene() : nullptr;
				if (scene)
				{
					auto& services = scene->GetServices();

					// 주사위 판정
					if (services.Has<DiceSystem>())
					{
						auto& diceSystem = services.Get<DiceSystem>();
						const DiceConfig rollConfig{ 1, 20, 0 };
						const int roll = diceSystem.RollTotal(rollConfig, RandomDomain::World);
						player->SetDoorSuccess(roll >= DoorRollThreshold);
					}
				}
			}
		});
	BindActionHandler("Door_Select", [this](const FSMAction& action)
		{
			// 안내 UI
			std::cout << "Door Select\n";
			auto* owner = GetOwner();
			auto* player = owner ? owner->GetComponent<PlayerComponent>() : nullptr;
			//const bool confirmed = player ? player->ConsumeDoorConfirmed() : false;
			//DispatchEvent(confirmed ? "Door_Confirm" : "Door_Revoke");
			DispatchEvent("Door_Confirm");
		});
	BindActionHandler("Door_Verdict", [this](const FSMAction& action)
		{
			// 문 여는 거 판단
			auto* owner = GetOwner();
			auto* player = owner ? owner->GetComponent<PlayerComponent>() : nullptr;
			const bool success = player ? player->ConsumeDoorSuccess() : false;
			DispatchEvent(success ? "Door_Open" : "Door_Fail");
		});
	BindActionHandler("Door_Open", [this](const FSMAction& action)
		{
			// 이동 가능하게 바꾸기
			// 애니메이션
			std::cout << "Door Success" << std::endl;
			if (auto* owner = GetOwner())
			{
				if (auto* player = owner->GetComponent<PlayerComponent>())
				{
					if (auto* door = player->ConsumePendingDoor())
					{
						door->OpenDoor();
					}
				}
			}
		});
	BindActionHandler("Door_Fail", [this](const FSMAction& action)
		{
			std::cout << "Door Fail" << std::endl;
			if (auto* owner = GetOwner())
			{
				if (auto* player = owner->GetComponent<PlayerComponent>())
				{
					player->ConsumePendingDoor();
				}
			}
			DispatchEvent("Door_Revoke");
			return;
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
