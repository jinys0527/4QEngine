#include "PlayerFSMComponent.h"
#include "Event.h"
#include "FSMEventRegistry.h"
#include "FSMActionRegistry.h"
#include "GameState.h"
#include "PlayerComponent.h"
#include "Object.h"
#include "ReflectionMacro.h"

void RegisterPlayerFSMDefinitions()
{
	auto& actionRegistry = FSMActionRegistry::Instance();
	actionRegistry.RegisterAction({
		"Player_ResetTurnResources",
		"Player",
		{}
		});
	actionRegistry.RegisterAction({
		"Player_RequestTurnEnd",
		"Player",
		{}
		});
	actionRegistry.RegisterAction({
		"Player_ConsumeActResource",
		"Player",
		{
			{ "amount", "int", 0, false }
		}
		});

	// Move_Begin
	actionRegistry.RegisterAction({
		"Move_Begin",
		"Move",
		{}
		});

	// Move_DragBegin
	actionRegistry.RegisterAction({
		"Move_DragBegin",
		"Move",
		{}
		});

	// Move_DragEnd
	actionRegistry.RegisterAction({
		"Move_DragEnd",
		"Move",
		{}
		});

	// Move_CommitPending  (pending(q,r) 사용, 파라미터 없음)
	actionRegistry.RegisterAction({
		"Move_CommitPending",
		"Move",
		{}
		});

	// Move_ClearPending
	actionRegistry.RegisterAction({
		"Move_ClearPending",
		"Move",
		{}
		});

	// Move_Revert (드래그 시작 위치로 원복, 파라미터 없음)
	actionRegistry.RegisterAction({
		"Move_Revert",
		"Move",
		{}
		});

	// Move_End
	actionRegistry.RegisterAction({
		"Move_End",
		"Move",
		{}
		});

	actionRegistry.RegisterAction({
		"Push_ConsumeActResource",
		"Push",
		{
			{ "amount", "int", 0, false }
		}
		});
	actionRegistry.RegisterAction({
		"Combat_ConsumeActResource",
		"Combat",
		{
			{ "amount", "int", 0, false }
		}
		});
	actionRegistry.RegisterAction({
		"Combat_Enter",
		"Combat",
		{
			{ "initiatorId", "int", 0, false },
			{ "targetId", "int", 0, false }
		}
		});
	actionRegistry.RegisterAction({
		"Combat_Exit",
		"Combat",
		{}
		});
	actionRegistry.RegisterAction({
		"Inventory_ConsumeActResource",
		"Inventory",
		{
			{ "amount", "int", 0, false }
		}
		});
	actionRegistry.RegisterAction({
		"Shop_ConsumeActResource",
		"Shop",
		{
			{ "amount", "int", 0, false }
		}
		});
	actionRegistry.RegisterAction({
		"Door_ConsumeActResource",
		"Door",
		{
			{ "amount", "int", 0, false }
		}
		});

	auto& eventRegistry = FSMEventRegistry::Instance();
	eventRegistry.RegisterEvent({ "Player_TurnStart", "Player" });
	eventRegistry.RegisterEvent({ "Player_TurnEnd",   "Player" });

	eventRegistry.RegisterEvent({ "Move_Start",       "Player" });
	eventRegistry.RegisterEvent({ "Move_Complete",    "Player" });
	eventRegistry.RegisterEvent({ "Move_Cancel",	  "Player" });
	eventRegistry.RegisterEvent({ "Push_Start",		  "Player" });
	eventRegistry.RegisterEvent({ "Push_Complete",	  "Player" });
	eventRegistry.RegisterEvent({ "Push_Cancel",	  "Player" });
	eventRegistry.RegisterEvent({ "Combat_Start",	  "Player" });
	eventRegistry.RegisterEvent({ "Combat_End",		  "Player" });
	eventRegistry.RegisterEvent({ "Inventory_Open",	  "Player" });
	eventRegistry.RegisterEvent({ "Inventory_Close",  "Player" });
	eventRegistry.RegisterEvent({ "Shop_Open",		  "Player" });
	eventRegistry.RegisterEvent({ "Shop_Close",		  "Player" });
	eventRegistry.RegisterEvent({ "Door_Interact",    "Player" });
	eventRegistry.RegisterEvent({ "Door_Complete",    "Player" });
	eventRegistry.RegisterEvent({ "Door_Cancel",      "Player" });
															          
	eventRegistry.RegisterEvent({ "Move_Select",       "Move" });
	eventRegistry.RegisterEvent({ "Move_PointValid",   "Move" });
	eventRegistry.RegisterEvent({ "Move_PointInvalid", "Move" });
	eventRegistry.RegisterEvent({ "Move_Confirm",      "Move" });
	eventRegistry.RegisterEvent({ "Move_Revoke",       "Move" });
															          
	eventRegistry.RegisterEvent({ "Push_Possible",       "Push"	});
	eventRegistry.RegisterEvent({ "Push_TargetFound",    "Push" });
	eventRegistry.RegisterEvent({ "Push_TargetNone",     "Push" });
	eventRegistry.RegisterEvent({ "Push_TargetSelected", "Push" });
																      
	eventRegistry.RegisterEvent({ "Combat_CheckRange",   "Combat" });
	eventRegistry.RegisterEvent({ "Combat_RangeOk",      "Combat" });
	eventRegistry.RegisterEvent({ "Combat_RangeFail",    "Combat" });
	eventRegistry.RegisterEvent({ "Combat_CostOk",       "Combat" });
	eventRegistry.RegisterEvent({ "Combat_CostFail",     "Combat" });
	eventRegistry.RegisterEvent({ "Combat_Confirm",      "Combat" });
	eventRegistry.RegisterEvent({ "Combat_Cancel",       "Combat" });
	eventRegistry.RegisterEvent({ "Combat_StartTurn",    "Combat" });
	eventRegistry.RegisterEvent({ "Combat_TurnResolved", "Combat" });
																      
	eventRegistry.RegisterEvent({ "Inventory_Drop",        "Inventory" });
	eventRegistry.RegisterEvent({ "Inventory_DropNoShop",  "Inventory" });
	eventRegistry.RegisterEvent({ "Inventory_DropHasShop", "Inventory" });
	eventRegistry.RegisterEvent({ "Inventory_Complete",    "Inventory" });

	eventRegistry.RegisterEvent({ "Shop_Buy",          "Shop" });
	eventRegistry.RegisterEvent({ "Shop_InputConfirm", "Shop" });
	eventRegistry.RegisterEvent({ "Shop_SpaceOk",      "Shop" });
	eventRegistry.RegisterEvent({ "Shop_SpaceFail",    "Shop" });
	eventRegistry.RegisterEvent({ "Shop_MoneyOk",      "Shop" });
	eventRegistry.RegisterEvent({ "Shop_MoneyFail",    "Shop" });
	eventRegistry.RegisterEvent({ "Shop_Complete",     "Shop" });

	eventRegistry.RegisterEvent({ "Door_Open",     "Door" });
	eventRegistry.RegisterEvent({ "Door_Confirm",  "Door" });
	eventRegistry.RegisterEvent({ "Door_CostPaid", "Door" });
}

