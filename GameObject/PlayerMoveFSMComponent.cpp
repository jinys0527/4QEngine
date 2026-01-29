#include "PlayerMoveFSMComponent.h"
#include "PlayerComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"

REGISTER_COMPONENT_DERIVED(PlayerMoveFSMComponent, FSMComponent)

PlayerMoveFSMComponent::PlayerMoveFSMComponent()
{
	BindActionHandler("Move_Begin", [this](const FSMAction&)
		{
			auto* owner = GetOwner();
			auto* player = owner ? owner->GetComponent<PlayerComponent>() : nullptr;
			if (player)
			{
				player->BeginMove();
			}
		});

	BindActionHandler("Move_Commit", [this](const FSMAction& action)
		{
			auto* owner = GetOwner();
			auto* player = owner ? owner->GetComponent<PlayerComponent>() : nullptr;
			if (!player)
			{
				return;
			}

			const int q = action.params.value("q", player->GetQ());
			const int r = action.params.value("r", player->GetR());
			player->CommitMove(q, r);
		});
}

void PlayerMoveFSMComponent::Start()
{
	FSMComponent::Start();
}