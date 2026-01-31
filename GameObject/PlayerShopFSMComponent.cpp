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
			const bool hasSpace = true; // 인벤토리 빈 공간 확인 함수
			DispatchEvent(hasSpace ? "Shop_SpaceOk" : "Shop_SpaceFail");
		});
	BindActionHandler("Shop_MoneyCheck", [this](const FSMAction& action)
		{
			const bool hasMoney = true; // 재화와 아이템 가격 비교 함수
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