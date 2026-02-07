#include "PlayerShopFSMComponent.h"
#include "PlayerComponent.h"
#include "PlayerStatComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"
#include "Event.h"
#include <algorithm>
#include <cmath>

REGISTER_COMPONENT_DERIVED(PlayerShopFSMComponent, FSMComponent)

namespace
{
	int ResolveDiscountedPrice(int basePrice, const PlayerStatComponent* stat)
	{
		const float discountRate = stat ? stat->GetShopDiscountRate() : 0.0f;
		const float clampedRate = std::clamp(discountRate, 0.0f, 0.9f);
		return max(0, static_cast<int>(std::round(
			static_cast<float>(basePrice) * (1.0f - clampedRate))));
	}
}

PlayerShopFSMComponent::PlayerShopFSMComponent()
{
	BindActionHandler("Shop_ItemSelect", [this](const FSMAction& action)
		{
			// UI에 해당하는 아이템 가격 받아오기
			//m_SelectedPrice = max(0, action.params.value("price", 0));
		});
	BindActionHandler("Shop_Select", [this](const FSMAction&)
		{
			GetEventDispatcher().Dispatch(EventType::PlayerShopOpen, nullptr);
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
			auto* playerStat = owner ? owner->GetComponent<PlayerStatComponent>() : nullptr;
			if (!player)
			{
				DispatchEvent("Shop_MoneyFail");
				return;
			}

			const int price = ResolveDiscountedPrice(m_SelectedPrice, playerStat);
			const bool hasMoney = player->GetMoney() >= price;
			DispatchEvent(hasMoney ? "Shop_MoneyOk" : "Shop_MoneyFail");
		});
	BindActionHandler("Shop_Buy", [this](const FSMAction&)
		{
			auto* owner = GetOwner();
			auto* player = owner ? owner->GetComponent<PlayerComponent>() : nullptr;
			auto* playerStat = owner ? owner->GetComponent<PlayerStatComponent>() : nullptr;
			if (!player)
			{
				return;
			}

			const int discountedPrice = ResolveDiscountedPrice(m_SelectedPrice, playerStat);

			if (player->GetMoney() < discountedPrice)
			{
				return;
			}

			player->SetMoney(player->GetMoney() - discountedPrice);

			// 인벤토리 갱신
		});
	BindActionHandler("Shop_Close", [this](const FSMAction&)
		{
			DispatchEvent("Shop_Complete");
			GetEventDispatcher().Dispatch(EventType::ShopDone, nullptr);
			GetEventDispatcher().Dispatch(EventType::PlayerShopClose, nullptr);
		});
}

void PlayerShopFSMComponent::Start()
{
	FSMComponent::Start();
}