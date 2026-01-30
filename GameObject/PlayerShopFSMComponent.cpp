#include "PlayerShopFSMComponent.h"
#include "PlayerComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"

REGISTER_COMPONENT_DERIVED(PlayerShopFSMComponent, FSMComponent)

PlayerShopFSMComponent::PlayerShopFSMComponent()
{
	BindActionHandler("Shop_ConsumeActResource", [this](const FSMAction& action)
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

void PlayerShopFSMComponent::Start()
{
	FSMComponent::Start();
}