#include "PlayerDoorFSMComponent.h"
#include "PlayerComponent.h"
#include "PlayerStatComponent.h"
#include "DoorComponent.h"
#include "ReflectionMacro.h"
#include "PlayerFSMComponent.h"
#include "Object.h"
#include "Scene.h"
#include "ServiceRegistry.h"
#include "SoundManager.h"
#include "DiceSystem.h"
#include "Event.h"

REGISTER_COMPONENT_DERIVED(PlayerDoorFSMComponent, FSMComponent)

namespace
{
	constexpr int DoorCost = 1;
	constexpr int DoorRollThreshold = 7; // 문 성공 값(이상)
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
				GetEventDispatcher().Dispatch(EventType::PlayerDoorCancel, nullptr);
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
			auto* playerStat = owner ? owner->GetComponent<PlayerStatComponent>() : nullptr;

			if (!player)
			{
				DispatchEvent("Door_Revoke");
				GetEventDispatcher().Dispatch(EventType::PlayerDoorCancel, nullptr);
				return;
			}

			DispatchEvent("Door_Confirm");

			if (playerStat)
			{
				auto* scene = owner ? owner->GetScene() : nullptr;
				if (scene)
				{
					auto& services = scene->GetServices();

					GetEventDispatcher().Dispatch(EventType::PlayerDiceRoll, nullptr);

					// 주사위 판정
					if (services.Has<DiceSystem>())
					{
						auto& diceSystem = services.Get<DiceSystem>();
						int bonus = playerStat->GetCalculatedSkillModifier();
						const DiceConfig rollConfig{ 1, 20, bonus };

						const auto roll = diceSystem.Roll(rollConfig, RandomDomain::World);		// Door UI는 슬롯 설정에 따라 total/individual 중 하나만 표시하도록 구성될 수 있어
						// 1d20 결과를 두 형태 모두 브로드캐스트한다.
						const int rolledFace = !roll.faces.empty() ? roll.faces.front() : roll.total;
						const Events::DiceRollEvent individualEvent{ rolledFace, rollConfig.count, rollConfig.sides, rollConfig.bonus, "DoorRoll", false, { rolledFace } };
						GetEventDispatcher().Dispatch(EventType::DiceRolled, &individualEvent);

						player->SetDoorSuccess(roll.total >= DoorRollThreshold);
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
			//DispatchEvent(confirmed ? "Door_Confirm" : "Door_Revoke");'
			GetEventDispatcher().Dispatch(EventType::PlayerDiceUIReset, nullptr);
			GetEventDispatcher().Dispatch(EventType::PlayerDoorInteract, nullptr);
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
			// 애니메이션`1
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

				if (auto* playerFsm = owner->GetComponent<PlayerFSMComponent>())
				{
					playerFsm->DispatchEvent("Door_Complete");
				}

				auto* scene = owner->GetScene();
				if (scene)
				{
					auto& services = scene->GetServices();
					if (services.Has<SoundManager>())
					{
						services.Get<SoundManager>().SFX_Shot(L"Dice_Success");
					}
				}
			}
			GetEventDispatcher().Dispatch(EventType::PlayerDoorSuccess, nullptr);

			//DispatchEvent("Door_Complete");
			DispatchEvent("None");
		});
	BindActionHandler("Door_Fail", [this](const FSMAction& action)
		{
			std::cout << "Door Fail" << std::endl;
			auto* owner = GetOwner();
			if (owner)
			{
				if (auto* player = owner->GetComponent<PlayerComponent>())
				{
					player->ConsumePendingDoor();
				}

				if (auto* playerFsm = owner->GetComponent<PlayerFSMComponent>())
				{
					playerFsm->DispatchEvent("Door_Complete");
				}

				auto* scene = owner->GetScene();
				if (scene)
				{
					auto& services = scene->GetServices();
					if (services.Has<SoundManager>())
					{
						services.Get<SoundManager>().SFX_Shot(L"Dice_Fail");
					}
				}
			}
			GetEventDispatcher().Dispatch(EventType::PlayerDoorFail, nullptr);

			//DispatchEvent("Door_Complete");
			DispatchEvent("None");
		});

	BindActionHandler("Door_Revoke", [this](const FSMAction& action)
		{
			auto* owner = GetOwner();
			if (owner)
			{
				if (auto* player = owner->GetComponent<PlayerComponent>())
				{
					player->ConsumePendingDoor();
				}
				if (auto* playerFsm = owner->GetComponent<PlayerFSMComponent>())
				{
					playerFsm->DispatchEvent("Door_Complete");
				}

			}

			GetEventDispatcher().Dispatch(EventType::PlayerDoorCancel, nullptr);
			DispatchEvent("None");
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
