#include "PlayerFSMFlowComponent.h"
#include "Event.h"
#include "Object.h"
#include "PlayerComponent.h"
#include "PlayerCombatFSMComponent.h"
#include "PlayerDoorFSMComponent.h"
#include "PlayerInventoryFSMComponent.h"
#include "PlayerPushFSMComponent.h"
#include "PlayerShopFSMComponent.h"
#include "PlayerFSMComponent.h"
#include "ReflectionMacro.h"

namespace
{
	void DispatchPlayerEvent(Object* owner, const char* eventName)
	{
		if (!owner || !eventName)
			return;

		if (auto* fsm = owner->GetComponent<PlayerFSMComponent>())
		{
			fsm->DispatchEvent(eventName);
		}
	}

	void DispatchPushEvent(Object* owner, const char* eventName)
	{
		if (!owner || !eventName)
			return;

		if (auto* fsm = owner->GetComponent<PlayerPushFSMComponent>())
		{
			fsm->DispatchEvent(eventName);
		}
	}

	void DispatchCombatEvent(Object* owner, const char* eventName)
	{
		if (!owner || !eventName)
			return;

		if (auto* fsm = owner->GetComponent<PlayerCombatFSMComponent>())
		{
			fsm->DispatchEvent(eventName);
		}
	}

	void DispatchInventoryEvent(Object* owner, const char* eventName)
	{
		if (!owner || !eventName)
			return;

		if (auto* fsm = owner->GetComponent<PlayerInventoryFSMComponent>())
		{
			fsm->DispatchEvent(eventName);
		}
	}

	void DispatchShopEvent(Object* owner, const char* eventName)
	{
		if (!owner || !eventName)
			return;

		if (auto* fsm = owner->GetComponent<PlayerShopFSMComponent>())
		{
			fsm->DispatchEvent(eventName);
		}
	}

	void DispatchDoorEvent(Object* owner, const char* eventName)
	{
		if (!owner || !eventName)
			return;

		if (auto* fsm = owner->GetComponent<PlayerDoorFSMComponent>())
		{
			fsm->DispatchEvent(eventName);
		}
	}
}

REGISTER_COMPONENT(PlayerFSMFlowComponent)
REGISTER_PROPERTY(PlayerFSMFlowComponent, PushCost)
REGISTER_PROPERTY(PlayerFSMFlowComponent, CombatCost)
REGISTER_PROPERTY(PlayerFSMFlowComponent, DoorCost)
REGISTER_PROPERTY(PlayerFSMFlowComponent, HasShopNearby)
REGISTER_PROPERTY(PlayerFSMFlowComponent, HasCombatRange)
REGISTER_PROPERTY(PlayerFSMFlowComponent, HasPushTarget)
REGISTER_PROPERTY(PlayerFSMFlowComponent, HasShopSpace)
REGISTER_PROPERTY(PlayerFSMFlowComponent, HasShopMoney)

PlayerFSMFlowComponent::PlayerFSMFlowComponent() = default;

PlayerFSMFlowComponent::~PlayerFSMFlowComponent()
{
	GetEventDispatcher().RemoveListener(EventType::KeyDown, this);
}

void PlayerFSMFlowComponent::Start()
{
	GetEventDispatcher().AddListener(EventType::KeyDown, this);
}

void PlayerFSMFlowComponent::Update(float deltaTime)
{
	(void)deltaTime;
}

void PlayerFSMFlowComponent::OnEvent(EventType type, const void* data)
{
	if (type != EventType::KeyDown)
		return;

	const auto* keyData = static_cast<const Events::KeyEvent*>(data);
	if (!keyData)
		return;

	switch (keyData->key)
	{
	case 'P':
		HandlePushFlow();
		break;
	case 'C':
		HandleCombatFlow();
		break;
	case 'I':
		HandleInventoryFlow();
		break;
	case 'B':
		HandleShopFlow();
		break;
	case 'O':
		HandleDoorFlow();
		break;
	case 'T':
		m_HasPushTarget = !m_HasPushTarget;
		break;
	case 'R':
		m_HasCombatRange = !m_HasCombatRange;
		break;
	case 'H':
		m_HasShopNearby = !m_HasShopNearby;
		break;
	case 'S':
		m_HasShopSpace = !m_HasShopSpace;
		break;
	case 'M':
		m_HasShopMoney = !m_HasShopMoney;
		break;
	default:
		break;
	}
}

