#include "PlayerFSMComponent.h"
#include "Event.h"
#include "FSMEventRegistry.h"
#include "FSMActionRegistry.h"
#include "GameState.h"
#include "PlayerComponent.h"
#include "PlayerCombatFSMComponent.h"
#include "PlayerDoorFSMComponent.h"
#include "Object.h"
#include "PlayerInventoryFSMComponent.h"
#include "PlayerMoveFSMComponent.h"
#include "PlayerPushFSMComponent.h"
#include "PlayerShopFSMComponent.h"
#include "ReflectionMacro.h"
#include <algorithm>
#include <cctype>

namespace
{
	std::string ToLower(std::string value)
	{
		std::transform(value.begin(), value.end(), value.begin(),
			[](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
		return value;
	}

	void DispatchSubFSMEvent(Object* owner, const std::string& target, const std::string& eventName)
	{
		if (!owner || eventName.empty())
		{
			return;
		}

		const auto normalized = ToLower(target);

		if (normalized == "move")
		{
			if (auto* fsm = owner->GetComponent<PlayerMoveFSMComponent>())
			{
				fsm->DispatchEvent(eventName);
			}
			return;
		}

		if (normalized == "door")
		{
			if (auto* fsm = owner->GetComponent<PlayerDoorFSMComponent>())
			{
				fsm->DispatchEvent(eventName);
			}
			return;
		}

		if (normalized == "combat")
		{
			if (auto* fsm = owner->GetComponent<PlayerCombatFSMComponent>())
			{
				fsm->DispatchEvent(eventName);
			}
			return;
		}

		if (normalized == "inventory")
		{
			if (auto* fsm = owner->GetComponent<PlayerInventoryFSMComponent>())
			{
				fsm->DispatchEvent(eventName);
			}
			return;
		}

		if (normalized == "shop")
		{
			if (auto* fsm = owner->GetComponent<PlayerShopFSMComponent>())
			{
				fsm->DispatchEvent(eventName);
			}
			return;
		}

		if (normalized == "push")
		{
			if (auto* fsm = owner->GetComponent<PlayerPushFSMComponent>())
			{
				fsm->DispatchEvent(eventName);
			}
			return;
		}
	}
}

// Editor 설정 시 Player FSM 진입/종료 이벤트 정리:
	// - Player_Move       : onEnter -> Player_DispatchSubFSMEvent(target="Move", event="Move_Select")
	//                       onExit  -> Player_DispatchEvent(event="Move_Complete")
	// - Player_Shop       : onEnter -> Player_DispatchSubFSMEvent(target="Shop", event="Shop_ItemSelect")
	//                       onExit  -> Player_DispatchEvent(event="Shop_Close")
	// - Player_Inventory  : onEnter -> Player_DispatchSubFSMEvent(target="Inventory", event="Inventory_Drop")
	//                       onExit  -> Player_DispatchEvent(event="Inventory_Close")
	// - Player_Combat     : onEnter -> Player_DispatchSubFSMEvent(target="Combat", event="Combat_CheckRange")
	//                       onExit  -> Player_DispatchEvent(event="Combat_End")
	// - Player_Push       : onEnter -> Player_DispatchSubFSMEvent(target="Push", event="Push_Start")
	//                       onExit  -> Player_DispatchEvent(event="Push_Complete")
	// - Player_Door       : onEnter -> Player_DispatchSubFSMEvent(target="Door", event="Door Select")
	//                       onExit  -> Player_DispatchEvent(event="Door_Complete")

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

	actionRegistry.RegisterAction({
	"Player_DispatchSubFSMEvent",
	"Player",
	{
		{ "target", "string", "", true },
		{ "event", "string", "", true }
	}
		});
	actionRegistry.RegisterAction({
		"Player_DispatchEvent",
		"Player",
		{
			{ "event", "string", "", true }
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

	// Push 행동력
	actionRegistry.RegisterAction({
		"Push_ConsumeActResource",
		"Push",
		{
			{ "amount", "int", 0, false }
		}
		});

	// Push 가능 여부 체크
	actionRegistry.RegisterAction({
		"Push_CheckPossible",
		"Push",
		{
			{ "canPush", "bool", true, false }
		}
		});

	// Push 대상 탐색
	actionRegistry.RegisterAction({
		"Push_SearchTarget",
		"Push",
		{
			{ "hasTarget", "bool", false, false }
		}
		});

	// Push 대상 선택
	actionRegistry.RegisterAction({
		"Push_SelectTarget",
		"Push",
		{}
		});

	// Push 판정
	actionRegistry.RegisterAction({
		"Push_Resolve",
		"Push",
		{
			{ "success", "bool", false, false }
		}
		});

	// Combat 행동력
	actionRegistry.RegisterAction({
		"Combat_ConsumeActResource",
		"Combat",
		{
			{ "amount", "int", 0, false }
		}
		});

	// Combat RangeCheck
	actionRegistry.RegisterAction({
		"Combat_RangeCheck",
		"Combat",
		{
			{ "inRange", "bool", true, false }
		}
		});

	// Combat Confirm
	actionRegistry.RegisterAction({
		"Combat_Confirm",
		"Combat",
		{
			{ "confirmed", "bool", true, false }
		}
		});

	// Combat Attack
	actionRegistry.RegisterAction({
		"Combat_Attack",
		"Combat",
		{
			{ "resolved", "bool", true, false }
		}
		});

	// Combat Result
	actionRegistry.RegisterAction({
		"Combat_Result",
		"Combat",
		{
			{ "resolved", "bool", true, false }
		}
		});

	// Combat 전투 진입
	actionRegistry.RegisterAction({
		"Combat_Enter",
		"Combat",
		{
			{ "initiatorId", "int", 0, false },
			{ "targetId", "int", 0, false }
		}
		});

	// Combat 전투 종료
	actionRegistry.RegisterAction({
		"Combat_Exit",
		"Combat",
		{}
		});

	// Inventory DropAttempt
	actionRegistry.RegisterAction({
		"Inventory_DropAttempt",
		"Inventory",
		{
			{ "isShop", "bool", false, false }
		}
		});

	// Inventory Drop
	actionRegistry.RegisterAction({
		"Inventory_Drop",
		"Inventory",
		{
			{ "canDrop", "bool", true, false }
		}
		});

	// Inventory Sell
	actionRegistry.RegisterAction({
		"Inventory_Sell",
		"Inventory",
		{
			{ "sold", "bool", true, false }
		}
		});

	// Shop ItemSelect
	actionRegistry.RegisterAction({
		"Shop_ItemSelect",
		"Shop",
		{}
		});

	// Shop SpaceCheck
	actionRegistry.RegisterAction({
		"Shop_SpaceCheck",
		"Shop",
		{
			{ "hasSpace", "bool", true, false }
		}
		});

	// Shop MoneyCheck
	actionRegistry.RegisterAction({
		"Shop_MoneyCheck",
		"Shop",
		{
			{ "hasMoney", "bool", true, false }
		}
		});

	// Shop Buy
	actionRegistry.RegisterAction({
		"Shop_Buy",
		"Shop",
		{}
		});

	// Door 행동력 차감
	actionRegistry.RegisterAction({
		"Door_ConsumeActResource",
		"Door",
		{
			{ "amount", "int", 0, false }
		}
		});

	// Door 열기 시도
	actionRegistry.RegisterAction({
		"Door_Attempt",
		"Door",
		{
			{ "confirmed", "bool", true, false }
		}
		});

	// Door 선택
	actionRegistry.RegisterAction({
		"Door_Select",
		"Door",
		{
			{ "confirmed", "bool", true, false }
		}
		});

	// Door 성공/실패 판정
	actionRegistry.RegisterAction({
		"Door_Verdict",
		"Door",
		{
			{ "success", "bool", false, false }
		}
		});

	// Door 성공
	actionRegistry.RegisterAction({
		"Door_Open",
		"Door",
		{
			{ "notifyPlayer", "bool", true, false }
		}
		});

	// Door 실패
	actionRegistry.RegisterAction({
		"Door_Fail",
		"Door",
		{
			{ "notifyPlayer", "bool", true, false }
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
	
	eventRegistry.RegisterEvent({ "Push_Start",			 "Push" });
	eventRegistry.RegisterEvent({ "Push_Possible",       "Push"	});
	eventRegistry.RegisterEvent({ "Push_TargetFound",    "Push" });
	eventRegistry.RegisterEvent({ "Push_TargetNone",     "Push" });
	eventRegistry.RegisterEvent({ "Push_TargetSelected", "Push" });
	eventRegistry.RegisterEvent({ "Push_Success",		 "Push" });
	eventRegistry.RegisterEvent({ "Push_Fail",			 "Push" });
	eventRegistry.RegisterEvent({ "Push_Revoke",		 "Push" });
																      
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
	eventRegistry.RegisterEvent({ "Inventory_DropAtShop",  "Inventory" });
	eventRegistry.RegisterEvent({ "Inventory_Complete",    "Inventory" });

	eventRegistry.RegisterEvent({ "Shop_ItemSelect",   "Shop" });
	eventRegistry.RegisterEvent({ "Shop_BuyAttempt",   "Shop" });
	eventRegistry.RegisterEvent({ "Shop_SpaceOk",      "Shop" });
	eventRegistry.RegisterEvent({ "Shop_SpaceFail",    "Shop" });
	eventRegistry.RegisterEvent({ "Shop_MoneyOk",      "Shop" });
	eventRegistry.RegisterEvent({ "Shop_MoneyFail",    "Shop" });
	eventRegistry.RegisterEvent({ "Shop_Complete",     "Shop" });

	eventRegistry.RegisterEvent({ "Door Select",   "Door" });
	eventRegistry.RegisterEvent({ "Door_Confirm",  "Door" });
	eventRegistry.RegisterEvent({ "Door_CostPaid", "Door" });
	eventRegistry.RegisterEvent({ "Door_Open",	   "Door" });
	eventRegistry.RegisterEvent({ "Door_Fail",	   "Door" });
	eventRegistry.RegisterEvent({ "Door_Revoke",   "Door" });
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

	BindActionHandler("Player_DispatchSubFSMEvent", [this](const FSMAction& action)
		{
			auto* owner = GetOwner();
			if (!owner)
			{
				return;
			}

			const std::string target = action.params.value("target", "");
			const std::string eventName = action.params.value("event", "");
			DispatchSubFSMEvent(owner, target, eventName);
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
