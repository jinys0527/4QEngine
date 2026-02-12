#include "PlayerShopFSMComponent.h"
#include "PlayerComponent.h"
#include "PlayerStatComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"
#include "Event.h"
#include "GameDataRepository.h"
#include "GameManager.h"
#include "ItemComponent.h"
#include "MeshRenderer.h"
#include "Scene.h"
#include "ServiceRegistry.h"
#include "ItemSpawnerComponent.h"
#include <algorithm>
#include <cmath>
#include <memory>
#include <string>

REGISTER_COMPONENT_DERIVED(PlayerShopFSMComponent, FSMComponent)

namespace
{
	int ToItemType(ItemCategory category)
	{
		switch (category)
		{
		case ItemCategory::Currency:
			return static_cast<int>(ItemType::GOLD);
		case ItemCategory::Healing:
			return static_cast<int>(ItemType::HEAL);
		case ItemCategory::Equipment:
			return static_cast<int>(ItemType::EQUIPMENT);
		case ItemCategory::Throwable:
			return static_cast<int>(ItemType::THROW);
		default:
			return static_cast<int>(ItemType::GOLD);
		}
	}

	void ApplyItemDefinition(ItemComponent& item, const ItemDefinition& definition)
	{
		item.SetItemIndex(definition.index);
		item.SetType(ToItemType(definition.category));
		item.SetIconPath(definition.iconPath);
		item.SetMeshPath(definition.meshPath);
		item.SetPrice(definition.basePrice);
		item.SetMeleeAttackRange(definition.range);
		item.SetThrowRange(definition.throwRange);
		item.SetActionPointCost(definition.actionPointCost);
		item.SetDifficultyGroup(definition.difficultyGroup);
		item.SetHealth(definition.constitutionModifier);
		item.SetStrength(definition.strengthModifier);
		item.SetAgility(definition.agilityModifier);
		item.SetSense(definition.senseModifier);
		item.SetSkill(definition.skillModifier);
		item.SetDEF(definition.defenseBonus);
		if (definition.diceType > 0)
		{
			item.SetDiceType(definition.diceType);
		}
		if (definition.baseModifier > 0)
		{
			item.SetBaseModifier(definition.baseModifier);
		}
	}

	std::shared_ptr<GameObject> SpawnPurchasedItem(Scene& scene, const ItemDefinition& definition)
	{
		static int counter = 0;
		const std::string name = definition.name + "_Shop_" + std::to_string(counter++);
		auto spawned = scene.CreateGameObject(name);
		if (!spawned)
		{
			return nullptr;
		}

		auto* itemComponent = spawned->AddComponent<ItemComponent>();
		if (!itemComponent)
		{
			return nullptr;
		}

		ApplyItemDefinition(*itemComponent, definition);
		spawned->Start();
		if (auto* renderer = spawned->GetComponent<MeshRenderer>())
		{
			renderer->SetVisible(false);
			renderer->SetRenderLayer(static_cast<UINT8>(RenderData::RenderLayer::None));
		}
		return spawned;
	}

	int ResolveDiscountedPrice(int basePrice, const PlayerStatComponent* stat)
	{
		const float discountRate = stat ? stat->GetShopDiscountRate() : 0.0f;
		const float clampedRate = std::clamp(discountRate, 0.0f, 0.9f);
		return max(0, static_cast<int>(std::round(
			static_cast<float>(basePrice) * (1.0f - clampedRate))));
	}

	PlayerComponent* FindOwnerPlayer(const Object* owner)
	{
		return owner ? owner->GetComponent<PlayerComponent>() : nullptr;
	}
}

