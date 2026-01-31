#include "PlayerShopFSMComponent.h"
#include "PlayerComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"

REGISTER_COMPONENT_DERIVED(PlayerShopFSMComponent, FSMComponent)

PlayerShopFSMComponent::PlayerShopFSMComponent()
{
	BindActionHandler("Shop_ItemSelect", [this](const FSMAction&)
		{
			DispatchEvent("Shop_BuyAttempt");
		});
	BindActionHandler("Shop_SpaceCheck", [this](const FSMAction& action)
		{
			auto* owner = GetOwner();
			auto* player = owner ? owner->GetComponent<PlayerComponent>() : nullptr;
			const bool hasSpace = player ? player->ConsumeShopHasSpace() : false;
			DispatchEvent(hasSpace ? "Shop_SpaceOk" : "Shop_SpaceFail");
		});
	BindActionHandler("Shop_MoneyCheck", [this](const FSMAction& action)
		{
			auto* owner = GetOwner();
			auto* player = owner ? owner->GetComponent<PlayerComponent>() : nullptr;
			const bool hasMoney = player ? player->ConsumeShopHasMoney() : false;
			DispatchEvent(hasMoney ? "Shop_MoneyOk" : "Shop_MoneyFail");
		});
	BindActionHandler("Shop_Buy", [this](const FSMAction&)
		{
			// 할인율 계산
			// 재화 반영
			// 인벤토리 갱신
			DispatchEvent("Shop_Complete");
		});
}

void PlayerShopFSMComponent::Start()
{
	FSMComponent::Start();
}