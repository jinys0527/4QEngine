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
}


void PlayerDoorFSMComponent::Start()
{
	FSMComponent::Start();
}