void PlayerFSMFlowComponent::HandlePushFlow()
{
	auto* owner = GetOwner();
	if (!owner)
		return;

	auto* player = owner->GetComponent<PlayerComponent>();
	if (!player)
		return;

	if (!player->ConsumeActResource(m_PushCost))
	{
		DispatchPlayerEvent(owner, "Push_Cancel");
		DispatchPushEvent(owner, "Push_Cancel");
		return;
	}

	DispatchPlayerEvent(owner, "Push_Start");
	DispatchPushEvent(owner, "Push_Possible");
	if (m_HasPushTarget)
	{
		DispatchPushEvent(owner, "Push_TargetFound");
		DispatchPushEvent(owner, "Push_TargetSelected");
		DispatchPushEvent(owner, "Push_Complete");
		DispatchPlayerEvent(owner, "Push_Complete");
	}
	else
	{
		DispatchPushEvent(owner, "Push_TargetNone");
		DispatchPushEvent(owner, "Push_Cancel");
		DispatchPlayerEvent(owner, "Push_Cancel");
	}
}

void PlayerFSMFlowComponent::HandleCombatFlow()
{
	auto* owner = GetOwner();
	if (!owner)
		return;

	auto* player = owner->GetComponent<PlayerComponent>();
	if (!player)
		return;

	DispatchPlayerEvent(owner, "Combat_Start");
	DispatchCombatEvent(owner, "Combat_CheckRange");
	if (!m_HasCombatRange)
	{
		DispatchCombatEvent(owner, "Combat_RangeFail");
		DispatchPlayerEvent(owner, "Combat_End");
		return;
	}

	DispatchCombatEvent(owner, "Combat_RangeOk");
	if (!player->ConsumeActResource(m_CombatCost))
	{
		DispatchCombatEvent(owner, "Combat_CostFail");
		DispatchPlayerEvent(owner, "Combat_End");
		return;
	}

	DispatchCombatEvent(owner, "Combat_CostOk");
	DispatchCombatEvent(owner, "Combat_Confirm");
	DispatchCombatEvent(owner, "Combat_StartTurn");
	DispatchCombatEvent(owner, "Combat_TurnResolved");
	DispatchCombatEvent(owner, "Combat_End");
	DispatchPlayerEvent(owner, "Combat_End");
}

void PlayerFSMFlowComponent::HandleInventoryFlow()
{
	auto* owner = GetOwner();
	if (!owner)
		return;

	if (!m_InventoryOpen)
	{
		m_InventoryOpen = true;
		DispatchPlayerEvent(owner, "Inventory_Open");
	}

	DispatchInventoryEvent(owner, "Inventory_Drop");
	if (m_HasShopNearby)
	{
		DispatchInventoryEvent(owner, "Inventory_DropHasShop");
	}
	else
	{
		DispatchInventoryEvent(owner, "Inventory_DropNoShop");
	}

	DispatchInventoryEvent(owner, "Inventory_Complete");
	m_InventoryOpen = false;
	DispatchPlayerEvent(owner, "Inventory_Close");
}

void PlayerFSMFlowComponent::HandleShopFlow()
{
	auto* owner = GetOwner();
	if (!owner)
		return;

	if (!m_ShopOpen)
	{
		m_ShopOpen = true;
		DispatchPlayerEvent(owner, "Shop_Open");
	}

	DispatchShopEvent(owner, "Shop_Buy");
	DispatchShopEvent(owner, "Shop_InputConfirm");

	if (!m_HasShopSpace)
	{
		DispatchShopEvent(owner, "Shop_SpaceFail");
	}
	else
	{
		DispatchShopEvent(owner, "Shop_SpaceOk");
		if (m_HasShopMoney)
		{
			DispatchShopEvent(owner, "Shop_MoneyOk");
			DispatchShopEvent(owner, "Shop_Complete");
		}
		else
		{
			DispatchShopEvent(owner, "Shop_MoneyFail");
		}
	}

	m_ShopOpen = false;
	DispatchPlayerEvent(owner, "Shop_Close");
}

void PlayerFSMFlowComponent::HandleDoorFlow()
{
	auto* owner = GetOwner();
	if (!owner)
		return;

	auto* player = owner->GetComponent<PlayerComponent>();
	if (!player)
		return;

	DispatchPlayerEvent(owner, "Door_Interact");
	DispatchDoorEvent(owner, "Door_Open");
	m_DoorOpen = true;

	DispatchDoorEvent(owner, "Door_Confirm");
	if (player->ConsumeActResource(m_DoorCost))
	{
		DispatchDoorEvent(owner, "Door_CostPaid");
		DispatchDoorEvent(owner, "Door_Complete");
		DispatchPlayerEvent(owner, "Door_Complete");
	}
	else
	{
		DispatchDoorEvent(owner, "Door_Cancel");
		DispatchPlayerEvent(owner, "Door_Cancel");
	}

	m_DoorOpen = false;
}