PlayerShopFSMComponent::PlayerShopFSMComponent()
{
	BindActionHandler("Shop_ItemSelect", [this](const FSMAction& action)
		{
			const int itemId = action.params.value("itemId", -1);
			int price = action.params.value("price", -1);

			if (price < 0 && itemId >= 0)
			{
				auto* owner = GetOwner();
				auto* scene = owner ? owner->GetScene() : nullptr;
				if (scene)
				{
					auto& services = scene->GetServices();
					if (services.Has<GameDataRepository>())
					{
						const auto* definition = services.Get<GameDataRepository>().GetItem(itemId);
						if (definition)
						{
							price = definition->basePrice;
						}
					}
				}
			}

			m_SelectedItemId = itemId;
			m_SelectedPrice = max(0, price);
		});
	BindActionHandler("Shop_Select", [this](const FSMAction&)
		{
			GetEventDispatcher().Dispatch(EventType::PlayerShopOpen, nullptr);
			DispatchCurrentMoneyState();
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
				DispatchMoneyStateEvent(false);
				return;
			}

			const int price = ResolveActivePrice();
			const bool hasMoney = player->GetMoney() >= price;
			DispatchMoneyStateEvent(hasMoney);
		});
	BindActionHandler("Shop_Buy", [this](const FSMAction&)
		{
			auto* owner = GetOwner();
			auto* player = owner ? owner->GetComponent<PlayerComponent>() : nullptr;
			auto* playerStat = owner ? owner->GetComponent<PlayerStatComponent>() : nullptr;
			auto* scene = owner ? owner->GetScene() : nullptr;
			if (!player)
			{
				return;
			}

			//const int discountedPrice = ResolveDiscountedPrice(ResolveActivePrice(), playerStat);
			const int price = ResolveActivePrice();
			if (player->GetMoney() < price)
			{
				DispatchMoneyStateEvent(false);
				return;
			}

			player->SetMoney(player->GetMoney() - price);

			if (m_UseVendingOffer && m_VendingSpawner)
			{
				m_VendingSpawner->SpawnVendingRandomItem(player, m_VendingItemIds);
			}
			else
			{
				const int purchaseItemId = ResolvePurchaseItemId();
				if (scene && purchaseItemId >= 0)
				{
					auto& services = scene->GetServices();
					if (services.Has<GameDataRepository>())
					{
						const auto* definition = services.Get<GameDataRepository>().GetItem(purchaseItemId);
						if (definition)
						{
							auto spawnedItem = SpawnPurchasedItem(*scene, *definition);
							if (spawnedItem)
							{
								if (auto* itemComponent = spawnedItem->GetComponent<ItemComponent>())
								{
									player->AddToInventory(itemComponent);
								}
							}
						}
					}
				}
			}

			DispatchEvent("Shop_BuyComplete");
		});
	BindActionHandler("Shop_Close", [this](const FSMAction&)
		{
			m_SelectedItemId = -1;
			m_SelectedPrice = 0;
			ClearVendingOffer();

			auto* owner = GetOwner();
			auto* scene = owner ? owner->GetScene() : nullptr;
			auto* gameManager = scene ? scene->GetGameManager() : nullptr;

			DispatchEvent("Shop_Complete");

			if (gameManager && gameManager->GetPhase() == Phase::Shop)
			{
				GetEventDispatcher().Dispatch(EventType::ShopDone, nullptr);
			}
			GetEventDispatcher().Dispatch(EventType::PlayerShopClose, nullptr);
		});
}

void PlayerShopFSMComponent::Start()
{
	FSMComponent::Start();
}

void PlayerShopFSMComponent::ConfigureVendingOffer(int fixedPrice, const std::vector<int>& itemIds, ItemSpawnerComponent* spawner)
{
	m_UseVendingOffer = true;
	m_VendingFixedPrice = max(0, fixedPrice);
	m_VendingItemIds = itemIds;
	m_VendingSpawner = spawner;
}

void PlayerShopFSMComponent::ClearVendingOffer()
{
	m_UseVendingOffer = false;
	m_VendingFixedPrice = 10;
	m_VendingItemIds.clear();
	m_VendingSpawner = nullptr;
}

int PlayerShopFSMComponent::ResolveActivePrice() const
{
	if (m_UseVendingOffer)
	{
		return max(0, m_VendingFixedPrice);
	}
	return max(0, m_SelectedPrice);
}

int PlayerShopFSMComponent::ResolvePurchaseItemId() const
{
	if (m_UseVendingOffer)
	{
		return -1;
	}
	return m_SelectedItemId;
}

void PlayerShopFSMComponent::DispatchCurrentMoneyState()
{
	auto* owner = GetOwner();
	auto* player = FindOwnerPlayer(owner);
	auto* playerStat = owner ? owner->GetComponent<PlayerStatComponent>() : nullptr;
	if (!player)
	{
		DispatchEvent("Shop_MoneyFail");
		return;
	}

	const int price = ResolveDiscountedPrice(ResolveActivePrice(), playerStat);
	DispatchEvent(player->GetMoney() >= price ? "Shop_MoneyOk" : "Shop_MoneyFail");
}

void PlayerShopFSMComponent::DispatchMoneyStateEvent(bool hasMoney)
{
	DispatchEvent(hasMoney ? "Shop_MoneyOk" : "Shop_MoneyFail");
	GetEventDispatcher().Dispatch(hasMoney ? EventType::ShopMoneyOk : EventType::ShopMoneyFail, nullptr);
}