#include "PlayerInventoryFSMComponent.h"
#include "PlayerComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"

REGISTER_COMPONENT_DERIVED(PlayerInventoryFSMComponent, FSMComponent)

PlayerInventoryFSMComponent::PlayerInventoryFSMComponent()
{
	BindActionHandler("Inventory_DropAttempt", [this](const FSMAction& action)
		{
			const bool isShop = action.params.value("isShop", false);
			DispatchEvent(isShop ? "Inventory_DropAtShop" : "Inventory_DropNoShop");
		});
	BindActionHandler("Inventory_Drop", [this](const FSMAction& action)
		{
			const bool canDrop = action.params.value("canDrop", true);
			if (canDrop)
			{
				DispatchEvent("Inventory_Complete");
			}
			// 버리기 가능한 곳 찾기 6개 모서리 랜덤 -> 시계방향 탐색
			// 빈 자리 메시 배치
		});
	BindActionHandler("Inventory_Sell", [this](const FSMAction& action)
		{
			// 환금율 계산
			// 재화 반영, 아이템 소멸
			const bool sold = action.params.value("sold", true);
			if (sold)
			{
				DispatchEvent("Inventory_Complete");
			}
		});
}


void PlayerInventoryFSMComponent::Start()
{
	FSMComponent::Start();
}