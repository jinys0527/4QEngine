#include "PlayerDoorFSMComponent.h"
#include "PlayerComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"

REGISTER_COMPONENT_DERIVED(PlayerDoorFSMComponent, FSMComponent)

PlayerDoorFSMComponent::PlayerDoorFSMComponent()
{
	BindActionHandler("Door_ConsumeActResource", [this](const FSMAction& action)
		{
			auto* owner = GetOwner();
			auto* player = owner ? owner->GetComponent<PlayerComponent>() : nullptr;
			if (!player)
			{
				return;
			}

			const int amount = action.params.value("amount", 0);
			player->ConsumeActResource(amount);
		});
	BindActionHandler("Door_Attempt", [this](const FSMAction& action)
		{
			// 난이도 표시 UI
			// 
		});
	BindActionHandler("Door_Verdict", [this](const FSMAction& action)
		{
			// 문 여는 거 판단
		});
}


void PlayerDoorFSMComponent::Start()
{
	FSMComponent::Start();
}