REGISTER_COMPONENT_DERIVED(PlayerFSMComponent, FSMComponent)

PlayerFSMComponent::PlayerFSMComponent()
{
	BindActionHandler("Player_ResetTurnResources", [this](const FSMAction&)
		{
			auto* owner = GetOwner();
			auto* player = owner ? owner->GetComponent<PlayerComponent>() : nullptr;
			if (player)
			{
				player->ResetTurnResources();
			}
		});

	BindActionHandler("Player_RequestTurnEnd", [this](const FSMAction&)
		{
			auto* owner = GetOwner();
			auto* player = owner ? owner->GetComponent<PlayerComponent>() : nullptr;
			if (player && player->GetCurrentTurn() != Turn::PlayerTurn)
			{
				return;
			}
			GetEventDispatcher().Dispatch(EventType::PlayerTurnEndRequested, nullptr);
		});

	BindActionHandler("Player_ConsumeActResource", [this](const FSMAction& action)
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

PlayerFSMComponent::~PlayerFSMComponent()
{
	GetEventDispatcher().RemoveListener(EventType::TurnChanged, this);
}

void PlayerFSMComponent::Start()
{
	FSMComponent::Start();
	GetEventDispatcher().AddListener(EventType::TurnChanged, this);
}

std::optional<std::string> PlayerFSMComponent::TranslateEvent(EventType type, const void* data)
{
	if (type != EventType::TurnChanged || !data)
	{
		return std::nullopt;
	}

	const auto* payload = static_cast<const Events::TurnChanged*>(data);
	if (!payload)
	{
		return std::nullopt;
	}

	const auto turn = static_cast<Turn>(payload->turn);
	return turn == Turn::PlayerTurn ? "Player_TurnStart" : "Player_TurnEnd";
}
