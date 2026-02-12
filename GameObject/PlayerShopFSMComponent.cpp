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

	constexpr int kMaxVendingOfferSlots = 6;

	std::vector<int> BuildVendingPurchasableCandidates(const std::vector<int>& itemIds, const std::vector<int>& offerCounts)
	{
		std::vector<int> candidates;
		const size_t count = (std::min)(itemIds.size(), offerCounts.size());
		candidates.reserve(count);
		for (size_t i = 0; i < count; ++i)
		{
			if (itemIds[i] <= 0 || offerCounts[i] <= 0)
			{
				continue;
			}
			candidates.push_back(itemIds[i]);
		}
		return candidates;
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
			OnShopSelected();
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
				const auto purchaseCandidates = BuildVendingPurchasableCandidates(m_VendingItemIds, m_VendingOfferRemainingCounts);
				if (purchaseCandidates.empty())
				{
					return;
				}

				const int purchasedItemId = m_VendingSpawner->SpawnVendingRandomItem(player, purchaseCandidates);
				if (purchasedItemId <= 0)
				{
					return;
				}

				for (size_t i = 0; i < m_VendingItemIds.size() && i < m_VendingOfferRemainingCounts.size(); ++i)
				{
					if (m_VendingItemIds[i] == purchasedItemId)
					{
						m_VendingOfferRemainingCounts[i] = (std::max)(0, m_VendingOfferRemainingCounts[i] - 1);
						break;
					}
				}

				auto* sceneForUpdate = owner ? owner->GetScene() : nullptr;
				if (sceneForUpdate)
				{
					Events::VendingOfferUpdatedEvent payload;
					payload.vendingObjectName = m_VendingObjectName;
					payload.itemIds = m_VendingItemIds;
					payload.itemCounts = m_VendingOfferRemainingCounts;
					GetEventDispatcher().Dispatch(EventType::VendingOfferUpdated, &payload);
				}
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

void PlayerShopFSMComponent::OnShopSelected()
{
	GetEventDispatcher().Dispatch(EventType::PlayerShopOpen, nullptr);
	DispatchCurrentMoneyState();
}

void PlayerShopFSMComponent::ConfigureVendingOffer(int fixedPrice, const std::vector<int>& itemIds, ItemSpawnerComponent* spawner, const std::string& vendingObjectName)
{
	m_UseVendingOffer = true;
	m_VendingFixedPrice = max(0, fixedPrice);
	m_VendingItemIds.clear();
	m_VendingOfferRemainingCounts.clear();
	m_VendingItemIds.reserve((std::min)(kMaxVendingOfferSlots, static_cast<int>(itemIds.size())));
	m_VendingOfferRemainingCounts.reserve((std::min)(kMaxVendingOfferSlots, static_cast<int>(itemIds.size())));
	for (const int itemId : itemIds)
	{
		if (itemId <= 0)
		{
			continue;
		}

		m_VendingItemIds.push_back(itemId);
		int cappedCount = 0;
		if (spawner)
		{
			cappedCount = (std::min)(kMaxVendingOfferSlots, (std::max)(0, spawner->GetRemainingDropQuantity(itemId)));
		}
		m_VendingOfferRemainingCounts.push_back(cappedCount);

		if (static_cast<int>(m_VendingItemIds.size()) >= kMaxVendingOfferSlots)
		{
			break;
		}
	}
	m_VendingSpawner = spawner;
	m_VendingObjectName = vendingObjectName;
}

void PlayerShopFSMComponent::ClearVendingOffer()
{
	m_UseVendingOffer = false;
	m_VendingFixedPrice = 10;
	m_VendingItemIds.clear();
	m_VendingOfferRemainingCounts.clear();
	m_VendingSpawner = nullptr;
	m_VendingObjectName.clear();
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

	const int price = ResolveActivePrice();
	DispatchEvent(player->GetMoney() >= price ? "Shop_MoneyOk" : "Shop_MoneyFail");
}

void PlayerShopFSMComponent::DispatchMoneyStateEvent(bool hasMoney)
{
	DispatchEvent(hasMoney ? "Shop_MoneyOk" : "Shop_MoneyFail");
	GetEventDispatcher().Dispatch(hasMoney ? EventType::ShopMoneyOk : EventType::ShopMoneyFail, nullptr);
}