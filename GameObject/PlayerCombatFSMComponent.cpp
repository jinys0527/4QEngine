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
				return;
			}

			const int amount = action.params.value("amount", 0);
			player->ConsumeActResource(amount);
		});

	BindActionHandler("Combat_Enter", [this](const FSMAction& action)
		{
			const int initiatorId = action.params.value("initiatorId", 0);
			const int targetId = action.params.value("targetId", 0);
			const CombatEnterEvent eventData{ initiatorId, targetId };
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